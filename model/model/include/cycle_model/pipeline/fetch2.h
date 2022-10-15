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
#include "fetch1_fetch2.h"
#include "fetch2_decode.h"
#include "fetch2.h"
#include "commit.h"

namespace pipeline
{
    typedef struct fetch2_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        bool stall = false;
        
        virtual json get_json()
        {
            json t;
            return t;
        }
    }fetch2_feedback_pack_t;
    
    class fetch2 : public if_print_t, public if_reset_t
    {
        private:
            component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port;
            component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo;
            fetch1_fetch2_pack_t rev_pack;
            bool busy;
            trace::trace_database tdb;
        
        public:
            fetch2(component::port<fetch1_fetch2_pack_t> *fetch1_fetch2_port, component::fifo<fetch2_decode_pack_t> *fetch2_decode_fifo);
            virtual void reset();
            fetch2_feedback_pack_t run(commit_feedback_pack_t commit_feedback_pack);
    };
}