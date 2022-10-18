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
#include "config.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/rename.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/rename_dispatch.h"

namespace cycle_model::pipeline
{
    rename::rename(component::fifo<decode_rename_pack_t> *decode_rename_fifo, component::port<rename_dispatch_pack_t> *rename_dispatch_port, component::rat *speculative_rat, component::rob *rob, component::free_list *phy_id_free_list) : tdb(TRACE_RENAME)
    {
        this->decode_rename_fifo = decode_rename_fifo;
        this->rename_dispatch_port = rename_dispatch_port;
        this->speculative_rat = speculative_rat;
        this->rob = rob;
        this->phy_id_free_list = phy_id_free_list;
        this->rename::reset();
    }
    
    void rename::reset()
    {
        phy_id_free_list->reset();
    }
    
    rename_feedback_pack_t rename::run(const dispatch_feedback_pack_t &dispatch_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        rename_feedback_pack_t feedback_pack;
        rename_dispatch_pack_t send_pack;
        bool ready_to_stop_rename = false;
        bool found_fence = false;
        feedback_pack.idle = decode_rename_fifo->customer_is_empty();
        
        if(!commit_feedback_pack.flush)
        {
            if(!dispatch_feedback_pack.stall)
            {
                //generate base send_pack
                for(uint32_t i = 0;i < RENAME_WIDTH;i++)
                {
                    component::rob_item_t rob_item;
                    
                    if(!decode_rename_fifo->customer_is_empty())
                    {
                        //start to rename
                        if(!phy_id_free_list->customer_is_empty() && !rob->customer_is_full())
                        {
                            decode_rename_pack_t rev_pack;
                            verify(decode_rename_fifo->customer_get_front(&rev_pack));
                            
                            if(rev_pack.enable && rev_pack.valid && !rev_pack.has_exception)
                            {
                                if((rev_pack.op_unit == op_unit_t::csr) || (rev_pack.op == op_t::mret))
                                {
                                    if(i == 0)
                                    {
                                        ready_to_stop_rename = true;
                                    }
                                    else
                                    {
                                        break;//stop rename immediately
                                    }
                                }
                                else if(rev_pack.op == op_t::fence)
                                {
                                    found_fence = true;
                                }
                                else if(found_fence && (rev_pack.op_unit == op_unit_t::lsu))
                                {
                                    break;//stop rename immediately
                                }
                            }
                            
                            verify(decode_rename_fifo->pop(&rev_pack));
                            
                            send_pack.op_info[i].enable = rev_pack.enable;
                            send_pack.op_info[i].value = rev_pack.value;
                            send_pack.op_info[i].valid = rev_pack.valid;
                            send_pack.op_info[i].last_uop = rev_pack.last_uop;
                            send_pack.op_info[i].pc = rev_pack.pc;
                            send_pack.op_info[i].imm = rev_pack.imm;
                            send_pack.op_info[i].has_exception = rev_pack.has_exception;
                            send_pack.op_info[i].exception_id = rev_pack.exception_id;
                            send_pack.op_info[i].exception_value = rev_pack.exception_value;
                            send_pack.op_info[i].rs1 = rev_pack.rs1;
                            send_pack.op_info[i].arg1_src = rev_pack.arg1_src;
                            send_pack.op_info[i].rs1_need_map = rev_pack.rs1_need_map;
                            send_pack.op_info[i].rs2 = rev_pack.rs2;
                            send_pack.op_info[i].arg2_src = rev_pack.arg2_src;
                            send_pack.op_info[i].rs2_need_map = rev_pack.rs2_need_map;
                            send_pack.op_info[i].rd = rev_pack.rd;
                            send_pack.op_info[i].rd_enable = rev_pack.rd_enable;
                            send_pack.op_info[i].need_rename = rev_pack.need_rename;
                            send_pack.op_info[i].csr = rev_pack.csr;
                            send_pack.op_info[i].op = rev_pack.op;
                            send_pack.op_info[i].op_unit = rev_pack.op_unit;
                            
                            memcpy(&send_pack.op_info[i].sub_op, &rev_pack.sub_op, sizeof(rev_pack.sub_op));
                            
                            //generate rob items
                            if(rev_pack.enable)
                            {
                                phy_id_free_list->save(&rob_item.old_phy_id_free_list_rptr, &rob_item.old_phy_id_free_list_rstage);
                                
                                if(rev_pack.valid)
                                {
                                    if(rev_pack.need_rename)
                                    {
                                        rob_item.old_phy_reg_id_valid = true;
                                        verify(speculative_rat->producer_get_phy_id(rev_pack.rd, &rob_item.old_phy_reg_id));
                                        verify(phy_id_free_list->pop(&send_pack.op_info[i].rd_phy));
                                        speculative_rat->set_map(rev_pack.rd, send_pack.op_info[i].rd_phy);
                                    }
                                    else
                                    {
                                        rob_item.old_phy_reg_id_valid = false;
                                        rob_item.old_phy_reg_id = 0;
                                    }
                                }
                                
                                rob_item.finish = false;
                                rob_item.new_phy_reg_id = send_pack.op_info[i].rd_phy;
                                rob_item.pc = rev_pack.pc;
                                rob_item.inst_value = rev_pack.value;
                                rob_item.rd = rev_pack.rd;
                                rob_item.has_exception = rev_pack.has_exception;
                                rob_item.exception_id = rev_pack.exception_id;
                                rob_item.exception_value = rev_pack.exception_value;
                                rob_item.bru_op = rev_pack.op_unit == op_unit_t::bru;
                                rob_item.bru_jump = false;
                                rob_item.bru_next_pc = 0;
                                rob_item.is_mret = rev_pack.op == op_t::mret;
                                rob_item.csr_addr = rev_pack.csr;
                                rob_item.csr_newvalue = 0;
                                rob_item.csr_newvalue_valid = false;
                                phy_id_free_list->save(&rob_item.new_phy_id_free_list_rptr, &rob_item.new_phy_id_free_list_rstage);
                                //write to rob
                                verify(rob->get_new_id(&send_pack.op_info[i].rob_id));
                                verify(rob->get_new_id_stage(&send_pack.op_info[i].rob_id_stage));
                                verify(rob->push(rob_item));
                                
                                //start to map source registers
                                if(rev_pack.rs1_need_map)
                                {
                                    verify(speculative_rat->producer_get_phy_id(rev_pack.rs1, &send_pack.op_info[i].rs1_phy));
                                }
                                
                                if(rev_pack.rs2_need_map)
                                {
                                    verify(speculative_rat->producer_get_phy_id(rev_pack.rs2, &send_pack.op_info[i].rs2_phy));
                                }
                            }
                            
                            if(ready_to_stop_rename)
                            {
                                break;
                            }
                        }
                        else if(phy_id_free_list->customer_is_empty())
                        {
                            //phy_regfile_full_add();
                            
                            if(rob->customer_is_full())
                            {
                                //rob_full_add();
                            }
                            
                            break;
                        }
                        else
                        {
                            //rob_full_add();
                            break;
                        }
                    }
                    else
                    {
                        //idle
                        break;
                    }
                }
                
                rename_dispatch_port->set(send_pack);
            }
        }
        else
        {
            rename_dispatch_port->set(send_pack);
        }
        
        //assertion
        if(send_pack.op_info[0].enable && send_pack.op_info[0].valid && !send_pack.op_info[0].has_exception && (send_pack.op_info[0].op_unit == op_unit_t::csr))
        {
            for(auto i = 1;i < RENAME_WIDTH;i++)
            {
                verify(!send_pack.op_info[i].enable);
            }
        }
        
        auto assertion_found_fence = false;
        
        for(auto i = 0;i < RENAME_WIDTH;i++)
        {
            if(send_pack.op_info[i].enable && send_pack.op_info[i].valid && !send_pack.op_info[i].has_exception)
            {
                verify((i == 0) || (send_pack.op_info[i].op_unit != op_unit_t::csr));
                verify(!assertion_found_fence || (send_pack.op_info[i].op_unit != op_unit_t::lsu));
                
                if(send_pack.op_info[i].op == op_t::fence)
                {
                    assertion_found_fence = true;
                }
            }
        }
        
        return feedback_pack;
    }
}