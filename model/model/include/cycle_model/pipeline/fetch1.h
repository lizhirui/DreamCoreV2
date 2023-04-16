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
#include "../component/retired_store_buffer.h"
#include "../component/branch_predictor_set.h"
#include "fetch1_fetch2.h"
#include "fetch2.h"
#include "decode.h"
#include "rename.h"
#include "commit.h"
#include "cycle_model/pipeline/execute/bru_define.h"
#include "cycle_model/pipeline/execute/sau_define.h"

namespace cycle_model::pipeline
{
    class fetch1 : public if_print_t, public if_reset_t
    {
        private:
            global_inst *global;
            component::bus *bus;
            component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port;
            component::store_buffer *store_buffer;
            component::retired_store_buffer *retired_store_buffer;
            component::branch_predictor_set *branch_predictor_set;
            uint32_t init_pc = 0;
            uint32_t pc = 0;
            bool jump_wait = false;
            uint64_t next_inst_id = 0;
            trace::trace_database tdb;
    
        public:
            fetch1(global_inst *global, component::bus *bus, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::store_buffer *store_buffer, component::retired_store_buffer *retired_store_buffer, component::branch_predictor_set *branch_predictor_set, uint32_t init_pc);
            virtual void reset();
            void run(const fetch2_feedback_pack_t &fetch2_feedback_pack_t, const decode_feedback_pack_t &decode_feedback_pack, const rename_feedback_pack_t &rename_feedback_pack, const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
            uint32_t get_pc() const;
            virtual void print(std::string indent);
            virtual json get_json();
    };
}