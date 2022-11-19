/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-18     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "cycle_model/global_inst.h"
#include "cycle_model/component/slave/memory.h"
#include "cycle_model/component/slave/clint.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/csr_all.h"
#include "cycle_model/component/csrfile.h"
#include "cycle_model/component/dff.h"
#include "cycle_model/component/fifo.h"
#include "cycle_model/component/free_list.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/interrupt_interface.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/rat.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/rob.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/component/branch_predictor_set.h"
#include "cycle_model/component/checkpoint.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/fetch1.h"
#include "cycle_model/pipeline/fetch1_fetch2.h"
#include "cycle_model/pipeline/fetch2.h"
#include "cycle_model/pipeline/fetch2_decode.h"
#include "cycle_model/pipeline/decode.h"
#include "cycle_model/pipeline/decode_rename.h"
#include "cycle_model/pipeline/rename.h"
#include "cycle_model/pipeline/rename_dispatch.h"
#include "cycle_model/pipeline/dispatch.h"
#include "cycle_model/pipeline/dispatch_issue.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/lsu_issue.h"
#include "cycle_model/pipeline/integer_issue_readreg.h"
#include "cycle_model/pipeline/lsu_issue_readreg.h"
#include "cycle_model/pipeline/integer_readreg.h"
#include "cycle_model/pipeline/lsu_readreg.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/execute/alu.h"
#include "cycle_model/pipeline/execute/bru.h"
#include "cycle_model/pipeline/execute/csr.h"
#include "cycle_model/pipeline/execute/div.h"
#include "cycle_model/pipeline/execute/mul.h"
#include "cycle_model/pipeline/execute/lsu.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/wb_commit.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model
{
    class cycle_model
    {
        private:
            static cycle_model *instance;
            
            charfifo_send_fifo_t *charfifo_send_fifo;
            charfifo_rev_fifo_t *charfifo_rev_fifo;
            
            cycle_model(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo);
            ~cycle_model();
            
        public:
            uint64_t cpu_clock_cycle = 0;
            uint64_t committed_instruction_num = 0;
            global_inst global;
            
            component::port<pipeline::fetch1_fetch2_pack_t> fetch1_fetch2_port;
            component::fifo<pipeline::fetch2_decode_pack_t> fetch2_decode_fifo;
            component::fifo<pipeline::decode_rename_pack_t> decode_rename_fifo;
            component::port<pipeline::rename_dispatch_pack_t> rename_dispatch_port;
            component::port<pipeline::dispatch_issue_pack_t> dispatch_integer_issue_port;
            component::port<pipeline::dispatch_issue_pack_t> dispatch_lsu_issue_port;
            component::port<pipeline::integer_issue_readreg_pack_t> integer_issue_readreg_port;
            component::port<pipeline::lsu_issue_readreg_pack_t> lsu_issue_readreg_port;
            component::handshake_dff<pipeline::integer_readreg_execute_pack_t> *readreg_alu_hdff[ALU_UNIT_NUM];
            component::handshake_dff<pipeline::integer_readreg_execute_pack_t> *readreg_bru_hdff[BRU_UNIT_NUM];
            component::handshake_dff<pipeline::integer_readreg_execute_pack_t> *readreg_csr_hdff[CSR_UNIT_NUM];
            component::handshake_dff<pipeline::integer_readreg_execute_pack_t> *readreg_div_hdff[DIV_UNIT_NUM];
            component::handshake_dff<pipeline::integer_readreg_execute_pack_t> *readreg_mul_hdff[MUL_UNIT_NUM];
            component::handshake_dff<pipeline::lsu_readreg_execute_pack_t> *readreg_lsu_hdff[LSU_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *alu_wb_port[ALU_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *bru_wb_port[BRU_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *csr_wb_port[CSR_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *div_wb_port[DIV_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *mul_wb_port[MUL_UNIT_NUM];
            component::port<pipeline::execute_wb_pack_t> *lsu_wb_port[LSU_UNIT_NUM];
            component::port<pipeline::wb_commit_pack_t> wb_commit_port;
            
            component::bus bus;
            component::csrfile csr_file;
            component::free_list phy_id_free_list;
            component::interrupt_interface interrupt_interface;
            component::rat speculative_rat;
            component::rat retire_rat;
            component::regfile<uint32_t> phy_regfile;
            component::rob rob;
            component::store_buffer store_buffer;
            component::slave::clint clint;
            component::branch_predictor_set branch_predictor_set;
            component::fifo<component::checkpoint_t> checkpoint_buffer;
            
            pipeline::fetch1 fetch1_stage;
            pipeline::fetch2 fetch2_stage;
            pipeline::decode decode_stage;
            pipeline::rename rename_stage;
            pipeline::dispatch dispatch_stage;
            pipeline::integer_issue integer_issue_stage;
            pipeline::lsu_issue lsu_issue_stage;
            pipeline::integer_readreg integer_readreg_stage;
            pipeline::lsu_readreg lsu_readreg_stage;
            pipeline::execute::alu *execute_alu_stage[ALU_UNIT_NUM];
            pipeline::execute::bru *execute_bru_stage[BRU_UNIT_NUM];
            pipeline::execute::csr *execute_csr_stage[CSR_UNIT_NUM];
            pipeline::execute::div *execute_div_stage[DIV_UNIT_NUM];
            pipeline::execute::mul *execute_mul_stage[MUL_UNIT_NUM];
            pipeline::execute::lsu *execute_lsu_stage[LSU_UNIT_NUM];
            pipeline::wb wb_stage;
            pipeline::commit commit_stage;
            
            pipeline::fetch2_feedback_pack_t fetch2_feedback_pack;
            pipeline::decode_feedback_pack_t decode_feedback_pack;
            pipeline::rename_feedback_pack_t rename_feedback_pack;
            pipeline::dispatch_feedback_pack_t dispatch_feedback_pack;
            pipeline::integer_issue_feedback_pack_t integer_issue_feedback_pack;
            pipeline::integer_issue_output_feedback_pack_t integer_issue_output_feedback_pack;
            pipeline::lsu_issue_output_feedback_pack_t lsu_issue_output_feedback_pack;
            pipeline::lsu_issue_feedback_pack_t lsu_issue_feedback_pack;
            pipeline::lsu_readreg_feedback_pack_t lsu_readreg_feedback_pack;
            pipeline::execute::bru_feedback_pack_t bru_feedback_pack;
            pipeline::execute_feedback_pack_t execute_feedback_pack;
            pipeline::wb_feedback_pack_t wb_feedback_pack;
            pipeline::commit_feedback_pack_t commit_feedback_pack;
            
            static cycle_model *create(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo);
            static void destroy();
            
            void load(void *mem, size_t size);
            void reset();
            void run();
    };
}