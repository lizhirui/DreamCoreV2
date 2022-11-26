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

namespace cycle_model::pipeline::execute
{
    typedef struct bru_feedback_pack_t : if_print_t
    {
        bool flush = false;
        uint32_t next_pc = 0;
        bool rob_id_stage = false;
        uint32_t rob_id = 0;
    }bru_feedback_pack_t;
}