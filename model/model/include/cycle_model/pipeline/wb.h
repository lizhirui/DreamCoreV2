/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../component/port.h"
#include "../component/regfile.h"
#include "execute_wb.h"
#include "execute_commit.h"
#include "execute/bru.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct wb_feedback_channel_t : if_print_t
    {
        bool enable = false;
        uint32_t phy_id = 0;
        uint32_t value = 0;
        
        virtual json get_json()
        {
            json t;
            t["enable"] = enable;
            t["phy_id"] = phy_id;
            t["value"] = value;
            return t;
        }
    }wb_feedback_channel_t;
    
    typedef struct wb_feedback_pack_t : if_print_t
    {
        wb_feedback_channel_t channel[EXECUTE_UNIT_NUM];
        
        virtual json get_json()
        {
            json t = json::array();
            
            for(uint32_t i = 0;i < EXECUTE_UNIT_NUM;i++)
            {
                t.push_back(channel[i].get_json());
            }
            
            return t;
        }
    }wb_feedback_pack_t;
    
    class wb : public if_reset_t
    {
        private:
            global_inst *global;
            component::port<execute_wb_pack_t> **alu_wb_port;
            component::port<execute_wb_pack_t> **bru_wb_port;
            component::port<execute_wb_pack_t> **csr_wb_port;
            component::port<execute_wb_pack_t> **div_wb_port;
            component::port<execute_wb_pack_t> **mul_wb_port;
            component::port<execute_wb_pack_t> **lu_wb_port;
            
            std::vector<component::port<execute_wb_pack_t> *> execute_wb_port;
            
            component::regfile<uint32_t> *phy_regfile;
            trace::trace_database tdb;
        
        public:
            wb(global_inst *global, component::port<execute_wb_pack_t> **alu_wb_port, component::port<execute_wb_pack_t> **bru_wb_port, component::port<execute_wb_pack_t> **csr_wb_port, component::port<execute_wb_pack_t> **div_wb_port, component::port<execute_wb_pack_t> **mul_wb_port, component::port<execute_wb_pack_t> **lu_wb_port, component::regfile<uint32_t> *phy_regfile);
            void init();
            virtual void reset();
            wb_feedback_pack_t run(const execute::bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}