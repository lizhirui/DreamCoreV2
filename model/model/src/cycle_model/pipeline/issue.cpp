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
#include "cycle_model/pipeline/issue.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/issue_queue.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/pipeline/rename_issue.h"
#include "cycle_model/pipeline/issue_readreg.h"
#include "cycle_model/pipeline/readreg.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/commit.h"

namespace pipeline
{
    issue::issue(component::port<rename_issue_pack_t> *rename_issue_port, component::port<issue_readreg_pack_t> *issue_readreg_port, component::store_buffer *store_buffer) : issue_q(component::issue_queue<issue_queue_item_t>(ISSUE_QUEUE_SIZE)), tdb(TRACE_ISSUE)
    {
        this->rename_issue_port = rename_issue_port;
        this->issue_readreg_port = issue_readreg_port;
        this->store_buffer = store_buffer;
        this->reset();
    }
    
    void issue::reset()
    {
        this->issue_q.reset();
        this->busy = false;
        this->is_inst_waiting = false;
        this->inst_waiting_rob_id = 0;
        this->hold_rev_pack = rename_issue_pack_t();
        this->alu_index = 0;
        this->bru_index = 0;
        this->csr_index = 0;
        this->div_index = 0;
        this->lsu_index = 0;
        this->mul_index = 0;
        
        for(auto i = 0;i < ALU_UNIT_NUM;i++)
        {
            this->alu_busy[i] = false;
            this->alu_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < BRU_UNIT_NUM;i++)
        {
            this->bru_busy[i] = false;
            this->bru_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < CSR_UNIT_NUM;i++)
        {
            this->csr_busy[i] = false;
            this->csr_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < DIV_UNIT_NUM;i++)
        {
            this->div_busy[i] = false;
            this->div_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < LSU_UNIT_NUM;i++)
        {
            this->lsu_busy[i] = false;
            this->lsu_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < MUL_UNIT_NUM;i++)
        {
            this->mul_busy[i] = false;
            this->mul_busy_countdown[i] = 0;
        }
        
        for(auto i = 0;i < ISSUE_QUEUE_SIZE;i++)
        {
            wakeup_countdown_src1[i] = 0;
            wakeup_valid_src1[i] = false;
            wakeup_countdown_src2[i] = 0;
            wakeup_valid_src2[i] = false;
        }
    }
    
    issue_feedback_pack_t issue::run(readreg_feedback_pack_t readreg_feedback_pack, commit_feedback_pack_t commit_feedback_pack)
    {
        issue_feedback_pack_t feedback_pack;
        
        feedback_pack.stall = false;//cancel stall signal temporarily
        
        if(!commit_feedback_pack.flush)
        {
            auto inst_waiting_ok = false;
            
            for(auto i = 0;i < COMMIT_WIDTH;i++)
            {
                if(commit_feedback_pack.committed_rob_id_valid[i] && (commit_feedback_pack.committed_rob_id[i] == inst_waiting_rob_id))
                {
                    inst_waiting_ok = true;
                    break;
                }
            }
            
            //handle output
            if(issue_q.customer_is_empty())
            {
                if(inst_waiting_ok)
                {
                    this->is_inst_waiting = false;
                }
            }
            else
            {
                if(!this->is_inst_waiting || inst_waiting_ok)
                {
                    this->is_inst_waiting = false;
                    
                    //get up to 2 items from issue_queue
                    auto has_lsu_op = false;
                    auto has_csr_op = false;
                    auto has_mret_op = false;
                    auto item_id = 0;
                    issue_queue_item_t items[ISSUE_WIDTH];
                    
                    for(auto i = 0;i < issue_q.get_size();i++)
                    {
                        items[item_id] = issue_q.customer_get_item(i);
                        
                        if(issue_q.is_valid(i) &&
                           (items[item_id].src1_ready || wakeup_valid_src1[item_id]) &&
                           (items[item_id].src2_ready || wakeup_valid_src2[item_id]) &&
                           ())
                        {
                        
                        }
                    }
                }
            }
        }
    }
    
    void issue::print(std::string indent)
    {
        issue_q.print(indent);
    }
    
    json issue::get_json()
    {
        return issue_q.get_json();
    }
}