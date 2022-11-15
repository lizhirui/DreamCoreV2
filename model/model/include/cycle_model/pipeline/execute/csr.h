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
#include "config.h"
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../../component/csrfile.h"
#include "../integer_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"

namespace cycle_model::pipeline::execute
{
    class csr : if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<integer_readreg_execute_pack_t> *readreg_csr_hdff;
            component::port<execute_wb_pack_t> *csr_wb_port;
            component::csrfile *csr_file;
            trace::trace_database tdb;
        
        public:
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
            typedef struct csr_read_item_t
            {
                uint32_t rob_id;
                uint32_t csr;
                uint32_t value;
            }csr_read_item_t;
            
            std::queue<csr_read_item_t> csr_read_queue;
#endif
            csr(global_inst *global, uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_csr_hdff, component::port<execute_wb_pack_t> *csr_wb_port, component::csrfile *csr_file);
            virtual void reset();
            execute_feedback_channel_t run(commit_feedback_pack_t commit_feedback_pack);
    };
}