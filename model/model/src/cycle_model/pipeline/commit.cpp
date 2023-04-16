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
#include "cycle_model/pipeline/execute_commit.h"
#include "cycle_model/component/csr_all.h"
#include "cycle_model/component/branch_predictor_base.h"
#include "breakpoint.h"

namespace cycle_model::pipeline
{
    commit::commit(global_inst *global, component::port<execute_commit_pack_t> **alu_commit_port, component::port<execute_commit_pack_t> **bru_commit_port, component::port<execute_commit_pack_t> **csr_commit_port, component::port<execute_commit_pack_t> **div_commit_port, component::port<execute_commit_pack_t> **mul_commit_port, component::port<execute_commit_pack_t> **lu_commit_port, component::port<execute_commit_pack_t> **sau_commit_port, component::port<execute_commit_pack_t> **sdu_commit_port, component::rat *speculative_rat, component::rat *retire_rat, component::rob *rob, component::csrfile *csr_file, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::interrupt_interface *interrupt_interface, component::branch_predictor_set *branch_predictor_set, component::fifo<component::checkpoint_t> *checkpoint_buffer, component::load_queue *load_queue, component::store_buffer *store_buffer, component::retired_store_buffer *retired_store_buffer, component::fifo<decode_rename_pack_t> *decode_rename_fifo, component::bus *bus_if, component::pma_unit *pma_unit) :
#ifdef BRANCH_PREDICTOR_UPDATE_DUMP
    branch_predictor_update_dump_stream(BRANCH_PREDICTOR_UPDATE_DUMP_FILE),
#endif
#ifdef BRANCH_PREDICTOR_DUMP
            branch_predictor_dump_stream(BRANCH_PREDICTOR_DUMP_FILE),
#endif
    tdb(TRACE_COMMIT)
    {
        this->global = global;
        this->alu_commit_port = alu_commit_port;
        this->bru_commit_port = bru_commit_port;
        this->csr_commit_port = csr_commit_port;
        this->div_commit_port = div_commit_port;
        this->mul_commit_port = mul_commit_port;
        this->lu_commit_port = lu_commit_port;
        this->sau_commit_port = sau_commit_port;
        this->sdu_commit_port = sdu_commit_port;
        this->speculative_rat = speculative_rat;
        this->retire_rat = retire_rat;
        this->rob = rob;
        this->csr_file = csr_file;
        this->phy_regfile = phy_regfile;
        this->phy_id_free_list = phy_id_free_list;
        this->interrupt_interface = interrupt_interface;
        this->branch_predictor_set = branch_predictor_set;
        this->checkpoint_buffer = checkpoint_buffer;
        this->load_queue = load_queue;
        this->store_buffer = store_buffer;
        this->retired_store_buffer = retired_store_buffer;
        this->decode_rename_fifo = decode_rename_fifo;
        this->bus_if = bus_if;
        this->pma_unit = pma_unit;
        this->commit::reset();
    }
    
    void commit::reset()
    {
        is_waiting_ack = false;
    }
    
