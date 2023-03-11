/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-11     lizhirui     the first version
 */

#pragma once
#include <common.h>
#include <config.h>
#include "dff.h"

namespace cycle_model::component
{
    class wait_table
    {
        private:
            uint32_t size = 0;
            uint32_t addr_mask = 0;
            dff<bool> *buffer = nullptr;
            
        public:
            wait_table(uint32_t size) : size(size), addr_mask(size - 1)
            {
                this->buffer = new dff<bool>[size];
                this->reset();
            }
            
            void reset()
            {
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->buffer[i].set(false);
                }
            }
            
            void run(uint64_t cycle)
            {
                if((cycle & 0x3fff) == 0x3fff)
                {
                    this->reset();
                }
            }
            
            void set_wait_bit(uint32_t addr)
            {
                this->buffer[(addr >> 2) & this->addr_mask].set(true);
            }
            
            bool get_wait_bit(uint32_t addr)
            {
                return this->buffer[(addr >> 2) & this->addr_mask].get();
            }
    };
}