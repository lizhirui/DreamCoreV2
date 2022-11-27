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
#include "cycle_model/pipeline/decode.h"
#include "cycle_model/component/fifo.h"
#include "cycle_model/pipeline/fetch2_decode.h"
#include "cycle_model/pipeline/decode_rename.h"

#define MAX_UOP_NUM 2

namespace cycle_model::pipeline
{
    decode::decode(global_inst *global, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo, component::fifo<decode_rename_pack_t> *decode_rename_fifo) : tdb(TRACE_DECODE)
    {
        this->global = global;
        this->fetch2_decode_fifo = fetch2_decode_fifo;
        this->decode_rename_fifo = decode_rename_fifo;
        this->decode::reset();
    }
    
    void decode::reset()
    {
    
    }
    
    decode_feedback_pack_t decode::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        decode_feedback_pack_t feedback_pack;
        uint32_t send_uop_num = 0;
        
        feedback_pack.idle = this->fetch2_decode_fifo->customer_is_empty();
        
        if(!commit_feedback_pack.flush && !bru_feedback_pack.flush && !sau_feedback_pack.flush)
        {
            for(uint32_t i = 0;i < DECODE_WIDTH;i++)
            {
                if(send_uop_num == DECODE_WIDTH)
                {
                    break;
                }
                
                if(!this->fetch2_decode_fifo->customer_is_empty() && !this->decode_rename_fifo->producer_is_full())
                {
                    fetch2_decode_pack_t rev_pack;
                    decode_rename_pack_t op_info[MAX_UOP_NUM];
                    verify(this->fetch2_decode_fifo->customer_get_front(&rev_pack));
                    
                    if(rev_pack.enable)
                    {
                        auto op_data = rev_pack.value;
                        auto opcode = op_data & 0x7f;
                        auto rd = (op_data >> 7) & 0x1f;
                        auto funct3 = (op_data >> 12) & 0x07;
                        auto rs1 = (op_data >> 15) & 0x1f;
                        auto rs2 = (op_data >> 20) & 0x1f;
                        auto funct7 = (op_data >> 25) & 0x7f;
                        auto imm_i = (op_data >> 20) & 0xfff;
                        auto imm_s = (((op_data >> 7) & 0x1f)) | (((op_data >> 25) & 0x7f) << 5);
                        auto imm_b = (((op_data >> 8) & 0x0f) << 1) | (((op_data >> 25) & 0x3f) << 5) | (((op_data >> 7) & 0x01) << 11) | (((op_data >> 31) & 0x01) << 12);
                        auto imm_u = op_data & (~0xfff);
                        auto imm_j = (((op_data >> 12) & 0xff) << 12) | (((op_data >> 20) & 0x01) << 11) | (((op_data >> 21) & 0x3ff) << 1) | (((op_data >> 31) & 0x01) << 20);
                        
                        for(auto i = 0;i < MAX_UOP_NUM;i++)
                        {
                            op_info[i].enable = false;
                            op_info[i].value = 0;
                            op_info[i].valid = !rev_pack.has_exception;
                            op_info[i].last_uop = false;
                            op_info[i].pc = rev_pack.pc;
                            op_info[i].imm = 0;
                            op_info[i].has_exception = false;
                            op_info[i].exception_id = riscv_exception_t::instruction_address_misaligned;
                            op_info[i].exception_value = 0;
                            op_info[i].branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
                            op_info[i].rs1 = 0;
                            op_info[i].arg1_src = arg_src_t::reg;
                            op_info[i].rs1_need_map = false;
                            op_info[i].rs2 = 0;
                            op_info[i].rs2_need_map = false;
                            op_info[i].rd = 0;
                            op_info[i].rd_enable = false;
                            op_info[i].need_rename = false;
                            op_info[i].csr = 0;
                            op_info[i].op = op_t::add;
                            op_info[i].op_unit = op_unit_t::alu;
                            op_info[i].sub_op.alu_op = alu_op_t::add;
                        }
                        
                        op_info[0].enable = true;
                        
                        switch(opcode)
                        {
                            case 0x37://lui
                                op_info[0].op = op_t::lui;
                                op_info[0].op_unit = op_unit_t::alu;
                                op_info[0].sub_op.alu_op = alu_op_t::lui;
                                op_info[0].arg1_src = arg_src_t::disable;
                                op_info[0].arg2_src = arg_src_t::disable;
                                op_info[0].imm = imm_u;
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                break;
                            
                            case 0x17://auipc
                                op_info[0].op = op_t::auipc;
                                op_info[0].op_unit = op_unit_t::alu;
                                op_info[0].sub_op.alu_op = alu_op_t::auipc;
                                op_info[0].arg1_src = arg_src_t::disable;
                                op_info[0].arg2_src = arg_src_t::disable;
                                op_info[0].imm = imm_u;
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                break;
                            
                            case 0x6f://jal
                                op_info[0].op = op_t::jal;
                                op_info[0].op_unit = op_unit_t::bru;
                                op_info[0].sub_op.bru_op = bru_op_t::jal;
                                op_info[0].arg1_src = arg_src_t::disable;
                                op_info[0].arg2_src = arg_src_t::disable;
                                op_info[0].imm = sign_extend(imm_j, 21);
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                break;
                            
                            case 0x67://jalr
                                op_info[0].op = op_t::jalr;
                                op_info[0].op_unit = op_unit_t::bru;
                                op_info[0].sub_op.bru_op = bru_op_t::jalr;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::disable;
                                op_info[0].imm = sign_extend(imm_i, 12);
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                break;
                            
                            case 0x63://beq bne blt bge bltu bgeu
                                op_info[0].op_unit = op_unit_t::bru;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::reg;
                                op_info[0].rs2 = rs2;
                                op_info[0].imm = sign_extend(imm_b, 13);
                                
                                switch(funct3)
                                {
                                    case 0x0://beq
                                        op_info[0].op = op_t::beq;
                                        op_info[0].sub_op.bru_op = bru_op_t::beq;
                                        break;
                                    
                                    case 0x1://bne
                                        op_info[0].op = op_t::bne;
                                        op_info[0].sub_op.bru_op = bru_op_t::bne;
                                        break;
                                    
                                    case 0x4://blt
                                        op_info[0].op = op_t::blt;
                                        op_info[0].sub_op.bru_op = bru_op_t::blt;
                                        break;
                                    
                                    case 0x5://bge
                                        op_info[0].op = op_t::bge;
                                        op_info[0].sub_op.bru_op = bru_op_t::bge;
                                        break;
                                    
                                    case 0x6://bltu
                                        op_info[0].op = op_t::bltu;
                                        op_info[0].sub_op.bru_op = bru_op_t::bltu;
                                        break;
                                    
                                    case 0x7://bgeu
                                        op_info[0].op = op_t::bgeu;
                                        op_info[0].sub_op.bru_op = bru_op_t::bgeu;
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                
                                break;
                            
                            case 0x03://lb lh lw lbu lhu
                                op_info[0].op_unit = op_unit_t::lu;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::disable;
                                op_info[0].imm = sign_extend(imm_i, 12);
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                
                                switch(funct3)
                                {
                                    case 0x0://lb
                                        op_info[0].op = op_t::lb;
                                        op_info[0].sub_op.lu_op = lu_op_t::lb;
                                        break;
                                    
                                    case 0x1://lh
                                        op_info[0].op = op_t::lh;
                                        op_info[0].sub_op.lu_op = lu_op_t::lh;
                                        break;
                                    
                                    case 0x2://lw
                                        op_info[0].op = op_t::lw;
                                        op_info[0].sub_op.lu_op = lu_op_t::lw;
                                        break;
                                    
                                    case 0x4://lbu
                                        op_info[0].op = op_t::lbu;
                                        op_info[0].sub_op.lu_op = lu_op_t::lbu;
                                        break;
                                    
                                    case 0x5://lhu
                                        op_info[0].op = op_t::lhu;
                                        op_info[0].sub_op.lu_op = lu_op_t::lhu;
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                
                                break;
                            
                            case 0x23://sb sh sw
                            {
                                op_info[0].op_unit = op_unit_t::sdu;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::reg;
                                op_info[0].rs2 = rs2;
                                op_info[0].imm = sign_extend(imm_s, 12);
    
                                switch(funct3)
                                {
                                    case 0x0://sb
                                        op_info[0].op = op_t::sb;
                                        op_info[0].sub_op.sdu_op = sdu_op_t::sb;
                                        break;
        
                                    case 0x1://sh
                                        op_info[0].op = op_t::sh;
                                        op_info[0].sub_op.sdu_op = sdu_op_t::sh;
                                        break;
        
                                    case 0x2://sw
                                        op_info[0].op = op_t::sw;
                                        op_info[0].sub_op.sdu_op = sdu_op_t::sw;
                                        break;
        
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                
                                break;
                            }
                            
                            case 0x13://addi slti sltiu xori ori andi slli srli srai
                                op_info[0].op_unit = op_unit_t::alu;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::imm;
                                op_info[0].imm = sign_extend(imm_i, 12);
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                
                                switch(funct3)
                                {
                                    case 0x0://addi
                                        op_info[0].op = op_t::addi;
                                        op_info[0].sub_op.alu_op = alu_op_t::add;
                                        break;
                                    
                                    case 0x2://slti
                                        op_info[0].op = op_t::slti;
                                        op_info[0].sub_op.alu_op = alu_op_t::slt;
                                        break;
                                    
                                    case 0x3://sltiu
                                        op_info[0].op = op_t::sltiu;
                                        op_info[0].sub_op.alu_op = alu_op_t::sltu;
                                        break;
                                    
                                    case 0x4://xori
                                        op_info[0].op = op_t::xori;
                                        op_info[0].sub_op.alu_op = alu_op_t::_xor;
                                        break;
                                    
                                    case 0x6://ori
                                        op_info[0].op = op_t::ori;
                                        op_info[0].sub_op.alu_op = alu_op_t::_or;
                                        break;
                                    
                                    case 0x7://andi
                                        op_info[0].op = op_t::andi;
                                        op_info[0].sub_op.alu_op = alu_op_t::_and;
                                        break;
                                    
                                    case 0x1://slli
                                        if(funct7 == 0x00)//slli
                                        {
                                            op_info[0].op = op_t::slli;
                                            op_info[0].sub_op.alu_op = alu_op_t::sll;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x5://srli srai
                                        if(funct7 == 0x00)//srli
                                        {
                                            op_info[0].op = op_t::srli;
                                            op_info[0].sub_op.alu_op = alu_op_t::srl;
                                        }
                                        else if(funct7 == 0x20)//srai
                                        {
                                            op_info[0].op = op_t::srai;
                                            op_info[0].sub_op.alu_op = alu_op_t::sra;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                
                                break;
                            
                            case 0x33://add sub sll slt sltu xor srl sra or and mul mulh mulhsu mulhu div divu rem remu
                                op_info[0].op_unit = op_unit_t::alu;
                                op_info[0].arg1_src = arg_src_t::reg;
                                op_info[0].rs1 = rs1;
                                op_info[0].arg2_src = arg_src_t::reg;
                                op_info[0].rs2 = rs2;
                                op_info[0].rd = rd;
                                op_info[0].rd_enable = true;
                                
                                switch(funct3)
                                {
                                    case 0x0://add sub mul
                                        if(funct7 == 0x00)//add
                                        {
                                            op_info[0].op = op_t::add;
                                            op_info[0].sub_op.alu_op = alu_op_t::add;
                                        }
                                        else if(funct7 == 0x20)//sub
                                        {
                                            op_info[0].op = op_t::sub;
                                            op_info[0].sub_op.alu_op = alu_op_t::sub;
                                        }
                                        else if(funct7 == 0x01)//mul
                                        {
                                            op_info[0].op = op_t::mul;
                                            op_info[0].op_unit = op_unit_t::mul;
                                            op_info[0].sub_op.mul_op = mul_op_t::mul;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x1://sll mulh
                                        if(funct7 == 0x00)//sll
                                        {
                                            op_info[0].op = op_t::sll;
                                            op_info[0].sub_op.alu_op = alu_op_t::sll;
                                        }
                                        else if(funct7 == 0x01)//mulh
                                        {
                                            op_info[0].op = op_t::mulh;
                                            op_info[0].op_unit = op_unit_t::mul;
                                            op_info[0].sub_op.mul_op = mul_op_t::mulh;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x2://slt mulhsu
                                        if(funct7 == 0x00)//slt
                                        {
                                            op_info[0].op = op_t::slt;
                                            op_info[0].sub_op.alu_op = alu_op_t::slt;
                                        }
                                        else if(funct7 == 0x01)//mulhsu
                                        {
                                            op_info[0].op = op_t::mulhsu;
                                            op_info[0].op_unit = op_unit_t::mul;
                                            op_info[0].sub_op.mul_op = mul_op_t::mulhsu;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x3://sltu mulhu
                                        if(funct7 == 0x00)//sltu
                                        {
                                            op_info[0].op = op_t::sltu;
                                            op_info[0].sub_op.alu_op = alu_op_t::sltu;
                                        }
                                        else if(funct7 == 0x01)//mulhu
                                        {
                                            op_info[0].op = op_t::mulhu;
                                            op_info[0].op_unit = op_unit_t::mul;
                                            op_info[0].sub_op.mul_op = mul_op_t::mulhu;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x4://xor div
                                        if(funct7 == 0x00)//xor
                                        {
                                            op_info[0].op = op_t::_xor;
                                            op_info[0].sub_op.alu_op = alu_op_t::_xor;
                                        }
                                        else if(funct7 == 0x01)//div
                                        {
                                            op_info[0].op = op_t::div;
                                            op_info[0].op_unit = op_unit_t::div;
                                            op_info[0].sub_op.div_op = div_op_t::div;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x5://srl sra divu
                                        if(funct7 == 0x00)//srl
                                        {
                                            op_info[0].op = op_t::srl;
                                            op_info[0].sub_op.alu_op = alu_op_t::srl;
                                        }
                                        else if(funct7 == 0x20)//sra
                                        {
                                            op_info[0].op = op_t::sra;
                                            op_info[0].sub_op.alu_op = alu_op_t::sra;
                                        }
                                        else if(funct7 == 0x01)//divu
                                        {
                                            op_info[0].op = op_t::divu;
                                            op_info[0].op_unit = op_unit_t::div;
                                            op_info[0].sub_op.div_op = div_op_t::divu;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x6://or rem
                                        if(funct7 == 0x00)//or
                                        {
                                            op_info[0].op = op_t::_or;
                                            op_info[0].sub_op.alu_op = alu_op_t::_or;
                                        }
                                        else if(funct7 == 0x01)//rem
                                        {
                                            op_info[0].op = op_t::rem;
                                            op_info[0].op_unit = op_unit_t::div;
                                            op_info[0].sub_op.div_op = div_op_t::rem;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x7://and remu
                                        if(funct7 == 0x00)//and
                                        {
                                            op_info[0].op = op_t::_and;
                                            op_info[0].sub_op.alu_op = alu_op_t::_and;
                                        }
                                        else if(funct7 == 0x01)//remu
                                        {
                                            op_info[0].op = op_t::remu;
                                            op_info[0].op_unit = op_unit_t::div;
                                            op_info[0].sub_op.div_op = div_op_t::remu;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                
                                break;
                            
                            case 0x0f://fence fence.i
                                switch(funct3)
                                {
                                    case 0x0://fence
                                        if((rd == 0x00) && (rs1 == 0x00) && (((op_data >> 28) & 0x0f) == 0x00))//fence
                                        {
                                            op_info[0].op = op_t::fence;
                                            op_info[0].op_unit = op_unit_t::alu;
                                            op_info[0].sub_op.alu_op = alu_op_t::fence;
                                            op_info[0].arg1_src = arg_src_t::disable;
                                            op_info[0].arg2_src = arg_src_t::disable;
                                            op_info[0].imm = imm_i;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x1://fence.i
                                        if((rd == 0x00) && (rs1 == 0x00) && (imm_i == 0x00))//fence.i
                                        {
                                            op_info[0].op = op_t::fence_i;
                                            op_info[0].op_unit = op_unit_t::alu;
                                            op_info[0].sub_op.alu_op = alu_op_t::fence_i;
                                            op_info[0].arg1_src = arg_src_t::disable;
                                            op_info[0].arg2_src = arg_src_t::disable;
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                break;
                            
                            case 0x73://ecall ebreak csrrw csrrs csrrc csrrwi csrrsi csrrci mret
                                switch(funct3)
                                {
                                    case 0x0://ecall ebreak mret
                                        if((rd == 0x00) && (rs1 == 0x00))//ecall ebreak mret
                                        {
                                            switch(funct7)
                                            {
                                                case 0x00://ecall ebreak
                                                    switch(rs2)
                                                    {
                                                        case 0x00://ecall
                                                            op_info[0].op = op_t::ecall;
                                                            op_info[0].op_unit = op_unit_t::alu;
                                                            op_info[0].sub_op.alu_op = alu_op_t::ecall;
                                                            op_info[0].arg1_src = arg_src_t::disable;
                                                            op_info[0].arg2_src = arg_src_t::disable;
                                                            break;
                                                        
                                                        case 0x01://ebreak
                                                            op_info[0].op = op_t::ebreak;
                                                            op_info[0].op_unit = op_unit_t::alu;
                                                            op_info[0].sub_op.alu_op = alu_op_t::ebreak;
                                                            op_info[0].arg1_src = arg_src_t::disable;
                                                            op_info[0].arg2_src = arg_src_t::disable;
                                                            break;
                                                        
                                                        default://invalid
                                                            op_info[0].valid = false;
                                                            break;
                                                    }
                                                    
                                                    break;
                                                
                                                case 0x18://mret
                                                    switch(rs2)
                                                    {
                                                        case 0x02://mret
                                                            op_info[0].op = op_t::mret;
                                                            op_info[0].op_unit = op_unit_t::bru;
                                                            op_info[0].sub_op.bru_op = bru_op_t::mret;
                                                            op_info[0].arg1_src = arg_src_t::disable;
                                                            op_info[0].arg2_src = arg_src_t::disable;
                                                            break;
                                                        
                                                        default://invalid
                                                            op_info[0].valid = false;
                                                            break;
                                                    }
                                                    
                                                    break;
                                                
                                                default://invalid
                                                    op_info[0].valid = false;
                                                    break;
                                            }
                                        }
                                        else//invalid
                                        {
                                            op_info[0].valid = false;
                                        }
                                        
                                        break;
                                    
                                    case 0x1://csrrw
                                        op_info[0].op = op_t::csrrw;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrw;
                                        op_info[0].arg1_src = arg_src_t::reg;
                                        op_info[0].rs1 = rs1;
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    case 0x2://csrrs
                                        op_info[0].op = op_t::csrrs;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrs;
                                        op_info[0].arg1_src = arg_src_t::reg;
                                        op_info[0].rs1 = rs1;
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    case 0x3://csrrc
                                        op_info[0].op = op_t::csrrc;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrc;
                                        op_info[0].arg1_src = arg_src_t::reg;
                                        op_info[0].rs1 = rs1;
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    case 0x5://csrrwi
                                        op_info[0].op = op_t::csrrwi;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrw;
                                        op_info[0].arg1_src = arg_src_t::imm;
                                        op_info[0].imm = rs1;//zimm
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    case 0x6://csrrsi
                                        op_info[0].op = op_t::csrrsi;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrs;
                                        op_info[0].arg1_src = arg_src_t::imm;
                                        op_info[0].imm = rs1;//zimm
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    case 0x7://csrrci
                                        op_info[0].op = op_t::csrrci;
                                        op_info[0].op_unit = op_unit_t::csr;
                                        op_info[0].sub_op.csr_op = csr_op_t::csrrc;
                                        op_info[0].arg1_src = arg_src_t::imm;
                                        op_info[0].imm = rs1;//zimm
                                        op_info[0].arg2_src = arg_src_t::disable;
                                        op_info[0].csr = imm_i;
                                        op_info[0].rd = rd;
                                        op_info[0].rd_enable = true;
                                        break;
                                    
                                    default://invalid
                                        op_info[0].valid = false;
                                        break;
                                }
                                break;
                            
                            default://invalid
                                op_info[0].valid = false;
                                break;
                        }
                        
                        if(!op_info[0].valid)
                        {
                            op_info[0].op_unit = op_unit_t::alu;
                        }
                        
                        for(auto i = 0;i < MAX_UOP_NUM;i++)
                        {
                            op_info[i].rs1_need_map = (op_info[i].arg1_src == arg_src_t::reg) && (op_info[i].rs1 > 0);
                            op_info[i].rs2_need_map = (op_info[i].arg2_src == arg_src_t::reg) && (op_info[i].rs2 > 0);
                            op_info[i].need_rename = op_info[i].rd_enable && (op_info[i].rd > 0);
                            op_info[i].value = rev_pack.value;
                            op_info[i].has_exception = rev_pack.has_exception;
                            op_info[i].exception_id = rev_pack.exception_id;
                            op_info[i].exception_value = rev_pack.exception_value;
                        }
                    }
                    
                    uint32_t cur_uop_num = 0;
                    
                    for(auto i = 0;i < MAX_UOP_NUM;i++)
                    {
                        if(op_info[i].enable)
                        {
                            cur_uop_num++;
                        }
                    }
                    
                    verify_only(cur_uop_num > 0);
                    op_info[cur_uop_num - 1].last_uop = true;
                    
                    if(decode_rename_fifo->producer_get_free_space() < cur_uop_num)
                    {
                        break;//decode_rename_fifo is not enough to store all uops
                    }
    
                    verify(this->fetch2_decode_fifo->pop(&rev_pack));
                    
                    for(uint32_t i = 0;i < cur_uop_num;i++)
                    {
                        verify(decode_rename_fifo->push(op_info[i]));
                    }
                    
                    send_uop_num += cur_uop_num;
                }
                else if(!this->fetch2_decode_fifo->customer_is_empty() && this->decode_rename_fifo->producer_is_full())
                {
                    //decode_rename_fifo_full
                }
            }
        }
        else
        {
            decode_rename_fifo->flush();
        }
        
        return feedback_pack;
    }
}