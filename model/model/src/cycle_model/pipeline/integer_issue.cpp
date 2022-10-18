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
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/ooo_issue_queue.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/pipeline/rename_dispatch.h"
#include "cycle_model/pipeline/integer_issue_readreg.h"
#include "cycle_model/pipeline/integer_readreg.h"
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model::pipeline
{
    integer_issue::integer_issue(component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port, component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port, component::regfile<uint32_t> *phy_regfile) : issue_q(component::ooo_issue_queue<issue_queue_item_t>(INTEGER_ISSUE_QUEUE_SIZE)), tdb(TRACE_INTEGER_ISSUE)
    {
        this->dispatch_integer_issue_port = dispatch_integer_issue_port;
        this->integer_issue_readreg_port = integer_issue_readreg_port;
        this->phy_regfile = phy_regfile;
        this->integer_issue::reset();
    }
    
    void integer_issue::reset()
    {
        this->issue_q.reset();
        this->busy = false;
        this->hold_rev_pack = dispatch_issue_pack_t();
        
        for(auto i = 0;i < ALU_UNIT_NUM;i++)
        {
            this->alu_idle[i] = 1;
            this->alu_idle_shift[i] = 0;
            this->alu_busy_shift[i] = 0;
        }
        
        for(auto i = 0;i < BRU_UNIT_NUM;i++)
        {
            this->bru_idle[i] = 1;
            this->bru_idle_shift[i] = 0;
            this->bru_busy_shift[i] = 0;
        }
        
        for(auto i = 0;i < CSR_UNIT_NUM;i++)
        {
            this->csr_idle[i] = 1;
            this->csr_idle_shift[i] = 0;
            this->csr_busy_shift[i] = 0;
        }
        
        for(auto i = 0;i < DIV_UNIT_NUM;i++)
        {
            this->div_idle[i] = 1;
            this->div_idle_shift[i] = 0;
            this->div_busy_shift[i] = 0;
        }
        
        for(auto i = 0;i < MUL_UNIT_NUM;i++)
        {
            this->mul_idle[i] = 1;
            this->mul_idle_shift[i] = 0;
            this->mul_busy_shift[i] = 0;
        }
        
        for(auto i = 0;i < INTEGER_ISSUE_QUEUE_SIZE;i++)
        {
            this->wakeup_shift_src1[i] = 0;
            this->src1_ready[i] = false;
            this->wakeup_shift_src2[i] = 0;
            this->src2_ready[i] = false;
    
            this->port_index[i] = 0;
            this->op_unit_seq[i] = 0;
            this->rob_id[i] = 0;
            this->rob_id_stage[i] = 0;
            this->wakeup_rd[i] = 0;
            this->wakeup_rd_valid[i] = false;
            this->wakeup_shift[i] = 0;
            this->new_idle_shift[i] = 0;
            this->new_busy_shift[i] = 0;
        }
    
        this->next_port_index = 0;
    }
    
    integer_issue_output_feedback_pack_t integer_issue::run_output(const commit_feedback_pack_t &commit_feedback_pack)
    {
        integer_issue_output_feedback_pack_t feedback_pack;
        integer_issue_readreg_pack_t send_pack;
    
        uint32_t op_unit_seq_mask[INTEGER_ISSUE_WIDTH] = {((alu_idle[0] << ALU_SHIFT) | (mul_idle[0] << MUL_SHIFT) | (csr_idle[0] << CSR_SHIFT) | (bru_idle[0] << BRU_SHIFT)),
                                                          ((alu_idle[1] << ALU_SHIFT) | (mul_idle[1] << MUL_SHIFT) | (div_idle[0] << DIV_SHIFT))};
        
        if(!commit_feedback_pack.flush)
        {
            uint32_t selected_issue_id[INTEGER_ISSUE_WIDTH] = {0};
            uint32_t selected_rob_id[INTEGER_ISSUE_WIDTH] = {0};
            bool selected_rob_id_stage[INTEGER_ISSUE_WIDTH] = {false};
            bool selected_valid[INTEGER_ISSUE_WIDTH] = {false};
            
            //select instructions
            for(auto i = 0;i < INTEGER_ISSUE_WIDTH;i++)
            {
                for(auto j = 0;j < INTEGER_ISSUE_QUEUE_SIZE;j++)
                {
                    if(issue_q.is_valid(i) && (op_unit_seq[j] & op_unit_seq_mask[i]) && src1_ready[i] && src2_ready[i] && (!selected_valid[i] ||
                      ((rob_id_stage[j] == selected_rob_id_stage[i]) && (rob_id[j] < selected_rob_id[i])) || ((rob_id_stage[j] != selected_rob_id_stage[i]) && (rob_id[j] > selected_rob_id[i]))))
                    {
                        selected_issue_id[i] = j;
                        selected_rob_id[i] = rob_id[j];
                        selected_rob_id_stage[i] = rob_id_stage[j];
                        selected_valid[i] = true;
                    }
                }
            }
            
            //build send_pack
            for(auto i = 0;i < INTEGER_ISSUE_WIDTH;i++)
            {
                if(selected_valid[i])
                {
                    auto rev_pack = issue_q.customer_get_item(selected_issue_id[i]);
                    issue_q.pop(selected_issue_id[i]);
                    
                    send_pack.op_info[i].enable = rev_pack.enable;
                    send_pack.op_info[i].value = rev_pack.value;
                    send_pack.op_info[i].valid = rev_pack.valid;
                    send_pack.op_info[i].last_uop = rev_pack.last_uop;
                    
                    send_pack.op_info[i].rob_id = rev_pack.rob_id;
                    send_pack.op_info[i].pc = rev_pack.pc;
                    send_pack.op_info[i].imm = rev_pack.imm;
                    send_pack.op_info[i].has_exception = rev_pack.has_exception;
                    send_pack.op_info[i].exception_id = rev_pack.exception_id;
                    send_pack.op_info[i].exception_value = rev_pack.exception_value;
                    
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
                    send_pack.op_info[i].op = rev_pack.op;
                    send_pack.op_info[i].op_unit = rev_pack.op_unit;
                    memcpy(&send_pack.op_info[i].sub_op, &rev_pack.sub_op, sizeof(rev_pack.sub_op));
                }
                else
                {
                    send_pack.op_info[i] = integer_issue_readreg_op_info_t();
                }
            }
            
            //build feedback_pack
            for(auto i = 0;i < INTEGER_ISSUE_WIDTH;i++)
            {
                feedback_pack.wakeup_valid[i] = selected_valid[i] && wakeup_rd_valid[selected_issue_id[i]];
                feedback_pack.wakeup_rd[i] = wakeup_rd_valid[selected_issue_id[i]];
                feedback_pack.wakeup_shift[i] = wakeup_shift[selected_issue_id[i]];
            }
            
            //calculate busy
            for(auto i = 0;i < ALU_UNIT_NUM;i++)
            {
                if(alu_idle[i])
                {
                    if(alu_busy_shift[i] == 1)
                    {
                        alu_idle[i] = false;
                    }
                    
                    if(alu_busy_shift[i] > 0)
                    {
                        alu_busy_shift[i]--;
                    }
                }
                else
                {
                    if(alu_idle_shift[i] == 1)
                    {
                        alu_idle[i] = true;
                    }
                    
                    if(alu_idle_shift[i] > 0)
                    {
                        alu_idle_shift[i]--;
                    }
                }
            }
    
            for(auto i = 0;i < BRU_UNIT_NUM;i++)
            {
                if(bru_idle[i])
                {
                    if(bru_busy_shift[i] == 1)
                    {
                        bru_idle[i] = false;
                    }
            
                    if(bru_busy_shift[i] > 0)
                    {
                        bru_busy_shift[i]--;
                    }
                }
                else
                {
                    if(bru_idle_shift[i] == 1)
                    {
                        bru_idle[i] = true;
                    }
            
                    if(bru_idle_shift[i] > 0)
                    {
                        bru_idle_shift[i]--;
                    }
                }
            }
    
            for(auto i = 0;i < CSR_UNIT_NUM;i++)
            {
                if(csr_idle[i])
                {
                    if(csr_busy_shift[i] == 1)
                    {
                        csr_idle[i] = false;
                    }
            
                    if(csr_busy_shift[i] > 0)
                    {
                        csr_busy_shift[i]--;
                    }
                }
                else
                {
                    if(csr_idle_shift[i] == 1)
                    {
                        csr_idle[i] = true;
                    }
            
                    if(csr_idle_shift[i] > 0)
                    {
                        csr_idle_shift[i]--;
                    }
                }
            }
    
            for(auto i = 0;i < DIV_UNIT_NUM;i++)
            {
                if(div_idle[i])
                {
                    if(div_busy_shift[i] == 1)
                    {
                        div_idle[i] = false;
                    }
            
                    if(div_busy_shift[i] > 0)
                    {
                        div_busy_shift[i]--;
                    }
                }
                else
                {
                    if(div_idle_shift[i] == 1)
                    {
                        div_idle[i] = true;
                    }
            
                    if(div_idle_shift[i] > 0)
                    {
                        div_idle_shift[i]--;
                    }
                }
            }
    
            for(auto i = 0;i < MUL_UNIT_NUM;i++)
            {
                if(mul_idle[i])
                {
                    if(mul_busy_shift[i] == 1)
                    {
                        mul_idle[i] = false;
                    }
            
                    if(mul_busy_shift[i] > 0)
                    {
                        mul_busy_shift[i]--;
                    }
                }
                else
                {
                    if(mul_idle_shift[i] == 1)
                    {
                        mul_idle[i] = true;
                    }
            
                    if(mul_idle_shift[i] > 0)
                    {
                        mul_idle_shift[i]--;
                    }
                }
            }
            
            //update idle and busy shift
            if(selected_valid[0])
            {
                switch(op_unit_seq[selected_issue_id[0]])
                {
                    case 1 << ALU_SHIFT:
                        alu_idle_shift[0] = new_idle_shift[0];
                        alu_busy_shift[0] = new_busy_shift[0];
                        break;
                        
                    case 1 << BRU_SHIFT:
                        bru_idle_shift[0] = new_idle_shift[0];
                        bru_busy_shift[0] = new_busy_shift[0];
                        break;
                        
                    case 1 << CSR_SHIFT:
                        csr_idle_shift[0] = new_idle_shift[0];
                        csr_busy_shift[0] = new_busy_shift[0];
                        break;
    
                    case 1 << MUL_SHIFT:
                        mul_idle_shift[0] = new_idle_shift[0];
                        mul_busy_shift[0] = new_busy_shift[0];
                        break;
                        
                    default:
                        verify(0);
                        break;
                }
            }
    
            if(selected_valid[1])
            {
                switch(op_unit_seq[selected_issue_id[1]])
                {
                    case 1 << ALU_SHIFT:
                        alu_idle_shift[1] = new_idle_shift[1];
                        alu_busy_shift[1] = new_busy_shift[1];
                        break;
            
                    case 1 << DIV_SHIFT:
                        div_idle_shift[0] = new_idle_shift[1];
                        div_busy_shift[0] = new_busy_shift[1];
                        break;
            
                    case 1 << MUL_SHIFT:
                        mul_idle_shift[1] = new_idle_shift[1];
                        mul_busy_shift[1] = new_busy_shift[1];
                        break;
            
                    default:
                        verify(0);
                        break;
                }
            }
            
            integer_issue_readreg_port->set(send_pack);
        }
        else
        {
            integer_issue_readreg_port->set(integer_issue_readreg_pack_t());
        }
        
        return feedback_pack;
    }
    
    void integer_issue::run_wakeup(const integer_issue_output_feedback_pack_t &integer_issue_output_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        if(!commit_feedback_pack.flush)
        {
            for(auto i = 0;i < INTEGER_ISSUE_QUEUE_SIZE;i++)
            {
                if(issue_q.is_valid(i))
                {
                    auto item = issue_q.customer_get_item(i);
                    
                    //integer_issue_output feedback
                    for(auto j = 0;j < INTEGER_ISSUE_WIDTH;j++)
                    {
                        if(integer_issue_output_feedback_pack.wakeup_valid[j])
                        {
                            if(!this->src1_ready[i])
                            {
                                verify(item.arg1_src == arg_src_t::reg);
                                
                                if(item.rs1 == integer_issue_output_feedback_pack.wakeup_rd[j])
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
                                verify(item.arg2_src == arg_src_t::reg);
                                
                                if(item.rs2 == integer_issue_output_feedback_pack.wakeup_rd[j])
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
                    
                    //execute feedback
                    for(auto j = 0;j < EXECUTE_UNIT_NUM;j++)
                    {
                        if(execute_feedback_pack.channel[j].enable)
                        {
                            if(!this->src1_ready[i])
                            {
                                verify(item.arg1_src == arg_src_t::reg);
                                verify(item.rs1_need_map);
                                
                                if(item.rs1 == execute_feedback_pack.channel[j].phy_id)
                                {
                                    this->src1_ready[i] = true;
                                }
                            }
        
                            if(!this->src2_ready[i])
                            {
                                verify(item.arg2_src == arg_src_t::reg);
                                verify(item.rs2_need_map);
                                
                                if(item.rs2 == execute_feedback_pack.channel[j].phy_id)
                                {
                                    this->src2_ready[i] = true;
                                }
                            }
                        }
                    }
                    
                    issue_q.set_item(i, item);
                }
            }
        }
    }
    
    integer_issue_feedback_pack_t integer_issue::run_input(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        integer_issue_feedback_pack_t feedback_pack;
        feedback_pack.stall = this->busy;//generate stall signal to prevent dispatch from dispatching new instructions
        
        if(!commit_feedback_pack.flush)
        {
            dispatch_issue_pack_t rev_pack;
            
            if(this->busy)
            {
                rev_pack = this->hold_rev_pack;
            }
            else
            {
                rev_pack = dispatch_integer_issue_port->get();
            }
            
            for(auto i = 0;i < DISPATCH_WIDTH;i++)
            {
                if(rev_pack.op_info[i].enable)
                {
                    issue_queue_item_t item;
                    
                    item.enable = rev_pack.op_info[i].enable;
                    item.value = rev_pack.op_info[i].value;
                    item.valid = rev_pack.op_info[i].valid;
                    item.last_uop = rev_pack.op_info[i].last_uop;
                    
                    item.rob_id = rev_pack.op_info[i].rob_id;
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
                    memcpy(&item.sub_op, &rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
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
                                for(auto j = 0;j < EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs1_phy)
                                    {
                                        src1_ready[issue_id] = true;
                                        break;
                                    }
                                }
                                
                                if(!src1_ready[issue_id])
                                {
                                    for(auto j = 0;j < EXECUTE_UNIT_NUM;j++)
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
                                for(auto j = 0;j < EXECUTE_UNIT_NUM;j++)
                                {
                                    if(execute_feedback_pack.channel[j].enable && execute_feedback_pack.channel[j].phy_id == rev_pack.op_info[i].rs2_phy)
                                    {
                                        src2_ready[issue_id] = true;
                                        break;
                                    }
                                }
                                
                                if(!src2_ready[issue_id])
                                {
                                    for(auto j = 0;j < EXECUTE_UNIT_NUM;j++)
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
                        
                        //port assignment
                        if(rev_pack.op_info[i].op_unit == op_unit_t::alu || rev_pack.op_info[i].op_unit == op_unit_t::mul)
                        {
                            port_index[issue_id] = next_port_index;
                            next_port_index = (next_port_index + 1) % INTEGER_ISSUE_WIDTH;
                        }
                        else
                        {
                            port_index[issue_id] = 0;
                        }
                        
                        //op_unit_seq generation
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                op_unit_seq[issue_id] = 1 << ALU_SHIFT;
                                break;
                                
                            case op_unit_t::bru:
                                op_unit_seq[issue_id] = 1 << BRU_SHIFT;
                                break;
                                
                            case op_unit_t::csr:
                                op_unit_seq[issue_id] = 1 << CSR_SHIFT;
                                break;
                                
                            case op_unit_t::div:
                                op_unit_seq[issue_id] = 1 << DIV_SHIFT;
                                break;
                                
                            case op_unit_t::mul:
                                op_unit_seq[issue_id] = 1 << MUL_SHIFT;
                                break;
                                
                            default:
                                verify(0);
                                break;
                        }
                        
                        //set age information
                        rob_id[issue_id] = rev_pack.op_info[i].rob_id;
                        rob_id_stage[issue_id] = rev_pack.op_info[i].rob_id_stage;
                        
                        //set wakeup information
                        wakeup_rd[issue_id] = rev_pack.op_info[i].rd_phy;
                        wakeup_rd_valid[issue_id] = rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception && rev_pack.op_info[i].need_rename;
    
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                wakeup_shift[issue_id] = integer_issue::latency_to_wakeup_shift(ALU_LATENCY);
                                break;
        
                            case op_unit_t::bru:
                                wakeup_shift[issue_id] = integer_issue::latency_to_wakeup_shift(BRU_LATENCY);
                                break;
        
                            case op_unit_t::csr:
                                wakeup_shift[issue_id] = integer_issue::latency_to_wakeup_shift(CSR_LATENCY);
                                break;
        
                            case op_unit_t::div:
                                wakeup_shift[issue_id] = integer_issue::latency_to_wakeup_shift(DIV_LATENCY);
                                break;
        
                            case op_unit_t::mul:
                                wakeup_shift[issue_id] = integer_issue::latency_to_wakeup_shift(MUL_LATENCY);
                                break;
        
                            default:
                                verify(0);
                                break;
                        }
                        
                        //set execute unit information
                        switch(rev_pack.op_info[i].op_unit)
                        {
                            case op_unit_t::alu:
                                latency_to_idle_busy_shift(ALU_LATENCY, new_idle_shift[issue_id], new_busy_shift[issue_id]);
                                break;
        
                            case op_unit_t::bru:
                                latency_to_idle_busy_shift(BRU_LATENCY, new_idle_shift[issue_id], new_busy_shift[issue_id]);
                                break;
        
                            case op_unit_t::csr:
                                latency_to_idle_busy_shift(CSR_LATENCY, new_idle_shift[issue_id], new_busy_shift[issue_id]);
                                break;
        
                            case op_unit_t::div:
                                latency_to_idle_busy_shift(DIV_LATENCY, new_idle_shift[issue_id], new_busy_shift[issue_id]);
                                break;
        
                            case op_unit_t::mul:
                                latency_to_idle_busy_shift(MUL_LATENCY, new_idle_shift[issue_id], new_busy_shift[issue_id]);
                                break;
        
                            default:
                                verify(0);
                                break;
                        }
                    }
                    else
                    {
                        //entry busy state
                        this->busy = true;
    
                        //let remain instructions keep right alignment
                        for(auto j = 0;j < DISPATCH_WIDTH;j++)
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
                    }
                }
            }
        }
        else
        {
            issue_q.flush();
            busy = false;
            hold_rev_pack = dispatch_issue_pack_t();
            
            for(auto i = 0;i < ALU_UNIT_NUM;i++)
            {
                alu_idle[i] = 1;
                alu_idle_shift[i] = 0;
                alu_busy_shift[i] = 0;
            }
            
            for(auto i = 0;i < BRU_UNIT_NUM;i++)
            {
                bru_idle[i] = 1;
                bru_idle_shift[i] = 0;
                bru_busy_shift[i] = 0;
            }
            
            for(auto i = 0;i < CSR_UNIT_NUM;i++)
            {
                csr_idle[i] = 1;
                csr_idle_shift[i] = 0;
                csr_busy_shift[i] = 0;
            }
            
            for(auto i = 0;i < DIV_UNIT_NUM;i++)
            {
                div_idle[i] = 1;
                div_idle_shift[i] = 0;
                div_busy_shift[i] = 0;
            }
            
            for(auto i = 0;i < MUL_UNIT_NUM;i++)
            {
                mul_idle[i] = 1;
                mul_idle_shift[i] = 0;
                mul_busy_shift[i] = 0;
            }
            
            next_port_index = 0;
        }
        
        return feedback_pack;
    }
    
    uint32_t integer_issue::latency_to_wakeup_shift(uint32_t latency)
    {
        return (latency == 1) ? 0 : (1 << (latency - 1));
    }
    
    void integer_issue::latency_to_idle_busy_shift(uint32_t latency, uint32_t &idle_shift, uint32_t &busy_shift)
    {
        idle_shift = (latency == 1) ? 0 : (1 << (latency - 1));
        busy_shift = (latency == 1) ? 0 : (1 << 1);
    }
    
    void integer_issue::print(std::string indent)
    {
        issue_q.print(indent);
    }
    
    json integer_issue::get_json()
    {
        return issue_q.get_json();
    }
}