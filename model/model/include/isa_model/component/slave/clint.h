/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-18     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../slave_base.h"
#include "../bus.h"
#include "../interrupt_interface.h"

namespace isa_model::component::slave
{
    class clint : public slave_base
    {
        private:
            bool msip = false;
            uint64_t mtimecmp = 0;
            uint64_t mtime = 0;
            bool mtime_changed = false;
            
            static const uint32_t MSIP_ADDR = 0x0;
            static const uint32_t MTIMECMP_ADDR = 0x4000;
            static const uint32_t MTIME_ADDR = 0xBFF8;
            
            component::interrupt_interface *interrupt_interface;
            
            virtual void _reset()
            {
                msip = false;
                mtimecmp = 0;
                mtime = 0;
                mtime_changed = false;
            }
        
        public:
            clint(component::bus *bus_if, component::interrupt_interface *interrupt_interface) : slave_base(bus_if)
            {
                this->interrupt_interface = interrupt_interface;
                this->clint::_reset();
            }
            
            virtual void _write8(uint32_t addr, uint8_t value)
            {
            
            }
            
            virtual void _write16(uint32_t addr, uint16_t value)
            {
            
            }
            
            virtual void _write32(uint32_t addr, uint32_t value)
            {
                switch(addr)
                {
                    case MSIP_ADDR:
                        msip = (value & 0x01) != 0;
                        break;
                    
                    case MTIMECMP_ADDR:
                        mtimecmp = (mtimecmp & 0xFFFFFFFF00000000ULL) | value;
                        break;
                    
                    case MTIMECMP_ADDR + 4:
                        mtimecmp = (mtimecmp & 0xFFFFFFFFULL) | (((uint64_t)value) << 32);
                        break;
                    
                    case MTIME_ADDR:
                        mtime = (mtime & 0xFFFFFFFF00000000ULL) | value;
                        break;
                    
                    case MTIME_ADDR + 4:
                        mtime = (mtime & 0xFFFFFFFFULL) | (((uint64_t)value) << 32);
                        break;
                    
                    default:
                        break;
                }
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
                uint32_t value = 0;
                
                switch(addr)
                {
                    case MSIP_ADDR:
                        value = msip ? 1 : 0;
                        break;
                    
                    case MTIMECMP_ADDR:
                        value = (uint32_t)(mtimecmp & 0xFFFFFFFFULL);
                        break;
                    
                    case MTIMECMP_ADDR + 4:
                        value = (uint32_t)(mtimecmp >> 32);
                        break;
                    
                    case MTIME_ADDR:
                        value = (uint32_t)(mtime & 0xFFFFFFFFULL);
                        break;
                    
                    case MTIME_ADDR + 4:
                        value = (uint32_t)(mtime >> 32);
                        break;
                    
                    default:
                        value = 0;
                        break;
                }
                
                return value;
            }
            
            void run_pre()
            {
                interrupt_interface->set_pending(riscv_interrupt_t::machine_timer, mtime >= mtimecmp);
                interrupt_interface->set_pending(riscv_interrupt_t::machine_software, msip);
            }
            
            void run_post()
            {
                if(!mtime_changed)
                {
                    mtime++;
                    mtime_changed = false;
                }
            }
    };
}