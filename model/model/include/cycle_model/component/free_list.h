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

namespace cycle_model::component
{
    class free_list : public fifo<if_print_fake<uint32_t>>
    {
        private:
            std::unordered_set<uint32_t> free_list_set;
            
        public:
            free_list(uint32_t size) : fifo<if_print_fake<uint32_t>>(size)
            {
                this->free_list::reset();
            }
            
            virtual void reset()
            {
                fifo<if_print_fake<uint32_t>>::reset();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->push(i);
                }
            }
            
            virtual void flush()
            {
                fifo<if_print_fake<uint32_t>>::flush();
    
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->push(i);
                }
            }
            
            virtual bool push(if_print_fake<uint32_t> data)
            {
                verify(data.get() < size);
                verify(free_list_set.find(data.get()) == free_list_set.end());
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
                    verify(data->get() < size);
                    auto iter = free_list_set.find(data->get());
                    verify(iter != free_list_set.end());
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
            
            void save(uint32_t *rptr, bool *rstage)
            {
                *rptr = this->rptr.get_new();
                *rstage = this->rstage.get_new();
            }
            
            void restore(uint32_t rptr, bool rstage)
            {
                verify(rptr < this->size);
                this->rptr.set(rptr);
                this->rstage.set(rstage);
                
                uint32_t cur_rptr = rptr;
                bool cur_rstage = rstage;
    
                while((cur_rptr != this->rptr.get()) || (cur_rstage != this->rstage.get()))
                {
                    verify(this->free_list_set.find(cur_rptr) == this->free_list_set.end());
                    this->free_list_set.insert(cur_rptr);
                    cur_rptr++;
                    
                    if(cur_rptr >= this->size)
                    {
                        cur_rptr = 0;
                        cur_rstage = !cur_rstage;
                    }
                }
            }
    };
}