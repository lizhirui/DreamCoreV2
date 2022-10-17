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
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/rat.h"

namespace pipeline
{
    integer_readreg::integer_readreg(component::port<issue_readreg_pack_t> *issue_readreg_port, component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_lsu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff, component::regfile<uint32_t> *phy_regfile, component::rat *speculative_rat) : tdb(TRACE_READREG)
    {
        this->issue_readreg_port = issue_readreg_port;
        this->readreg_alu_hdff = readreg_alu_hdff;
        this->readreg_bru_hdff = readreg_bru_hdff;
        this->readreg_csr_hdff = readreg_csr_hdff;
        this->readreg_div_hdff = readreg_div_hdff;
        this->readreg_lsu_hdff = readreg_lsu_hdff;
        this->readreg_mul_hdff = readreg_mul_hdff;
        this->phy_regfile = phy_regfile;
        this->speculative_rat = speculative_rat;
        this->reset();
    }
    
    void integer_readreg::reset()
    {
    
    }
    
    readreg_feedback_pack_t integer_readreg::run(commit_feedback_pack_t commit_feedback_pack)
    {
        issue_readreg_pack_t rev_pack;
        readreg_feedback_pack_t feedback_pack;
        
        for(auto i = 0;i < READREG_WIDTH;i++)
        {
            feedback_pack.issue_id_valid[i] = false;
            feedback_pack.remove[i] = false;
            feedback_pack.issue_id[i] = 0;
        }
        
        if(!commit_feedback_pack.flush)
        {
            rev_pack = this->issue_readreg_port->get();
            
            for(auto i = 0;i < READREG_WIDTH;i++)
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
                
                bool need_replay_src1 = false;
                bool need_replay_src2 = false;
                bool need_replay_execute_unit = false;
                
                if(rev_pack.op_info[i].enable)
                {
                    if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                    {
                        if(rev_pack.op_info[i].rs1_need_map)
                        {
                            send_pack.src1_value = this->phy_regfile->read(rev_pack.op_info[i].rs1_phy);
    
                            if(!phy_regfile->read_data_valid(rev_pack.op_info[i].rs1_phy))
                            {
                                need_replay_src1 = true;
                            }
                        }
        
                        if(rev_pack.op_info[i].rs2_need_map)
                        {
                            send_pack.src2_value = this->phy_regfile->read(rev_pack.op_info[i].rs2_phy);
    
                            if(!phy_regfile->read_data_valid(rev_pack.op_info[i].rs2_phy))
                            {
                                need_replay_src2 = true;
                            }
                        }
                    }
                    
                    if(!(need_replay_src1 || need_replay_src2))
                    {
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                verify(rev_pack.op_info[i].unit_id < ALU_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_alu_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
    
                            case op_unit_t::bru:
                                verify(rev_pack.op_info[i].unit_id < BRU_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_bru_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
                            
                            case op_unit_t::csr:
                                verify(rev_pack.op_info[i].unit_id < CSR_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_csr_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
                            
                            case op_unit_t::div:
                                verify(rev_pack.op_info[i].unit_id < DIV_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_div_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
                            
                            case op_unit_t::lsu:
                                verify(rev_pack.op_info[i].unit_id < LSU_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_lsu_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
                            
                            case op_unit_t::mul:
                                verify(rev_pack.op_info[i].unit_id < MUL_UNIT_NUM);
                                need_replay_execute_unit = !this->readreg_mul_hdff[rev_pack.op_info[i].unit_id]->push(send_pack);
                                break;
                            
                            default:
                                verify(0);
                                break;
                        }
                    }
                }
                
                auto need_replay = need_replay_src1 || need_replay_src2 || need_replay_execute_unit;
                
                if(rev_pack.op_info[i].support_replay)
                {
                    feedback_pack.issue_id_valid[i] = true;
                    feedback_pack.remove[i] = !need_replay;
                    feedback_pack.issue_id[i] = rev_pack.op_info[i].issue_id;
                }
                else
                {
                    verify(!need_replay);
                    feedback_pack.issue_id_valid[i] = false;
                    feedback_pack.remove[i] = false;
                    feedback_pack.issue_id[i] = 0;
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
            
            for(auto i = 0;i < LSU_UNIT_NUM;i++)
            {
                this->readreg_lsu_hdff[i]->flush();
            }
            
            for(auto i = 0;i < MUL_UNIT_NUM;i++)
            {
                this->readreg_mul_hdff[i]->flush();
            }
        }
        
        return feedback_pack;
    }
}
