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
#include "dff.h"
#include "rat.h"
#include "age_compare.h"
#include "../pipeline/execute/lu_define.h"

namespace cycle_model::component
{
    class rat;
    
    template<typename T>
    class regfile : public if_print_t, if_reset_t
    {
        private:
            dff<T> *reg_data;
            dff<bool> *reg_data_valid;
            dff<uint32_t> *reg_rob_id;
            dff<bool> *reg_rob_id_stage;
            dff<bool> *reg_oldest;
            dff<uint32_t> *reg_lpv;
            uint32_t size = 0;
        
            trace::trace_database tdb;
            
        public:
            regfile(uint32_t size) : tdb(TRACE_PHY_REGFILE)
            {
                this->size = size;
                reg_data = new dff<T>[size];
                reg_data_valid = new dff<bool>[size];
                reg_rob_id = new dff<uint32_t>[size];
                reg_rob_id_stage = new dff<bool>[size];
                reg_oldest = new dff<bool>[size];
                reg_lpv = new dff<uint32_t>[size];
                this->reset();
            }
        
            ~regfile()
            {
                delete[] reg_data;
                delete[] reg_data_valid;
                delete[] reg_rob_id;
                delete[] reg_rob_id_stage;
                delete[] reg_oldest;
            }
        
            virtual void reset()
            {
                for(uint32_t i = 0;i < size;i++)
                {
                    reg_data[i].set(0);
                    reg_data_valid[i].set(false);
                    reg_rob_id[i].set(0);
                    reg_rob_id_stage[i].set(false);
                    reg_oldest[i].set(true);
                }
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
        
            void write(uint32_t addr, T value, bool valid, uint32_t rob_id, bool rob_id_stage, bool oldest)
            {
                verify_only(addr < size);
                reg_data[addr].set(value);
                reg_data_valid[addr].set(valid);
                reg_rob_id[addr].set(rob_id);
                reg_rob_id_stage[addr].set(rob_id_stage);
                reg_oldest[addr].set(oldest);
            }
        
            T read(uint32_t addr)
            {
                verify_only(addr < size);
                return reg_data[addr];
            }
        
            bool read_data_valid(uint32_t addr)
            {
                verify_only(addr < size);
                return reg_data_valid[addr];
            }
            
            void write_age_information(uint32_t addr, uint32_t rob_id, bool rob_id_stage, bool oldest)
            {
                verify_only(addr < size);
                reg_rob_id[addr].set(rob_id);
                reg_rob_id_stage[addr].set(rob_id_stage);
                reg_oldest[addr].set(oldest);
            }
            
            std::tuple<uint32_t, bool, bool> read_age_information(uint32_t addr)
            {
                verify_only(addr < size);
                return {reg_rob_id[addr], reg_rob_id_stage[addr], reg_oldest[addr]};
            }
        
            void set_lpv(uint32_t addr, uint32_t lpv)
            {
                verify_only(addr < size);
                reg_lpv[addr].set(lpv);
            }
            
            uint32_t get_lpv(uint32_t addr)
            {
                verify_only(addr < size);
                return reg_lpv[addr].get();
            }
            
            void restore(rat *element)
            {
                for(uint32_t i = 0;i < size;i++)
                {
                    reg_data_valid[i].set(element->producer_get_valid(i));
                }
            }
            
            void restore(uint32_t rob_id, bool rob_id_stage)
            {
                for(uint32_t i = 0;i < size;i++)
                {
                    if(reg_data_valid[i].get() && !reg_oldest[i].get())
                    {
                        if(age_compare(reg_rob_id[i].get(), reg_rob_id_stage[i].get()) < age_compare(rob_id, rob_id_stage))
                        {
                            reg_data_valid[i].set(false);
                            reg_oldest[i].set(true);
                        }
                    }
                }
            }
        
            void restore_equal(uint32_t rob_id, bool rob_id_stage)
            {
                for(uint32_t i = 0;i < size;i++)
                {
                    if(reg_data_valid[i].get() && !reg_oldest[i].get())
                    {
                        if(age_compare(reg_rob_id[i].get(), reg_rob_id_stage[i].get()) <= age_compare(rob_id, rob_id_stage))
                        {
                            reg_data_valid[i].set(false);
                            reg_oldest[i].set(true);
                        }
                    }
                }
            }
            
            void run(const pipeline::execute::lu_feedback_pack_t &lu_feedback_pack)
            {
                for(uint32_t i = 0;i < size;i++)
                {
                    if(lu_feedback_pack.replay && ((reg_lpv[i].get() & 1) != 0))
                    {
                        reg_data_valid[i].set(false);
                    }
                    
                    if(!lu_feedback_pack.stall)
                    {
                        reg_lpv[i].set(reg_lpv[i].get() >> 1);
                    }
                }
            }
            
            virtual json get_json()
            {
                json t;
                json value = json::array();
                json valid = json::array();
                
                for(uint32_t i = 0;i < size;i++)
                {
                    value.push_back(reg_data[i].get());
                    valid.push_back(reg_data_valid[i].get());
                }
                
                t["value"] = value;
                t["valid"] = valid;
                return t;
            }
    };
}