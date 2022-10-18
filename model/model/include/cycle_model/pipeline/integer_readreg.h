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
            void run(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}