    void commit::init()
    {
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(alu_commit_port[i]);
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(bru_commit_port[i]);
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(csr_commit_port[i]);
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(div_commit_port[i]);
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(mul_commit_port[i]);
        }
        
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(lu_commit_port[i]);
        }
    
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(sau_commit_port[i]);
        }
    
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            this->execute_commit_port.push_back(sdu_commit_port[i]);
        }
    }
    
    commit_feedback_pack_t commit::run()
    {
        commit_feedback_pack_t feedback_pack;
        riscv_interrupt_t interrupt_id;
        bool need_flush = false;
        feedback_pack.idle = true;
        feedback_pack.has_interrupt = false;//only for debug
        feedback_pack.waiting_for_interrupt = false;
        
        if(interrupt_interface->get_cause(&interrupt_id))
        {
            if(!rob->customer_is_empty() || decode_rename_fifo->customer_is_empty())
            {
                feedback_pack.waiting_for_interrupt = true;
            }
            else
            {
                decode_rename_pack_t t_pack;
                verify(decode_rename_fifo->customer_get_front(&t_pack));
                csr_file->write_sys(CSR_MEPC, t_pack.pc);
                csr_file->write_sys(CSR_MTVAL, 0);
                csr_file->write_sys(CSR_MCAUSE, 0x80000000 | static_cast<uint32_t>(interrupt_id));
                component::csr::mstatus mstatus;
                mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                mstatus.set_mpie(mstatus.get_mie());
                mstatus.set_mie(false);
                csr_file->write_sys(CSR_MSTATUS, mstatus.get_value());
                interrupt_interface->set_ack(interrupt_id);
                feedback_pack.has_exception = true;
                feedback_pack.exception_pc = csr_file->read_sys(CSR_MTVEC);
                feedback_pack.flush = true;
                feedback_pack.has_interrupt = true;//only for debug
                feedback_pack.interrupt_id = interrupt_id;//only for debug
                rob->set_committed(true);
                need_flush = true;
            }
        }
    
        //handle output
        if(!rob->customer_is_empty())
        {
            uint32_t rob_item_id = 0;
            verify(rob->get_front_id(&rob_item_id));
            feedback_pack.idle = false;
            feedback_pack.next_handle_rob_id = rob_item_id;
            feedback_pack.next_handle_rob_id_valid = true;
            auto first_id = rob_item_id;
            auto rob_item = rob->get_item(rob_item_id);
            
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
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
                            rob_retire_queue.emplace_back(rob_item_id, rob_item);
#endif
                            csr_file->write_sys(CSR_MEPC, rob_item.pc);
                            csr_file->write_sys(CSR_MTVAL, rob_item.exception_value);
                            csr_file->write_sys(CSR_MCAUSE, static_cast<uint32_t>(rob_item.exception_id));
                            feedback_pack.has_exception = true;
                            feedback_pack.exception_pc = csr_file->read_sys(CSR_MTVEC);
                            feedback_pack.flush = true;
                            speculative_rat->load(retire_rat);
                            rob->flush();
                            phy_regfile->restore(retire_rat);
                            phy_id_free_list->restore(rob_item.new_phy_id_free_list_rptr, rob_item.new_phy_id_free_list_rstage);
                            load_queue->flush();
                            store_buffer->flush();
                            checkpoint_buffer->flush();
                            rob->set_committed(true);
                            rob->add_commit_num(1);
                            need_flush = true;
                            break;
                        }
                        else
                        {
                            //store instruction must be retired in first order
                            if(rob_item.store_buffer_id_valid && (i != 0))
                            {
                                break;
                            }
                            
                            //store instruction needs a space of retired_store_buffer when addr is cacheable
                            if(rob_item.store_buffer_id_valid && pma_unit->is_cacheable(store_buffer->get_addr(rob_item.store_buffer_id)) && retired_store_buffer->producer_is_full())
                            {
                                break;
                            }

                            //load handle
                            if(rob_item.load_queue_id_valid)
                            {
                                global->load_num_add();
                                
                                if(load_queue->get_replay_num(rob_item.load_queue_id) > 0)
                                {
                                    global->replay_load_num_add();
                                    global->replay_num_add(load_queue->get_replay_num(rob_item.load_queue_id));
                                }
                                
                                if(load_queue->get_conflict(rob_item.load_queue_id))
                                {
                                    global->conflict_load_num_add();
                                }
                                
                                if(load_queue->get_conflict(rob_item.load_queue_id) && !rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                {
                                    feedback_pack.jump_enable = true;
                                    feedback_pack.jump = true;
                                    feedback_pack.jump_next_pc = rob_item.pc;
                                    feedback_pack.flush = true;
                                    speculative_rat->load(retire_rat);
                                    rob->flush();
                                    phy_regfile->restore(retire_rat);
                                    phy_id_free_list->restore(rob_item.old_phy_id_free_list_rptr, rob_item.old_phy_id_free_list_rstage);
                                    load_queue->flush();
                                    store_buffer->flush();
                                    checkpoint_buffer->flush();
                                    need_flush = true;
                                    break;
                                }
                                
                                if(rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                {
                                    component::checkpoint_t t_cp;
                                    checkpoint_buffer->pop(&t_cp);
                                }
    
                                component::load_queue_item_t t_item;
                                load_queue->pop(&t_item);
                            }
                            
                            //store handle
                            if(rob_item.store_buffer_id_valid)
                            {
                                component::store_buffer_item_t t_item = store_buffer->get_item(rob_item.store_buffer_id);
                                uint32_t t_addr = store_buffer->get_addr(rob_item.store_buffer_id);
                                uint32_t t_size = store_buffer->get_size(rob_item.store_buffer_id);
                                
                                verify_only((t_size == 1) || (t_size == 2) || (t_size == 4));
                                
                                if(is_waiting_ack)
                                {
                                    component::bus_errno_t write_errno;
                                    
                                    if(bus_if->get_data_write_ack(&write_errno))
                                    {
                                        is_waiting_ack = false;
                                        
                                        if(write_errno == component::bus_errno_t::error)
                                        {
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
                                            rob_retire_queue.emplace_back(rob_item_id, rob_item);
#endif
                                            csr_file->write_sys(CSR_MEPC, rob_item.pc);
                                            csr_file->write_sys(CSR_MTVAL, t_addr);
                                            csr_file->write_sys(CSR_MCAUSE, static_cast<uint32_t>(riscv_exception_t::store_amo_access_fault));
                                            feedback_pack.has_exception = true;
                                            feedback_pack.exception_pc = csr_file->read_sys(CSR_MTVEC);
                                            feedback_pack.flush = true;
                                            speculative_rat->load(retire_rat);
                                            rob->flush();
                                            phy_regfile->restore(retire_rat);
                                            phy_id_free_list->restore(rob_item.new_phy_id_free_list_rptr, rob_item.new_phy_id_free_list_rstage);
                                            load_queue->flush();
                                            store_buffer->flush();
                                            checkpoint_buffer->flush();
                                            rob->set_committed(true);
                                            rob->add_commit_num(1);
                                            need_flush = true;
                                            break;
                                        }
                                        
                                        //continue to retire
                                    }
                                    else
                                    {
                                        break;//pause retirement
                                    }
                                }
                                else
                                {
                                    if(pma_unit->is_cacheable(t_addr))
                                    {
                                        component::retired_store_buffer_item_t t_retired_item;
                                        t_retired_item.inst_common_info = t_item.inst_common_info;
                                        t_retired_item.addr = t_addr;
                                        t_retired_item.size = t_size;
                                        t_retired_item.data = t_item.data;
                                        verify(retired_store_buffer->push(t_retired_item));
                                    }
                                    else
                                    {
                                        switch(t_size)
                                        {
                                            case 1:
                                                bus_if->write8_sync(t_addr, (uint8_t)t_item.data);
                                                break;
                                            
                                            case 2:
                                                bus_if->write16_sync(t_addr, (uint16_t)t_item.data);
                                                break;
                                            
                                            case 4:
                                                bus_if->write32_sync(t_addr, t_item.data);
                                                break;
                                        }
                                        
                                        is_waiting_ack = true;
                                        break;//pause retirement
                                    }
                                }
                                
                                store_buffer->pop(&t_item);
                            }

#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
                            rob_retire_queue.emplace_back(rob_item_id, rob_item);
#endif
                            rob->pop();
                            
                            if(rob_item.old_phy_reg_id_valid)
                            {
                                retire_rat->set_map(rob_item.rd, rob_item.new_phy_reg_id);
                                phy_regfile->write(rob_item.old_phy_reg_id, 0, false, 0, false, true);
                                phy_regfile->write_age_information(rob_item.new_phy_reg_id, 0, false, true);
                                phy_id_free_list->push(rob_item.old_phy_reg_id);
                            }
        
                            if(rob_item.last_uop)
                            {
                                rob->set_committed(true);
                                rob->add_commit_num(1);
                            }
        
                            if(rob_item.csr_newvalue_valid)
                            {
                                csr_file->write(rob_item.csr_addr, rob_item.csr_newvalue);
                                breakpoint_csr_trigger(rob_item.csr_addr, rob_item.csr_newvalue, true);
                            }
        
                            //branch handle
                            if(rob_item.bru_op)
                            {
                                if(rob_item.branch_predictor_info_pack.condition_jump)
                                {
                                    global->branch_num_add();
                                }
            
                                if(rob_item.is_mret)
                                {
                                    component::csr::mstatus mstatus;
                                    mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                                    mstatus.set_mie(mstatus.get_mpie());
                                    csr_file->write_sys(CSR_MSTATUS, mstatus.get_value());
                                }
                                
                                if(rob_item.branch_predictor_info_pack.predicted)
                                {
                                    if(rob_item.branch_predictor_info_pack.condition_jump)
                                    {
                                        global->branch_predicted_add();
                                    }
                                    
                                    if((rob_item.bru_jump == rob_item.branch_predictor_info_pack.jump) && (rob_item.bru_next_pc == rob_item.branch_predictor_info_pack.next_pc))
                                    {
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            global->branch_hit_add();
                                        }
                                        
#ifdef BRANCH_PREDICTOR_UPDATE_DUMP
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            branch_predictor_update_dump_stream << outhex(rob_item.pc) << "," << outbool(rob_item.bru_jump) << "," << outhex(rob_item.bru_next_pc) << "," << outbool(true) << std::endl;
                                        }
#endif
#ifdef BRANCH_PREDICTOR_DUMP
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            branch_predictor_dump_stream << outhex(rob_item.pc) << "," << outhex(rob_item.inst_value) << "," <<
                                            outhex(branch_predictor_set->bi_mode.get_global_history_retired()) << "," <<
                                            outhex(rob_item.branch_predictor_info_pack.bi_mode_global_history) << "," << rob_item.branch_predictor_info_pack.bi_mode_pht_value << "," <<
                                            outbool(rob_item.bru_jump) << "," << outhex(rob_item.bru_next_pc) << "," << outbool(true) << std::endl;
                                        }
#endif
                                        //if(!rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                        {
                                            branch_predictor_set->bi_mode.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, true, rob_item.branch_predictor_info_pack);
                                            //branch_predictor_set->bi_modal.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, true, rob_item.branch_predictor_info_pack);
                                            //branch_predictor_set->l0_btb.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, true, rob_item.branch_predictor_info_pack);
                                            branch_predictor_set->l1_btb.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, true, rob_item.branch_predictor_info_pack);
                                        }
                                    }
                                    else if((rob_item.bru_jump == rob_item.branch_predictor_info_pack.jump) && (rob_item.bru_next_pc != rob_item.branch_predictor_info_pack.next_pc) && rob_item.branch_predictor_info_pack.condition_jump)
                                    {
                                        verify_only(false);
                                    }
                                    else
                                    {
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            global->branch_miss_add();
                                        }

