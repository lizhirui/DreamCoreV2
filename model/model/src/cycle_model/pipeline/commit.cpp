/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/commit.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/rat.h"
#include "cycle_model/component/rob.h"
#include "cycle_model/component/csrfile.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/interrupt_interface.h"
#include "cycle_model/pipeline/wb_commit.h"
#include "cycle_model/component/csr_all.h"

namespace cycle_model::pipeline
{
    commit::commit(component::port<wb_commit_pack_t> *wb_commit_port, component::rat *speculative_rat, component::rat *retire_rat, component::rob *rob, component::csrfile *csr_file, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::interrupt_interface *interrupt_interface) : tdb(TRACE_COMMIT)
    {
        this->wb_commit_port = wb_commit_port;
        this->speculative_rat = speculative_rat;
        this->retire_rat = retire_rat;
        this->rob = rob;
        this->csr_file = csr_file;
        this->phy_regfile = phy_regfile;
        this->phy_id_free_list = phy_id_free_list;
        this->interrupt_interface = interrupt_interface;
        this->commit::reset();
    }
    
    void commit::reset()
    {
    
    }
    
    commit_feedback_pack_t commit::run()
    {
        commit_feedback_pack_t feedback_pack;
        bool need_flush = false;
    
        //handle output
        if(!rob->customer_is_empty())
        {
            uint32_t rob_item_id = 0;
            verify(rob->get_front_id(&rob_item_id));
            feedback_pack.next_handle_rob_id = rob_item_id;
            feedback_pack.next_handle_rob_id_valid = true;
            auto first_id = rob_item_id;
            auto rob_item = rob->get_item(rob_item_id);
            
            riscv_interrupt_t interrupt_id;
        
            if(interrupt_interface->get_cause(&interrupt_id))
            {
                csr_file->write_sys(CSR_MEPC, rob_item.pc);
                csr_file->write_sys(CSR_MTVAL, 0);
                csr_file->write_sys(CSR_MCAUSE, 0x80000000 | static_cast<uint32_t>(interrupt_id));
                component::csr::mstatus mstatus;
                mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                mstatus.set_mpie(mstatus.get_mie());
                mstatus.set_mie(false);
                csr_file->write_sys(CSR_MSTATUS, mstatus.get_value());
                interrupt_interface->set_ack(interrupt_id);
                feedback_pack.exception_pc = csr_file->read_sys(CSR_MTVEC);
                feedback_pack.flush = true;
                speculative_rat->load(retire_rat);
                rob->flush();
                phy_regfile->restore(retire_rat);
                phy_id_free_list->restore(rob_item.old_phy_id_free_list_rptr, rob_item.old_phy_id_free_list_rstage);
                rob->set_committed(true);
                need_flush = true;
            }
            else
            {
                for(uint32_t i = 0;i < COMMIT_WIDTH;i++)
                {
                    if(rob->get_front_id(&rob_item_id))
                    {
                        rob_item = rob->get_item(rob_item_id);
    
                        if(rob_item.finish)
                        {
                            feedback_pack.next_handle_rob_id_valid = rob->get_next_id(rob_item_id, &feedback_pack.next_handle_rob_id) && (feedback_pack.next_handle_rob_id != first_id);
                            feedback_pack.committed_rob_id_valid[i] = true;
                            feedback_pack.committed_rob_id[i] = rob_item_id;
        
                            if(rob_item.has_exception)
                            {
                                csr_file->write_sys(CSR_MEPC, rob_item.pc);
                                csr_file->write_sys(CSR_MTVAL, rob_item.exception_value);
                                csr_file->write_sys(CSR_MCAUSE, static_cast<uint32_t>(rob_item.exception_id));
                                feedback_pack.exception_pc = csr_file->read_sys(CSR_MTVEC);
                                feedback_pack.flush = true;
                                speculative_rat->load(retire_rat);
                                rob->flush();
                                phy_regfile->restore(retire_rat);
                                phy_id_free_list->restore(rob_item.new_phy_id_free_list_rptr, rob_item.new_phy_id_free_list_rstage);
                                rob->set_committed(true);
                                rob->add_commit_num(1);
                                need_flush = true;
                                break;
                            }
                            else
                            {
                                rob->pop();
            
                                if(rob_item.old_phy_reg_id_valid)
                                {
                                    speculative_rat->release_map(rob_item.old_phy_reg_id);
                                    retire_rat->commit_map(rob_item.rd, rob_item.new_phy_reg_id);
                                    retire_rat->release_map(rob_item.old_phy_reg_id);
                                    phy_regfile->write(rob_item.old_phy_reg_id, 0, false);
                                }
            
                                rob->set_committed(true);
                                rob->add_commit_num(1);
            
                                if(rob_item.csr_newvalue_valid)
                                {
                                    csr_file->write(rob_item.csr_addr, rob_item.csr_newvalue);
                                }
            
                                //branch handle
                                if(rob_item.bru_op)
                                {
                                    //branch_num_add();
                
                                    if(rob_item.is_mret)
                                    {
                                        component::csr::mstatus mstatus;
                                        mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                                        mstatus.set_mie(mstatus.get_mpie());
                                        csr_file->write_sys(CSR_MSTATUS, mstatus.get_value());
                                    }
                                    
                                    feedback_pack.jump_enable = true;
                                    feedback_pack.jump = rob_item.bru_jump;
                                    feedback_pack.jump_next_pc = rob_item.bru_jump ? rob_item.bru_next_pc : (rob_item.pc + 4);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                
                if(!need_flush)
                {
                    auto rev_pack = wb_commit_port->get();
                    
                    for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
                    {
                        if(rev_pack.op_info[i].enable)
                        {
                            auto rob_item = rob->get_item(rev_pack.op_info[i].rob_id);
                            rob_item.finish = true;
                            rob_item.has_exception = rev_pack.op_info[i].has_exception;
                            rob_item.exception_id = rev_pack.op_info[i].exception_id;
                            rob_item.exception_value = rev_pack.op_info[i].exception_value;
                            rob_item.bru_op = rev_pack.op_info[i].op_unit == op_unit_t::bru;
                            rob_item.bru_jump = rev_pack.op_info[i].bru_jump;
                            rob_item.bru_next_pc = rev_pack.op_info[i].bru_next_pc;
                            rob_item.csr_newvalue = rev_pack.op_info[i].csr_newvalue;
                            rob_item.csr_newvalue_valid = rev_pack.op_info[i].csr_newvalue_valid;
                            rob->set_item(rev_pack.op_info[i].rob_id, rob_item);
                        }
                    }
                }
            }
        }
        
        return feedback_pack;
    }
}