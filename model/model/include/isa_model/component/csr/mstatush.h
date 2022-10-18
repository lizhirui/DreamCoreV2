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

namespace isa_model::component::csr
{
    class mstatush : public csr_base
    {
        public:
            mstatush() : csr_base("mstatush", 0x00000000)
            {
        
            }

            virtual uint32_t filter(uint32_t value)
            {
                return 0;
            }
    };
}