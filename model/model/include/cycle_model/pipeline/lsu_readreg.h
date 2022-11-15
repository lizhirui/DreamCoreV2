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
#include "../component/handshake_dff.h"
#include "../component/regfile.h"
#include "lsu_issue_readreg.h"
#include "lsu_readreg_execute.h"
#include "execute.h"
#include "wb.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct lsu_readreg_feedback_pack_t
    {
        bool stall = false;
    
        virtual json get_json()
        {
            json t;
            t["stall"] = stall;
            return t;
        }
    }lsu_readreg_feedback_pack_t;
    
    class lsu_readreg : public if_reset_t
    {
        private:
            global_inst *global;
            component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port;
            component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_lsu_hdff;
            component::regfile<uint32_t> *phy_regfile;
            
            bool busy = false;
            lsu_issue_readreg_pack_t hold_rev_pack;
            
            trace::trace_database tdb;
        
        public:
            lsu_readreg(global_inst *global, component::port<lsu_issue_readreg_pack_t> *lsu_issue_readreg_port, component::handshake_dff<lsu_readreg_execute_pack_t> **readreg_lsu_hdff, component::regfile<uint32_t> *phy_regfile);
            virtual void reset();
            lsu_readreg_feedback_pack_t run(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
            virtual json get_json();
    };
}