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
    class mcounteren : public csr_base
    {
        public:
            mcounteren() : csr_base("mcounteren", 0x00000000)
            {
        
            }

            virtual uint32_t filter(uint32_t value)
            {
                return value & 0x07;
            }

            void set_cy(bool value)
            {
                this->setbit(0, value);
            }

            bool get_cy()
            {
                return this->getbit(0);
            }

            void set_tm(bool value)
            {
                this->setbit(1, value);
            }

            bool get_tm()
            {
                return this->getbit(1);
            }

            void set_ir(bool value)
            {
                this->setbit(2, value);
            }

            bool get_ir()
            {
                return this->getbit(2);
            }
    };
}