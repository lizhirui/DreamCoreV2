/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-28     lizhirui     the first version
 */

#pragma once
#include "common.h"

namespace cycle_model::pipeline::execute
{
    typedef struct lu_feedback_pack_t : if_print_t
    {
        bool replay = false;
        bool replay_following_only = false;
        uint32_t rob_id = 0;
        bool rob_id_stage = false;
        bool stall = false;
    }lu_feedback_pack_t;
}