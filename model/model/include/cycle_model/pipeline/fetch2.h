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
#include "../component/fifo.h"
#include "../component/port.h"
#include "../component/branch_predictor_set.h"
#include "fetch1_fetch2.h"
#include "fetch2_decode.h"
#include "fetch2.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct fetch2_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        bool stall = false;
        bool pc_redirect = false;
        uint32_t new_pc = 0;
        
        virtual json get_json()
        {
            json t;
            t["idle"] = idle;
            t["stall"] = stall;
            return t;
        }
    }fetch2_feedback_pack_t;
    
    class fetch2 : public if_print_t, public if_reset_t
    {
        private:
            global_inst *global;
            component::branch_predictor_set *branch_predictor_set;
            component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port;
            component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo;
            fetch1_fetch2_pack_t rev_pack;
            bool busy = false;
            trace::trace_database tdb;
        
        public:
            fetch2(global_inst *global, component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo, component::branch_predictor_set *branch_predictor_set);
            virtual void reset();
            fetch2_feedback_pack_t run(const commit_feedback_pack_t &commit_feedback_pack);
            virtual json get_json();
    };
}