#ifdef BRANCH_PREDICTOR_UPDATE_DUMP
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            branch_predictor_update_dump_stream << outhex(rob_item.pc) << "," << outbool(rob_item.bru_jump) << "," << outhex(rob_item.bru_next_pc) << "," << outbool(false) << std::endl;
                                        }
#endif
#ifdef BRANCH_PREDICTOR_DUMP
                                        if(rob_item.branch_predictor_info_pack.condition_jump)
                                        {
                                            branch_predictor_dump_stream << outhex(rob_item.pc) << "," << outhex(rob_item.inst_value) << "," <<
                                                                         outhex(branch_predictor_set->bi_mode.get_global_history_retired()) << "," <<
                                                                         outhex(rob_item.branch_predictor_info_pack.bi_mode_global_history) << "," << rob_item.branch_predictor_info_pack.bi_mode_pht_value << "," <<
                                                                         outbool(rob_item.bru_jump) << "," << outhex(rob_item.bru_next_pc) << "," << outbool(false) << std::endl;
                                        }
#endif
                                        //if(!rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                        {
                                            branch_predictor_set->bi_mode.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, false, rob_item.branch_predictor_info_pack);
                                            //branch_predictor_set->bi_modal.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, false, rob_item.branch_predictor_info_pack);
                                            //branch_predictor_set->l0_btb.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, false, rob_item.branch_predictor_info_pack);
                                            branch_predictor_set->l1_btb.update_sync(rob_item.pc, rob_item.bru_jump, rob_item.bru_next_pc, false, rob_item.branch_predictor_info_pack);
                                        }
                                        
                                        if(!rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                        {
                                            feedback_pack.jump_enable = true;
                                            feedback_pack.jump = true;
                                            feedback_pack.jump_next_pc = rob_item.bru_next_pc;
                                            feedback_pack.flush = true;
                                            speculative_rat->load(retire_rat);
                                            rob->flush();
                                            phy_regfile->restore(retire_rat);
                                            phy_id_free_list->restore(rob_item.new_phy_id_free_list_rptr, rob_item.new_phy_id_free_list_rstage);
                                            load_queue->flush();
                                            store_buffer->flush();
                                            checkpoint_buffer->flush();
                                            need_flush = true;
                                        }
                                    }
                                    
                                    if(rob_item.branch_predictor_info_pack.checkpoint_id_valid)
                                    {
                                        component::checkpoint_t t_cp;
                                        checkpoint_buffer->pop(&t_cp);
                                    }
                                }
                                else
                                {
                                    feedback_pack.jump_enable = true;
                                    feedback_pack.jump = rob_item.bru_jump;
                                    feedback_pack.jump_next_pc = rob_item.bru_jump ? rob_item.bru_next_pc : (rob_item.pc + 4);
                                }
                                
                                break;//only handle a bru op
                            }
                        }
                    }
                    else
                    {
                        break;//no more finish rob_item
                    }
                }
            }
            
            for(uint32_t i = 0;i < ROB_SIZE;i++)
            {
                if(rob->customer_check_id_valid(i))
                {
                    auto item = rob->get_item(i);
                }
            }
            
            if(!need_flush)
            {
                for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
                {
                    auto rev_pack = execute_commit_port[i]->get();
                    
                    if(rev_pack.enable)
                    {
                        auto rob_item = rob->get_item(rev_pack.rob_id);
                        
                        if(rev_pack.last_uop)
                        {
                            rob_item.finish = true;
                        }
                        
                        if(!rob_item.has_exception)
                        {
                            rob_item.has_exception = rev_pack.has_exception;
                            rob_item.exception_id = rev_pack.exception_id;
                            rob_item.exception_value = rev_pack.exception_value;
                        }
                        
                        rob_item.branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
                        rob_item.bru_op = rev_pack.op_unit == op_unit_t::bru;
                        rob_item.bru_jump = rev_pack.bru_jump;
                        rob_item.bru_next_pc = rev_pack.bru_next_pc;
                        rob_item.csr_newvalue = rev_pack.csr_newvalue;
                        rob_item.csr_newvalue_valid = rev_pack.csr_newvalue_valid;
                        rob->set_item(rev_pack.rob_id, rob_item);
                    }
                }
            }
        }
        
        return feedback_pack;
    }
}