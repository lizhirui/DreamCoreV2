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
#include "integer_issue_readreg.h"
#include "integer_readreg_execute.h"
#include "integer_issue.h"
#include "execute.h"
#include "wb.h"
#include "commit.h"

namespace pipeline
{
    typedef struct integer_readreg_feedback_pack_t
    {
        bool issue_id_valid[INTEGER_READREG_WIDTH];
        bool remove[INTEGER_READREG_WIDTH];//if true, instruction will be removed from issue_queue, otherwise instruction will be replayed
        uint32_t issue_id[INTEGER_READREG_WIDTH];
    }integer_readreg_feedback_pack_t;
    
    class integer_readreg : public if_reset_t
    {
        private:
            component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port;
            component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff;
            component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff;
            component::regfile<uint32_t> *phy_regfile;
            trace::trace_database tdb;
        
        public:
            integer_readreg(component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port, component::handshake_dff<readreg_execute_pack_t> **readreg_alu_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_bru_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_csr_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_div_hdff, component::handshake_dff<readreg_execute_pack_t> **readreg_mul_hdff, component::regfile<uint32_t> *phy_regfile);
            virtual void reset();
            integer_readreg_feedback_pack_t run(execute_feedback_pack_t execute_feedback_pack, wb_feedback_pack_t wb_feedback_pack, commit_feedback_pack_t commit_feedback_pack);
    };
}