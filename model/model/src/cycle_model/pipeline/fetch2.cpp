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
        this->busy = false;
        rev_pack = fetch1_fetch2_pack_t();
    }
    
    void fetch2::reset()
    {
        busy = false;
        rev_pack = fetch1_fetch2_pack_t();
    }
    
    fetch2_feedback_pack_t fetch2::run(commit_feedback_pack_t commit_feedback_pack)
    {
        fetch2_decode_pack_t send_pack;
        fetch2_feedback_pack_t feedback_pack;
    
        feedback_pack.idle = true;//set idle state temporarily
        
        if(!commit_feedback_pack.flush)
        {
            //if no hold rev_pack exists, get a new instruction pack
            if(!busy)
            {
                rev_pack = fetch1_fetch2_port->get();
            }
            
            busy = false;//set not busy state temporarily
            uint32_t fail_index = 0;//busy item index
            
            for(auto i = 0;i < FETCH_WIDTH;i++)
            {
                send_pack.enable = rev_pack.op_info[i].enable;
                send_pack.pc = rev_pack.op_info[i].pc;
                send_pack.value = rev_pack.op_info[i].value;
                send_pack.has_exception = rev_pack.op_info[i].has_exception;
                send_pack.exception_id = rev_pack.op_info[i].exception_id;
                send_pack.exception_value = rev_pack.op_info[i].exception_value;
                
                if(rev_pack.op_info[i].enable)
                {
                    //cancel idle state
                    feedback_pack.idle = false;
                    
                    if(!fetch2_decode_fifo->push(send_pack))
                    {
                        //fifo is full, some data must be saved
                        busy = true;
                        fail_index = i;
                        break;
                    }
                }
            }
            
            //let remain instructions keep right alignment
            for(auto i = fail_index;i < FETCH_WIDTH;i++)
            {
                rev_pack.op_info[i - fail_index] = rev_pack.op_info[i];
                rev_pack.op_info[i].enable = false;
            }
        }
    
        feedback_pack.stall = busy;//if busy state is asserted, stall signal must be asserted too
        return feedback_pack;
    }
}