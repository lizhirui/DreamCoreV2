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
#include "../component/rat.h"
#include "../component/rob.h"
#include "../component/free_list.h"
#include "decode_rename.h"
#include "rename_dispatch.h"
#include "dispatch.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct rename_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        
        virtual json get_json()
        {
            json t;
            t["idle"] = idle;
            return t;
        }
    }rename_feedback_pack_t;
    
    class rename : public if_reset_t
    {
        private:
            component::fifo<decode_rename_pack_t> *decode_rename_fifo;
            component::port<rename_dispatch_pack_t> *rename_dispatch_port;
            component::rat *speculative_rat;
            component::rob *rob;
            component::free_list *phy_id_free_list;
            
            trace::trace_database tdb;
        
        public:
            rename(component::fifo<decode_rename_pack_t> *decode_rename_fifo, component::port<rename_dispatch_pack_t> *rename_dispatch_port, component::rat *speculative_rat, component::rob *rob, component::free_list *phy_id_free_list);
            virtual void reset();
            rename_feedback_pack_t run(const dispatch_feedback_pack_t &dispatch_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}