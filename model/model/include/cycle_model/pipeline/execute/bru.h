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
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../../component/csrfile.h"
#include "../../component/csr_all.h"
#include "../../component/rat.h"
#include "../../component/rob.h"
#include "../../component/regfile.h"
#include "../../component/free_list.h"
#include "../../component/checkpoint.h"
#include "../../component/branch_predictor_set.h"
#include "../integer_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"

namespace cycle_model::pipeline::execute
{
    typedef struct bru_feedback_pack_t : if_print_t
    {
        bool flush = false;
        uint32_t next_pc = 0;
        bool rob_id_stage = false;
        uint32_t rob_id = 0;
    }bru_feedback_pack_t;
    
    class bru : public if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<integer_readreg_execute_pack_t> *readreg_bru_hdff;
            component::port<execute_wb_pack_t> *bru_wb_port;
            component::csrfile *csr_file;
            component::rat *speculative_rat;
            component::rob *rob;
            component::regfile<uint32_t> *phy_regfile;
            component::free_list *phy_id_free_list;
            component::fifo<component::checkpoint_t> *checkpoint_buffer;
            component::branch_predictor_set *branch_predictor_set;
            trace::trace_database tdb;
        
        public:
            bru(global_inst *global, uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_bru_hdff, component::port<execute_wb_pack_t> *bru_wb_port, component::csrfile *csr_file, component::rat *speculative_rat, component::rob *rob, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::fifo<component::checkpoint_t> *checkpoint_buffer, component::branch_predictor_set *branch_predictor_set);
            virtual void reset();
            std::variant<execute_feedback_channel_t, bru_feedback_pack_t> run(const commit_feedback_pack_t &commit_feedback_pack, bool need_bru_feedback_only);
    };
}