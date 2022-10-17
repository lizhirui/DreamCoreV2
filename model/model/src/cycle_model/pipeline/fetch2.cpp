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

namespace pipeline
{
    fetch2::fetch2(component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo) : tdb(TRACE_FETCH2)
    {
        this->fetch1_fetch2_port = fetch1_fetch2_port;
        this->fetch2_decode_fifo = fetch2_decode_fifo;
        this->fetch2::reset();
    }
    
    void fetch2::reset()
    {
        this->busy = false;
        this->rev_pack = fetch1_fetch2_pack_t();
    }
    
    fetch2_feedback_pack_t fetch2::run(const commit_feedback_pack_t &commit_feedback_pack)
    {
        fetch2_decode_pack_t send_pack;
        fetch2_feedback_pack_t feedback_pack;
    
        feedback_pack.idle = true;//set idle state temporarily
        feedback_pack.stall = this->busy;//if busy state is asserted, stall signal must be asserted too
        
        if(!commit_feedback_pack.flush)
        {
            //if no hold rev_pack exists, get a new instruction pack
            if(!this->busy)
            {
                this->rev_pack = fetch1_fetch2_port->get();
            }
            
            this->busy = false;//set not busy state temporarily
            uint32_t fail_index = 0;//busy item index
            
            for(auto i = 0;i < FETCH_WIDTH;i++)
            {
                send_pack.enable = this->rev_pack.op_info[i].enable;
                send_pack.pc = this->rev_pack.op_info[i].pc;
                send_pack.value = this->rev_pack.op_info[i].value;
                send_pack.has_exception = this->rev_pack.op_info[i].has_exception;
                send_pack.exception_id = this->rev_pack.op_info[i].exception_id;
                send_pack.exception_value = this->rev_pack.op_info[i].exception_value;
                
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
            for(auto i = fail_index;i < FETCH_WIDTH;i++)
            {
                this->rev_pack.op_info[i - fail_index] = this->rev_pack.op_info[i];
                this->rev_pack.op_info[i].enable = false;
            }
        }
        else
        {
            this->busy = false;
            fetch2_decode_fifo->flush();
        }
        
        return feedback_pack;
    }
}