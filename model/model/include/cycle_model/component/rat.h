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

namespace component
{
    class rat : public if_print_t, public if_reset_t
    {
        private:
            uint32_t phy_reg_num;
            uint32_t arch_reg_num;
            dff<uint32_t> *phy_map_table;
            dff<bool> *phy_map_table_valid;
            dff<bool> *phy_map_table_visible;
            bool init_rat;
            dff<uint32_t> freelist_rptr;
            dff<bool> freelist_rstage;
            dff<uint32_t> freelist_wptr;
            dff<bool> freelist_wstage;
            
            trace::trace_database tdb;
            
            void set_valid(uint32_t phy_id, bool v)
            {
                assert(phy_id < phy_reg_num);
                phy_map_table_valid[phy_id].set(v);
            }
            
            bool get_valid(uint32_t phy_id)
            {
                assert(phy_id < phy_reg_num);
                return phy_map_table_valid[phy_id].get();
            }
            
            void set_visible(uint32_t phy_id, bool v)
            {
                assert(phy_id < phy_reg_num);
                phy_map_table_visible[phy_id].set(v);
            }
            
            bool get_visible(uint32_t phy_id)
            {
                assert(phy_id < phy_reg_num);
                return phy_map_table_visible[phy_id].get();
            }
            
            bool freelist_is_empty()
            {
                return (freelist_rptr.get_new() == freelist_wptr.get()) && (freelist_rstage.get_new() == freelist_wstage.get());
            }
            
            bool freelist_is_full()
            {
                return (freelist_rptr.get() == freelist_wptr.get_new()) && (freelist_rstage.get() == freelist_wstage.get_new());
            }
            
            bool freelist_pop(uint32_t *phy_id)
            {
                if(freelist_is_empty())
                {
                    return false;
                }
                
                *phy_id = freelist_rptr.get_new();
                
                if(freelist_rptr.get_new() >= phy_reg_num - 1)
                {
                    freelist_rptr.set(0);
                    freelist_rstage.set(!freelist_rstage);
                }
                else
                {
                    freelist_rptr.set(freelist_rptr.get_new() + 1);
                }
                
                return true;
            }
            
            bool freelist_push(uint32_t phy_id)
            {
                if(freelist_is_full())
                {
                    return false;
                }
                
                assert(freelist_wptr.get_new() == phy_id);
    
                if(freelist_wptr.get_new() >= phy_reg_num - 1)
                {
                    freelist_wptr.set(0);
                    freelist_wstage.set(!freelist_wstage);
                }
                else
                {
                    freelist_wptr.set(freelist_wptr.get_new() + 1);
                }
                
                return true;
            }
            
        public:
            rat(uint32_t phy_reg_num, uint32_t arch_reg_num) : freelist_rptr(0), freelist_rstage(false), freelist_wptr(0), freelist_wstage(false), tdb(TRACE_RAT)
            {
                this->phy_reg_num = phy_reg_num;
                this->arch_reg_num = arch_reg_num;
                phy_map_table = new dff<uint32_t>[phy_reg_num];
                phy_map_table_valid = new dff<bool>[phy_reg_num];
                phy_map_table_visible = new dff<bool>[phy_reg_num];
                init_rat = false;
            }
            
            ~rat()
            {
                delete[] phy_map_table;
                delete[] phy_map_table_valid;
                delete[] phy_map_table_visible;
            }
            
            void init_start()
            {
                for(auto i = 0;i < phy_reg_num;i++)
                {
                    phy_map_table[i].set(i);
                    phy_map_table_valid[i].set(false);
                    phy_map_table_visible[i].set(false);
                }
                
                init_rat = true;
            }
            
            void init_finish()
            {
                init_rat = false;
            }
            
            virtual void reset()
            {
                freelist_rptr.set(0);
                freelist_rstage.set(false);
                freelist_wptr.set(0);
                freelist_wstage.set(false);
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
            
            uint32_t get_free_phy_id(uint32_t num, uint32_t *ret, bool revert = true)
            {
                uint32_t ret_cnt = 0;
                
                while(freelist_pop(&ret[ret_cnt]))
                {
                    ret_cnt++;
    
                    if(ret_cnt >= num)
                    {
                        break;
                    }
                }
                
                if(revert)
                {
                    freelist_rptr.revert();
                    freelist_rstage.revert();
                }
                
                return ret_cnt;
            }
            
            bool get_phy_id(uint32_t arch_id, uint32_t *phy_id)
            {
                int cnt = 0;
                assert((arch_id > 0) && (arch_id < arch_reg_num));
                
                for(uint32_t i = 0;i < phy_reg_num;i++)
                {
                    if(get_valid(i) && get_visible(i) && (phy_map_table[i] == arch_id))
                    {
                        *phy_id = i;
                        cnt++;
                    }
                }
                
                assert(cnt <= 1);
                return cnt == 1;
            }
            
            uint32_t set_map(uint32_t arch_id, uint32_t phy_id)
            {
                uint32_t old_phy_id;
                assert(phy_id < phy_reg_num);
                assert((arch_id > 0) && (arch_id < arch_reg_num));
                assert(!get_valid(phy_id));
                bool ret = get_phy_id(arch_id, &old_phy_id);
                
                if(!init_rat)
                {
                    assert(ret);
                    assert(!get_valid(phy_id));
                }
                
                phy_map_table[phy_id] = arch_id;
                set_valid(phy_id, true);
                set_visible(phy_id, true);
                
                if(ret)
                {
                    set_visible(old_phy_id, false);
                }
                
                return old_phy_id;
            }
            
            void release_map(uint32_t phy_id)
            {
                assert(phy_id < phy_reg_num);
                assert(get_valid(phy_id));
                assert(!get_visible(phy_id));
                phy_map_table[phy_id] = 0;
                set_valid(phy_id, false);
            }
            
            void restore_map(uint32_t new_phy_id, uint32_t old_phy_id)
            {
                assert(new_phy_id < phy_reg_num);
                assert(old_phy_id < phy_reg_num);
                assert(get_valid(new_phy_id));
                assert(get_valid(old_phy_id));
                assert(get_visible(new_phy_id));
                assert(!get_visible(old_phy_id));
                phy_map_table[new_phy_id] = 0;
                set_valid(new_phy_id, false);
                set_visible(new_phy_id, false);
                set_valid(old_phy_id, true);
                set_visible(old_phy_id, true);
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
                    
                    std::cout << "Phy_ID\tArch_ID\tVisible\tValid";
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
                            
                            std::cout << phy_id << "\t" << phy_map_table[phy_id] << "\t" << outbool(get_visible(phy_id)) << "\t" << outbool(get_valid(phy_id));
                        }
                    }
                    
                    std::cout << std::endl;
                }
            }
    };
}