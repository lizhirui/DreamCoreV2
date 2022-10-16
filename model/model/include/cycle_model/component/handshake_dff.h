/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-16     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "dff.h"

namespace component
{
    template<typename T>
    class handshake_dff
    {
        private:
            dff<bool> has_data;
            dff<T> data;
        
        public:
            handshake_dff() : has_data(false), data(T())
            {
            
            }
            
            void reset()
            {
                has_data.set(false);
            }
            
            void flush()
            {
                has_data.set(false);
            }
            
            bool is_empty()
            {
                return !has_data.get();
            }
            
            bool is_full()
            {
                return has_data.get_new();
            }
            
            bool push(T element)
            {
                if(is_full())
                {
                    return false;
                }
                
                data.set(element);
                has_data.set(true);
                return true;
            }
            
            bool pop(T *element)
            {
                if(is_empty())
                {
                    return false;
                }
                
                *element = data.get();
                has_data.set(false);
                return true;
            }
    };
}