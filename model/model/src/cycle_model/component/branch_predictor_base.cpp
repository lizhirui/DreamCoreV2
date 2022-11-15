/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-15     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/component/branch_predictor_base.h"

namespace cycle_model::component
{
    std::unordered_set<branch_predictor_base *> branch_predictor_base::bp_list;
}