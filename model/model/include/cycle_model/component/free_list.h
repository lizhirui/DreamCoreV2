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
    class free_list : public fifo<if_print_direct<uint32_t>>
    {
        private:
            std::unordered_set<uint32_t> free_list_set;
            
        public:
            free_list(uint32_t size) : fifo<if_print_direct<uint32_t>>(size)
            {
                this->free_list::reset();
            }
            
            virtual void reset()
            {
                fifo<if_print_direct<uint32_t>>::reset();
                free_list_set.clear();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->new_push(i);
                    free_list_set.insert(i);
                }
    
                verify_only(new_get_used_space() == this->free_list_set.size());
            }
            
            virtual void flush()
            {
                reset();
            }
            
            virtual bool push(if_print_direct<uint32_t> data)
            {
                verify_only(data.get() < size);
                verify_only(free_list_set.find(data.get()) == free_list_set.end());
                auto ret = fifo<if_print_direct<uint32_t>>::push(data);
                
                if(ret)
                {
                    free_list_set.insert(data.get());
                }
    
                verify_only(new_get_used_space() == this->free_list_set.size());
                return ret;
            }
            
            virtual bool push(uint32_t data)
            {
                return push(if_print_direct<uint32_t>(data));
            }
        
            virtual bool pop(if_print_direct<uint32_t> *data)
            {
                auto ret = fifo<if_print_direct<uint32_t>>::pop(data);
                
                if(ret)
                {
                    verify_only(data->get() < size);
                    auto iter = free_list_set.find(data->get());
                    verify_only(iter != free_list_set.end());
                    free_list_set.erase(iter);
                }
                
                verify_only(new_get_used_space() == this->free_list_set.size());
                return ret;
            }
            
            virtual bool pop(uint32_t *data)
            {
                if_print_direct<uint32_t> temp;
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
                verify_only(rptr < this->size);
                this->rptr.set(rptr);
                this->rstage.set(rstage);
                
                uint32_t cur_rptr = rptr;
                bool cur_rstage = rstage;
                verify_only(producer_get_used_space() == this->free_list_set.size());
    
                while((cur_rptr != this->rptr.get()) || (cur_rstage != this->rstage.get()))
                {
                    verify_only(this->free_list_set.find(this->buffer[cur_rptr].get().get()) == this->free_list_set.end());
                    this->free_list_set.insert(this->buffer[cur_rptr].get().get());
                    cur_rptr++;
                    
                    if(cur_rptr >= this->size)
                    {
                        cur_rptr = 0;
                        cur_rstage = !cur_rstage;
                    }
                }
    
                verify_only(new_get_used_space() == this->free_list_set.size());
            }
    };
}