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
#include "cycle_model/pipeline/execute/alu.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/age_compare.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/execute.h"

namespace cycle_model::pipeline::execute
{
    alu::alu(global_inst *global, uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_alu_hdff, component::port<execute_wb_pack_t> *alu_wb_port) : tdb(TRACE_EXECUTE_ALU)
    {
        this->global = global;
        this->id = id;
        this->readreg_alu_hdff = readreg_alu_hdff;
        this->alu_wb_port = alu_wb_port;
        this->alu::reset();
    }
    
    void alu::reset()
    {
    
    }
    
    execute_feedback_channel_t alu::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        execute_wb_pack_t send_pack;
        
        if(!readreg_alu_hdff->is_empty() && !commit_feedback_pack.flush)
        {
            integer_readreg_execute_pack_t rev_pack;
            verify(readreg_alu_hdff->pop(&rev_pack));
            
            if(bru_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
            {
                goto exit;
            }
    
            if(sau_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
            {
                goto exit;
            }
            
            send_pack.inst_common_info = rev_pack.inst_common_info;
            send_pack.enable = rev_pack.enable;
            send_pack.value = rev_pack.value;
            send_pack.valid = rev_pack.valid;
            send_pack.last_uop = rev_pack.last_uop;
            send_pack.rob_id = rev_pack.rob_id;
            send_pack.rob_id_stage = rev_pack.rob_id_stage;
            send_pack.pc = rev_pack.pc;
            send_pack.imm = rev_pack.imm;
            send_pack.has_exception = rev_pack.has_exception;
            send_pack.exception_id = rev_pack.exception_id;
            send_pack.exception_value = rev_pack.exception_value;
            send_pack.branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
            
            send_pack.rs1 = rev_pack.rs1;
            send_pack.arg1_src = rev_pack.arg1_src;
            send_pack.rs1_need_map = rev_pack.rs1_need_map;
            send_pack.rs1_phy = rev_pack.rs1_phy;
            send_pack.src1_value = rev_pack.src1_value;
            
            send_pack.rs2 = rev_pack.rs2;
            send_pack.arg2_src = rev_pack.arg2_src;
            send_pack.rs2_need_map = rev_pack.rs2_need_map;
            send_pack.rs2_phy = rev_pack.rs2_phy;
            send_pack.src2_value = rev_pack.src2_value;
            
            send_pack.rd = rev_pack.rd;
            send_pack.rd_enable = rev_pack.rd_enable;
            send_pack.need_rename = rev_pack.need_rename;
            send_pack.rd_phy = rev_pack.rd_phy;
            
            send_pack.csr = rev_pack.csr;
            send_pack.op = rev_pack.op;
            send_pack.op_unit = rev_pack.op_unit;
            memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
            
            if(rev_pack.enable)
            {
                verify_only(rev_pack.op_unit == op_unit_t::alu);
            }
            
            if(rev_pack.enable && (!rev_pack.has_exception))
            {
                if(rev_pack.enable && (!rev_pack.valid))
                {
                    send_pack.has_exception = true;
                    send_pack.exception_id = riscv_exception_t::illegal_instruction;
                    send_pack.exception_value = 0;
                }
                
                if(rev_pack.enable && rev_pack.valid)
                {
                    switch(rev_pack.sub_op.alu_op)
                    {
                        case alu_op_t::add:
                            send_pack.rd_value = rev_pack.src1_value + rev_pack.src2_value;
                            break;
                        
                        case alu_op_t::_and:
                            send_pack.rd_value = rev_pack.src1_value & rev_pack.src2_value;
                            break;
                        
                        case alu_op_t::auipc:
                            send_pack.rd_value = rev_pack.imm + rev_pack.pc;
                            break;
                        
                        case alu_op_t::ebreak:
                            send_pack.rd_value = 0;
                            send_pack.has_exception = true;
                            send_pack.exception_id = riscv_exception_t::breakpoint;
                            send_pack.exception_value = 0;
                            break;
                        
                        case alu_op_t::ecall:
                            send_pack.rd_value = 0;
                            send_pack.has_exception = true;
                            send_pack.exception_id = riscv_exception_t::environment_call_from_m_mode;
                            send_pack.exception_value = 0;
                            break;
                        
                        case alu_op_t::fence:
                        case alu_op_t::fence_i:
                            send_pack.rd_value = 0;
                            break;
                        
                        case alu_op_t::lui:
                            send_pack.rd_value = rev_pack.imm;
                            break;
                        
                        case alu_op_t::_or:
                            send_pack.rd_value = rev_pack.src1_value | rev_pack.src2_value;
                            break;
                        
                        case alu_op_t::sll:
                            send_pack.rd_value = rev_pack.src1_value << (rev_pack.src2_value & 0x1f);
                            break;
                        
                        case alu_op_t::slt:
                            send_pack.rd_value = (((int32_t)rev_pack.src1_value) < ((int32_t)rev_pack.src2_value)) ? 1 : 0;
                            break;
                        
                        case alu_op_t::sltu:
                            send_pack.rd_value = (rev_pack.src1_value < rev_pack.src2_value) ? 1 : 0;
                            break;
                        
                        case alu_op_t::sra:
                            send_pack.rd_value = (uint32_t)(((int32_t)rev_pack.src1_value) >> (rev_pack.src2_value & 0x1f));
                            break;
                        
                        case alu_op_t::srl:
                            send_pack.rd_value = rev_pack.src1_value >> (rev_pack.src2_value & 0x1f);
                            break;
                        
                        case alu_op_t::sub:
                            send_pack.rd_value = rev_pack.src1_value - rev_pack.src2_value;
                            break;
                        
                        case alu_op_t::_xor:
                            send_pack.rd_value = rev_pack.src1_value ^ rev_pack.src2_value;
                            break;
                    }
                }
            }
        }
        
        exit:
        alu_wb_port->set(send_pack);

        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        return feedback_pack;
    }
}