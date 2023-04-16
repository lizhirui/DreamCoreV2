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
#include "cycle_model/component/checkpoint.h"
#include "cycle_model/component/fifo.h"

namespace cycle_model::pipeline
{
    rename::rename(global_inst *global, component::fifo<decode_rename_pack_t> *decode_rename_fifo, component::port<rename_dispatch_pack_t> *rename_dispatch_port, component::rat *speculative_rat, component::rob *rob, component::free_list *phy_id_free_list, component::fifo<component::checkpoint_t> *checkpoint_buffer, component::load_queue *load_queue, component::store_buffer *store_buffer) : tdb(TRACE_RENAME)
    {
        this->global = global;
        this->decode_rename_fifo = decode_rename_fifo;
        this->rename_dispatch_port = rename_dispatch_port;
        this->speculative_rat = speculative_rat;
        this->rob = rob;
        this->phy_id_free_list = phy_id_free_list;
        this->checkpoint_buffer = checkpoint_buffer;
        this->load_queue = load_queue;
        this->store_buffer = store_buffer;
        this->rename::reset();
    }
    
    void rename::reset()
    {
    
    }
    
    rename_feedback_pack_t rename::run(const dispatch_feedback_pack_t &dispatch_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        rename_feedback_pack_t feedback_pack;
        rename_dispatch_pack_t send_pack;
        bool ready_to_stop_rename = false;
        bool found_fence = false;
        feedback_pack.idle = decode_rename_fifo->customer_is_empty();
        
        if(!commit_feedback_pack.flush && !bru_feedback_pack.flush && !sau_feedback_pack.flush)
        {
            if(!dispatch_feedback_pack.stall)
            {
                if(!commit_feedback_pack.waiting_for_interrupt)
                {
                    //generate base send_pack
                    for(uint32_t i = 0;i < RENAME_WIDTH;i++)
                    {
                        component::rob_item_t rob_item;
                        
                        if(!decode_rename_fifo->customer_is_empty())
                        {
                            //start to rename
                            if(!phy_id_free_list->producer_is_empty() && !rob->producer_is_full())
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
                                    else if(found_fence && ((rev_pack.op_unit == op_unit_t::lu) || (rev_pack.op_unit == op_unit_t::sau) || (rev_pack.op_unit == op_unit_t::sdu)))
                                    {
                                        break;//stop rename immediately
                                    }
                                    else if(rev_pack.op_unit == op_unit_t::lu)
                                    {
                                        if(load_queue->producer_is_full())
                                        {
                                            break;//stop rename immediately
                                        }
                                    }
                                    else if(rev_pack.op_unit == op_unit_t::sdu)
                                    {
                                        if(store_buffer->producer_is_full())
                                        {
                                            break;//stop rename immediately
                                        }
                                    }
                                }
                                
                                verify(decode_rename_fifo->pop(&rev_pack));
                                
                                send_pack.op_info[i].inst_common_info = rev_pack.inst_common_info;
                                send_pack.op_info[i].enable = rev_pack.enable;
                                send_pack.op_info[i].value = rev_pack.value;
                                send_pack.op_info[i].valid = rev_pack.valid;
                                send_pack.op_info[i].last_uop = rev_pack.last_uop;
                                send_pack.op_info[i].pc = rev_pack.pc;
                                send_pack.op_info[i].imm = rev_pack.imm;
                                send_pack.op_info[i].has_exception = rev_pack.has_exception;
                                send_pack.op_info[i].exception_id = rev_pack.exception_id;
                                send_pack.op_info[i].exception_value = rev_pack.exception_value;
                                send_pack.op_info[i].branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
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
                                
                                memcpy((void *)&send_pack.op_info[i].sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
                                
                                //generate rob items
                                if(rev_pack.enable)
                                {
                                    //start to map source registers
                                    if(rev_pack.rs1_need_map)
                                    {
                                        verify(speculative_rat->producer_get_phy_id(rev_pack.rs1, &send_pack.op_info[i].rs1_phy));
                                    }
        
                                    if(rev_pack.rs2_need_map)
                                    {
                                        verify(speculative_rat->producer_get_phy_id(rev_pack.rs2, &send_pack.op_info[i].rs2_phy));
                                    }
                                    
                                    phy_id_free_list->save(&rob_item.old_phy_id_free_list_rptr, &rob_item.old_phy_id_free_list_rstage);
                                    rob_item.inst_common_info = rev_pack.inst_common_info;
                                    
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
                                    rob_item.last_uop = rev_pack.last_uop;
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
                                    
                                    //only for debug
                                    switch(rev_pack.op_unit)
                                    {
                                        case op_unit_t::alu:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.alu_op);
                                            break;
            
                                        case op_unit_t::bru:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.bru_op);
                                            break;
            
                                        case op_unit_t::csr:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.csr_op);
                                            break;
            
                                        case op_unit_t::div:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.div_op);
                                            break;
            
                                        case op_unit_t::mul:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.mul_op);
                                            break;
            
                                        case op_unit_t::lu:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.lu_op);
                                            break;
        
                                        case op_unit_t::sau:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.sau_op);
                                            break;
        
                                        case op_unit_t::sdu:
                                            rob_item.sub_op = outenum(rev_pack.sub_op.sdu_op);
                                            break;
            
                                        default:
                                            rob_item.sub_op = "<Unsupported>";
                                            break;
                                    }
                                    
                                    //get rob id
                                    verify(rob->get_new_id(&send_pack.op_info[i].rob_id));
                                    verify(rob->get_new_id_stage(&send_pack.op_info[i].rob_id_stage));
                                    
