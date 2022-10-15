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
#include "../readreg_execute.h"
#include "../execute_wb.h"
#include "../commit.h"

namespace pipeline
{
    namespace execute
    {
        class alu : public if_reset_t
        {
            private:
                uint32_t id;
                component::fifo<readreg_execute_pack_t> *readreg_alu_fifo;
                component::port<execute_wb_pack_t> *alu_wb_port;
                trace::trace_database tdb;
            
            public:
                alu(uint32_t id, component::fifo<readreg_execute_pack_t> *readreg_alu_fifo, component::port<execute_wb_pack_t> *alu_wb_port);
                virtual void reset();
                void run(commit_feedback_pack_t commit_feedback_pack);
        };
    }
}