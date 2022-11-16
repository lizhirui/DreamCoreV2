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
#include "pipeline_common.h"
#include "../component/port.h"
#include "../component/rat.h"
#include "../component/rob.h"
#include "../component/csrfile.h"
#include "../component/regfile.h"
#include "../component/free_list.h"
#include "../component/interrupt_interface.h"
#include "../component/branch_predictor_set.h"
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
        uint32_t jump_next_pc = 0;
        
        bool has_interrupt = false;//only for debug
        riscv_interrupt_t interrupt_id = riscv_interrupt_t::user_software;//only for debug
        
        virtual json get_json()
        {
            json t;
            t["idle"] = idle;
            t["next_handle_rob_id_valid"] = next_handle_rob_id_valid;
            t["next_handle_rob_id"] = next_handle_rob_id;
            t["has_exception"] = has_exception;
            t["exception_pc"] = exception_pc;
            t["flush"] = flush;
            
            json t1 = json::array();
            json t2 = json::array();
            
            for(uint32_t i = 0;i < COMMIT_WIDTH;i++)
            {
                t1.push_back(committed_rob_id[i]);
                t2.push_back(committed_rob_id_valid[i]);
            }
            
            t["committed_rob_id"] = t1;
            t["committed_rob_id_valid"] = t2;
            
            t["jump_enable"] = jump_enable;
            t["jump"] = jump;
            t["jump_next_pc"] = jump_next_pc;
            return t;
        }
    }commit_feedback_pack_t;
    
    class commit : public if_reset_t
    {
        private:
            global_inst *global;
            component::port<wb_commit_pack_t> *wb_commit_port;
            component::rat *speculative_rat;
            component::rat *retire_rat;
            component::rob *rob;
            component::csrfile *csr_file;
            component::regfile<uint32_t> *phy_regfile;
            component::free_list *phy_id_free_list;
            component::interrupt_interface *interrupt_interface;
            component::branch_predictor_set *branch_predictor_set;
        
#ifdef BRANCH_PREDICTOR_UPDATE_DUMP
            std::ofstream branch_predictor_update_dump_stream;
#endif
            trace::trace_database tdb;
        
        public:
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
            std::queue<std::pair<uint32_t, component::rob_item_t>> rob_retire_queue;
#endif
            commit(global_inst *global, component::port<wb_commit_pack_t> *wb_commit_port, component::rat *speculative_rat, component::rat *retire_rat, component::rob *rob, component::csrfile *csr_file, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::interrupt_interface *interrupt_interface, component::branch_predictor_set *branch_predictor_set);
            virtual void reset();
            commit_feedback_pack_t run();
    };
}