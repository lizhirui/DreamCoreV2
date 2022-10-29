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
#include "../../component/bus.h"
#include "../../component/store_buffer.h"
#include "../../component/slave/clint.h"
#include "../lsu_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"

namespace cycle_model::pipeline::execute
{
    class lsu : if_reset_t
    {
        private:
            uint32_t id = 0;
            component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lsu_hdff;
            component::port<execute_wb_pack_t> *lsu_wb_port;
            component::bus *bus;
            component::store_buffer *store_buffer;
            
            bool l2_stall = false;
            lsu_readreg_execute_pack_t l2_rev_pack;
            uint32_t l2_addr = 0;
            uint32_t l2_feedback_value = 0;
            uint32_t l2_feedback_mask = 0;
            trace::trace_database tdb;
            
            component::slave::clint *clint;//only for debug
        
        public:
            typedef struct clint_sync_info_t
            {
                uint64_t mtime;
                uint64_t mtimecmp;
                uint32_t msip;
            }clint_sync_info;
            
            clint_sync_info_t clint_sync_list[ROB_SIZE] = {{0, 0, 0}};
            
            lsu(uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lsu_hdff, component::port<execute_wb_pack_t> *lsu_wb_port, component::bus *bus, component::store_buffer *store_buffer, component::slave::clint *clint);
            virtual void reset();
            execute_feedback_channel_t run(commit_feedback_pack_t commit_feedback_pack);
            virtual json get_json();
    };
}