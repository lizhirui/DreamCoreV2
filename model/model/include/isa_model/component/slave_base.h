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

namespace isa_model::component
{
    class bus;
    
    class slave_base : public if_reset_t
    {
        private:
            bool check(uint32_t addr, uint32_t access_size) const
            {
                verify_only(!(addr & (access_size - 1)));//align check
                verify_only(addr < size);//boundary check
                verify_only((size - addr) >= access_size);//boundary check
                return true;
            }
        
        protected:
            friend class bus;
            uint32_t size;
            component::bus *bus_if;
            
            virtual void _write8(uint32_t addr, uint8_t value)
            {
            
            }
            
            virtual void _write16(uint32_t addr, uint16_t value)
            {
            
            }
            
            virtual void _write32(uint32_t addr, uint32_t value)
            {
            
            }
            
            virtual uint8_t _read8(uint32_t addr)
            {
                return 0;
            }
            
            virtual uint16_t _read16(uint32_t addr)
            {
                return 0;
            }
            
            virtual uint32_t _read32(uint32_t addr)
            {
                return 0;
            }
            
            virtual void _init()
            {
            
            }
            
            void init(uint32_t size)
            {
                this -> size = size;
                _init();
            }
            
            virtual void _reset()
            {
            
            }
        
        public:
            slave_base(component::bus *bus_if)
            {
                this->bus_if = bus_if;
                size = 0;
                this->slave_base::reset();
            }
            
            virtual ~slave_base()
            {
            
            }
            
            virtual void reset()
            {
                _reset();
            }
            
            static bool check_align(uint32_t addr, uint32_t access_size)
            {
                return !(addr & (access_size - 1));
            }
            
            bool check_boundary(uint32_t addr, uint32_t access_size) const
            {
                return (addr < size) && ((size - addr) >= access_size);
            }
            
            void write8(uint32_t addr, uint8_t value)
            {
                if(check(addr, 1))
                {
                    _write8(addr, value);
                }
            }
            
            void write16(uint32_t addr, uint16_t value)
            {
                if(check(addr, 2))
                {
                    _write16(addr, value);
                }
            }
            
            void write32(uint32_t addr, uint32_t value)
            {
                if(check(addr, 4))
                {
                    _write32(addr, value);
                }
            }
            
            uint8_t read8(uint32_t addr)
            {
                if(check(addr, 1))
                {
                    return _read8(addr);
                }
                
                return 0;
            }
            
            uint16_t read16(uint32_t addr)
            {
                if(check(addr, 2))
                {
                    return _read16(addr);
                }
                
                return 0;
            }
            
            uint32_t read32(uint32_t addr)
            {
                if(check(addr, 4))
                {
                    return _read32(addr);
                }
                
                return 0;
            }
    };
}