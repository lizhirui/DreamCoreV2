/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-16     lizhirui     the first version
 */

#include "common.h"
#include "config.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/lsu_readreg.h"
#include "cycle_model/pipeline/lsu_issue_readreg.h"
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/commit.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/age_compare.h"

namespace cycle_model::pipeline
{
    lsu_readreg::lsu_readreg(global_inst *global, component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port, component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_lu_hdff, component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_sau_hdff, component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_sdu_hdff, component::regfile<uint32_t> *phy_regfile) : tdb(TRACE_LSU_READREG)
    {
        this->global = global;
        this->lsu_issue_readreg_port = lsu_issue_readreg_port;
        this->readreg_lu_hdff = readreg_lu_hdff;
        this->readreg_sau_hdff = readreg_sau_hdff;
        this->readreg_sdu_hdff = readreg_sdu_hdff;
        this->phy_regfile = phy_regfile;
        this->lsu_readreg::reset();
    }
    
    void lsu_readreg::reset()
    {
        this->busy = false;
        this->hold_rev_pack = lsu_issue_readreg_pack_t();
    }
    
    lsu_readreg_feedback_pack_t lsu_readreg::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        lsu_readreg_feedback_pack_t feedback_pack;
        feedback_pack.stall = this->busy;
    
        if(!commit_feedback_pack.flush)
        {
            if(bru_feedback_pack.flush)
            {
                for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
                {
                    if(!this->readreg_lu_hdff[i]->is_empty())
                    {
                        lsu_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_lu_hdff[i]->get_data(&tmp_pack));
                
                        if(component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_lu_hdff[i]->flush();
                        }
                    }
                }
    
                for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
                {
                    if(!this->readreg_sau_hdff[i]->is_empty())
                    {
                        lsu_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_sau_hdff[i]->get_data(&tmp_pack));
            
                        if(component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_sau_hdff[i]->flush();
                        }
                    }
                }
                
                for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
                {
                    if(!this->readreg_sdu_hdff[i]->is_empty())
                    {
                        lsu_readreg_execute_pack_t tmp_pack;
                        verify(this->readreg_sdu_hdff[i]->get_data(&tmp_pack));
            
                        if(component::age_compare(tmp_pack.rob_id, tmp_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            this->readreg_sdu_hdff[i]->flush();
                        }
                    }
                }
            }
            
            lsu_issue_readreg_pack_t rev_pack;
            
            if(this->busy)
            {
                rev_pack = this->hold_rev_pack;
                this->busy = false;
            }
            else
            {
                rev_pack = this->lsu_issue_readreg_port->get();
            }
        
            for(uint32_t i = 0;i < LSU_READREG_WIDTH;i++)
            {
                lsu_readreg_execute_pack_t send_pack;
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
                send_pack.store_buffer_id = rev_pack.op_info[i].store_buffer_id;
                send_pack.op = rev_pack.op_info[i].op;
                send_pack.op_unit = rev_pack.op_info[i].op_unit;
                memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
            
                if(rev_pack.op_info[i].enable)
                {
                    if(bru_feedback_pack.flush && (component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
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
                        else if(rev_pack.op_info[i].arg1_src == arg_src_t::imm)
                        {
                            send_pack.src2_value = rev_pack.op_info[i].imm;
                        }
                    }
                
                    //send pack
                    switch(rev_pack.op_info[i].op_unit)
                    {
                        case op_unit_t::lu:
                            verify(this->readreg_lu_hdff[i]->push(send_pack));
                            break;
                        
                        case op_unit_t::sau:
                            verify(this->readreg_sau_hdff[i]->push(send_pack));
                            break;
                        
                        case op_unit_t::sdu:
                            verify(this->readreg_sdu_hdff[i]->push(send_pack));
                            break;
                        
                        default:
                            verify_only(0);
                            break;
                    }
                }
            }
        }
        else
        {
            for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
            {
                this->readreg_lu_hdff[i]->flush();
            }
    
            for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
            {
                this->readreg_sau_hdff[i]->flush();
            }
    
            for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
            {
                this->readreg_sdu_hdff[i]->flush();
            }
            
            this->busy = false;
            this->hold_rev_pack = lsu_issue_readreg_pack_t();
        }
        
        return feedback_pack;
    }
    
    json lsu_readreg::get_json()
    {
        json t;
        
        t["busy"] = this->busy;
        return t;
    }
}