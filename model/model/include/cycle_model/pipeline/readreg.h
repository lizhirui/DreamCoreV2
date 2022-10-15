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
#include "../component/port.h"
#include "../component/fifo.h"
#include "../component/regfile.h"
#include "../component/rat.h"
#include "issue_readreg.h"
#include "readreg_execute.h"
#include "issue.h"
#include "commit.h"

namespace pipeline
{
    typedef struct readreg_feedback_pack_t
    {
        bool issue_id_valid[READREG_WIDTH];
        bool remove[READREG_WIDTH];//if true, instruction will be removed from issue_queue, otherwise instruction will be replayed
        uint32_t issue_id[READREG_WIDTH];
    }readreg_feedback_pack_t;
    
    class readreg : public if_reset_t
    {
        private:
            component::port<issue_readreg_pack_t> *issue_readreg_port;
            component::fifo<readreg_execute_pack_t> **readreg_alu_fifo;
            component::fifo<readreg_execute_pack_t> **readreg_bru_fifo;
            component::fifo<readreg_execute_pack_t> **readreg_csr_fifo;
            component::fifo<readreg_execute_pack_t> **readreg_div_fifo;
            component::fifo<readreg_execute_pack_t> **readreg_lsu_fifo;
            component::fifo<readreg_execute_pack_t> **readreg_mul_fifo;
            component::regfile<uint32_t> *phy_regfile;
            component::rat *rat;
            trace::trace_database tdb;
        
        public:
            readreg(component::port<rename_readreg_pack_t> *issue_readreg_port, component::fifo<readreg_execute_pack_t> **issue_alu_fifo, component::fifo<readreg_execute_pack_t> **issue_bru_fifo, component::fifo<readreg_execute_pack_t> **issue_csr_fifo, component::fifo<readreg_execute_pack_t> **issue_div_fifo, component::fifo<readreg_execute_pack_t> **issue_lsu_fifo, component::fifo<readreg_execute_pack_t> **issue_mul_fifo, component::regfile<uint32_t> *phy_regfile, component::rat *rat);
            virtual void reset();
            readreg_feedback_pack_t run(commit_feedback_pack_t commit_feedback_pack);
    };
}