                                    phy_id_free_list->save(&rob_item.new_phy_id_free_list_rptr, &rob_item.new_phy_id_free_list_rstage);
                                    auto old_load_queue_wptr = load_queue->producer_get_wptr();
                                    auto old_load_queue_wstage = load_queue->producer_get_wstage();
                                    auto old_store_buffer_wptr = store_buffer->producer_get_wptr();
                                    auto old_store_buffer_wstage = store_buffer->producer_get_wstage();
                                    
                                    if(rev_pack.enable && rev_pack.valid && !rev_pack.has_exception)
                                    {
                                        if(rev_pack.op_unit == op_unit_t::lu)
                                        {
                                            component::load_queue_item_t load_queue_item;
                                            rob_item.load_queue_id_valid = true;
                                            rob_item.load_queue_id = old_load_queue_wptr;
                                            send_pack.op_info[i].load_queue_id = old_load_queue_wptr;
                                            verify(load_queue->push(load_queue_item));
                                            load_queue->clear_replay_num(rob_item.load_queue_id);
                                        }
                                        else if(rev_pack.op_unit == op_unit_t::sdu)
                                        {
                                            component::store_buffer_item_t store_buffer_item;
                                            rob_item.store_buffer_id_valid = true;
                                            rob_item.store_buffer_id = old_store_buffer_wptr;
                                            store_buffer_item.inst_common_info = rev_pack.inst_common_info;
                                            store_buffer_item.rob_id = send_pack.op_info[i].rob_id;//set the age of sta instruction temporarily
                                            store_buffer_item.rob_id_stage = send_pack.op_info[i].rob_id_stage;
                                            verify(store_buffer->push(store_buffer_item));
                                            verify(store_buffer->producer_get_tail_id(&send_pack.op_info[i].store_buffer_id));
                                            store_buffer->write_addr(send_pack.op_info[i].store_buffer_id, 0, 0, false);
                                        }
                                    }
                                    
                                    //write to rob
                                    verify(rob->push(rob_item));
                                    
                                    //generate checkpoint items
                                    if(rev_pack.enable && rev_pack.valid && (rev_pack.branch_predictor_info_pack.predicted || (rev_pack.op_unit == op_unit_t::lu)))
                                    {
    #ifdef ENABLE_CHECKPOINT
                                        if(!checkpoint_buffer->producer_is_full())
                                        {
                                            component::checkpoint_t cp;
                                            speculative_rat->producer_save_to_checkpoint(cp);
                                            cp.old_phy_id_free_list_rptr = rob_item.old_phy_id_free_list_rptr;
                                            cp.old_phy_id_free_list_rstage = rob_item.old_phy_id_free_list_rstage;
                                            cp.new_phy_id_free_list_rptr = rob_item.new_phy_id_free_list_rptr;
                                            cp.new_phy_id_free_list_rstage = rob_item.new_phy_id_free_list_rstage;
                                            uint32_t new_checkpoint_wptr = checkpoint_buffer->producer_get_wptr();
                                            bool new_checkpoint_wstage = checkpoint_buffer->producer_get_wstage();
                                            checkpoint_buffer->static_get_next_id_stage(new_checkpoint_wptr, new_checkpoint_wstage, &new_checkpoint_wptr, &new_checkpoint_wstage);
                                            checkpoint_buffer->producer_get_tail_id_stage(&cp.old_checkpoint_buffer_wptr, &cp.old_checkpoint_buffer_wstage);
                                            cp.new_checkpoint_buffer_wptr = new_checkpoint_wptr;
                                            cp.new_checkpoint_buffer_wstage = new_checkpoint_wstage;
                                            cp.old_load_queue_wptr = old_load_queue_wptr;
                                            cp.old_load_queue_wstage = old_load_queue_wstage;
                                            cp.old_store_buffer_wptr = old_store_buffer_wptr;
                                            cp.old_store_buffer_wstage = old_store_buffer_wstage;
                                            verify(checkpoint_buffer->push(cp));
                                            send_pack.op_info[i].branch_predictor_info_pack.checkpoint_id_valid = true;
                                            verify(checkpoint_buffer->producer_get_tail_id(&send_pack.op_info[i].branch_predictor_info_pack.checkpoint_id));
                                        }
    #endif
                                    }
                                }
                                
                                if(ready_to_stop_rename)
                                {
                                    break;
                                }
                            }
                            else if(phy_id_free_list->producer_is_empty())
                            {
                                //phy_regfile_full_add();
                                
                                if(rob->producer_is_full())
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
            for(uint32_t i = 1;i < RENAME_WIDTH;i++)
            {
                verify_only(!send_pack.op_info[i].enable);
            }
        }
        
        auto assertion_found_fence = false;
        
        for(uint32_t i = 0;i < RENAME_WIDTH;i++)
        {
            if(send_pack.op_info[i].enable && send_pack.op_info[i].valid && !send_pack.op_info[i].has_exception)
            {
                verify_only((i == 0) || (send_pack.op_info[i].op_unit != op_unit_t::csr));
                verify_only(!assertion_found_fence || ((send_pack.op_info[i].op_unit != op_unit_t::lu) && (send_pack.op_info[i].op_unit != op_unit_t::sau) && (send_pack.op_info[i].op_unit != op_unit_t::sdu)));
                
                if(send_pack.op_info[i].op == op_t::fence)
                {
                    assertion_found_fence = true;
                }
            }
        }
        
        return feedback_pack;
    }
}