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
    class mstatus : public csr_base
    {
        public:
            mstatus() : csr_base("mstatus", 0x00000000)
            {
        
            }

            virtual uint32_t filter(uint32_t value)
            {
                return value & 0x88;
            }

            void set_mie(bool value)
            {
                this->setbit(3, value);
            }

            bool get_mie()
            {
                return this->getbit(3);
            }

            void set_mpie(bool value)
            {
                this->setbit(7, value);
            }

            bool get_mpie()
            {
                return this->getbit(7);
            }
    };
}