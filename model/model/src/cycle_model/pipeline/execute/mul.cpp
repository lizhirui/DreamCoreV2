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
#include "cycle_model/pipeline/execute/mul.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"

namespace pipeline
{
    namespace execute
    {
        mul::mul(uint32_t id, component::handshake_dff<readreg_execute_pack_t> *readreg_mul_hdff, component::port<execute_wb_pack_t> *mul_wb_port) : tdb(TRACE_EXECUTE_MUL)
        {
            this->id = id;
            this->readreg_mul_hdff = readreg_mul_hdff;
            this->mul_wb_port = mul_wb_port;
            this->reset();
        }
        
        void mul::reset()
        {
        
        }
        
        void mul::run(commit_feedback_pack_t commit_feedback_pack)
        {
            execute_wb_pack_t send_pack;
            
            if(!readreg_mul_hdff->is_empty() && !commit_feedback_pack.flush)
            {
                readreg_execute_pack_t rev_pack;
                assert(readreg_mul_hdff->pop(&rev_pack));
                
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
                memcpy(&send_pack.sub_op, &rev_pack.sub_op, sizeof(rev_pack.sub_op));
                
                if(rev_pack.enable)
                {
                    assert(rev_pack.valid);
                    assert(rev_pack.op_unit == op_unit_t::mul);
                    
                    switch(rev_pack.sub_op.mul_op)
                    {
                        case mul_op_t::mul:
                            send_pack.rd_value = (uint32_t)(((uint64_t)(((int64_t)(int32_t)rev_pack.src1_value) * ((int64_t)(int32_t)rev_pack.src2_value))) & 0xffffffffull);
                            break;
                        
                        case mul_op_t::mulh:
                            send_pack.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)rev_pack.src1_value) * ((int64_t)(int32_t)rev_pack.src2_value))) >> 32) & 0xffffffffull);
                            break;
                        
                        case mul_op_t::mulhsu:
                            send_pack.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)rev_pack.src1_value) * ((uint64_t)rev_pack.src2_value))) >> 32) & 0xffffffffull);
                            break;
                        
                        case mul_op_t::mulhu:
                            send_pack.rd_value = (uint32_t)((((uint64_t)(((uint64_t)rev_pack.src1_value) * ((uint64_t)rev_pack.src2_value))) >> 32) & 0xffffffffull);
                            break;
                    }
                }
            }
            
            mul_wb_port->set(send_pack);
        }
    }
}