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
#include "../component/bus.h"
#include "../component/store_buffer.h"
#include "fetch1_fetch2.h"
#include "fetch2.h"
#include "decode.h"
#include "rename.h"
#include "commit.h"

namespace pipeline
{
    class fetch1 : public if_print_t, public if_reset_t
    {
        private:
            component::bus *bus;
            component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port;
            component::store_buffer *store_buffer;
            uint32_t init_pc;
            uint32_t pc;
            bool jump_wait;
            trace::trace_database tdb;
    
        public:
            fetch1(component::bus *bus, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::store_buffer *store_buffer, uint32_t init_pc);
            virtual void reset();
            void run(fetch2_feedback_pack_t fetch2_feedback_pack_t, decode_feedback_pack_t decode_feedback_pack, rename_feedback_pack_t rename_feedback_pack, commit_feedback_pack_t commit_feedback_pack);
            uint32_t get_pc();
            virtual void print(std::string indent);
            virtual json get_json();
    };
}