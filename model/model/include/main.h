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

#define NEED_ISA_MODEL (MODE == MODE_ISA_MODEL_ONLY) || (MODE == MODE_ISA_AND_CYCLE_MODEL_DIFFTEST)
#define NEED_CYCLE_MODEL (MODE == MODE_CYCLE_MODEL_ONLY) || (MODE == MODE_ISA_AND_CYCLE_MODEL_DIFFTEST)

#if NEED_ISA_MODEL
#include "isa_model/isa_model.h"
isa_model::isa_model *isa_model_inst;
#endif

#if NEED_CYCLE_MODEL
#include "cycle_model/cycle_model.h"
cycle_model::cycle_model *cycle_model_inst;
#endif

void set_pause_state(bool value);
void set_step_state(bool value);
void set_wait_commit(bool value);
void set_pause_detected(bool value);
static uint32_t get_current_pc();
void reset();