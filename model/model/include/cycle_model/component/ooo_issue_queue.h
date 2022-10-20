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

namespace cycle_model::component
{
    template<typename T>
    class ooo_issue_queue : public fifo<T>
    {
        private:
            dff<bool> *valid;
            free_list id_free_list;
            
        public:
            ooo_issue_queue(uint32_t size) : fifo<T>(size), id_free_list(size)
            {
                valid = new dff<bool>[size];
                this->reset();
            }
            
            virtual void reset()
            {
                fifo<T>::reset();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    valid[i].set(false);
                }
                
                id_free_list.reset();
            }
            
            virtual void flush()
            {
                fifo<T>::flush();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    valid[i].set(false);
                }
                
                id_free_list.flush();
            }
            
            virtual bool push(T data)
            {
                return false;
            }
            
            virtual bool push(T data, uint32_t *index)
            {
                if(!id_free_list.customer_is_empty())
                {
                    verify(id_free_list.pop(index));
                    this->set_item(*index, data);
                    valid[*index].set(true);
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
                verify(valid[index]);
                verify(id_free_list.push(index));
                valid[index].set(false);
                return true;
            }
            
            bool is_valid(uint32_t index)
            {
                return valid[index];
            }
    };
}