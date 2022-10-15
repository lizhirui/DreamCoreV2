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
#include "cycle_model/pipeline/fetch1.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/slave/memory.h"
#include "cycle_model/pipeline/fetch1_fetch2.h"

namespace pipeline
{
    fetch1::fetch1(component::bus *bus, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::store_buffer *store_buffer, uint32_t init_pc) : tdb(TRACE_FETCH1)
    {
        this->bus = bus;
        this->fetch1_fetch2_port = fetch1_fetch2_port;
        this->store_buffer = store_buffer;
        this->init_pc = init_pc;
        this->pc = init_pc;
        this->jump_wait = false;
    }
    
    void fetch1::reset()
    {
        this->pc = init_pc;
        this->jump_wait = false;
    }
    
    void fetch1::run(fetch2_feedback_pack_t fetch2_feedback_pack, decode_feedback_pack_t decode_feedback_pack, rename_feedback_pack_t rename_feedback_pack, commit_feedback_pack_t commit_feedback_pack)
    {
        fetch1_fetch2_pack_t send_pack;
        
        if(!commit_feedback_pack.flush)
        {
            if(jump_wait)
            {
                if(commit_feedback_pack.jump_enable)
                {
                    this->jump_wait = false;
                    
                    if(commit_feedback_pack.jump)
                    {
                        this->pc = commit_feedback_pack.next_pc;
                    }
                }
            }
            else
            {
                uint32_t old_pc = this->pc;
                uint32_t instruction_value[FETCH_WIDTH];
                
                if(!fetch2_feedback_pack.stall && this->bus->get_instruction_value(instruction_value))
                {
                    for(auto i = 0;i < FETCH_WIDTH;i++)
                    {
                        uint32_t cur_pc = old_pc + i * 4;
                        bool has_exception = !bus->check_align(cur_pc, 4);
                        uint32_t opcode = has_exception ? 0 : instruction_value[i];
                        bool jump = ((opcode & 0x7f) == 0x6f) || ((opcode & 0x7f) == 0x67) || ((opcode & 0x7f) == 0x63) || (opcode == 0x30200073);
                        bool fence_i = ((opcode & 0x7f) == 0x0f) && (((opcode >> 12) & 0x07) == 0x01);
    
                        if(fence_i && ((i != 0) || (!fetch2_feedback_pack.idle) || (!decode_feedback_pack.idle) || (!rename_feedback_pack.idle) || commit_feedback_pack.idle || (!store_buffer->customer_is_empty())))
                        {
                            break;
                        }
    
                        if(jump)
                        {
                            uint32_t jump_next_pc = 0;
                            bool jump_result = false;
                            this->jump_wait = true;
                            this->pc = cur_pc + 4;
                        }
                        else
                        {
                            this->pc = cur_pc + 4;
                        }

                        send_pack.op_info[i].enable = true;
                        send_pack.op_info[i].value = opcode;
                        send_pack.op_info[i].pc = cur_pc;
                        send_pack.op_info[i].has_exception = has_exception;
                        send_pack.op_info[i].exception_id = !bus->check_align(cur_pc, 4) ? riscv_exception_t::instruction_address_misaligned : riscv_exception_t::instruction_access_fault;
                        send_pack.op_info[i].exception_value = cur_pc;
                        
                        if(jump)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            this->jump_wait = false;
            
            if(commit_feedback_pack.has_exception)
            {
                this->pc = commit_feedback_pack.exception_pc;
            }
            else if(commit_feedback_pack.jump_enable)
            {
                this->pc = commit_feedback_pack.next_pc;
            }
        }
    
        this->fetch1_fetch2_port->set(send_pack);
        this->bus->read_instruction(this->pc);
    }
    
    uint32_t fetch1::get_pc()
    {
        return this->pc;
    }
    
    void fetch1::print(std::string indent)
    {
        std::cout << indent << "pc = 0x" << fillzero(8) << outhex(this->pc);
        std::cout << "    jump_wait = " << outbool(this->jump_wait) << std::endl;
    }
    
    json fetch1::get_json()
    {
        json j;
        j["pc"] = this->pc;
        j["jump_wait"] = this->jump_wait;
        return j;
    }
}