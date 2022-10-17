/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-16     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../component/port.h"
#include "../component/store_buffer.h"
#include "rename_dispatch.h"
#include "dispatch_issue.h"
#include "integer_issue.h"
#include "lsu_issue.h"
#include "commit.h"

namespace pipeline
{
    typedef struct dispatch_feedback_pack_t : public if_print_t
    {
        bool stall = false;
        
        virtual json get_json()
        {
            json t;
            t["stall"] = stall;
            return t;
        }
    }dispatch_feedback_pack_t;
    
    class dispatch : public if_reset_t
    {
        private:
            component::port<rename_dispatch_pack_t> *rename_dispatch_port;
            component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port;
            component::port<dispatch_issue_pack_t> *dispatch_lsu_issue_port;
            component::store_buffer *store_buffer;
            
            bool integer_busy = false;
            bool lsu_busy = false;
            bool busy = false;
            dispatch_issue_pack_t hold_integer_issue_pack;
            dispatch_issue_pack_t hold_lsu_issue_pack;
            rename_dispatch_pack_t rev_pack;
            
            bool is_inst_waiting = false;
            uint32_t inst_waiting_rob_id = 0;
            
            bool is_stbuf_empty_waiting = false;
            
            trace::trace_database tdb;
        
        public:
            dispatch(component::port<rename_dispatch_pack_t> *rename_dispatch_port, component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port, component::port<dispatch_issue_pack_t> *dispatch_lsu_issue_port, component::store_buffer *store_buffer);
            virtual void reset();
            dispatch_feedback_pack_t run(const integer_issue_feedback_pack_t &integer_issue_feedback_pack, const lsu_issue_feedback_pack_t &lsu_issue_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}