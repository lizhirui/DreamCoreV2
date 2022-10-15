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
#include "rename_issue.h"
#include "issue.h"
#include "commit.h"

namespace pipeline
{
    typedef struct rename_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        
        virtual json get_json()
        {
            json t;
            return t;
        }
    }rename_feedback_pack_t;
    
    class rename : public if_reset_t
    {
        private:
            component::fifo<decode_rename_pack_t> *decode_rename_fifo;
            component::port<rename_issue_pack_t> *rename_issue_port;
            component::rat *speculative_rat;
            component::rob *rob;
            component::free_list *phy_id_free_list;
            
            trace::trace_database tdb;
        
        public:
            rename(component::fifo<decode_rename_pack_t> *decode_rename_fifo, component::port<rename_issue_pack_t> *rename_issue_port, component::rat *speculative_rat, component::rob *rob, component::free_list *phy_id_free_list);
            virtual void reset();
            rename_feedback_pack_t run(issue_feedback_pack_t issue_feedback_pack, commit_feedback_pack_t commit_feedback_pack);
    };
}