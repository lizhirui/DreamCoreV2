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
            
            virtual bool producer_get_new_id(uint32_t *new_id)
            {
                for(uint32_t i = 0;i < this->size;i++)
                {
                    if(!this->valid[i].get_new())
                    {
                        *new_id = i;
                        return true;
                    }
                }
    
                return false;
            }
            
        public:
            ooo_issue_queue(uint32_t size) : fifo<T>(size)
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
            }
            
            virtual void flush()
            {
                fifo<T>::flush();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    valid[i].set(false);
                }
            }
        
            virtual bool producer_is_full()
            {
                for(uint32_t i = 0;i < this->size;i++)
                {
                    if(!this->valid[i].get_new())
                    {
                        return false;
                    }
                }
            
                return true;
            }
            
            virtual bool push(T data)
            {
                return false;
            }
            
            virtual bool push(T data, uint32_t *index)
            {
                if(!producer_is_full())
                {
                    verify(producer_get_new_id(index));
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
                verify_only(valid[index]);
                valid[index].set(false);
                return true;
            }
            
            bool is_valid(uint32_t index)
            {
                return valid[index];
            }
            
            void set_valid(uint32_t index, bool is_valid)
            {
                this->valid[index].set(is_valid);
            }
            
            virtual json get_json()
            {
                json ret;
                json value = json::array();
                json valid = json::array();
                if_print_t *if_print;
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    auto item = this->buffer[i].get();
                    if_print = dynamic_cast<if_print_t *>(&item);
                    value.push_back(if_print->get_json());
                    valid.push_back(this->valid[i].get());
                }
                
                ret["value"] = value;
                ret["valid"] = valid;
                ret["rptr"] = this->rptr.get();
                ret["rstage"] = this->rstage.get();
                ret["wptr"] = this->wptr.get();
                ret["wstage"] = this->wstage.get();
                return ret;
            }
    };
}