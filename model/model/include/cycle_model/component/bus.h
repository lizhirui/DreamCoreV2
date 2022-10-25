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
#include "dff.h"
#include "slave_base.h"

namespace cycle_model::component
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
            enum class sync_request_type_t
            {
                write8,
                write16,
                write32
            };
            
            typedef struct sync_request_t
            {
                sync_request_type_t req;
                uint32_t arg1;
                union
                {
                    uint8_t u8;
                    uint16_t u16;
                    uint32_t u32;
                }arg2;
            }sync_request_t;
            
            std::queue<sync_request_t> sync_request_q;
            std::vector<slave_info_t> slave_info_list;
            
            dff<uint32_t> instruction_value[FETCH_WIDTH];
            dff<bool> instruction_value_valid;
            dff<uint32_t> data_value;
            dff<bool> data_value_valid;
            
            trace::trace_database tdb;
            
            bool check_addr_override(uint32_t base, uint32_t size)
            {
                for(uint32_t i = 0;i < slave_info_list.size();i++)
                {
                    if(((base >= slave_info_list[i].base) && (base < (slave_info_list[i].base + slave_info_list[i].size))) || ((base < slave_info_list[i].base) && ((base + size) > slave_info_list[i].base)))
                    {
                        return true;
                    }
                }
                
                return false;
            }
        
        public:
            bus() : instruction_value_valid(false), data_value_valid(false), tdb(TRACE_BUS)
            {
                this->bus::reset();
            }
            
            int find_slave_info(uint32_t addr, bool is_fetch)
            {
                for(uint32_t i = 0;i < slave_info_list.size();i++)
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
                clear_queue(sync_request_q);
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
            
            void write8_sync(uint32_t addr, uint8_t value)
            {
                sync_request_t t_req;
                
                t_req.req = sync_request_type_t::write8;
                t_req.arg1 = addr;
                t_req.arg2.u8 = value;
                sync_request_q.push(t_req);
            }
            
            void write16(uint32_t addr, uint16_t value)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->write16(addr - slave_info_list[slave_index].base, value);
                }
            }
            
            void write16_sync(uint32_t addr, uint16_t value)
            {
                sync_request_t t_req;
                
                t_req.req = sync_request_type_t::write16;
                t_req.arg1 = addr;
                t_req.arg2.u16 = value;
                sync_request_q.push(t_req);
            }
            
            void write32(uint32_t addr, uint32_t value)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->write32(addr - slave_info_list[slave_index].base, value);
                }
            }
            
            void write32_sync(uint32_t addr, uint32_t value)
            {
                sync_request_t t_req;
                
                t_req.req = sync_request_type_t::write32;
                t_req.arg1 = addr;
                t_req.arg2.u32 = value;
                sync_request_q.push(t_req);
            }
            
            void read8(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->read8(addr - slave_info_list[slave_index].base);
                }
            }
            
            void read16(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->read16(addr - slave_info_list[slave_index].base);
                }
            }
            
            void read32(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->read32(addr - slave_info_list[slave_index].base);
                }
            }
            
            uint8_t read8_sys(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    return slave_info_list[slave_index].slave->read8_sys(addr - slave_info_list[slave_index].base);
                }
                
                return 0;
            }
            
            uint16_t read16_sys(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    return slave_info_list[slave_index].slave->read16_sys(addr - slave_info_list[slave_index].base);
                }
    
                return 0;
            }
        
            uint16_t read32_sys(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, false);slave_index >= 0)
                {
                    return slave_info_list[slave_index].slave->read32_sys(addr - slave_info_list[slave_index].base);
                }
            
                return 0;
            }
            
            void read_instruction(uint32_t addr)
            {
                if(auto slave_index = find_slave_info(addr, true);slave_index >= 0)
                {
                    slave_info_list[slave_index].slave->read_instruction(addr - slave_info_list[slave_index].base);
                }
            }
            
            bool get_instruction_value(uint32_t *value)
            {
                if(instruction_value_valid)
                {
                    for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                    {
                        value[i] = instruction_value[i];
                    }
                    
                    return true;
                }
                
                return false;
            }
            
            bool get_data_value(uint32_t *value)
            {
                if(data_value_valid)
                {
                    *value = data_value;
                    return true;
                }
                
                return false;
            }
            
            void set_instruction_value(const uint32_t *value)
            {
                instruction_value_valid.set(true);
                
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    instruction_value[i] = value[i];
                }
            }
            
            void set_data_value(uint32_t value)
            {
                data_value_valid.set(true);
                data_value = value;
            }
            
            void run()
            {
                if(!instruction_value_valid.is_changed())
                {
                    instruction_value_valid.set(false);
                }
                
                if(!data_value_valid.is_changed())
                {
                    data_value_valid.set(false);
                }
            }
            
            void sync()
            {
                sync_request_t t_req;
                
                while(!sync_request_q.empty())
                {
                    t_req = sync_request_q.front();
                    sync_request_q.pop();
                    
                    switch(t_req.req)
                    {
                        case sync_request_type_t::write8:
                            write8(t_req.arg1, t_req.arg2.u8);
                            break;
                        
                        case sync_request_type_t::write16:
                            write16(t_req.arg1, t_req.arg2.u16);
                            break;
                        
                        case sync_request_type_t::write32:
                            write32(t_req.arg1, t_req.arg2.u32);
                            break;
                    }
                }
            }
    };
}