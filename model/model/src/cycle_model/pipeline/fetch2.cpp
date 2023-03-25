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
#include "cycle_model/pipeline/fetch2.h"
#include "cycle_model/component/fifo.h"
#include "cycle_model/component/port.h"
#include "cycle_model/pipeline/fetch1_fetch2.h"
#include "cycle_model/pipeline/fetch2_decode.h"

namespace cycle_model::pipeline
{
    fetch2::fetch2(global_inst *global, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo, component::branch_predictor_set *branch_predictor_set) : tdb(TRACE_FETCH2)
    {
        this->global = global;
        this->fetch1_fetch2_port = fetch1_fetch2_port;
        this->fetch2_decode_fifo = fetch2_decode_fifo;
        this->branch_predictor_set = branch_predictor_set;
        this->fetch2::reset();
    }
    
    void fetch2::reset()
    {
        this->busy = false;
        this->rev_pack = fetch1_fetch2_pack_t();
    }
    
    fetch2_feedback_pack_t fetch2::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        fetch2_decode_pack_t send_pack;
        fetch2_feedback_pack_t feedback_pack;
    
        feedback_pack.idle = true;//set idle state temporarily
        feedback_pack.stall = this->busy;//if busy state is asserted, stall signal must be asserted too
        feedback_pack.pc_redirect = false;
        feedback_pack.new_pc = 0;
        
        if(!commit_feedback_pack.flush && !bru_feedback_pack.flush && !sau_feedback_pack.flush)
        {
            //if no hold rev_pack exists, get a new instruction pack
            if(!this->busy)
            {
                this->rev_pack = fetch1_fetch2_port->get();
                uint32_t last_valid_item = FETCH_WIDTH;
                
                //level2 branch prediction
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    if(this->rev_pack.op_info[i].enable && this->rev_pack.op_info[i].branch_predictor_info_pack.predicted)
                    {
                        if(this->rev_pack.op_info[i].branch_predictor_info_pack.uncondition_indirect_jump)
                        {
                            /*auto new_next_pc = branch_predictor_set->l1_btb.get_next_pc(i);
                            
                            if(new_next_pc != this->rev_pack.op_info[i].branch_predictor_info_pack.next_pc)
                            {
                                branch_predictor_set->l0_btb.update_sync(this->rev_pack.op_info[i].pc, true, new_next_pc, false, this->rev_pack.op_info[i].branch_predictor_info_pack);
                                this->rev_pack.op_info[i].branch_predictor_info_pack.next_pc = new_next_pc;
                                feedback_pack.pc_redirect = true;
                                feedback_pack.new_pc = new_next_pc;
                                last_valid_item = i;
                                break;
                            }*/
                        }
                        else if(this->rev_pack.op_info[i].branch_predictor_info_pack.condition_jump)
                        {
                            auto new_next_pc = branch_predictor_set->bi_mode.get_next_pc(i);
                            auto new_jump = branch_predictor_set->bi_mode.is_jump(i);
                            
                            if(new_jump != this->rev_pack.op_info[i].branch_predictor_info_pack.jump || new_next_pc != this->rev_pack.op_info[i].branch_predictor_info_pack.next_pc)
                            {
                                branch_predictor_set->bi_modal.update_sync(this->rev_pack.op_info[i].pc, new_jump, new_next_pc, false, this->rev_pack.op_info[i].branch_predictor_info_pack);
                                component::branch_predictor_base::batch_restore_sync(this->rev_pack.op_info[i].branch_predictor_info_pack);
                                this->rev_pack.op_info[i].branch_predictor_info_pack.jump = new_jump;
                                this->rev_pack.op_info[i].branch_predictor_info_pack.next_pc = new_next_pc;
                                feedback_pack.pc_redirect = true;
                                feedback_pack.new_pc = new_next_pc;
                                last_valid_item = i;
                                break;
                            }
                        }
                    }
                }
                
                //invalid instructions after jump
                for(uint32_t i = last_valid_item + 1;i < FETCH_WIDTH;i++)
                {
                    this->rev_pack.op_info[i].enable = false;
                }
                
                if(feedback_pack.pc_redirect)
                {
                    //branch history update
                    for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                    {
                        if(this->rev_pack.op_info[i].enable && this->rev_pack.op_info[i].branch_predictor_info_pack.predicted && this->rev_pack.op_info[i].branch_predictor_info_pack.condition_jump)
                        {
                            component::branch_predictor_base::batch_speculative_update_sync(this->rev_pack.op_info[i].pc, this->rev_pack.op_info[i].branch_predictor_info_pack.jump);
                        }
                    }
                }
            }
            
            this->busy = false;//set not busy state temporarily
            uint32_t fail_index = 0;//busy item index
            
            for(uint32_t i = 0;i < FETCH_WIDTH;i++)
            {
                send_pack.inst_common_info = this->rev_pack.op_info[i].inst_common_info;
                send_pack.enable = this->rev_pack.op_info[i].enable;
                send_pack.pc = this->rev_pack.op_info[i].pc;
                send_pack.value = this->rev_pack.op_info[i].value;
                send_pack.has_exception = this->rev_pack.op_info[i].has_exception;
                send_pack.exception_id = this->rev_pack.op_info[i].exception_id;
                send_pack.exception_value = this->rev_pack.op_info[i].exception_value;
                send_pack.branch_predictor_info_pack = this->rev_pack.op_info[i].branch_predictor_info_pack;
                
                if(this->rev_pack.op_info[i].enable)
                {
                    //cancel idle state
                    feedback_pack.idle = false;
                    
                    if(!fetch2_decode_fifo->push(send_pack))
                    {
                        //fifo is full, some data must be saved
                        this->busy = true;
                        fail_index = i;
                        break;
                    }
                }
            }
            
            //let remain instructions keep right alignment
            for(uint32_t i = 0;i < FETCH_WIDTH;i++)
            {
                if((i + fail_index) < FETCH_WIDTH)
                {
                    this->rev_pack.op_info[i] = this->rev_pack.op_info[i + fail_index];
                }
                else
                {
                    this->rev_pack.op_info[i].enable = false;
                }
            }
        }
        else
        {
            this->busy = false;
            fetch2_decode_fifo->flush();
        }
        
        return feedback_pack;
    }
    
    json fetch2::get_json()
    {
        json t;
        
        t["busy"] = this->busy;
        return t;
    }
}