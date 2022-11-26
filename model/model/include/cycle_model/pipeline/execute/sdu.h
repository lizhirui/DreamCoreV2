/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../../component/bus.h"
#include "../../component/store_buffer.h"
#include "../../component/slave/clint.h"
#include "../lsu_readreg_execute.h"
#include "../execute_wb.h"
#include "bru.h"
#include "../execute.h"
#include "../commit.h"

namespace cycle_model::pipeline::execute
{
    class sdu : if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sdu_hdff;
            component::port<execute_wb_pack_t> *sdu_wb_port;
            component::store_buffer *store_buffer;
            trace::trace_database tdb;
        
        public:
            sdu(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sdu_hdff, component::port<execute_wb_pack_t> *sdu_wb_port, component::store_buffer *store_buffer);
            virtual void reset();
            void run(const bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
            virtual json get_json();
    };
}