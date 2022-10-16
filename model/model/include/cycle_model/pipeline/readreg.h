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
#include "../component/rat.h"
#include "issue_readreg.h"
#include "readreg_execute.h"
#include "issue.h"
#include "commit.h"

namespace pipeline
{
    typedef struct readreg_feedback_pack_t
    {
        bool issue_id_valid[READREG_WIDTH];
        bool remove[READREG_WIDTH];//if true, instruction will be removed from issue_queue, otherwise instruction will be replayed
        uint32_t issue_id[READREG_WIDTH];
    }readreg_feedback_pack_t;
    
    class readreg : public if_reset_t
    {
        private:
            component::port<issue_readreg_pack_t> *issue_readreg_port;
            component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_lsu_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff;
            component::regfile<uint32_t> *phy_regfile;
            component::rat *speculative_rat;
            trace::trace_database tdb;
        
        public:
            readreg(component::port<issue_readreg_pack_t> *issue_readreg_port, component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_lsu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff, component::regfile<uint32_t> *phy_regfile, component::rat *speculative_rat);
            virtual void reset();
            readreg_feedback_pack_t run(commit_feedback_pack_t commit_feedback_pack);
    };
}