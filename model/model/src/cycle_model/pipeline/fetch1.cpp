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

namespace cycle_model::pipeline
{
    fetch1::fetch1(global_inst *global, component::bus *bus, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::store_buffer *store_buffer, component::branch_predictor_set *branch_predictor_set, uint32_t init_pc) : tdb(TRACE_FETCH1)
    {
        this->global = global;
        this->bus = bus;
        this->fetch1_fetch2_port = fetch1_fetch2_port;
        this->store_buffer = store_buffer;
        this->branch_predictor_set = branch_predictor_set;
        this->init_pc = init_pc;
        this->fetch1::reset();
    }
    
    void fetch1::reset()
    {
        this->pc = init_pc;
        this->jump_wait = false;
    }
    
    void fetch1::run(const fetch2_feedback_pack_t &fetch2_feedback_pack, const decode_feedback_pack_t &decode_feedback_pack, const rename_feedback_pack_t &rename_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        fetch1_fetch2_pack_t send_pack;
        
        if(!commit_feedback_pack.flush && !bru_feedback_pack.flush && !sau_feedback_pack.flush && !fetch2_feedback_pack.pc_redirect)
        {
            if(jump_wait)
            {
                if(commit_feedback_pack.jump_enable)
                {
                    this->jump_wait = false;
                    
                    if(commit_feedback_pack.jump)
                    {
                        this->pc = commit_feedback_pack.jump_next_pc;
                    }
                }
            }
            else
            {
                uint32_t old_pc = this->pc;
                uint32_t instruction_value[FETCH_WIDTH];
                
                if(!fetch2_feedback_pack.stall && this->bus->get_instruction_value(instruction_value))
                {
                    for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                    {
                        uint32_t cur_pc = old_pc + i * 4;
                        bool has_exception = !component::bus::check_align(cur_pc, 4);
                        uint32_t opcode = has_exception ? 0 : instruction_value[i];
                        bool jump = ((opcode & 0x7f) == 0x6f) || ((opcode & 0x7f) == 0x67) || ((opcode & 0x7f) == 0x63) || (opcode == 0x30200073);
                        bool fence_i = ((opcode & 0x7f) == 0x0f) && (((opcode >> 12) & 0x07) == 0x01);
                        uint32_t rd = (opcode >> 7) & 0x1f;
                        uint32_t funct3 = (opcode >> 12) & 0x07;
                        uint32_t rs1 = (opcode >> 15) & 0x1f;
                        uint32_t rs2 = (opcode >> 20) & 0x1f;
                        uint32_t funct7 = (opcode >> 25) & 0x7f;
                        uint32_t imm_i = (opcode >> 20) & 0xfff;
                        uint32_t imm_s = (((opcode >> 7) & 0x1f)) | (((opcode >> 25) & 0x7f) << 5);
                        uint32_t imm_b = (((opcode >> 8) & 0x0f) << 1) | (((opcode >> 25) & 0x3f) << 5) | (((opcode >> 7) & 0x01) << 11) | (((opcode >> 31) & 0x01) << 12);
                        uint32_t imm_u = opcode & (~0xfff);
                        uint32_t imm_j = (((opcode >> 12) & 0xff) << 12) | (((opcode >> 20) & 0x01) << 11) | (((opcode >> 21) & 0x3ff) << 1) | (((opcode >> 31) & 0x01) << 20);
                        bool rd_is_link = (rd == 1) || (rd == 5);
                        bool rs1_is_link = (rs1 == 1) || (rs1 == 5);
    
                        if(fence_i && ((i != 0) || (!fetch2_feedback_pack.idle) || (!decode_feedback_pack.idle) || (!rename_feedback_pack.idle) || (!commit_feedback_pack.idle) || (!store_buffer->customer_is_empty())))
                        {
                            break;
                        }

                        send_pack.op_info[i].enable = true;
                        send_pack.op_info[i].value = opcode;
                        send_pack.op_info[i].pc = cur_pc;
                        send_pack.op_info[i].has_exception = has_exception;
                        send_pack.op_info[i].exception_id = !component::bus::check_align(cur_pc, 4) ? riscv_exception_t::instruction_address_misaligned : riscv_exception_t::instruction_access_fault;
                        send_pack.op_info[i].exception_value = cur_pc;
                        
                        component::branch_predictor_base::batch_predict(i, cur_pc, opcode);
                        
                        switch(opcode & 0x7f)
                        {
                            case 0x6f://jal
                                send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                send_pack.op_info[i].branch_predictor_info_pack.next_pc = cur_pc + sign_extend(imm_j, 21);
                                
                                if(rd_is_link)
                                {
                                    branch_predictor_set->main_ras.push_addr(cur_pc + 4);
                                }
                                
                                break;
                            
                            case 0x67://jalr
                                if(rd_is_link)
                                {
                                    if(rs1_is_link)
                                    {
                                        if(rs1 == rd)
                                        {
                                            //push
                                            send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                            send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                            send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->l0_btb.get_next_pc(i);
                                            branch_predictor_set->l0_btb.fill_info_pack(send_pack.op_info[i].branch_predictor_info_pack);
                                            branch_predictor_set->main_ras.push_addr(cur_pc + 4);
                                        }
                                        else
                                        {
                                            //pop, then push for coroutine context switch
                                            send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                            send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                            send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->main_ras.pop_addr();
                                            branch_predictor_set->main_ras.push_addr(cur_pc + 4);
                                        }
                                    }
                                    else
                                    {
                                        //push
                                        send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->l0_btb.get_next_pc(i);
                                        branch_predictor_set->l0_btb.fill_info_pack(send_pack.op_info[i].branch_predictor_info_pack);
                                        branch_predictor_set->main_ras.push_addr(cur_pc + 4);
                                    }
                                }
                                else
                                {
                                    if(rs1_is_link)
                                    {
                                        //pop
                                        send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->main_ras.pop_addr();
                                    }
                                    else
                                    {
                                        //none
                                        send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.jump = true;
                                        send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->l0_btb.get_next_pc(i);
                                        branch_predictor_set->l0_btb.fill_info_pack(send_pack.op_info[i].branch_predictor_info_pack);
                                    }
                                }
                                
                                break;
    
                            case 0x63://beq bne blt bge bltu bgeu
                                send_pack.op_info[i].branch_predictor_info_pack.predicted = true;
                                send_pack.op_info[i].branch_predictor_info_pack.jump = branch_predictor_set->bi_modal.is_jump(i);
                                send_pack.op_info[i].branch_predictor_info_pack.next_pc = branch_predictor_set->bi_modal.get_next_pc(i);
                                branch_predictor_set->bi_modal.fill_info_pack(send_pack.op_info[i].branch_predictor_info_pack);
                                branch_predictor_set->bi_mode.fill_info_pack_debug(cur_pc, send_pack.op_info[i].branch_predictor_info_pack);
        
                                switch(funct3)
                                {
                                    case 0x0://beq
                                    case 0x1://bne
                                    case 0x4://blt
                                    case 0x5://bge
                                    case 0x6://bltu
                                    case 0x7://bgeu
                                        break;
            
                                    default://invalid
                                        verify_only(false);
                                        break;
                                }
                                
                                break;
                                
                            default:
                                if(jump)
                                {
                                    this->jump_wait = true;
                                }
                                
                                break;
                        }
    
                        this->pc = cur_pc + 4;
    
                        if(fence_i)
                        {
                            break;
                        }
                        
                        if(jump)
                        {
                            if(send_pack.op_info[i].branch_predictor_info_pack.predicted && send_pack.op_info[i].branch_predictor_info_pack.jump && send_pack.op_info[i].branch_predictor_info_pack.next_pc != (cur_pc + 4))
                            {
                                this->pc = send_pack.op_info[i].branch_predictor_info_pack.next_pc;
                                break;
                            }
                            
                            if(!send_pack.op_info[i].branch_predictor_info_pack.predicted)
                            {
                                break;
                            }
                        }
                    }
                }
            }
    
            if(!fetch2_feedback_pack.stall)
            {
                //branch history update
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    if(send_pack.op_info[i].enable && send_pack.op_info[i].branch_predictor_info_pack.predicted && send_pack.op_info[i].branch_predictor_info_pack.condition_jump)
                    {
                        component::branch_predictor_base::batch_speculative_update_sync(send_pack.op_info[i].pc, send_pack.op_info[i].branch_predictor_info_pack.jump);
                    }
                }
                
                this->fetch1_fetch2_port->set(send_pack);
            }
        }
        else if(commit_feedback_pack.flush)
        {
            this->jump_wait = false;
            
            if(commit_feedback_pack.has_exception)
            {
                this->pc = commit_feedback_pack.exception_pc;
            }
            else if(commit_feedback_pack.jump_enable)
            {
                verify_only(commit_feedback_pack.jump);
                this->pc = commit_feedback_pack.jump_next_pc;
            }
    
            this->fetch1_fetch2_port->set(send_pack);
        }
        else if(bru_feedback_pack.flush)
        {
            this->jump_wait = false;
            this->pc = bru_feedback_pack.next_pc;
            this->fetch1_fetch2_port->set(send_pack);
        }
        else if(sau_feedback_pack.flush)
        {
            this->jump_wait = false;
            this->pc = sau_feedback_pack.next_pc;
            this->fetch1_fetch2_port->set(send_pack);
        }
        else if(fetch2_feedback_pack.pc_redirect)
        {
            this->jump_wait = false;
            this->pc = fetch2_feedback_pack.new_pc;
            this->fetch1_fetch2_port->set(send_pack);
        }
        
        this->bus->read_instruction(this->pc);
    }
    
    uint32_t fetch1::get_pc() const
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
        json t;
        
        t["pc"] = this->pc;
        t["jump_wait"] = this->jump_wait;
        return t;
    }
}