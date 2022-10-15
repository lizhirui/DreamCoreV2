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
#include "wb_commit.h"
#include "commit.h"

namespace pipeline
{
    class wb : public if_reset_t
    {
        private:
            component::port<execute_wb_pack_t> **alu_wb_port;
            component::port<execute_wb_pack_t> **bru_wb_port;
            component::port<execute_wb_pack_t> **csr_wb_port;
            component::port<execute_wb_pack_t> **div_wb_port;
            component::port<execute_wb_pack_t> **lsu_wb_port;
            component::port<execute_wb_pack_t> **mul_wb_port;
            
            std::vector<component::port<execute_wb_pack_t> *> execute_wb_port;
            
            component::port<wb_commit_pack_t> *wb_commit_port;
            
            component::regfile<uint32_t> *phy_regfile;
            trace::trace_database tdb;
        
        public:
            wb(component::port<execute_wb_pack_t> **alu_wb_port, component::port<execute_wb_pack_t> **bru_wb_port, component::port<execute_wb_pack_t> **csr_wb_port, component::port<execute_wb_pack_t> **div_wb_port, component::port<execute_wb_pack_t> **lsu_wb_port, component::port<execute_wb_pack_t> **mul_wb_port, component::port<wb_commit_pack_t> *wb_commit_port, component::regfile<uint32_t> *phy_regfile);
            void init();
            virtual void reset();
            void run(commit_feedback_pack_t commit_feedback_pack);
    };
}