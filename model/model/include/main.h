/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-20     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "breakpoint.h"

#ifdef NEED_ISA_MODEL
#include "isa_model/isa_model.h"
extern isa_model::isa_model *isa_model_inst;
#endif

#ifdef NEED_CYCLE_MODEL
#include "cycle_model/cycle_model.h"
extern cycle_model::cycle_model *cycle_model_inst;
#endif

typedef struct command_line_arg_t
{
    bool no_controller = false;
    bool no_telnet = false;
    bool load_elf = false;
    bool load_bin = false;
    std::string path;
    uint32_t controller_port = 10240;
    uint32_t telnet_port = 10241;
    std::vector<breakpoint_info_t> breakpoint_list;
}command_line_arg_t;

#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
charfifo_rev_fifo_t *get_isa_model_charfifo_rev_fifo();
charfifo_rev_fifo_t *get_cycle_model_charfifo_rev_fifo();
#endif
void set_pause_state(bool value);
void set_step_state(bool value);
void set_wait_commit(bool value);
void set_pause_detected(bool value);
uint32_t get_current_pc();
void load(void *buf, size_t size);
void reset();