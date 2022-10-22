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

std::pair<std::shared_ptr<uint8_t[]>, size_t> load_elf(std::string path);