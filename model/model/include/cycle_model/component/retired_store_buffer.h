/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-16     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "fifo.h"
#include "bus.h"
#include "../pipeline/pipeline_common.h"
#include "slave/memory.h"
#include "slave/clint.h"

namespace cycle_model::component
{
    struct retired_store_buffer_item_t : public if_print_t
    {
        pipeline::inst_common_info_t inst_common_info;
        uint32_t addr = 0;
        uint32_t size = 0;
        uint32_t data = 0;
    };
    
    class retired_store_buffer : public fifo<retired_store_buffer_item_t>
    {
        private:
            bus *bus_if;
            bool is_waiting_ack = false;
            
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
            retired_store_buffer(uint32_t size, bus *bus_if) : fifo<retired_store_buffer_item_t>(size), tdb(TRACE_RETIRED_STORE_BUFFER)
            {
                this->bus_if = bus_if;
                this->retired_store_buffer::reset();
            }
            
            ~retired_store_buffer()
            {
            }
            
            virtual void reset()
            {
                fifo<retired_store_buffer_item_t>::reset();
                this->is_waiting_ack = false;
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
            
            virtual void set_item(uint32_t id, retired_store_buffer_item_t value)
            {
                verify(check_id_valid(id));
                this->buffer[id].set(value);
            }
            
            retired_store_buffer_item_t get_item(uint32_t id)
            {
                verify(check_id_valid(id));
                return this->buffer[id].get();
            }
            
            std::tuple<uint32_t, uint32_t> get_feedback_value(uint32_t addr, uint32_t size)
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
                        
                        if((cur_item.addr >= addr) && (cur_item.addr < (addr + size)))
                        {
                            uint32_t bit_offset = (cur_item.addr - addr) << 3;
                            uint32_t bit_length = std::min(cur_item.size, addr + size - cur_item.addr) << 3;
                            uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                            result &= ~(bit_mask << bit_offset);
                            result |= (cur_item.data & bit_mask) << bit_offset;
                            feedback_mask |= bit_mask << bit_offset;
                        }
                        else if((cur_item.addr < addr) && ((cur_item.addr + cur_item.size) > addr))
                        {
                            uint32_t bit_offset = (addr - cur_item.addr) << 3;
                            uint32_t bit_length = std::min(size, cur_item.addr + cur_item.size - addr) << 3;
                            uint32_t bit_mask = (bit_length == 32) ? 0xffffffffu : ((1 << bit_length) - 1);
                            result &= ~bit_mask;
                            result |= (cur_item.data >> bit_offset) & bit_mask;
                            feedback_mask |= bit_mask;
                        }
                    }while(producer_get_next_id(cur_id, &cur_id) && (cur_id != first_id));
                }
                
                return std::tuple{result, feedback_mask};
            }
            
            void run()
            {
                if(is_waiting_ack)
                {
                    bus_errno_t write_errno;
                    retired_store_buffer_item_t item;
                    
                    if(bus_if->get_data_write_ack(&write_errno))
                    {
                        is_waiting_ack = false;
                        verify(pop(&item));
                    }
                }
                
                if(!is_waiting_ack)
                {
                    retired_store_buffer_item_t item;
                    
                    if(customer_get_front(&item))
                    {
                        verify_only((item.size == 1) || (item.size == 2) || (item.size == 4));
                        
                        switch(item.size)
                        {
                            case 1:
                                bus_if->write8(item.addr, item.data);
                                break;
                                
                            case 2:
                                bus_if->write16(item.addr, item.data);
                                break;
                                
                            case 4:
                                bus_if->write32(item.addr, item.data);
                                break;
                        }
                        
                        is_waiting_ack = true;
                    }
                }
            }
    };
}
