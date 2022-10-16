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
#include "free_list.h"
#include "dff.h"

namespace component
{
    template<typename T>
    class ooo_issue_queue : public fifo<T>
    {
        private:
            dff<uint32_t> *age_matrix;
            dff<bool> *valid;
            free_list id_free_list;
            
        public:
            ooo_issue_queue(uint32_t size) : fifo<T>(size), id_free_list(size)
            {
                age_matrix = new dff<uint32_t>[size * size];
                this->reset();
            }
            
            virtual void reset()
            {
                fifo<T>::reset();
                
                for(auto i = 0;i < this->size * this->size;i++)
                {
                    age_matrix[i].set(0);
                }
                
                for(auto i = 0;i < this->size;i++)
                {
                    valid[i].set(false);
                }
                
                id_free_list.reset();
            }
            
            virtual void flush()
            {
                fifo<T>::flush();
                
                for(auto i = 0;i < this->size * this->size;i++)
                {
                    age_matrix[i].set(0);
                }
                
                for(auto i = 0;i < this->size;i++)
                {
                    valid[i].set(false);
                }
                
                id_free_list.flush();
            }
            
            virtual bool push(T data)
            {
                if(!id_free_list.customer_is_empty())
                {
                    uint32_t index = 0;
                    assert(id_free_list.pop(&index));
                    this->producer_set_item(index, data);
                    
                    for(auto i = 0;i < this->size;i++)
                    {
                        age_matrix[index * this->size + i].set(valid[i].get_new());
                    }
                    
                    valid[index].set(true);
                    return true;
                }
                
                return false;
            }
            
            virtual bool pop(T *data)
            {
                return false;
            }
            
            virtual bool pop(uint32_t index)
            {
                assert(valid[index]);
                assert(id_free_list.push(index));
                
                for(auto i = 0;i < this->size;i++)
                {
                    age_matrix[i * this->size + index].set(false);
                }
                
                valid[index].set(false);
                return true;
            }
            
            bool is_valid(uint32_t index)
            {
                return valid[index];
            }
            
            uint32_t get_age(uint32_t index)
            {
                uint32_t sum = 0;
                
                for(auto i = 0;i < this->size;i++)
                {
                    sum += age_matrix[index * this->size + i].get();
                }
                
                return sum;
            }
    };
}