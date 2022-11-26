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
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/age_compare.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/execute_commit.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model::pipeline
{
    wb::wb(global_inst *global, component::port<execute_wb_pack_t> **alu_wb_port, component::port<execute_wb_pack_t> **bru_wb_port, component::port<execute_wb_pack_t> **csr_wb_port, component::port<execute_wb_pack_t> **div_wb_port, component::port<execute_wb_pack_t> **mul_wb_port, component::port<execute_wb_pack_t> **lu_wb_port, component::regfile<uint32_t> *phy_regfile) : tdb(TRACE_WB)
    {
        this->global = global;
        this->alu_wb_port = alu_wb_port;
        this->bru_wb_port = bru_wb_port;
        this->csr_wb_port = csr_wb_port;
        this->div_wb_port = div_wb_port;
        this->mul_wb_port = mul_wb_port;
        this->lu_wb_port = lu_wb_port;
        this->phy_regfile = phy_regfile;
        this->wb::reset();
    }
    
    void wb::reset()
    {
    
    }
    
    void wb::init()
    {
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(alu_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(bru_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(csr_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(div_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(mul_wb_port[i]);
        }
    
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            this->execute_wb_port.push_back(lu_wb_port[i]);
        }
    }
    
    wb_feedback_pack_t wb::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        wb_feedback_pack_t feedback_pack;
        
        for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
        {
            feedback_pack.channel[i].enable = false;
            feedback_pack.channel[i].phy_id = 0;
            feedback_pack.channel[i].value = 0;
        }
        
        if(!commit_feedback_pack.flush)
        {
            for(uint32_t i = 0;i < this->execute_wb_port.size();i++)
            {
                execute_wb_pack_t rev_pack;
                rev_pack = this->execute_wb_port[i]->get();
    
                if(bru_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                {
                    continue;//skip this instruction due to which is younger than the flush age
                }
    
                if(sau_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                {
                    continue;//skip this instruction due to which is younger than the flush age
                }
                
                if(rev_pack.enable && rev_pack.valid && !rev_pack.has_exception && rev_pack.need_rename)
                {
                    phy_regfile->write(rev_pack.rd_phy, rev_pack.rd_value, true, rev_pack.rob_id, rev_pack.rob_id_stage, false);
                }
                
                feedback_pack.channel[i].enable = rev_pack.enable && rev_pack.valid && rev_pack.need_rename && !rev_pack.has_exception;
                feedback_pack.channel[i].phy_id = rev_pack.rd_phy;
                feedback_pack.channel[i].value = rev_pack.rd_value;
            }
        }
        
        return feedback_pack;
    }
}