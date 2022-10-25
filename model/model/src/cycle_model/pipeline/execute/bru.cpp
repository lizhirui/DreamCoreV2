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
#include "cycle_model/pipeline/execute/bru.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/execute.h"

namespace cycle_model::pipeline::execute
{
    bru::bru(uint32_t id,component::handshake_dff<integer_readreg_execute_pack_t> *readreg_bru_hdff, component::port<execute_wb_pack_t> *bru_wb_port, component::csrfile *csr_file) : tdb(TRACE_EXECUTE_BRU)
    {
        this->id = id;
        this->readreg_bru_hdff = readreg_bru_hdff;
        this->bru_wb_port = bru_wb_port;
        this->csr_file = csr_file;
        this->bru::reset();
    }
    
    void bru::reset()
    {
    
    }
    
    execute_feedback_channel_t bru::run(commit_feedback_pack_t commit_feedback_pack)
    {
        execute_wb_pack_t send_pack;
        
        if(!readreg_bru_hdff->is_empty() && !commit_feedback_pack.flush)
        {
            integer_readreg_execute_pack_t rev_pack;
            verify(readreg_bru_hdff->pop(&rev_pack));
            
            send_pack.enable = rev_pack.enable;
            send_pack.value = rev_pack.value;
            send_pack.valid = rev_pack.valid;
            send_pack.last_uop = rev_pack.last_uop;
            send_pack.rob_id = rev_pack.rob_id;
            send_pack.pc = rev_pack.pc;
            send_pack.imm = rev_pack.imm;
            send_pack.has_exception = rev_pack.has_exception;
            send_pack.exception_id = rev_pack.exception_id;
            send_pack.exception_value = rev_pack.exception_value;
            
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
            send_pack.bru_next_pc = rev_pack.pc + rev_pack.imm;
            
            if(rev_pack.enable)
            {
                verify_only(rev_pack.valid);
                verify_only(rev_pack.op_unit == op_unit_t::bru);
                
                switch(rev_pack.sub_op.bru_op)
                {
                    case bru_op_t::beq:
                        send_pack.bru_jump = rev_pack.src1_value == rev_pack.src2_value;
                        break;
                    
                    case bru_op_t::bge:
                        send_pack.bru_jump = ((int32_t)rev_pack.src1_value) >= ((int32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bgeu:
                        send_pack.bru_jump = ((uint32_t)rev_pack.src1_value) >= ((uint32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::blt:
                        send_pack.bru_jump = ((int32_t)rev_pack.src1_value) < ((int32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bltu:
                        send_pack.bru_jump = ((uint32_t)rev_pack.src1_value) < ((uint32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bne:
                        send_pack.bru_jump = rev_pack.src1_value != rev_pack.src2_value;
                        break;
                    
                    case bru_op_t::jal:
                        send_pack.rd_value = rev_pack.pc + 4;
                        send_pack.bru_jump = true;
                        break;
                    
                    case bru_op_t::jalr:
                        send_pack.rd_value = rev_pack.pc + 4;
                        send_pack.bru_jump = true;
                        send_pack.bru_next_pc = (rev_pack.imm + rev_pack.src1_value) & (~0x01);
                        break;
                    
                    case bru_op_t::mret:
                        send_pack.bru_jump = true;
                        send_pack.bru_next_pc = csr_file->read_sys(CSR_MEPC);
                        break;
                }
            }
            
            if(!send_pack.bru_jump)
            {
                send_pack.bru_next_pc = rev_pack.pc + 4;
            }
        }
        
        bru_wb_port->set(send_pack);

        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        return feedback_pack;
    }
}