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
#include "fifo.h"
#include "bus.h"
#include "../pipeline/commit.h"
#include "slave/memory.h"
#include "slave/clint.h"

namespace component
{
    typedef struct store_buffer_item_t : public if_print_t
    {
        bool enable = false;
        bool committed = false;
        uint32_t rob_id = 0;
        uint32_t pc = 0;
        uint32_t addr = 0;
        uint32_t data = 0;
        uint32_t size = 0;
    }store_buffer_item_t;
    
    typedef struct store_buffer_state_pack_t
    {
        uint32_t wptr;
        bool wstage;
    }store_buffer_state_pack_t;
    
    class store_buffer : public fifo<store_buffer_item_t>
    {
        private:
            bus *bus_if;
            
            trace::trace_database tdb;
            
            bool check_id_valid(uint32_t id)
            {
                if(this->customer_is_empty())
                {
                    return false;
                }
                else if(this->wstage.get() == this->rstage.get_new())
                {
                    return (id >= this->rptr.get_new()) && (id < this->wptr.get());
                }
                else
                {
                    return ((id >= this->rptr.get_new()) && (id < this->size)) || (id < this->wptr.get());
                }
            }
            
            store_buffer_item_t get_item(uint32_t id)
            {
                verify(check_id_valid(id));
                return this->buffer[id].get();
            }
            
            void set_item(uint32_t id, store_buffer_item_t value)
            {
                verify(check_id_valid(id));
                this->buffer[id].set(value);
            }
            
            bool get_front_id(uint32_t *front_id)
            {
                return this->customer_get_front_id(front_id);
            }
            
            bool get_front_id_stage(uint32_t *front_id, bool *front_stage)
            {
                if(this->customer_is_empty())
                {
                    return false;
                }
                
                *front_id = this->rptr.get_new();
                *front_stage = this->rstage.get_new();
                return true;
            }
            
            bool get_tail_id(uint32_t *tail_id)
            {
                return this->customer_get_tail_id(tail_id);
            }
            
            bool get_next_id(uint32_t id, uint32_t *next_id)
            {
                return this->customer_get_next_id(id, next_id);
            }
            
            bool get_next_id_stage(uint32_t id, bool stage, uint32_t *next_id, bool *next_stage)
            {
                return this->customer_get_next_id_stage(id, stage, next_id, next_stage);
            }
        
        public:
            store_buffer(uint32_t size, bus *bus_if) : fifo<store_buffer_item_t>(size), tdb(TRACE_STORE_BUFFER)
            {
                this->bus_if = bus_if;
                this->store_buffer::reset();
            }
            
            virtual void reset()
            {
                fifo<store_buffer_item_t>::reset();
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
            
            store_buffer_state_pack_t save()
            {
                store_buffer_state_pack_t pack;
                
                pack.wptr = wptr.get();
                pack.wstage = wstage.get();
                return pack;
            }
            
            void restore(store_buffer_state_pack_t pack)
            {
                wptr.set(pack.wptr);
                wstage.set(pack.wstage);
            }
            
            uint32_t get_feedback_value(uint32_t addr, uint32_t size, uint32_t bus_value)
            {
                uint32_t result = bus_value;
                uint32_t cur_id;
                
                if(get_front_id(&cur_id))
                {
                    auto first_id = cur_id;
                    
                    do
                    {
                        auto cur_item = get_item(cur_id);
                        
                        if((cur_item.addr >= addr) && (cur_item.addr < (addr + size)))
                        {
                            uint32_t bit_offset = (cur_item.addr - addr) << 3;
                            uint32_t bit_length = std::min(cur_item.size, addr + size - cur_item.addr) << 3;
                            uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                            result &= ~(bit_mask << bit_offset);
                            result |= (cur_item.data & bit_mask) << bit_offset;
                        }
                        else if((cur_item.addr < addr) && ((cur_item.addr + cur_item.size) > addr))
                        {
                            uint32_t bit_offset = (addr - cur_item.addr) << 3;
                            uint32_t bit_length = std::min(size, cur_item.addr + cur_item.size - addr) << 3;
                            uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                            result &= ~bit_mask;
                            result |= (cur_item.data >> bit_offset) & bit_mask;
                        }
                    }while(get_next_id(cur_id, &cur_id) && (cur_id != first_id));
                }
                
                return result;
            }
            
            void run(pipeline::commit_feedback_pack_t commit_feedback_pack)
            {
                if(commit_feedback_pack.flush)
                {
                    uint32_t cur_id;
                    bool cur_stage;
                    bool found = false;
                    uint32_t found_id;
                    bool found_stage;
                    
                    if(get_front_id_stage(&cur_id, &cur_stage))
                    {
                        auto first_id = cur_id;
                        
                        do
                        {
                            auto cur_item = get_item(cur_id);
                            
                            if(!cur_item.committed)
                            {
                                bool ready_to_commit = false;
                                
                                for(auto i = 0;i < COMMIT_WIDTH;i++)
                                {
                                    if(commit_feedback_pack.committed_rob_id_valid[i] && (commit_feedback_pack.committed_rob_id[i] == cur_item.rob_id))
                                    {
                                        ready_to_commit = true;
                                        break;
                                    }
                                }
                                
                                if(!ready_to_commit)
                                {
                                    found = true;
                                    found_id = cur_id;
                                    found_stage = cur_stage;
                                    break;
                                }
                            }
                        }while(get_next_id_stage(cur_id, cur_stage, &cur_id, &cur_stage) && (cur_id != first_id));
                        
                        if(found)
                        {
                            wptr.set(found_id);
                            wstage.set(found_stage);
                        }
                    }
                }
                else
                {
                    //handle write back
                    store_buffer_item_t item;
                    
                    if(customer_get_front(&item))
                    {
                        if(item.committed)
                        {
                            store_buffer_item_t t_item;
                            pop(&t_item);
                            verify((item.size == 1) || (item.size == 2) || (item.size == 4));
                            
                            switch(item.size)
                            {
                                case 1:
                                    bus_if->write8_sync(item.addr, (uint8_t)item.data);
                                    break;
                                
                                case 2:
                                    bus_if->write16_sync(item.addr, (uint16_t)item.data);
                                    break;
                                
                                case 4:
                                    bus_if->write32_sync(item.addr, item.data);
                                    break;
                            }
                        }
                    }
                }
                
                //handle feedback
                uint32_t cur_id;
                
                if(get_front_id(&cur_id))
                {
                    auto first_id = cur_id;
                    
                    do
                    {
                        auto cur_item = get_item(cur_id);
                        
                        for(auto i = 0;i < COMMIT_WIDTH;i++)
                        {
                            if(commit_feedback_pack.committed_rob_id_valid[i] && (commit_feedback_pack.committed_rob_id[i] == cur_item.rob_id))
                            {
                                cur_item.committed = true;
                            }
                        }
                        
                        set_item(cur_id, cur_item);
                    }while(get_next_id(cur_id, &cur_id) && (cur_id != first_id));
                }
            }
    };
}