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
#include "dff.h"
#include "rat.h"

namespace cycle_model::component
{
    class rat;
    
    template<typename T>
    class regfile : public if_reset_t
    {
        private:
            dff<T> *reg_data;
            dff<bool> *reg_data_valid;
            uint32_t size = 0;
        
            trace::trace_database tdb;
            
        public:
            regfile(uint32_t size) : tdb(TRACE_PHY_REGFILE)
            {
                this->size = size;
                reg_data = new dff<T>[size];
                reg_data_valid = new dff<bool>[size];
                this->reset();
            }
        
            ~regfile()
            {
                delete[] reg_data;
                delete[] reg_data_valid;
            }
        
            virtual void reset()
            {
                for(auto i = 0;i < size;i++)
                {
                    reg_data[i].set(0);
                    reg_data_valid[i].set(false);
                }
            }
        
            void trace_pre()
            {
            
            }
        
            void trace_post()
            {
            
            }
        
            trace::trace_database *get_tdb()
            {
                return &tdb;
            }
        
            void write(uint32_t addr, T value, bool valid)
            {
                verify(addr < size);
                reg_data[addr].set(value);
                reg_data_valid[addr].set(value);
            }
        
            T read(uint32_t addr)
            {
                verify(addr < size);
                return reg_data[addr];
            }
        
            bool read_data_valid(uint32_t addr)
            {
                verify(addr < size);
                return reg_data_valid[addr];
            }
            
            void restore(rat *element)
            {
                for(auto i = 0;i < size;i++)
                {
                    reg_data_valid[i].set(element->producer_get_valid(i));
                }
            }
    };
}