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
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/commit.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/age_compare.h"

namespace cycle_model::pipeline
{
    integer_readreg::integer_readreg(global_inst *global, component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port, component::handshake_dff<integer_readreg_execute_pack_t> **readreg_alu_hdff, component::handshake_dff<integer_readreg_execute_pack_t> **readreg_bru_hdff, component::handshake_dff<integer_readreg_execute_pack_t> **readreg_csr_hdff, component::handshake_dff<integer_readreg_execute_pack_t> **readreg_div_hdff, component::handshake_dff<integer_readreg_execute_pack_t> **readreg_mul_hdff, component::regfile<uint32_t> *phy_regfile) : tdb(TRACE_INTEGER_READREG)
    {
        this->global = global;
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
    
    void integer_readreg::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        if(!commit_feedback_pack.flush)
        {
            if(bru_feedback_pack.flush || sau_feedback_pack.flush)
            {
                for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
                {
                    if(!this->readreg_alu_hdff[i]->is_empty())
                    {
                        integer_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_alu_hdff[i]->get_data(&tmp_pack));
                
                        if(bru_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_alu_hdff[i]->flush();
                        }
    
                        if(sau_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            this->readreg_alu_hdff[i]->flush();
                        }
                    }
                }
        
                for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
                {
                    if(!this->readreg_bru_hdff[i]->is_empty())
                    {
                        integer_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_bru_hdff[i]->get_data(&tmp_pack));
    
                        if(bru_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_bru_hdff[i]->flush();
                        }
    
                        if(sau_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            this->readreg_bru_hdff[i]->flush();
                        }
                    }
                }
        
                for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
                {
                    if(!this->readreg_csr_hdff[i]->is_empty())
                    {
                        integer_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_csr_hdff[i]->get_data(&tmp_pack));
    
                        if(bru_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_csr_hdff[i]->flush();
                        }
    
                        if(sau_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            this->readreg_csr_hdff[i]->flush();
                        }
                    }
                }
        
                for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
                {
                    if(!this->readreg_div_hdff[i]->is_empty())
                    {
                        integer_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_div_hdff[i]->get_data(&tmp_pack));
    
                        if(bru_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_div_hdff[i]->flush();
                        }
    
                        if(sau_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            this->readreg_div_hdff[i]->flush();
                        }
                    }
                }
        
                for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
                {
                    if(!this->readreg_mul_hdff[i]->is_empty())
                    {
                        integer_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_mul_hdff[i]->get_data(&tmp_pack));
    
                        if(bru_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_mul_hdff[i]->flush();
                        }
    
                        if(sau_feedback_pack.flush && component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            this->readreg_mul_hdff[i]->flush();
                        }
                    }
                }
            }
            
            auto rev_pack = this->integer_issue_readreg_port->get();
            
            for(uint32_t i = 0;i < INTEGER_READREG_WIDTH;i++)
            {
                integer_readreg_execute_pack_t send_pack;
                send_pack.enable = rev_pack.op_info[i].enable;
                send_pack.value = rev_pack.op_info[i].value;
                send_pack.valid = rev_pack.op_info[i].valid;
                send_pack.last_uop = rev_pack.op_info[i].last_uop;
                
                send_pack.rob_id = rev_pack.op_info[i].rob_id;
                send_pack.rob_id_stage = rev_pack.op_info[i].rob_id_stage;
                send_pack.pc = rev_pack.op_info[i].pc;
                send_pack.imm = rev_pack.op_info[i].imm;
                send_pack.has_exception = rev_pack.op_info[i].has_exception;
                send_pack.exception_id = rev_pack.op_info[i].exception_id;
                send_pack.exception_value = rev_pack.op_info[i].exception_value;
                send_pack.branch_predictor_info_pack = rev_pack.op_info[i].branch_predictor_info_pack;
                
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
                memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                
                if(rev_pack.op_info[i].enable)
                {
                    if(bru_feedback_pack.flush && (component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                    {
                        continue;//skip this instruction due to which is younger than the flush age
                    }
    
                    if(sau_feedback_pack.flush && (component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                    {
                        continue;//skip this instruction due to which is younger than the flush age
                    }
                    
                    //get value from physical register file/execute feedback and wb feedback
                    if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                    {
                        if(rev_pack.op_info[i].rs1_need_map)
                        {
                            bool from_prf = true;
                            send_pack.src1_value = this->phy_regfile->read(rev_pack.op_info[i].rs1_phy);
                            
                            for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
                            {
                                if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                {
                                    send_pack.src1_value = execute_feedback_pack.channel[j].value;
                                    from_prf = false;
                                    break;
                                }
                                
                                if(wb_feedback_pack.channel[j].enable && wb_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                {
                                    send_pack.src1_value = wb_feedback_pack.channel[j].value;
                                    from_prf = false;
                                    break;
                                }
                            }
                            
                            if(from_prf)
                            {
                                //verify_only(this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs1_phy));
                            }
                        }
                        else if(rev_pack.op_info[i].arg1_src == arg_src_t::imm)
                        {
                            send_pack.src1_value = rev_pack.op_info[i].imm;
                        }
        
                        if(rev_pack.op_info[i].rs2_need_map)
                        {
                            bool from_prf = true;
                            send_pack.src2_value = this->phy_regfile->read(rev_pack.op_info[i].rs2_phy);
                            
                            for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
                            {
                                if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                {
                                    send_pack.src2_value = execute_feedback_pack.channel[j].value;
                                    from_prf = false;
                                    break;
                                }
                                
                                if(wb_feedback_pack.channel[j].enable && wb_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                {
                                    send_pack.src2_value = wb_feedback_pack.channel[j].value;
                                    from_prf = false;
                                    break;
                                }
                            }
    
                            if(from_prf)
                            {
                                //verify_only(this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs2_phy));
                            }
                        }
                        else if(rev_pack.op_info[i].arg2_src == arg_src_t::imm)
                        {
                            send_pack.src2_value = rev_pack.op_info[i].imm;
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
                                verify_only(0);
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
                                verify_only(0);
                                break;
                        }
                    }
                }
            }
        }
        else
        {
            for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
            {
                this->readreg_alu_hdff[i]->flush();
            }
            
            for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
            {
                this->readreg_bru_hdff[i]->flush();
            }
            
            for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
            {
                this->readreg_csr_hdff[i]->flush();
            }
            
            for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
            {
                this->readreg_div_hdff[i]->flush();
            }
            
            for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
            {
                this->readreg_mul_hdff[i]->flush();
            }
        }
    }
}