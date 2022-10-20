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
#include "cycle_model/pipeline/execute/div.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"

namespace cycle_model::pipeline::execute
{
    div::div(uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_div_hdff, component::port<execute_wb_pack_t> *div_wb_port) : tdb(TRACE_EXECUTE_DIV)
    {
        this->id = id;
        this->readreg_div_hdff = readreg_div_hdff;
        this->div_wb_port = div_wb_port;
        this->div::reset();
    }
    
    void div::reset()
    {
        this->send_pack = execute_wb_pack_t();
        this->busy = false;
        this->progress = 0;
    }
    
    execute_feedback_channel_t div::run(commit_feedback_pack_t commit_feedback_pack)
    {
        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = false;
        feedback_pack.phy_id = 0;
        feedback_pack.value = 0;
        
        if(!commit_feedback_pack.flush)
        {
            if(!this->busy)
            {
                if(!readreg_div_hdff->is_empty())
                {
                    integer_readreg_execute_pack_t rev_pack;
                    verify(readreg_div_hdff->pop(&rev_pack));
                    
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
                    auto overflow = (rev_pack.src1_value == 0x80000000) && (rev_pack.src2_value == 0xFFFFFFFF);
                    
                    if(rev_pack.enable)
                    {
                        verify(rev_pack.valid);
                        verify(rev_pack.op_unit == op_unit_t::div);
                        
                        switch(rev_pack.sub_op.div_op)
                        {
                            case div_op_t::div:
                                send_pack.rd_value = (rev_pack.src2_value == 0) ? 0xFFFFFFFF : overflow ? 0x80000000 : ((uint32_t)(((int32_t)rev_pack.src1_value) / ((int32_t)rev_pack.src2_value)));
                                break;
                            
                            case div_op_t::divu:
                                send_pack.rd_value = (rev_pack.src2_value == 0) ? 0xFFFFFFFF : ((uint32_t)(((uint32_t)rev_pack.src1_value) / ((uint32_t)rev_pack.src2_value)));
                                break;
                            
                            case div_op_t::rem:
                                send_pack.rd_value = (rev_pack.src2_value == 0) ? rev_pack.src1_value : overflow ? 0 : ((uint32_t)(((int32_t)rev_pack.src1_value) % ((int32_t)rev_pack.src2_value)));
                                break;
                            
                            case div_op_t::remu:
                                send_pack.rd_value = (rev_pack.src2_value == 0) ? rev_pack.src1_value : (((uint32_t)((int32_t)rev_pack.src1_value) % ((int32_t)rev_pack.src2_value)));
                                break;
                        }
                    }
                    
                    this->busy = true;
                    this->progress = DIV_LATENCY - 2;
                }

                div_wb_port->set(execute_wb_pack_t());
            }
            else
            {
                if(this->progress == 0)
                {
                    div_wb_port->set(send_pack);
                    this->busy = false;
                    feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
                    feedback_pack.phy_id = send_pack.rd_phy;
                    feedback_pack.value = send_pack.rd_value;
                }
                else
                {
                    this->progress--;
                    div_wb_port->set(execute_wb_pack_t());
                }
            }
        }
        else
        {
            div_wb_port->set(execute_wb_pack_t());
        }
        
        return feedback_pack;
    }
    
    json div::get_json()
    {
        json t;
        
        t["progress"] = this->progress;
        t["busy"] = this->busy;
        return t;
    }
}