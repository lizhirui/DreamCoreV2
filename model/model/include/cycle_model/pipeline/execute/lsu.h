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
#include "../../component/fifo.h"
#include "../../component/port.h"
#include "../../component/bus.h"
#include "../../component/store_buffer.h"
#include "../readreg_execute.h"
#include "../execute_wb.h"
#include "../commit.h"

namespace pipeline
{
    namespace execute
    {
        class lsu : if_reset_t
        {
            private:
                uint32_t id;
                component::fifo<readreg_execute_pack_t> *readreg_lsu_fifo;
                component::port<execute_wb_pack_t> *lsu_wb_port;
                component::bus *bus;
                component::store_buffer *store_buffer;
                
                bool busy;
                readreg_execute_pack_t hold_rev_pack;
                trace::trace_database tdb;
            
            public:
                lsu(uint32_t id, component::fifo<readreg_execute_pack_t> *issue_lsu_fifo, component::port<execute_wb_pack_t> *lsu_wb_port, component::bus *bus, component::store_buffer *store_buffer);
                virtual void reset();
                void run(commit_feedback_pack_t commit_feedback_pack);
        };
    }
}