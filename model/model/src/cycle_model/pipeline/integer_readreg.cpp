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
#include "config.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/integer_readreg.h"
#include "cycle_model/pipeline/integer_issue_readreg.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/regfile.h"

namespace pipeline
{
    integer_readreg::integer_readreg(component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port, component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff, component::regfile<uint32_t> *phy_regfile) : tdb(TRACE_INTEGER_READREG)
    {
        this->integer_issue_readreg_port = integer_issue_readreg_port;
        this->readreg_alu_hdff = readreg_alu_hdff;
        this->readreg_bru_hdff = readreg_bru_hdff;
        this->readreg_csr_hdff = readreg_csr_hdff;
        this->readreg_div_hdff = readreg_div_hdff;
        this->readreg_mul_hdff = readreg_mul_hdff;
        this->phy_regfile = phy_regfile;
        this->integer_readreg::reset();
    }
    
    void integer_readreg::reset()
    {
    
    }
    
    void integer_readreg::run(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        if(!commit_feedback_pack.flush)
        {
            auto rev_pack = this->integer_issue_readreg_port->get();
            
            for(auto i = 0;i < INTEGER_READREG_WIDTH;i++)
            {
                readreg_execute_pack_t send_pack;
                send_pack.enable = rev_pack.op_info[i].enable;
                send_pack.value = rev_pack.op_info[i].value;
                send_pack.valid = rev_pack.op_info[i].valid;
                send_pack.last_uop = rev_pack.op_info[i].last_uop;
                
                send_pack.rob_id = rev_pack.op_info[i].rob_id;
                send_pack.pc = rev_pack.op_info[i].pc;
                send_pack.imm = rev_pack.op_info[i].imm;
                send_pack.has_exception = rev_pack.op_info[i].has_exception;
                send_pack.exception_id = rev_pack.op_info[i].exception_id;
                send_pack.exception_value = rev_pack.op_info[i].exception_value;
                
                send_pack.rs1 = rev_pack.op_info[i].rs1;
                send_pack.arg1_src = rev_pack.op_info[i].arg1_src;
                send_pack.rs1_need_map = rev_pack.op_info[i].rs1_need_map;
                send_pack.rs1_phy = rev_pack.op_info[i].rs1_phy;
                send_pack.src1_value = 0;//for default value and the value of x0 register
                
                send_pack.rs2 = rev_pack.op_info[i].rs2;
                send_pack.arg2_src = rev_pack.op_info[i].arg2_src;
                send_pack.rs2_need_map = rev_pack.op_info[i].rs2_need_map;
                send_pack.rs2_phy = rev_pack.op_info[i].rs2_phy;
                send_pack.src2_value = 0;//for default value and the value of x0 register
                
                send_pack.rd = rev_pack.op_info[i].rd;
                send_pack.rd_enable = rev_pack.op_info[i].rd_enable;
                send_pack.need_rename = rev_pack.op_info[i].need_rename;
                send_pack.rd_phy = rev_pack.op_info[i].rd_phy;
                
                send_pack.csr = rev_pack.op_info[i].csr;
                send_pack.op = rev_pack.op_info[i].op;
                send_pack.op_unit = rev_pack.op_info[i].op_unit;
                memcpy(&send_pack.sub_op, &rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                
                if(rev_pack.op_info[i].enable)
                {
                    //get value from physical register file/execute feedback and wb feedback
                    if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                    {
                        if(rev_pack.op_info[i].rs1_need_map)
                        {
                            if(this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs1_phy))
                            {
                                send_pack.src1_value = this->phy_regfile->read(rev_pack.op_info[i].rs1_phy);
                            }
                            else
                            {
                                for(auto i = 0;i < EXECUTE_UNIT_NUM;i++)
                                {
                                    if(execute_feedback_pack.channel[i].enable && execute_feedback_pack.channel[i].phy_id == rev_pack.op_info[i].rs1_phy)
                                    {
                                        send_pack.src1_value = execute_feedback_pack.channel[i].value;
                                        break;
                                    }
                                    
                                    if(wb_feedback_pack.channel[i].enable && wb_feedback_pack.channel[i].phy_id == rev_pack.op_info[i].rs1_phy)
                                    {
                                        send_pack.src1_value = wb_feedback_pack.channel[i].value;
                                        break;
                                    }
                                }
                            }
                        }
        
                        if(rev_pack.op_info[i].rs2_need_map)
                        {
                            if(this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs2_phy))
                            {
                                send_pack.src2_value = this->phy_regfile->read(rev_pack.op_info[i].rs2_phy);
                            }
                            else
                            {
                                for(auto i = 0;i < EXECUTE_UNIT_NUM;i++)
                                {
                                    if(execute_feedback_pack.channel[i].enable && execute_feedback_pack.channel[i].phy_id == rev_pack.op_info[i].rs2_phy)
                                    {
                                        send_pack.src2_value = execute_feedback_pack.channel[i].value;
                                        break;
                                    }
                                    
                                    if(wb_feedback_pack.channel[i].enable && wb_feedback_pack.channel[i].phy_id == rev_pack.op_info[i].rs2_phy)
                                    {
                                        send_pack.src2_value = wb_feedback_pack.channel[i].value;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    //send pack
                    if(i == 0)
                    {
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                verify(this->readreg_alu_hdff[0]->push(send_pack));
                                break;
                                
                            case op_unit_t::bru:
                                verify(this->readreg_bru_hdff[0]->push(send_pack));
                                break;
                                
                            case op_unit_t::csr:
                                verify(this->readreg_csr_hdff[0]->push(send_pack));
                                break;
                            
                            case op_unit_t::mul:
                                verify(this->readreg_mul_hdff[0]->push(send_pack));
                                break;
                                
                            default:
                                verify(0);
                                break;
                        }
                    }
                    else
                    {
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                verify(this->readreg_alu_hdff[1]->push(send_pack));
                                break;
                                
                            case op_unit_t::div:
                                verify(this->readreg_div_hdff[0]->push(send_pack));
                                break;
                            
                            case op_unit_t::mul:
                                verify(this->readreg_mul_hdff[1]->push(send_pack));
                                break;
                                
                            default:
                                verify(0);
                                break;
                        }
                    }
                }
            }
        }
        else
        {
            for(auto i = 0;i < ALU_UNIT_NUM;i++)
            {
                this->readreg_alu_hdff[i]->flush();
            }
            
            for(auto i = 0;i < BRU_UNIT_NUM;i++)
            {
                this->readreg_bru_hdff[i]->flush();
            }
            
            for(auto i = 0;i < CSR_UNIT_NUM;i++)
            {
                this->readreg_csr_hdff[i]->flush();
            }
            
            for(auto i = 0;i < DIV_UNIT_NUM;i++)
            {
                this->readreg_div_hdff[i]->flush();
            }
            
            for(auto i = 0;i < MUL_UNIT_NUM;i++)
            {
                this->readreg_mul_hdff[i]->flush();
            }
        }
    }
}