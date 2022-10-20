/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-21     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/component/dff.h"

namespace cycle_model::component
{
    std::unordered_set<dff_base *> dff_base::dff_list;
}