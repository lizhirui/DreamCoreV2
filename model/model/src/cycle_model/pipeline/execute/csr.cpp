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
#include "cycle_model/pipeline/execute/csr.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/execute.h"

namespace cycle_model::pipeline::execute
{
    csr::csr(uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_csr_hdff, component::port<execute_wb_pack_t> *csr_wb_port, component::csrfile *csr_file) : tdb(TRACE_EXECUTE_CSR)
    {
        this->id = id;
        this->readreg_csr_hdff = readreg_csr_hdff;
        this->csr_wb_port = csr_wb_port;
        this->csr_file = csr_file;
        this->csr::reset();
    }
    
    void csr::reset()
    {
    
    }
    
    execute_feedback_channel_t csr::run(commit_feedback_pack_t commit_feedback_pack)
    {
        integer_readreg_execute_pack_t rev_pack;
        execute_wb_pack_t send_pack;
        uint32_t csr_value = 0;
        
        if(!commit_feedback_pack.flush)
        {
            if(!readreg_csr_hdff->is_empty())
            {
                verify(readreg_csr_hdff->pop(&rev_pack));
                
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
                    verify(rev_pack.valid);
                    verify(rev_pack.op_unit == op_unit_t::csr);
                    
                    if(rev_pack.need_rename && !csr_file->read(rev_pack.csr, &csr_value))
                    {
                        send_pack.has_exception = true;
                        send_pack.exception_id = riscv_exception_t::illegal_instruction;
                        send_pack.exception_value = rev_pack.value;
                    }
                    else
                    {
                        send_pack.rd_value = csr_value;
                        send_pack.csr_newvalue_valid = false;
                        
                        if(!((send_pack.arg1_src == arg_src_t::reg) && (!send_pack.rs1_need_map)))
                        {
                            switch(rev_pack.sub_op.csr_op)
                            {
                                case csr_op_t::csrrc:
                                    csr_value = csr_value & ~(rev_pack.src1_value);
                                    break;
                                
                                case csr_op_t::csrrs:
                                    csr_value = csr_value | rev_pack.src1_value;
                                    break;
                                
                                case csr_op_t::csrrw:
                                    csr_value = rev_pack.src1_value;
                                    break;
                            }
                            
                            if(!csr_file->write_check(rev_pack.csr, csr_value))
                            {
                                send_pack.has_exception = true;
                                send_pack.exception_id = riscv_exception_t::illegal_instruction;
                                send_pack.exception_value = rev_pack.value;
                            }
                            else
                            {
                                send_pack.csr_newvalue = csr_value;
                                send_pack.csr_newvalue_valid = true;
                            }
                        }
                    }
                }
            }
        }
        
        csr_wb_port->set(send_pack);

        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        return feedback_pack;
    }
}