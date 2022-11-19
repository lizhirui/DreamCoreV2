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
#include "fetch2_decode.h"
#include "decode_rename.h"
#include "execute/bru.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct decode_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        
        virtual json get_json()
        {
            json t;
            t["idle"] = idle;
            return t;
        }
    }decode_feedback_pack_t;
    
    class decode : public if_reset_t
    {
        private:
            global_inst *global;
            component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo;
            component::fifo<decode_rename_pack_t> *decode_rename_fifo;
            trace::trace_database tdb;
        
        public:
            decode(global_inst *global, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo, component::fifo<decode_rename_pack_t> *decode_rename_fifo);
            virtual void reset();
            decode_feedback_pack_t run(const execute::bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}