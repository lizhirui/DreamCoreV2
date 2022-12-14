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

namespace cycle_model::component
{
    template<typename T>
    class handshake_dff : public if_print_t
    {
        private:
            dff<bool> has_data;
            dff<T> data;
        
        public:
            handshake_dff() : has_data(false), data(T())
            {
                this->reset();
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
            
            bool get_data(T *element)
            {
                if(is_empty())
                {
                    return false;
                }
                
                *element = data.get();
                return true;
            }
            
            virtual json get_json()
            {
                if(is_empty())
                {
                    return T().get_json();
                }
                else
                {
                    return data.get().get_json();
                }
            }
    };
}