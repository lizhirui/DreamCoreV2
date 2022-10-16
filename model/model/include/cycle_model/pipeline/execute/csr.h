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
#include "../../component/csrfile.h"
#include "../readreg_execute.h"
#include "../execute_wb.h"
#include "../commit.h"

namespace pipeline
{
    namespace execute
    {
        class csr : if_reset_t
        {
            private:
                uint32_t id;
                component::handshake_dff<readreg_execute_pack_t> *readreg_csr_hdff;
                component::port<execute_wb_pack_t> *csr_wb_port;
                component::csrfile *csr_file;
                trace::trace_database tdb;
            
            public:
                csr(uint32_t id, component::handshake_dff<readreg_execute_pack_t> *readreg_csr_hdff, component::port<execute_wb_pack_t> *csr_wb_port, component::csrfile *csr_file);
                virtual void reset();
                void run(commit_feedback_pack_t commit_feedback_pack);
        };
    }
}