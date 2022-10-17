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
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../integer_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"

namespace pipeline::execute
{
    class mul : public if_reset_t
    {
        private:
            uint32_t id;
            component::handshake_dff<readreg_execute_pack_t> *readreg_mul_hdff;
            component::port<execute_wb_pack_t> *mul_wb_port;
            trace::trace_database tdb;
        
        public:
            mul(uint32_t id, component::handshake_dff<readreg_execute_pack_t> *readreg_mul_hdff, component::port<execute_wb_pack_t> *mul_wb_port);
            virtual void reset();
            execute_feedback_channel_t run(commit_feedback_pack_t commit_feedback_pack);
    };
}