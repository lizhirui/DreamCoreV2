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

namespace cycle_model::pipeline
{
    lsu_readreg::lsu_readreg(component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port, component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_lsu_hdff, component::regfile<uint32_t> *phy_regfile) : tdb(TRACE_LSU_READREG)
    {
        this->lsu_issue_readreg_port = lsu_issue_readreg_port;
        this->readreg_lsu_hdff = readreg_lsu_hdff;
        this->phy_regfile = phy_regfile;
        this->lsu_readreg::reset();
    }
    
    void lsu_readreg::reset()
    {
        this->busy = false;
        this->hold_rev_pack = lsu_issue_readreg_pack_t();
    }
    
    lsu_readreg_feedback_pack_t lsu_readreg::run(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        lsu_readreg_feedback_pack_t feedback_pack;
        feedback_pack.stall = this->busy;
    
        if(!commit_feedback_pack.flush)
        {
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
                memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
            
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
                                for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
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
                                for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
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
                    verify(rev_pack.op_info[i].op_unit == op_unit_t::lsu);
                    
                    if(!this->readreg_lsu_hdff[i]->push(send_pack))
                    {
                        this->busy = true;
                        this->hold_rev_pack = rev_pack;
                    }
                }
            }
        }
        else
        {
            for(uint32_t i = 0;i < LSU_UNIT_NUM;i++)
            {
                this->readreg_lsu_hdff[i]->flush();
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