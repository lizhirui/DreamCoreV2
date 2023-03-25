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
#include "../pipeline/pipeline_common.h"
#include "../pipeline/execute/bru_define.h"
#include "../pipeline/execute/sau_define.h"
#include "../pipeline/commit.h"
#include "slave/memory.h"
#include "slave/clint.h"
#include "age_compare.h"

namespace cycle_model::component
{
    typedef struct store_buffer_item_t : public if_print_t
    {
        pipeline::inst_common_info_t inst_common_info;
        bool data_valid = false;
        bool committed = false;
        uint32_t rob_id = 0;
        bool rob_id_stage = false;
        uint32_t pc = 0;
        uint32_t data = 0;
        uint64_t cycle = 0;//only for debug
    }store_buffer_item_t;
    
    class store_buffer : public fifo<store_buffer_item_t>
    {
        private:
            bus *bus_if;
            
            trace::trace_database tdb;
            
            dff<bool> *item_addr_valid;
            dff<uint32_t> *item_addr;
            dff<uint32_t> *item_size;
            
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
                this->item_addr = new dff<uint32_t>[size];
                this->item_addr_valid = new dff<bool>[size];
                this->item_size = new dff<uint32_t>[size];
                this->store_buffer::reset();
            }
            
            ~store_buffer()
            {
                delete[] this->item_addr;
                delete[] this->item_addr_valid;
                delete[] this->item_size;
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
        
            virtual void set_item(uint32_t id, store_buffer_item_t value)
            {
                verify(check_id_valid(id));
                this->buffer[id].set(value);
            }
            
            void write_addr(uint32_t id, uint32_t addr, uint32_t size, bool addr_valid)
            {
                this->item_addr[id].set(addr);
                this->item_size[id].set(size);
                this->item_addr_valid[id].set(addr_valid);
            }
        
            store_buffer_item_t get_item(uint32_t id)
            {
                verify(check_id_valid(id));
                return this->buffer[id].get();
            }
            
            std::optional<std::tuple<uint32_t, uint32_t>> get_feedback_value(uint32_t addr, uint32_t size, uint32_t rob_id, bool rob_id_stage)
            {
                uint32_t result = 0;
                uint32_t cur_id;
                uint32_t feedback_mask = 0;
                
                if(producer_get_front_id(&cur_id))
                {
                    auto first_id = cur_id;
                    
                    do
                    {
                        auto cur_item = producer_get_item(cur_id);
                        auto cur_item_addr = item_addr[cur_id].get_new();
                        auto cur_item_size = item_size[cur_id].get_new();
                        auto cur_item_addr_valid = item_addr_valid[cur_id].get_new();
                        
                        if(!cur_item.data_valid && cur_item_addr_valid)
                        {
                            if(component::age_compare(cur_item.rob_id, cur_item.rob_id_stage) > component::age_compare(rob_id, rob_id_stage))
                            {
                                if(std::max(cur_item_addr, addr) < std::min(cur_item_addr + cur_item_size, addr + size))
                                {
                                    //conflict is found
                                    return std::nullopt;
                                }
                            }
                        }
                        
                        if(cur_item_addr_valid)
                        {
                            if(component::age_compare(cur_item.rob_id, cur_item.rob_id_stage) < component::age_compare(rob_id, rob_id_stage))
                            {
                                continue;//skip feedback from younger store item
                            }
                        }
                        
                        if(cur_item.data_valid && cur_item_addr_valid)
                        {
                            if((cur_item_addr >= addr) && (cur_item_addr < (addr + size)))
                            {
                                uint32_t bit_offset = (cur_item_addr - addr) << 3;
                                uint32_t bit_length = std::min(cur_item_size, addr + size - cur_item_addr) << 3;
                                uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                                result &= ~(bit_mask << bit_offset);
                                result |= (cur_item.data & bit_mask) << bit_offset;
                                feedback_mask |= bit_mask << bit_offset;
                            }
                            else if((cur_item_addr < addr) && ((cur_item_addr + cur_item_size) > addr))
                            {
                                uint32_t bit_offset = (addr - cur_item_addr) << 3;
                                uint32_t bit_length = std::min(size, cur_item_addr + cur_item_size - addr) << 3;
                                uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                                result &= ~bit_mask;
                                result |= (cur_item.data >> bit_offset) & bit_mask;
                                feedback_mask |= bit_mask;
                            }
                        }
                    }while(producer_get_next_id(cur_id, &cur_id) && (cur_id != first_id));
                }
                
                return std::tuple{result, feedback_mask};
            }
            
            void run(const pipeline::execute::bru_feedback_pack_t &bru_feedback_pack, const pipeline::execute::sau_feedback_pack_t &sau_feedback_pack, const pipeline::commit_feedback_pack_t &commit_feedback_pack)
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
                            auto cur_item_addr_valid = item_addr_valid[cur_id].get();
                            
                            if(!cur_item.committed)
                            {
                                bool ready_to_commit = false;
                                
                                for(uint32_t i = 0;i < COMMIT_WIDTH;i++)
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
                else if(bru_feedback_pack.flush)
                {
                    customer_foreach([=](uint32_t id, bool stage, const store_buffer_item_t &item)->bool
                    {
                        if(component::age_compare(item.rob_id, item.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage))
                        {
                            update_wptr(id, stage);
                            return false;
                        }
                        
                        return true;
                    });
                }
                else if(sau_feedback_pack.flush)
                {
                    customer_foreach([=](uint32_t id, bool stage, const store_buffer_item_t &item)->bool
                    {
                        if(component::age_compare(item.rob_id, item.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
                        {
                            update_wptr(id, stage);
                            return false;
                        }
                        
                        return true;
                    });
                }
                else
                {
                    //handle write back
                    store_buffer_item_t item;
                    
                    if(customer_get_front(&item))
                    {
                        uint32_t front_id = 0;
                        verify(customer_get_front_id(&front_id));
                        auto cur_item_addr = item_addr[front_id].get();
                        auto cur_item_size = item_size[front_id].get();
                        auto cur_item_addr_valid = item_addr_valid[front_id].get();
                        
                        if(item_addr_valid && item.data_valid && item.committed)
                        {
                            store_buffer_item_t t_item;
                            pop(&t_item);
                            verify_only((cur_item_size == 1) || (cur_item_size == 2) || (cur_item_size == 4));
                            
                            switch(cur_item_size)
                            {
                                case 1:
                                    bus_if->write8_sync(cur_item_addr, (uint8_t)item.data);
                                    break;
                                
                                case 2:
                                    bus_if->write16_sync(cur_item_addr, (uint16_t)item.data);
                                    break;
                                
                                case 4:
                                    bus_if->write32_sync(cur_item_addr, item.data);
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
                        auto cur_item_addr_valid = item_addr_valid[cur_id].get();
                        
                        if(cur_item_addr_valid && cur_item.data_valid)
                        {
                            for(uint32_t i = 0;i < COMMIT_WIDTH;i++)
                            {
                                if(commit_feedback_pack.committed_rob_id_valid[i] && (commit_feedback_pack.committed_rob_id[i] == cur_item.rob_id))
                                {
                                    cur_item.committed = true;
                                }
                            }
                            
                            set_item(cur_id, cur_item);
                        }
                    }while(get_next_id(cur_id, &cur_id) && (cur_id != first_id));
                }
            }
    };
}