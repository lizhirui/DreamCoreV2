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
#include "cycle_model/pipeline/dispatch.h"
#include "cycle_model/component/port.h"
#include "cycle_model/pipeline/rename_dispatch.h"
#include "cycle_model/pipeline/dispatch_issue.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/lsu_issue.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model::pipeline
{
    dispatch::dispatch(component::port<rename_dispatch_pack_t> *rename_dispatch_port, component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port, component::port<dispatch_issue_pack_t> *dispatch_lsu_issue_port, component::store_buffer *store_buffer) : tdb(TRACE_DISPATCH)
    {
        this->rename_dispatch_port = rename_dispatch_port;
        this->dispatch_integer_issue_port = dispatch_integer_issue_port;
        this->dispatch_lsu_issue_port = dispatch_lsu_issue_port;
        this->store_buffer = store_buffer;
        this->dispatch::reset();
    }
    
    void dispatch::reset()
    {
        this->integer_busy = false;
        this->lsu_busy = false;
        this->busy = false;
        this->hold_integer_issue_pack = dispatch_issue_pack_t();
        this->hold_lsu_issue_pack = dispatch_issue_pack_t();
        this->is_inst_waiting = false;
        this->inst_waiting_rob_id = 0;
        this->is_stbuf_empty_waiting = false;
    }
    
    dispatch_feedback_pack_t dispatch::run(const integer_issue_feedback_pack_t &integer_issue_feedback_pack, const lsu_issue_feedback_pack_t &lsu_issue_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        dispatch_issue_pack_t integer_issue_pack;
        dispatch_issue_pack_t lsu_issue_pack;
    
        dispatch_feedback_pack_t feedback_pack;
    
        //generate inst_waiting_ok signal
        auto inst_waiting_ok = false;
        
        for(uint32_t i = 0;i < COMMIT_WIDTH;i++)
        {
            if(commit_feedback_pack.committed_rob_id_valid[i] && commit_feedback_pack.committed_rob_id[i] == this->inst_waiting_rob_id)
            {
                inst_waiting_ok = true;
                break;
            }
        }
        
        //generate stall signal
        feedback_pack.stall = this->integer_busy || this->lsu_busy || this->busy || (this->is_inst_waiting && !inst_waiting_ok) || (this->is_stbuf_empty_waiting && !store_buffer->customer_is_empty());
        
        if(!commit_feedback_pack.flush)
        {
            if((!is_inst_waiting || inst_waiting_ok) && (!is_stbuf_empty_waiting || store_buffer->customer_is_empty()))
            {
                //remove all waiting state
                this->is_inst_waiting = false;
                this->is_stbuf_empty_waiting = false;
                
                //if integer and lsu issue are all not busy
                if(!(this->integer_busy || this->lsu_busy))
                {
                    //if it's busy state, then old rev_pack should be used to re-generate issue pack
                    if(!this->busy)
                    {
                        rev_pack = rename_dispatch_port->get();
                    }
                    else
                    {
                        this->busy = false;
                    }
    
                    uint32_t integer_issue_id = 0;
                    uint32_t lsu_issue_id = 0;
                    auto found_inst_waiting = false;
                    auto found_fence = false;
                    uint32_t found_rob_id = 0;
                    
                    for(uint32_t i = 0;i < RENAME_WIDTH;i++)
                    {
                        if(rev_pack.op_info[i].enable)
                        {
                            if(rev_pack.op_info[i].op_unit != op_unit_t::lsu)
                            {
                                integer_issue_pack.op_info[integer_issue_id].enable = rev_pack.op_info[i].enable;
                                integer_issue_pack.op_info[integer_issue_id].value = rev_pack.op_info[i].value;
                                integer_issue_pack.op_info[integer_issue_id].valid = rev_pack.op_info[i].valid;
                                integer_issue_pack.op_info[integer_issue_id].last_uop = rev_pack.op_info[i].last_uop;
                                
                                integer_issue_pack.op_info[integer_issue_id].rob_id = rev_pack.op_info[i].rob_id;
                                integer_issue_pack.op_info[integer_issue_id].rob_id_stage = rev_pack.op_info[i].rob_id_stage;
                                integer_issue_pack.op_info[integer_issue_id].pc = rev_pack.op_info[i].pc;
                                integer_issue_pack.op_info[integer_issue_id].imm = rev_pack.op_info[i].imm;
                                integer_issue_pack.op_info[integer_issue_id].has_exception = rev_pack.op_info[i].has_exception;
                                integer_issue_pack.op_info[integer_issue_id].exception_id = rev_pack.op_info[i].exception_id;
                                integer_issue_pack.op_info[integer_issue_id].exception_value = rev_pack.op_info[i].exception_value;
                                
                                integer_issue_pack.op_info[integer_issue_id].rs1 = rev_pack.op_info[i].rs1;
                                integer_issue_pack.op_info[integer_issue_id].arg1_src = rev_pack.op_info[i].arg1_src;
                                integer_issue_pack.op_info[integer_issue_id].rs1_need_map = rev_pack.op_info[i].rs1_need_map;
                                integer_issue_pack.op_info[integer_issue_id].rs1_phy = rev_pack.op_info[i].rs1_phy;
                                
                                integer_issue_pack.op_info[integer_issue_id].rs2 = rev_pack.op_info[i].rs2;
                                integer_issue_pack.op_info[integer_issue_id].arg2_src = rev_pack.op_info[i].arg2_src;
                                integer_issue_pack.op_info[integer_issue_id].rs2_need_map = rev_pack.op_info[i].rs2_need_map;
                                integer_issue_pack.op_info[integer_issue_id].rs2_phy = rev_pack.op_info[i].rs2_phy;
                                
                                integer_issue_pack.op_info[integer_issue_id].rd = rev_pack.op_info[i].rd;
                                integer_issue_pack.op_info[integer_issue_id].rd_enable = rev_pack.op_info[i].rd_enable;
                                integer_issue_pack.op_info[integer_issue_id].need_rename = rev_pack.op_info[i].need_rename;
                                integer_issue_pack.op_info[integer_issue_id].rd_phy = rev_pack.op_info[i].rd_phy;
                                
                                integer_issue_pack.op_info[integer_issue_id].csr = rev_pack.op_info[i].csr;
                                integer_issue_pack.op_info[integer_issue_id].op = rev_pack.op_info[i].op;
                                integer_issue_pack.op_info[integer_issue_id].op_unit = rev_pack.op_info[i].op_unit;
                                memcpy((void *)&integer_issue_pack.op_info[integer_issue_id].sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                                integer_issue_id++;
                            }
                            else
                            {
                                lsu_issue_pack.op_info[lsu_issue_id].enable = rev_pack.op_info[i].enable;
                                lsu_issue_pack.op_info[lsu_issue_id].value = rev_pack.op_info[i].value;
                                lsu_issue_pack.op_info[lsu_issue_id].valid = rev_pack.op_info[i].valid;
                                lsu_issue_pack.op_info[lsu_issue_id].last_uop = rev_pack.op_info[i].last_uop;
        
                                lsu_issue_pack.op_info[lsu_issue_id].rob_id = rev_pack.op_info[i].rob_id;
                                lsu_issue_pack.op_info[lsu_issue_id].rob_id_stage = rev_pack.op_info[i].rob_id_stage;
                                lsu_issue_pack.op_info[lsu_issue_id].pc = rev_pack.op_info[i].pc;
                                lsu_issue_pack.op_info[lsu_issue_id].imm = rev_pack.op_info[i].imm;
                                lsu_issue_pack.op_info[lsu_issue_id].has_exception = rev_pack.op_info[i].has_exception;
                                lsu_issue_pack.op_info[lsu_issue_id].exception_id = rev_pack.op_info[i].exception_id;
                                lsu_issue_pack.op_info[lsu_issue_id].exception_value = rev_pack.op_info[i].exception_value;
        
                                lsu_issue_pack.op_info[lsu_issue_id].rs1 = rev_pack.op_info[i].rs1;
                                lsu_issue_pack.op_info[lsu_issue_id].arg1_src = rev_pack.op_info[i].arg1_src;
                                lsu_issue_pack.op_info[lsu_issue_id].rs1_need_map = rev_pack.op_info[i].rs1_need_map;
                                lsu_issue_pack.op_info[lsu_issue_id].rs1_phy = rev_pack.op_info[i].rs1_phy;
        
                                lsu_issue_pack.op_info[lsu_issue_id].rs2 = rev_pack.op_info[i].rs2;
                                lsu_issue_pack.op_info[lsu_issue_id].arg2_src = rev_pack.op_info[i].arg2_src;
                                lsu_issue_pack.op_info[lsu_issue_id].rs2_need_map = rev_pack.op_info[i].rs2_need_map;
                                lsu_issue_pack.op_info[lsu_issue_id].rs2_phy = rev_pack.op_info[i].rs2_phy;
        
                                lsu_issue_pack.op_info[lsu_issue_id].rd = rev_pack.op_info[i].rd;
                                lsu_issue_pack.op_info[lsu_issue_id].rd_enable = rev_pack.op_info[i].rd_enable;
                                lsu_issue_pack.op_info[lsu_issue_id].need_rename = rev_pack.op_info[i].need_rename;
                                lsu_issue_pack.op_info[lsu_issue_id].rd_phy = rev_pack.op_info[i].rd_phy;
        
                                lsu_issue_pack.op_info[lsu_issue_id].csr = rev_pack.op_info[i].csr;
                                lsu_issue_pack.op_info[lsu_issue_id].op = rev_pack.op_info[i].op;
                                lsu_issue_pack.op_info[lsu_issue_id].op_unit = rev_pack.op_info[i].op_unit;
                                memcpy((void *)&lsu_issue_pack.op_info[lsu_issue_id].sub_op, (void *)&rev_pack.op_info[i].sub_op, sizeof(rev_pack.op_info[i].sub_op));
                                lsu_issue_id++;
                            }
        
                            if(rev_pack.op_info[i].valid && !rev_pack.op_info[i].has_exception)
                            {
                                if(rev_pack.op_info[i].op_unit == op_unit_t::csr || rev_pack.op_info[i].op == op_t::mret)
                                {
                                    found_inst_waiting = true;
                                    found_rob_id = this->rev_pack.op_info[i].rob_id;
                                    verify_only(i == 0);
                                    break;
                                }
                                else if(rev_pack.op_info[i].op == op_t::fence)
                                {
                                    found_fence = true;
                                    found_rob_id = this->rev_pack.op_info[i].rob_id;
                                }
                            }
                        }
                    }
        
                    if(found_inst_waiting)
                    {
                        if(!(!integer_issue_feedback_pack.stall && commit_feedback_pack.next_handle_rob_id_valid && (commit_feedback_pack.next_handle_rob_id == found_rob_id)))
                        {
                            this->busy = true;
                            dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                            dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                        }
                        else
                        {
                            dispatch_integer_issue_port->set(integer_issue_pack);
                            dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                            this->is_inst_waiting = true;
                            this->inst_waiting_rob_id = found_rob_id;
                        }
                    }
                    else if(found_fence && (((integer_issue_id > 0) && integer_issue_feedback_pack.stall) || ((lsu_issue_id > 0) && lsu_issue_feedback_pack.stall)))
                    {
                        this->busy = true;
                        dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                        dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                    }
                    else
                    {
                        if(found_fence)
                        {
                            this->is_stbuf_empty_waiting = true;
                            this->is_inst_waiting = true;
                            this->inst_waiting_rob_id = found_rob_id;
                        }
                        
                        if(integer_issue_id > 0)
                        {
                            if(integer_issue_feedback_pack.stall)
                            {
                                this->hold_integer_issue_pack = integer_issue_pack;
                                this->integer_busy = true;
                            }
                            else
                            {
                                dispatch_integer_issue_port->set(integer_issue_pack);
                            }
                        }
                        else
                        {
                            if(!integer_issue_feedback_pack.stall)
                            {
                                dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                            }
                        }
                        
                        if(lsu_issue_id > 0)
                        {
                            if(lsu_issue_feedback_pack.stall)
                            {
                                this->hold_lsu_issue_pack = lsu_issue_pack;
                                this->lsu_busy = true;
                            }
                            else
                            {
                                dispatch_lsu_issue_port->set(lsu_issue_pack);
                            }
                        }
                        else
                        {
                            if(!lsu_issue_feedback_pack.stall)
                            {
                                dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                            }
                        }
                    }
                }
                else
                {
                    if(!integer_issue_feedback_pack.stall)
                    {
                        if(this->integer_busy)
                        {
                            dispatch_integer_issue_port->set(this->hold_integer_issue_pack);
                            this->integer_busy = false;
                        }
                        else
                        {
                            dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                        }
                    }
                    
                    if(!lsu_issue_feedback_pack.stall)
                    {
                        if(this->lsu_busy)
                        {
                            dispatch_lsu_issue_port->set(this->hold_lsu_issue_pack);
                            this->lsu_busy = false;
                        }
                        else
                        {
                            dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                        }
                    }
                }
            }
            else if(is_inst_waiting)
            {
                dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                
                if(inst_waiting_ok)
                {
                    this->is_inst_waiting = false;
                    this->inst_waiting_rob_id = 0;
                    
                    if(is_stbuf_empty_waiting && store_buffer->customer_is_empty())
                    {
                        this->is_stbuf_empty_waiting = false;
                    }
                }
            }
            else if(is_stbuf_empty_waiting && store_buffer->customer_is_empty())
            {
                dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
                this->is_stbuf_empty_waiting = false;
            }
            else
            {
                dispatch_integer_issue_port->set(dispatch_issue_pack_t());
                dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
            }
        }
        else
        {
            dispatch_integer_issue_port->set(dispatch_issue_pack_t());
            dispatch_lsu_issue_port->set(dispatch_issue_pack_t());
            this->integer_busy = false;
            this->lsu_busy = false;
            this->busy = false;
            this->hold_integer_issue_pack = dispatch_issue_pack_t();
            this->hold_lsu_issue_pack = dispatch_issue_pack_t();
            this->rev_pack = rename_dispatch_pack_t();
            this->is_inst_waiting = false;
            this->inst_waiting_rob_id = 0;
            this->is_stbuf_empty_waiting = false;
        }
        
        return feedback_pack;
    }
    
    json dispatch::get_json()
    {
        json t;
        
        t["integer_busy"] = this->integer_busy;
        t["lsu_busy"] = this->lsu_busy;
        t["busy"] = this->busy;
        t["is_inst_waiting"] = this->is_inst_waiting;
        t["inst_waiting_rob_id"] = this->inst_waiting_rob_id;
        t["is_stbuf_empty_waiting"] = this->is_stbuf_empty_waiting;
        return t;
    }
}