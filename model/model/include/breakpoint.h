/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-22     lizhirui     the first version
 */

#pragma once
#include "common.h"

enum class breakpoint_type_t
{
    cycle,
    instruction_num,
    pc,
    csrw
};

typedef struct breakpoint_info_t
{
    breakpoint_type_t type;
    uint32_t value;
}breakpoint_info_t;

void breakpoint_clear();
void breakpoint_add(breakpoint_type_t type, uint32_t value);
void breakpoint_remove(uint32_t id);
std::string breakpoint_get_list();
void breakpoint_csr_trigger(uint32_t csr, uint32_t value, bool write);
bool breakpoint_check(uint32_t cycle, uint32_t instruction_num, uint32_t pc);