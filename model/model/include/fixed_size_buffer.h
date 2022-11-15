/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-15     lizhirui     the first version
 */

#pragma once
#include "common.h"

template<typename T>
class fixed_size_buffer
{
    private:
        T *buffer = nullptr;
        uint32_t size = 0;
        uint32_t rptr = 0;
        uint32_t wptr = 0;
    
    public:
        fixed_size_buffer(uint32_t size)
        {
            this->size = size;
            buffer = new T[size];
            clear();
        }
        
        ~fixed_size_buffer()
        {
            delete[] buffer;
        }
        
        void clear()
        {
            this->rptr = 0;
            this->wptr = 0;
        }
        
        bool is_empty()
        {
            return rptr == wptr;
        }
        
        bool is_full()
        {
            return (wptr + size - rptr) % size == (size - 1);
        }
        
        uint32_t count()
        {
            return (wptr + size - rptr) % size;
        }
        
        void pop()
        {
            if(!is_empty())
            {
                rptr++;
                
                if(rptr >= size)
                {
                    rptr = 0;
                }
            }
        }
        
        void push(T data)
        {
            if(is_full())
            {
                pop();
            }
            
            buffer[wptr++] = data;
            
            if(wptr >= size)
            {
                wptr = 0;
            }
        }
        
        T get(uint32_t index)
        {
            return buffer[(rptr + index) % size];
        }
};

