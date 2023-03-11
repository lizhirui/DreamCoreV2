/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../../component/bus.h"
#include "../../component/store_buffer.h"
#include "../../component/slave/clint.h"
#include "../../component/load_queue.h"
#include "../../component/wait_table.h"
#include "../lsu_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"
#include "bru_define.h"
#include "sau_define.h"

namespace cycle_model::pipeline::execute
{
    class sau : if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sau_hdff;
            component::port<execute_wb_pack_t> *sau_wb_port;
            component::store_buffer *store_buffer;
            component::load_queue *load_queue;
            component::rat *speculative_rat;
            component::rob *rob;
            component::regfile<uint32_t> *phy_regfile;
            component::free_list *phy_id_free_list;
            component::fifo<component::checkpoint_t> *checkpoint_buffer;
            component::wait_table *wait_table;
            
            trace::trace_database tdb;
        
        public:
            sau(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sau_hdff, component::port<execute_wb_pack_t> *sau_wb_port, component::store_buffer *store_buffer, component::load_queue *load_queue, component::rat *speculative_rat, component::rob *rob, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::fifo<component::checkpoint_t> *checkpoint_buffer, component::wait_table *wait_table);
            virtual void reset();
            sau_feedback_pack_t run(const bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack, bool need_sau_feedback_only);
            virtual json get_json();
    };
}