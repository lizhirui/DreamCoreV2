/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "fifo.h"

namespace component
{
    class free_list : public fifo<if_print_fake<uint32_t>>
    {
        private:
            std::unordered_set<uint32_t> free_list_set;
            
        public:
            free_list(uint32_t size) : fifo<if_print_fake<uint32_t>>(size)
            {
                for(auto i = 0;i < size;i++)
                {
                    this->push(i);
                }
            }
            
            virtual void reset()
            {
                fifo<if_print_fake<uint32_t>>::reset();
                
                for(auto i = 0;i < this->size;i++)
                {
                    this->push(i);
                }
            }
            
            virtual void flush()
            {
                fifo<if_print_fake<uint32_t>>::flush();
    
                for(auto i = 0;i < this->size;i++)
                {
                    this->push(i);
                }
            }
            
            virtual bool push(if_print_fake<uint32_t> data)
            {
                assert(data.get() < size);
                assert(free_list_set.find(data.get()) == free_list_set.end());
                return fifo<if_print_fake<uint32_t>>::push(data);
            }
            
            virtual bool push(uint32_t data)
            {
                return push(if_print_fake<uint32_t>(data));
            }
        
            virtual bool pop(if_print_fake<uint32_t> *data)
            {
                auto ret = fifo<if_print_fake<uint32_t>>::pop(data);
                
                if(ret)
                {
                    assert(data->get() < size);
                    auto iter = free_list_set.find(data->get());
                    assert(iter != free_list_set.end());
                    free_list_set.erase(iter);
                }
                
                return ret;
            }
            
            virtual bool pop(uint32_t *data)
            {
                if_print_fake<uint32_t> temp;
                auto ret = pop(&temp);
                
                if(ret)
                {
                    *data = temp.get();
                }
                
                return ret;
            }
    };
}