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

namespace cycle_model::component
{
    class pma_unit
    {
        private:
            struct pma_item_t
            {
                uint32_t addr;
                uint32_t size;
                bool is_cacheable;
            };
            
            std::vector<pma_item_t> pma_info;
            
            std::optional<pma_item_t> get_pma_item(uint32_t addr)
            {
                for(auto &pma_item : pma_info)
                {
                    if((addr >= pma_item.addr) && (addr <= (pma_item.addr + pma_item.size - 1)))
                    {
                        return pma_item;
                    }
                }
                
                return std::nullopt;
            }
            
        public:
            void add_fixed_pma_item(const pma_item_t &pma_item)
            {
                for(auto &t_pma_item : pma_info)
                {
                    //verify whether pma_item and t_pma_item has same space
                    verify_only(!((pma_item.addr >= t_pma_item.addr) && (pma_item.addr < (t_pma_item.addr + t_pma_item.size))) &&
                                !((t_pma_item.addr >= pma_item.addr) && (t_pma_item.addr < (pma_item.addr + pma_item.size))));
                }
                
                pma_info.push_back(pma_item);
            }
            
            bool is_cacheable(uint32_t addr)
            {
                auto pma_item = get_pma_item(addr);
                
                if(pma_item.has_value())
                {
                    return pma_item.value().is_cacheable;
                }
                else
                {
                    return false;
                }
            }
    };
}