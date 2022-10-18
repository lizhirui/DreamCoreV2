/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../csr_base.h"

namespace cycle_model::component::csr
{
    class mcycleh : public csr_base
    {
        public:
            mcycleh() : csr_base("mcycleh", 0x00000000)
            {
        
            }
    };
}