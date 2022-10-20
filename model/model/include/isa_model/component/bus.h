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
#include "config.h"
#include "slave_base.h"

namespace isa_model::component
{
    class slave_base;
    
    typedef struct slave_info_t
    {
        uint32_t base = 0;
        uint32_t size = 0;
        std::shared_ptr<slave_base> slave;
        bool support_fetch = false;
    }slave_info_t;
    
    class bus : public if_reset_t
    {
        private:
            std::vector<slave_info_t> slave_info_list;
            
            bool check_addr_override(uint32_t base, uint32_t size)
            {
                for(size_t i = 0;i < slave_info_list.size();i++)
                {
                    if(((base >= slave_info_list[i].base) && (base < (slave_info_list[i].base + slave_info_list[i].size))) || ((base < slave_info_list[i].base) && ((base + size) > slave_info_list[i].base)))
                    {
                        return true;
                    }
                }
                
                return false;
            }
        
        public:
            bus()
            {
                this->bus::reset();
            }
            
            int find_slave_info(uint32_t addr, bool is_fetch)
            {
                for(size_t i = 0;i < slave_info_list.size();i++)
                {
                    if((addr >= slave_info_list[i].base) && (addr < (slave_info_list[i].base + slave_info_list[i].size)))
                    {
                        if(!is_fetch || slave_info_list[i].support_fetch)
                        {
                            return i;
                        }
                    }
                }
                
                return -1;
            }
            
            slave_base *get_slave_obj(uint32_t addr, bool is_fetch)
            {
                int slave_id = find_slave_info(addr, is_fetch);
                
                if(slave_id >= 0)
                {
                    return slave_info_list[slave_id].slave.get();
                }
                
                return nullptr;
            }
            
            uint32_t convert_to_slave_addr(uint32_t addr, bool is_fetch)
            {
                int slave_id = find_slave_info(addr, is_fetch);
                uint32_t result = addr;
                
                if(slave_id >= 0)
                {
                    result = addr - slave_info_list[slave_id].base;
                }
                
                return result;
            }
            
            void map(uint32_t base, uint32_t size, const std::shared_ptr<slave_base> &slave, bool support_fetch)
            {
                verify(!check_addr_override(base, size));
                slave_info_t slave_info;
                slave_info.base = base;
                slave_info.size = size;
                slave_info.slave = slave;
                slave_info.support_fetch = support_fetch;
                slave->init(size);
                slave_info_list.push_back(slave_info);
            }
            
            virtual void reset()
            {
            
            }
            
            static bool check_align(uint32_t addr, uint32_t access_size)
            {
                return !(addr & (access_size - 1));
            }
            
            void write8(uint32_t addr, uint8_t value)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->write8(addr - slave_info_list[slave_index].base, value);
                }
            }
            
            void write16(uint32_t addr, uint16_t value)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->write16(addr - slave_info_list[slave_index].base, value);
                }
            }
            
            void write32(uint32_t addr, uint32_t value)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->write32(addr - slave_info_list[slave_index].base, value);
                }
            }
            
            uint8_t read8(uint32_t addr, bool is_fetch)
            {
                if(auto slave_index = find_slave_info(addr, is_fetch);slave_index >= 0)
                {
                   return slave_info_list[slave_index].slave->read8(addr - slave_info_list[slave_index].base);
                }
                
                return 0;
            }
            
           uint16_t read16(uint32_t addr, bool is_fetch)
            {
                if(auto slave_index = find_slave_info(addr, is_fetch);slave_index >= 0)
                {
                    return slave_info_list[slave_index].slave->read16(addr - slave_info_list[slave_index].base);
                }
                
                return 0;
            }
            
            uint32_t read32(uint32_t addr, bool is_fetch)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    return slave_info_list[slave_index].slave->read32(addr - slave_info_list[slave_index].base);
                }
                
                return 0;
            }
    };
}