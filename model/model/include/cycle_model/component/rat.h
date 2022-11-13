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

namespace cycle_model::component
{
    class rat : public if_print_t, public if_reset_t
    {
        private:
            uint32_t phy_reg_num;
            uint32_t arch_reg_num;
            dff<uint32_t> *phy_map_table;
            dff<bool> *phy_map_table_valid;
            bool init_rat;
            
            trace::trace_database tdb;
            
            void set_valid(uint32_t phy_id, bool v)
            {
                verify_only(phy_id < phy_reg_num);
                phy_map_table_valid[phy_id].set(v);
            }
            
        public:
            rat(uint32_t phy_reg_num, uint32_t arch_reg_num) : tdb(TRACE_RAT)
            {
                this->phy_reg_num = phy_reg_num;
                this->arch_reg_num = arch_reg_num;
                phy_map_table = new dff<uint32_t>[phy_reg_num];
                phy_map_table_valid = new dff<bool>[phy_reg_num];
                init_rat = false;
                this->rat::reset();
            }
            
            ~rat()
            {
                delete[] phy_map_table;
                delete[] phy_map_table_valid;
            }
            
            void init_start()
            {
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    phy_map_table[i].set(0);
                    phy_map_table_valid[i].set(false);
                }
                
                init_rat = true;
            }
            
            void init_finish()
            {
                init_rat = false;
            }
            
            virtual void reset()
            {
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    phy_map_table[i].set(0);
                    phy_map_table_valid[i].set(false);
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
        
            bool producer_get_valid(uint32_t phy_id)
            {
                verify_only(phy_id < phy_reg_num);
                return phy_map_table_valid[phy_id].get_new();
            }
        
            bool customer_get_valid(uint32_t phy_id)
            {
                verify_only(phy_id < phy_reg_num);
                return phy_map_table_valid[phy_id].get();
            }
            
            void load(rat *element)
            {
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    phy_map_table[i].set(element->phy_map_table[i].get_new());
                    phy_map_table_valid[i].set(element->phy_map_table_valid[i].get_new());
                }
            }
            
            bool producer_get_phy_id(uint32_t arch_id, uint32_t *phy_id)
            {
                int cnt = 0;
                verify_only((arch_id > 0) && (arch_id < arch_reg_num));
                
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    if(producer_get_valid(i) && (phy_map_table[i].get_new() == arch_id))
                    {
                        *phy_id = i;
                        cnt++;
                    }
                }
                
                verify_only(cnt <= 1);
                return cnt == 1;
            }
        
            bool customer_get_phy_id(uint32_t arch_id, uint32_t *phy_id)
            {
                int cnt = 0;
                verify_only((arch_id > 0) && (arch_id < arch_reg_num));
            
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    if(customer_get_valid(i) && (phy_map_table[i].get() == arch_id))
                    {
                        *phy_id = i;
                        cnt++;
                    }
                }
            
                verify_only(cnt <= 1);
                return cnt == 1;
            }
            
            uint32_t set_map(uint32_t arch_id, uint32_t phy_id)
            {
                uint32_t old_phy_id = 0;
                verify_only(phy_id < phy_reg_num);
                verify_only((arch_id > 0) && (arch_id < arch_reg_num));
                verify_only(!producer_get_valid(phy_id));
                bool ret = producer_get_phy_id(arch_id, &old_phy_id);
                
                if(!init_rat)
                {
                    verify_only(ret);
                    verify_only(producer_get_valid(old_phy_id));
                }
                
                phy_map_table[phy_id].set(arch_id);
                set_valid(phy_id, true);
                
                if(ret)
                {
                    set_valid(old_phy_id, false);
                }
                
                return old_phy_id;
            }
            
            void restore_map(uint32_t new_phy_id, uint32_t old_phy_id)
            {
                verify_only(new_phy_id < phy_reg_num);
                verify_only(old_phy_id < phy_reg_num);
                verify_only(producer_get_valid(new_phy_id));
                verify_only(producer_get_valid(old_phy_id));
                phy_map_table[new_phy_id] = 0;
                set_valid(new_phy_id, false);
                set_valid(old_phy_id, true);
            }
            
            virtual void print(std::string indent)
            {
                auto col = 5;
                
                std::cout << indent << "Register Allocation Table:" << std::endl;
                
                for(auto i = 0;i < col;i++)
                {
                    if(i == 0)
                    {
                        std::cout << indent;
                    }
                    else
                    {
                        std::cout << "\t\t";
                    }
                    
                    std::cout << "Phy_ID\tArch_ID\tValid";
                }
                
                std::cout << std::endl;
                
                auto numbycol = (phy_reg_num + col - 1) / col;
                
                for(uint32_t i = 0;i < numbycol;i++)
                {
                    for(auto j = 0;j < col;j++)
                    {
                        auto phy_id = j * numbycol + i;
                        
                        if(phy_id < phy_reg_num)
                        {
                            if(j == 0)
                            {
                                std::cout << indent;
                            }
                            else
                            {
                                std::cout << "\t\t";
                            }
                            
                            std::cout << phy_id << "\t" << phy_map_table[phy_id] << "\t" << outbool(customer_get_valid(phy_id));
                        }
                    }
                    
                    std::cout << std::endl;
                }
            }
            
            virtual json get_json()
            {
                json t;
                json value = json::array();
                json valid = json::array();
                
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    value.push_back(phy_map_table[i].get());
                    valid.push_back(phy_map_table_valid[i].get());
                }
                
                t["value"] = value;
                t["valid"] = valid;
                return t;
            }
    };
}