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
#include "pipeline_common.h"
#include "../component/port.h"
#include "../component/rat.h"
#include "../component/rob.h"
#include "../component/csrfile.h"
#include "../component/regfile.h"
#include "../component/free_list.h"
#include "../component/interrupt_interface.h"
#include "wb_commit.h"

namespace cycle_model::pipeline
{
    typedef struct commit_feedback_pack_t : public if_print_t
    {
        bool idle = false;
        bool next_handle_rob_id_valid = false;
        uint32_t next_handle_rob_id = 0;
        bool has_exception = false;
        uint32_t exception_pc = 0;
        bool flush = false;
        uint32_t committed_rob_id[COMMIT_WIDTH] = {0};
        bool committed_rob_id_valid[COMMIT_WIDTH] = {false};
        
        bool jump_enable = false;
        bool jump = false;
        uint32_t next_pc = 0;
        
        virtual json get_json()
        {
            json t;
            t["idle"] = idle;
            t["next_handle_rob_id_valid"] = next_handle_rob_id_valid;
            t["next_handle_rob_id"] = next_handle_rob_id;
            t["committed_rob_id_valid_0"] = committed_rob_id_valid[0];
            t["committed_rob_id_0"] = committed_rob_id[0];
            t["committed_rob_id_valid_1"] = committed_rob_id_valid[1];
            t["committed_rob_id_1"] = committed_rob_id[1];
            t["has_exception"] = has_exception;
            t["exception_pc"] = exception_pc;
            t["flush"] = flush;
            t["jump_enable"] = jump_enable;
            t["jump"] = jump;
            t["next_pc"] = next_pc;
            return t;
        }
    }commit_feedback_pack_t;
    
    class commit : public if_reset_t
    {
        private:
            component::port<wb_commit_pack_t> *wb_commit_port;
            component::rat *speculative_rat;
            component::rat *retire_rat;
            component::rob *rob;
            component::csrfile *csr_file;
            component::regfile<uint32_t> *phy_regfile;
            component::free_list *phy_id_free_list;
            component::interrupt_interface *interrupt_interface;
            
            trace::trace_database tdb;
        
        public:
            commit(component::port<wb_commit_pack_t> *wb_commit_port, component::rat *speculative_rat, component::rat *retire_rat, component::rob *rob, component::csrfile *csr_file, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::interrupt_interface *interrupt_interface);
            virtual void reset();
            commit_feedback_pack_t run();
    };
}