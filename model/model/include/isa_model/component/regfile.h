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

namespace isa_model::component
{
    class rat;
    
    template<typename T>
    class regfile : public if_reset_t
    {
        private:
            T *reg_data;
            uint32_t size = 0;
            
        public:
            regfile(uint32_t size)
            {
                this->size = size;
                reg_data = new T[size];
                this->reset();
            }
        
            ~regfile()
            {
                delete[] reg_data;
            }
        
            virtual void reset()
            {
                for(auto i = 0;i < size;i++)
                {
                    reg_data[i] = 0;
                }
            }
        
            void write(uint32_t addr, T value)
            {
                verify(addr < size);
                reg_data[addr] = value;
            }
        
            T read(uint32_t addr)
            {
                verify(addr < size);
                return reg_data[addr];
            }
    };
}