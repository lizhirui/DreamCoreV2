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
#include "cycle_model/component/ooo_issue_queue.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/wait_table.h"
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
    lsu_issue::lsu_issue(global_inst *global, component::port<dispatch_issue_pack_t> *dispatch_lsu_issue_port, component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port, component::regfile<uint32_t> *phy_regfile, component::wait_table *wait_table) : issue_q(component::ooo_issue_queue<issue_queue_item_t>(LSU_ISSUE_QUEUE_SIZE)), tdb(TRACE_LSU_ISSUE)
    {
        this->global = global;
        this->dispatch_lsu_issue_port = dispatch_lsu_issue_port;
        this->lsu_issue_readreg_port = lsu_issue_readreg_port;
        this->phy_regfile = phy_regfile;
        this->wait_table = wait_table;
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
            this->sau_part_issued[i] = false;
            this->sdu_part_issued[i] = false;
            this->is_store[i] = false;
            this->waiting_issue_id_ready[i] = false;
            this->waiting_issue_id[i] = 0;
            this->waiting_store[i] = false;
            this->issued[i].set(false);
        }
    }
    
    lsu_issue_output_feedback_pack_t lsu_issue::run_output(const lsu_readreg_feedback_pack_t &lsu_readreg_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::lu_feedback_pack_t &lu_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        lsu_issue_output_feedback_pack_t feedback_pack;
        lsu_issue_readreg_pack_t send_pack;
        
        if(!commit_feedback_pack.flush)
        {
            if(!lsu_readreg_feedback_pack.stall)
            {
                uint32_t selected_issue_id[LSU_ISSUE_WIDTH] = {0};
                uint32_t selected_rob_id[LSU_ISSUE_WIDTH] = {0};
                bool selected_rob_id_stage[LSU_ISSUE_WIDTH] = {false};
                bool selected_valid[LSU_ISSUE_WIDTH] = {false};
                
                //select instructions
                for(uint32_t i = 0;i < LSU_ISSUE_QUEUE_SIZE;i++)
                {
                    if(issue_q.is_valid(i) && waiting_issue_id_ready[i])
                    {
                        if(bru_feedback_pack.flush && (component::age_compare(rob_id[i], rob_id_stage[i]) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                        {
                            continue;
                        }

                        if(sau_feedback_pack.flush && (component::age_compare(rob_id[i], rob_id_stage[i]) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                        {
                            continue;
                        }
                        
                        if(lu_feedback_pack.replay)
                        {
                            if(((src1_lpv[i] & 1) != 0) || ((src2_lpv[i] & 1) != 0))
                            {
                                continue;//skip the instruction for replay
                            }
                        }
                        
                        if(this->issued[i].get())
                        {
                            continue;//skip the issued instruction
                        }
                        
                        if(this->waiting_store[i])
                        {
                            continue;//skip the waiting store instruction
                        }
                        
                        if(!is_store[i])
                        {
                            if(src1_ready[i] && src2_ready[i])
                            {
                                if(!selected_valid[0] || (component::age_compare(rob_id[i], rob_id_stage[i]) > component::age_compare(selected_rob_id[0], selected_rob_id_stage[0])))
                                {
                                    selected_issue_id[0] = i;
                                    selected_rob_id[0] = rob_id[i];
                                    selected_rob_id_stage[0] = rob_id_stage[i];
                                    selected_valid[0] = true;
                                }
                            }
                        }
                        else
                        {
                            if(src1_ready[i] && !sau_part_issued[i])
                            {
                                if(!selected_valid[1] || (component::age_compare(rob_id[i], rob_id_stage[i]) > component::age_compare(selected_rob_id[1], selected_rob_id_stage[1])))
                                {
                                    selected_issue_id[1] = i;
                                    selected_rob_id[1] = rob_id[i];
                                    selected_rob_id_stage[1] = rob_id_stage[i];
                                    selected_valid[1] = true;
                                }
                            }

                            if(src2_ready[i] && !sdu_part_issued[i])
                            {
                                if(!selected_valid[2] || (component::age_compare(rob_id[i], rob_id_stage[i]) > component::age_compare(selected_rob_id[2], selected_rob_id_stage[2])))
                                {
                                    selected_issue_id[2] = i;
                                    selected_rob_id[2] = rob_id[i];
                                    selected_rob_id_stage[2] = rob_id_stage[i];
                                    selected_valid[2] = true;
                                }
                            }
                        }
                    }
                }
                
                //build send_pack
                for(uint32_t i = 0;i < LSU_ISSUE_WIDTH;i++)
                {
                    if(selected_valid[i])
                    {
                        auto rev_pack = issue_q.customer_get_item(selected_issue_id[i]);

                        send_pack.op_info[i].enable = rev_pack.enable;
                        send_pack.op_info[i].value = rev_pack.value;
                        send_pack.op_info[i].valid = rev_pack.valid;
                        send_pack.op_info[i].last_uop = rev_pack.last_uop;

                        send_pack.op_info[i].rob_id = rev_pack.rob_id;
                        send_pack.op_info[i].rob_id_stage = rev_pack.rob_id_stage;
                        send_pack.op_info[i].pc = rev_pack.pc;
                        send_pack.op_info[i].imm = rev_pack.imm;
                        send_pack.op_info[i].has_exception = rev_pack.has_exception;
                        send_pack.op_info[i].exception_id = rev_pack.exception_id;
                        send_pack.op_info[i].exception_value = rev_pack.exception_value;
                        send_pack.op_info[i].branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;

                        send_pack.op_info[i].rs1 = rev_pack.rs1;
                        send_pack.op_info[i].arg1_src = rev_pack.arg1_src;
                        send_pack.op_info[i].rs1_need_map = rev_pack.rs1_need_map;
                        send_pack.op_info[i].rs1_phy = rev_pack.rs1_phy;

                        send_pack.op_info[i].rs2 = rev_pack.rs2;
                        send_pack.op_info[i].arg2_src = rev_pack.arg2_src;
                        send_pack.op_info[i].rs2_need_map = rev_pack.rs2_need_map;
                        send_pack.op_info[i].rs2_phy = rev_pack.rs2_phy;

                        send_pack.op_info[i].rd = rev_pack.rd;
                        send_pack.op_info[i].rd_enable = rev_pack.rd_enable;
                        send_pack.op_info[i].need_rename = rev_pack.need_rename;
                        send_pack.op_info[i].rd_phy = rev_pack.rd_phy;

                        send_pack.op_info[i].csr = rev_pack.csr;
                        send_pack.op_info[i].store_buffer_id = rev_pack.store_buffer_id;
                        send_pack.op_info[i].load_queue_id = rev_pack.load_queue_id;
                        send_pack.op_info[i].lpv = (src1_lpv[selected_issue_id[i]] | src2_lpv[selected_issue_id[i]]) >> 1;
                        send_pack.op_info[i].op = rev_pack.op;
                        send_pack.op_info[i].op_unit = rev_pack.op_unit;
                        memcpy((void *)&send_pack.op_info[i].sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
                        
                        if(i == 1)
                        {
                            send_pack.op_info[i].op_unit = op_unit_t::sau;
                            
                            switch(rev_pack.sub_op.sdu_op)
                            {
                                case sdu_op_t::sb:
                                    send_pack.op_info[i].sub_op.sau_op = sau_op_t::stab;
                                    break;
                                    
                                case sdu_op_t::sh:
                                    send_pack.op_info[i].sub_op.sau_op = sau_op_t::stah;
                                    break;
                                    
                                case sdu_op_t::sw:
                                    send_pack.op_info[i].sub_op.sau_op = sau_op_t::staw;
                                    break;
                            }
                            
                            send_pack.op_info[i].last_uop = sdu_part_issued[selected_issue_id[i]];
                        }
                        else if(i == 2)
                        {
                            send_pack.op_info[i].last_uop = sau_part_issued[selected_issue_id[i]] || (selected_valid[1] && (selected_issue_id[1] == selected_issue_id[2]));
                        }
                        
                        if(send_pack.op_info[i].last_uop && (send_pack.op_info[i].lpv == 0) && (lpv[selected_issue_id[i]] == 0))
                        {
                            issue_q.pop(selected_issue_id[i]);
                        }
                        
                        cur_lpv[selected_issue_id[i]] = lpv[selected_issue_id[i]];
                        issued[selected_issue_id[i]].set(send_pack.op_info[i].last_uop);
                    }
                    else
                    {
                        send_pack.op_info[i] = lsu_issue_readreg_op_info_t();
                    }
                }
                
                //set sau_part_issued and sdu_part_issued
                if(selected_valid[1])
                {
                    sau_part_issued[selected_issue_id[1]] = true;
                }
    
                if(selected_valid[2])
                {
                    sdu_part_issued[selected_issue_id[2]] = true;
                }
                
                //build feedback_pack
                for(uint32_t i = 0;i < LSU_ISSUE_WIDTH;i++)
                {
                    feedback_pack.wakeup_valid[i] = selected_valid[i] && wakeup_rd_valid[selected_issue_id[i]] && send_pack.op_info[i].last_uop;
                    feedback_pack.wakeup_issue_id[i] = selected_issue_id[i];
                    feedback_pack.wakeup_rd[i] = wakeup_rd[selected_issue_id[i]];
                    feedback_pack.wakeup_shift[i] = wakeup_shift[selected_issue_id[i]];
                    feedback_pack.wakeup_lpv[i] = ((src1_lpv[selected_issue_id[i]] | src2_lpv[selected_issue_id[i]]) >> 1) | lpv[selected_issue_id[i]];
                }
                
                lsu_issue_readreg_port->set(send_pack);
            }
        }
        else
        {
            lsu_issue_readreg_port->set(lsu_issue_readreg_pack_t());
        }
        
        return feedback_pack;
    }
    
    void lsu_issue::run_wakeup(const integer_issue_output_feedback_pack_t &integer_issue_output_feedback_pack, const lsu_issue_output_feedback_pack_t &lsu_issue_output_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::lu_feedback_pack_t &lu_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        if(!commit_feedback_pack.flush)
        {
            //find oldest store instruction
            bool has_store = false;
            uint32_t store_rob_id = 0;
            bool store_rob_id_stage = false;
            
            for(uint32_t i = 0;i < LSU_ISSUE_QUEUE_SIZE;i++)
            {
                if(issue_q.is_valid(i) && !issued[i] && is_store[i])
                {
                    if(!has_store)
                    {
                        has_store = true;
                        store_rob_id = issue_q.customer_get_item(i).rob_id;
                        store_rob_id_stage = issue_q.customer_get_item(i).rob_id_stage;
                    }
                    else if(component::age_compare(issue_q.customer_get_item(i).rob_id, issue_q.customer_get_item(i).rob_id_stage) > component::age_compare(store_rob_id, store_rob_id_stage))
                    {
                        store_rob_id = issue_q.customer_get_item(i).rob_id;
                        store_rob_id_stage = issue_q.customer_get_item(i).rob_id_stage;
                    }
                }
            }
            
            for(uint32_t i = 0;i < LSU_ISSUE_QUEUE_SIZE;i++)
            {
                if(issue_q.is_valid(i))
                {
                    auto item = issue_q.customer_get_item(i);
    
                    if(bru_feedback_pack.flush && (component::age_compare(item.rob_id, item.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                    {
                        issue_q.set_valid(i, false);
                        continue;
                    }
    
                    if(sau_feedback_pack.flush && (component::age_compare(item.rob_id, item.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                    {
                        issue_q.set_valid(i, false);
                        continue;
                    }
    
                    //lpv check
                    if(lu_feedback_pack.replay)
                    {
                        //cancel source operand ready and wakeup shift
                        if((this->src1_lpv[i] & 1) != 0)
                        {
                            this->src1_ready[i] = false;
                            this->wakeup_shift_src1[i] = 0;
                        }
        
                        if((this->src2_lpv[i] & 1) != 0)
                        {
                            this->src2_ready[i] = false;
                            this->wakeup_shift_src2[i] = 0;
                        }
                    }
    
                    if(this->issued[i].get())
                    {
                        if(lu_feedback_pack.replay && (((this->src1_lpv[i] & 1) != 0) || ((this->src2_lpv[i] & 1) != 0) || ((this->cur_lpv[i] & 1) != 0)))
                        {
                            issued[i].set(false);//replay the instruction
                        }
                        else if((this->src1_lpv[i] <= 1) && (this->src2_lpv[i] <= 1) && (this->cur_lpv[i] <= 1))
                        {
                            issue_q.set_valid(i, false);//remove the instruction from issue queue because it isn't in the speculative window of any load instruction
                        }
                        
                        this->cur_lpv[i] >>= 1;
                    }
    
                    //lpv shift
                    this->src1_lpv[i] >>= 1;
                    this->src2_lpv[i] >>= 1;
    
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
    
                                    this->src1_lpv[i] = integer_issue_output_feedback_pack.wakeup_lpv[j];
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
                                    
                                    this->src2_lpv[i] = integer_issue_output_feedback_pack.wakeup_lpv[j];
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
                                    
                                    this->src1_lpv[i] = lsu_issue_output_feedback_pack.wakeup_lpv[j];
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
                                    
                                    this->src2_lpv[i] = lsu_issue_output_feedback_pack.wakeup_lpv[j];
                                }
                            }
    
                            //store wakeup
                            if(!this->waiting_issue_id_ready[i] && this->waiting_issue_id[i] == lsu_issue_output_feedback_pack.wakeup_issue_id[j])
                            {
                                this->waiting_issue_id_ready[i] = true;
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
                    
                    //waiting_store wakeup
                    if(this->waiting_store[i] && (!has_store || (component::age_compare(store_rob_id, store_rob_id_stage) < component::age_compare(this->waiting_store_rob_id[i], this->waiting_store_rob_id_stage[i]))))
                    {
                        this->waiting_store[i] = false;
                    }
                }
            }
        }
    }
    
    lsu_issue_feedback_pack_t lsu_issue::run_input(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, commit_feedback_pack_t commit_feedback_pack)
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
            
            if(bru_feedback_pack.flush)
            {
                for(uint32_t i = 0;i < DISPATCH_WIDTH;i++)
                {
                    if(rev_pack.op_info[i].enable)
                    {
                        if((component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                        {
                            rev_pack.op_info[i].enable = false;//skip this instruction due to which is younger than the flush age
                        }
                    }
                }
            }
    
            if(sau_feedback_pack.flush)
            {
                for(uint32_t i = 0;i < DISPATCH_WIDTH;i++)
                {
                    if(rev_pack.op_info[i].enable)
                    {
                        if((component::age_compare(rev_pack.op_info[i].rob_id, rev_pack.op_info[i].rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                        {
                            rev_pack.op_info[i].enable = false;//skip this instruction due to which is younger than the flush age
                        }
                    }
                }
            }
            
            for(uint32_t i = 0;i < DISPATCH_WIDTH;i++)
            {
                if(rev_pack.op_info[i].enable)
                {
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
                    item.branch_predictor_info_pack = rev_pack.op_info[i].branch_predictor_info_pack;
    
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
                    item.store_buffer_id = rev_pack.op_info[i].store_buffer_id;
                    item.load_queue_id = rev_pack.op_info[i].load_queue_id;
                    item.op = rev_pack.op_info[i].op;
                    item.op_unit = rev_pack.op_info[i].op_unit;
                    memcpy((void *)&item.sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                    uint32_t issue_id = 0;
                    
                    if(issue_q.push(item, &issue_id))
                    {
                        wakeup_shift_src1[issue_id] = 0;
                        src1_ready[issue_id] = false;
                        wakeup_shift_src2[issue_id] = 0;
                        src2_ready[issue_id] = false;
        
                        //execute and wb feedback and physical register file check
                        if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                        {
                            if(rev_pack.op_info[i].rs1_need_map)
                            {
                                for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                    {
                                        src1_ready[issue_id] = true;
                                        break;
                                    }
                                }
        
                                if(!src1_ready[issue_id])
                                {
                                    for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
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
                                for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                    {
                                        src2_ready[issue_id] = true;
                                        break;
                                    }
                                }
                
                                if(!src2_ready[issue_id])
                                {
                                    for(uint32_t j = 0;j < FEEDBACK_EXECUTE_UNIT_NUM;j++)
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
                        
                        //set age information
                        rob_id[issue_id] = rev_pack.op_info[i].rob_id;
                        rob_id_stage[issue_id] = rev_pack.op_info[i].rob_id_stage;
                        
                        //set wakeup information
                        wakeup_rd_valid[issue_id] = rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception && rev_pack.op_info[i].need_rename;
                        wakeup_rd[issue_id] = rev_pack.op_info[i].rd_phy;
                        wakeup_shift[issue_id] = lsu_issue::latency_to_wakeup_shift(LSU_LATENCY);
                        is_store[issue_id] = rev_pack.op_info[i].op_unit == op_unit_t::sdu;
                        sau_part_issued[issue_id] = false;
                        sdu_part_issued[issue_id] = false;
                        waiting_issue_id_ready[issue_id] = true;
                        waiting_issue_id[issue_id] = 0;
                    
                        //find youngest store instruction
                        {
                            bool has_store = false;
                            uint32_t last_rob_id = 0;
                            bool last_rob_id_stage = false;
                            
                            for(uint32_t j = 0;j < LSU_ISSUE_QUEUE_SIZE;j++)
                            {
                                if(issue_q.is_valid(j) && !issued[j] && is_store[j])
                                {
                                    if(!has_store)
                                    {
                                        has_store = true;
                                        last_rob_id = rob_id[j];
                                        last_rob_id_stage = rob_id_stage[j];
                                    }
                                    else if(component::age_compare(rob_id[j], rob_id_stage[j]) < component::age_compare(last_rob_id, last_rob_id_stage))
                                    {
                                        last_rob_id = rob_id[j];
                                        last_rob_id_stage = rob_id_stage[j];
                                    }
                                }
                            }
    
                            waiting_store[issue_id] = (rev_pack.op_info[i].op_unit == op_unit_t::lu) ? (wait_table->get_wait_bit(rev_pack.op_info[i].pc) && has_store) : false;
                            waiting_store_rob_id[issue_id] = last_rob_id;
                            waiting_store_rob_id_stage[issue_id] = last_rob_id_stage;
                        }
    
                        //initialize replay information
                        src1_lpv[issue_id] = 0;
                        src2_lpv[issue_id] = 0;
                        cur_lpv[issue_id] = lpv[issue_id] = (rev_pack.op_info[i].op_unit == op_unit_t::lu) ? INIT_LPV : 0;
                        issued[issue_id].set(false);
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
        return (latency == 1) ? 0 : (1 << (latency - 2));
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
        
        for(uint32_t i = 0;i < LSU_ISSUE_QUEUE_SIZE;i++)
        {
            wakeup_shift_src1.push_back(this->wakeup_shift_src1[i]);
            src1_ready.push_back(this->src1_ready[i]);
            wakeup_shift_src2.push_back(this->wakeup_shift_src2[i]);
            src2_ready.push_back(this->src2_ready[i]);
        }
        
        t["wakeup_shift_src1"] = wakeup_shift_src1;
        t["src1_ready"] = src1_ready;
        t["wakeup_shift_src2"] = wakeup_shift_src2;
        t["src2_ready"] = src2_ready;
        return t;
    }
}