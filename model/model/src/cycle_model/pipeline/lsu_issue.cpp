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
#include "cycle_model/pipeline/lsu_issue.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/io_issue_queue.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/age_compare.h"
#include "cycle_model/pipeline/rename_dispatch.h"
#include "cycle_model/pipeline/lsu_issue_readreg.h"
#include "cycle_model/pipeline/lsu_readreg.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model::pipeline
{
    lsu_issue::lsu_issue(global_inst *global, component::port<dispatch_issue_pack_t> *dispatch_lsu_issue_port, component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port, component::regfile<uint32_t> *phy_regfile) : issue_q(component::io_issue_queue<issue_queue_item_t>(LSU_ISSUE_QUEUE_SIZE)), tdb(TRACE_LSU_ISSUE)
    {
        this->global = global;
        this->dispatch_lsu_issue_port = dispatch_lsu_issue_port;
        this->lsu_issue_readreg_port = lsu_issue_readreg_port;
        this->phy_regfile = phy_regfile;
        this->lsu_issue::reset();
    }
    
    void lsu_issue::reset()
    {
        this->issue_q.reset();
        this->busy = false;
        this->hold_rev_pack = dispatch_issue_pack_t();
        
        for(uint32_t i = 0;i < LSU_ISSUE_QUEUE_SIZE;i++)
        {
            this->wakeup_shift_src1[i] = 0;
            this->src1_ready[i] = false;
            this->wakeup_shift_src2[i] = 0;
            this->src2_ready[i] = false;
        }
    }
    
    lsu_issue_output_feedback_pack_t lsu_issue::run_output(const lsu_readreg_feedback_pack_t &lsu_readreg_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        lsu_issue_output_feedback_pack_t feedback_pack;
        lsu_issue_readreg_pack_t send_pack;
        
        if(!commit_feedback_pack.flush)
        {
            if(!lsu_readreg_feedback_pack.stall)
            {
                if(!issue_q.customer_is_empty())
                {
                    uint32_t issue_id = 0;
                    
                    verify(issue_q.customer_get_front_id(&issue_id));
                    
                    if(src1_ready[issue_id] && src2_ready[issue_id])
                    {
                        //build send_pack
                        issue_queue_item_t rev_pack;
                        
                        if(bru_feedback_pack.flush)
                        {
                            verify(issue_q.customer_get_front(&rev_pack));
                            
                            if(component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                            {
                                //skip this instruction
                                lsu_issue_readreg_port->set(send_pack);
                                return feedback_pack;
                            }
                        }
                        
                        verify(issue_q.pop(&rev_pack));
                        verify_only(rev_pack.op_unit == op_unit_t::lsu);
                        
                        send_pack.op_info[0].enable = rev_pack.enable;
                        send_pack.op_info[0].value = rev_pack.value;
                        send_pack.op_info[0].valid = rev_pack.valid;
                        send_pack.op_info[0].last_uop = rev_pack.last_uop;
    
                        send_pack.op_info[0].rob_id = rev_pack.rob_id;
                        send_pack.op_info[0].rob_id_stage = rev_pack.rob_id_stage;
                        send_pack.op_info[0].pc = rev_pack.pc;
                        send_pack.op_info[0].imm = rev_pack.imm;
                        send_pack.op_info[0].has_exception = rev_pack.has_exception;
                        send_pack.op_info[0].exception_id = rev_pack.exception_id;
                        send_pack.op_info[0].exception_value = rev_pack.exception_value;
    
                        send_pack.op_info[0].rs1 = rev_pack.rs1;
                        send_pack.op_info[0].arg1_src = rev_pack.arg1_src;
                        send_pack.op_info[0].rs1_need_map = rev_pack.rs1_need_map;
                        send_pack.op_info[0].rs1_phy = rev_pack.rs1_phy;
    
                        send_pack.op_info[0].rs2 = rev_pack.rs2;
                        send_pack.op_info[0].arg2_src = rev_pack.arg2_src;
                        send_pack.op_info[0].rs2_need_map = rev_pack.rs2_need_map;
                        send_pack.op_info[0].rs2_phy = rev_pack.rs2_phy;
    
                        send_pack.op_info[0].rd = rev_pack.rd;
                        send_pack.op_info[0].rd_enable = rev_pack.rd_enable;
                        send_pack.op_info[0].need_rename = rev_pack.need_rename;
                        send_pack.op_info[0].rd_phy = rev_pack.rd_phy;
    
                        send_pack.op_info[0].csr = rev_pack.csr;
                        send_pack.op_info[0].op = rev_pack.op;
                        send_pack.op_info[0].op_unit = rev_pack.op_unit;
                        memcpy((void *)&send_pack.op_info[0].sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
                        lsu_issue_readreg_port->set(send_pack);
                        //build feedback_pack
                        feedback_pack.wakeup_valid[0] = wakeup_rd_valid[issue_id];
                        feedback_pack.wakeup_rd[0] = wakeup_rd[issue_id];
                        feedback_pack.wakeup_shift[0] = wakeup_shift[issue_id];
                    }
                    else
                    {
                        lsu_issue_readreg_port->set(lsu_issue_readreg_pack_t());
                    }
                }
                else
                {
                    lsu_issue_readreg_port->set(lsu_issue_readreg_pack_t());
                }
            }
        }
        else
        {
            lsu_issue_readreg_port->set(lsu_issue_readreg_pack_t());
        }
        
        return feedback_pack;
    }
    
    void lsu_issue::run_wakeup(const integer_issue_output_feedback_pack_t &integer_issue_output_feedback_pack, const lsu_issue_output_feedback_pack_t &lsu_issue_output_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        if(!commit_feedback_pack.flush)
        {
            if(bru_feedback_pack.flush)
            {
                issue_q.customer_foreach([=](uint32_t id, bool stage, const issue_queue_item_t &item)->bool
                {
                    if(component::age_compare(item.rob_id, item.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                    {
                        issue_q.update_wptr(id, stage);
                        return false;
                    }
                    
                    return true;
                });
            }
            
            for(uint32_t i = 0;i < INTEGER_ISSUE_QUEUE_SIZE;i++)
            {
                if(issue_q.customer_check_id_valid(i))
                {
                    auto item = issue_q.customer_get_item(i);
    
                    //delay wakeup
                    if(!this->src1_ready[i] && this->wakeup_shift_src1[i] > 0)
                    {
                        if(this->wakeup_shift_src1[i] == 1)
                        {
                            this->src1_ready[i] = true;
                        }
        
                        this->wakeup_shift_src1[i] >>= 1;
                    }
    
                    if(!this->src2_ready[i] && this->wakeup_shift_src2[i] > 0)
                    {
                        if(this->wakeup_shift_src2[i] == 1)
                        {
                            this->src2_ready[i] = true;
                        }
        
                        this->wakeup_shift_src2[i] >>= 1;
                    }
                
                    //integer_issue_output feedback
                    for(uint32_t j = 0;j < INTEGER_ISSUE_WIDTH;j++)
                    {
                        if(integer_issue_output_feedback_pack.wakeup_valid[j])
                        {
                            if(!this->src1_ready[i])
                            {
                                verify_only(item.arg1_src == arg_src_t::reg);
                            
                                if(item.rs1_phy == integer_issue_output_feedback_pack.wakeup_rd[j])
                                {
                                    if(integer_issue_output_feedback_pack.wakeup_shift[j] == 0)
                                    {
                                        this->src1_ready[i] = true;
                                    }
                                    else
                                    {
                                        this->wakeup_shift_src1[i] = integer_issue_output_feedback_pack.wakeup_shift[j];
                                    }
                                }
                            }
                        
                            if(!this->src2_ready[i])
                            {
                                verify_only(item.arg2_src == arg_src_t::reg);
                            
                                if(item.rs2_phy == integer_issue_output_feedback_pack.wakeup_rd[j])
                                {
                                    if(integer_issue_output_feedback_pack.wakeup_shift[j] == 0)
                                    {
                                        this->src2_ready[i] = true;
                                    }
                                    else
                                    {
                                        this->wakeup_shift_src2[i] = integer_issue_output_feedback_pack.wakeup_shift[j];
                                    }
                                }
                            }
                        }
                    }
    
                    //lsu_issue_output feedback
                    for(uint32_t j = 0;j < LSU_ISSUE_WIDTH;j++)
                    {
                        if(lsu_issue_output_feedback_pack.wakeup_valid[j])
                        {
                            if(!this->src1_ready[i])
                            {
                                verify_only(item.arg1_src == arg_src_t::reg);
                
                                if(item.rs1_phy == lsu_issue_output_feedback_pack.wakeup_rd[j])
                                {
                                    if(lsu_issue_output_feedback_pack.wakeup_shift[j] == 0)
                                    {
                                        this->src1_ready[i] = true;
                                    }
                                    else
                                    {
                                        this->wakeup_shift_src1[i] = lsu_issue_output_feedback_pack.wakeup_shift[j];
                                    }
                                }
                            }
            
                            if(!this->src2_ready[i])
                            {
                                verify_only(item.arg2_src == arg_src_t::reg);
                
                                if(item.rs2_phy == lsu_issue_output_feedback_pack.wakeup_rd[j])
                                {
                                    if(lsu_issue_output_feedback_pack.wakeup_shift[j] == 0)
                                    {
                                        this->src2_ready[i] = true;
                                    }
                                    else
                                    {
                                        this->wakeup_shift_src2[i] = lsu_issue_output_feedback_pack.wakeup_shift[j];
                                    }
                                }
                            }
                        }
                    }
                
                    //execute feedback
                    for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++)
                    {
                        if(execute_feedback_pack.channel[j].enable)
                        {
                            if(!this->src1_ready[i])
                            {
                                verify_only(item.arg1_src == arg_src_t::reg);
                                verify_only(item.rs1_need_map);
                            
                                if(item.rs1_phy == execute_feedback_pack.channel[j].phy_id)
                                {
                                    this->src1_ready[i] = true;
                                }
                            }
                        
                            if(!this->src2_ready[i])
                            {
                                verify_only(item.arg2_src == arg_src_t::reg);
                                verify_only(item.rs2_need_map);
                            
                                if(item.rs2_phy == execute_feedback_pack.channel[j].phy_id)
                                {
                                    this->src2_ready[i] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    lsu_issue_feedback_pack_t lsu_issue::run_input(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, commit_feedback_pack_t commit_feedback_pack)
    {
        lsu_issue_feedback_pack_t feedback_pack;
        feedback_pack.stall = this->busy;//generate stall signal to prevent dispatch from dispatching new instructions
        
        if(!commit_feedback_pack.flush)
        {
            dispatch_issue_pack_t rev_pack;
            
            if(this->busy)
            {
                rev_pack = this->hold_rev_pack;
                this->hold_rev_pack = dispatch_issue_pack_t();
                this->busy = false;
            }
            else
            {
                rev_pack = dispatch_lsu_issue_port->get();
            }
            
            for(uint32_t i = 0;i < DISPATCH_WIDTH;i++)
            {
                if(rev_pack.op_info[i].enable)
                {
                    if(bru_feedback_pack.flush && (component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                    {
                        continue;//skip this instruction due to which is younger than the flush age
                    }
                    
                    issue_queue_item_t item;
    
                    item.enable = rev_pack.op_info[i].enable;
                    item.value = rev_pack.op_info[i].value;
                    item.valid = rev_pack.op_info[i].valid;
                    item.last_uop = rev_pack.op_info[i].last_uop;
    
                    item.rob_id = rev_pack.op_info[i].rob_id;
                    item.rob_id_stage = rev_pack.op_info[i].rob_id_stage;
                    item.pc = rev_pack.op_info[i].pc;
                    item.imm = rev_pack.op_info[i].imm;
                    item.has_exception = rev_pack.op_info[i].has_exception;
                    item.exception_id = rev_pack.op_info[i].exception_id;
                    item.exception_value = rev_pack.op_info[i].exception_value;
    
                    item.rs1 = rev_pack.op_info[i].rs1;
                    item.arg1_src = rev_pack.op_info[i].arg1_src;
                    item.rs1_need_map = rev_pack.op_info[i].rs1_need_map;
                    item.rs1_phy = rev_pack.op_info[i].rs1_phy;
    
                    item.rs2 = rev_pack.op_info[i].rs2;
                    item.arg2_src = rev_pack.op_info[i].arg2_src;
                    item.rs2_need_map = rev_pack.op_info[i].rs2_need_map;
                    item.rs2_phy = rev_pack.op_info[i].rs2_phy;
    
                    item.rd = rev_pack.op_info[i].rd;
                    item.rd_enable = rev_pack.op_info[i].rd_enable;
                    item.need_rename = rev_pack.op_info[i].need_rename;
                    item.rd_phy = rev_pack.op_info[i].rd_phy;
    
                    item.csr = rev_pack.op_info[i].csr;
                    item.op = rev_pack.op_info[i].op;
                    item.op_unit = rev_pack.op_info[i].op_unit;
                    memcpy((void *)&item.sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                    uint32_t issue_id = 0;
                    
                    if(issue_q.push(item))
                    {
                        verify(issue_q.producer_get_tail_id(&issue_id));
                        
                        wakeup_shift_src1[issue_id] = 0;
                        src1_ready[issue_id] = false;
                        wakeup_shift_src2[issue_id] = 0;
                        src2_ready[issue_id] = false;
        
                        //execute and wb feedback and physical register file check
                        if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                        {
                            if(rev_pack.op_info[i].rs1_need_map)
                            {
                                for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                    {
                                        src1_ready[issue_id] = true;
                                        break;
                                    }
                                }
        
                                if(!src1_ready[issue_id])
                                {
                                    for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++)
                                    {
                                        if(wb_feedback_pack.channel[j].enable && wb_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                        {
                                            src1_ready[issue_id] = true;
                                            break;
                                        }
                                    }
                                }
        
                                if(!src1_ready[issue_id])
                                {
                                    src1_ready[issue_id] = this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs1_phy);
                                }
                            }
                            else
                            {
                                src1_ready[issue_id] = true;
                            }
                        }
                        else
                        {
                            src1_ready[issue_id] = true;
                        }
        
                        if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                        {
                            if(rev_pack.op_info[i].rs2_need_map)
                            {
                                for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                    {
                                        src2_ready[issue_id] = true;
                                        break;
                                    }
                                }
                
                                if(!src2_ready[issue_id])
                                {
                                    for(uint32_t j = 0;j < EXECUTE_UNIT_NUM;j++)
                                    {
                                        if(wb_feedback_pack.channel[j].enable && wb_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                        {
                                            src2_ready[issue_id] = true;
                                            break;
                                        }
                                    }
                                }
                
                                if(!src2_ready[issue_id])
                                {
                                    src2_ready[issue_id] = this->phy_regfile->read_data_valid(rev_pack.op_info[i].rs2_phy);
                                }
                            }
                            else
                            {
                                src2_ready[issue_id] = true;
                            }
                        }
                        else
                        {
                            src2_ready[i] = true;
                        }
                        
                        //set wakeup information
                        wakeup_rd[issue_id] = rev_pack.op_info[i].rd_phy;
                        wakeup_rd_valid[issue_id] = rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception && rev_pack.op_info[i].need_rename;
                        wakeup_shift[issue_id] = lsu_issue::latency_to_wakeup_shift(LSU_LATENCY);
                    }
                    else
                    {
                        //entry busy state
                        this->busy = true;
    
                        //let remain instructions keep right alignment
                        for(uint32_t j = 0;j < DISPATCH_WIDTH;j++)
                        {
                            if(j + i < DISPATCH_WIDTH)
                            {
                                hold_rev_pack.op_info[j] = rev_pack.op_info[j + i];
                            }
                            else
                            {
                                hold_rev_pack.op_info[j].enable = false;
                            }
                        }
                        
                        break;
                    }
                }
            }
        }
        else if(commit_feedback_pack.flush)
        {
            issue_q.flush();
            busy = false;
            hold_rev_pack = dispatch_issue_pack_t();
        }
        
        return feedback_pack;
    }
    
    uint32_t lsu_issue::latency_to_wakeup_shift(uint32_t latency)
    {
        return (latency == 1) ? 0 : (1 << (latency - 1));
    }
    
    void lsu_issue::print(std::string indent)
    {
        issue_q.print(indent);
    }
    
    json lsu_issue::get_json()
    {
        json t;
        
        t["busy"] = this->busy;
        t["issue_q"] = issue_q.get_json();
        t["hold_rev_pack"] = this->hold_rev_pack.get_json();
        
        auto wakeup_shift_src1 = json::array();
        auto src1_ready = json::array();
        auto wakeup_shift_src2 = json::array();
        auto src2_ready = json::array();
    
        issue_q.customer_foreach([&](uint32_t id, bool stage, const issue_queue_item_t &item) -> bool
        {
            wakeup_shift_src1.push_back(this->wakeup_shift_src1[id]);
            src1_ready.push_back(this->src1_ready[id]);
            wakeup_shift_src2.push_back(this->wakeup_shift_src2[id]);
            src2_ready.push_back(this->src2_ready[id]);
            return true;
        });
        
        t["wakeup_shift_src1"] = wakeup_shift_src1;
        t["src1_ready"] = src1_ready;
        t["wakeup_shift_src2"] = wakeup_shift_src2;
        t["src2_ready"] = src2_ready;
        return t;
    }
}