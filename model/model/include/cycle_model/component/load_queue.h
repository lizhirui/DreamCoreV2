/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "fifo.h"
#include "age_compare.h"

namespace cycle_model::component
{
    typedef struct load_queue_item_t : public if_print_t
    {
        bool addr_valid = false;
        uint32_t pc = 0;
        uint32_t rob_id = 0;
        bool rob_id_stage = false;
        uint32_t addr = 0;
        uint32_t size = 0;
        bool checkpoint_id_valid = false;
        uint32_t checkpoint_id = 0;
    }load_queue_item_t;
    
    class load_queue : public fifo<load_queue_item_t>
    {
        private:
            dff<bool> *item_conflict;
            dff<uint32_t> *replay_num;
            
        public:
            load_queue(uint32_t size) : fifo<load_queue_item_t>(size)
            {
                this->item_conflict = new dff<bool>[size];
                this->replay_num = new dff<uint32_t>[size];
                this->load_queue::reset();
            }
            
            ~load_queue()
            {
                delete[] this->item_conflict;
            }
            
            virtual void reset()
            {
                fifo<load_queue_item_t>::reset();
                
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->item_conflict[i].set(false);
                    this->replay_num[i].set(0);
                }
            }
            
            virtual void flush()
            {
                fifo<load_queue_item_t>::flush();
    
                for(uint32_t i = 0;i < this->size;i++)
                {
                    this->item_conflict[i].set(false);
                    this->replay_num[i].set(0);
                }
            }
            
            void write_conflict(uint32_t id, bool conflict)
            {
                this->item_conflict[id].set(conflict);
            }
            
            bool get_conflict(uint32_t id)
            {
                return this->item_conflict[id].get();
            }
            
            void add_replay_num(uint32_t id)
            {
                this->replay_num[id].set(this->replay_num[id].get() + 1);
            }
            
            void clear_replay_num(uint32_t id)
            {
                this->replay_num[id].set(0);
            }
            
            uint32_t get_replay_num(uint32_t id)
            {
                return this->replay_num[id].get();
            }
            
            std::optional<std::tuple<uint32_t, bool, load_queue_item_t>> get_conflict_item(uint32_t addr, uint32_t size, uint32_t rob_id, bool rob_id_stage)
            {
                uint32_t cur_id;
                bool cur_stage;
                
                if(producer_get_front_id_stage(&cur_id, &cur_stage))
                {
                    auto first_id = cur_id;
                    
                    do
                    {
                        auto cur_item = producer_get_item(cur_id);
                        
                        if(cur_item.addr_valid)
                        {
                            if(component::age_compare(cur_item.rob_id, cur_item.rob_id_stage) < component::age_compare(rob_id, rob_id_stage))
                            {
                                if(std::max(cur_item.addr, addr) < std::min(cur_item.addr + cur_item.size, addr + size))
                                {
                                    //conflict is found
                                    return std::make_optional(std::make_tuple(cur_id, cur_stage, cur_item));
                                }
                            }
                        }
                    }while(producer_get_next_id_stage(cur_id, cur_stage, &cur_id, &cur_stage) && (cur_id != first_id));
                }
    
                return std::nullopt;
            }
    };
}