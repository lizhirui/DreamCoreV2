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
#include "../dff.h"
#include "../slave_base.h"
#include "../bus.h"
#include "../interrupt_interface.h"

namespace cycle_model::component::slave
{
    class clint : public slave_base
    {
        private:
            dff<bool> msip;
            dff<uint64_t> mtimecmp;
            dff<uint64_t> mtime;
            
            static const uint32_t MSIP_ADDR = 0x0;
            static const uint32_t MTIMECMP_ADDR = 0x4000;
            static const uint32_t MTIME_ADDR = 0xBFF8;
            
            component::interrupt_interface *interrupt_interface;
    
            trace::trace_database tdb;
    
            virtual void _reset()
            {
                msip.set(false);
                mtimecmp.set(0);
                mtime.set(0);
            }
            
        public:
            clint(component::bus *bus_if, component::interrupt_interface *interrupt_interface) : slave_base(bus_if), msip(false), mtimecmp(0), mtime(0), tdb(TRACE_CLINT)
            {
                this->interrupt_interface = interrupt_interface;
                this->clint::_reset();
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
    
            virtual void _write8(uint32_t addr, uint8_t value)
            {
                bus_if->set_data_write_ack(bus_errno_t::error);
            }
    
            virtual void _write16(uint32_t addr, uint16_t value)
            {
                bus_if->set_data_write_ack(bus_errno_t::error);
            }
    
            virtual void _write32(uint32_t addr, uint32_t value)
            {
                switch(addr)
                {
                    case MSIP_ADDR:
                        msip.set((value & 0x01) != 0);
                        bus_if->set_data_write_ack(bus_errno_t::none);
                        break;
            
                    case MTIMECMP_ADDR:
                        mtimecmp.set((mtimecmp.get() & 0xFFFFFFFF00000000ULL) | value);
                        bus_if->set_data_write_ack(bus_errno_t::none);
                        break;
            
                    case MTIMECMP_ADDR + 4:
                        mtimecmp.set((mtimecmp.get() & 0xFFFFFFFFULL) | (((uint64_t)value) << 32));
                        bus_if->set_data_write_ack(bus_errno_t::none);
                        break;
            
                    case MTIME_ADDR:
                        mtime.set((mtime.get() & 0xFFFFFFFF00000000ULL) | value);
                        bus_if->set_data_write_ack(bus_errno_t::none);
                        break;
            
                    case MTIME_ADDR + 4:
                        mtime.set((mtime.get() & 0xFFFFFFFFULL) | (((uint64_t)value) << 32));
                        bus_if->set_data_write_ack(bus_errno_t::none);
                        break;
                        
                    default:
                        bus_if->set_data_write_ack(bus_errno_t::error);
                        break;
                }
            }
    
            virtual void _read8(uint32_t addr)
            {
            
            }
    
            virtual void _read16(uint32_t addr)
            {
            
            }
    
            virtual void _read32(uint32_t addr)
            {
                uint32_t value = 0;
                
                switch(addr)
                {
                    case MSIP_ADDR:
                        value = msip.get() ? 1 : 0;
                        break;
            
                    case MTIMECMP_ADDR:
                        value = (uint32_t)(mtimecmp.get() & 0xFFFFFFFFULL);
                        break;
            
                    case MTIMECMP_ADDR + 4:
                        value = (uint32_t)(mtimecmp.get() >> 32);
                        break;
            
                    case MTIME_ADDR:
                        value = (uint32_t)(mtime.get() & 0xFFFFFFFFULL);
                        break;
            
                    case MTIME_ADDR + 4:
                        value = (uint32_t)(mtime.get() >> 32);
                        break;
                        
                    default:
                        value = 0;
                        break;
                }

                bus_if->set_data_value(value);
            }
            
            uint64_t get_mtime() const
            {
                return mtime.get();
            }
            
            uint64_t get_mtimecmp() const
            {
                return mtimecmp.get();
            }
            
            uint32_t get_msip() const
            {
                return msip.get();
            }
            
            void run_pre()
            {
                interrupt_interface->set_pending(riscv_interrupt_t::machine_timer, mtime >= mtimecmp);
                interrupt_interface->set_pending(riscv_interrupt_t::machine_software, msip);
            }
    
            void run_post()
            {
                if(!mtime.is_changed())
                {
                    mtime.set(mtime.get() + 1);
                }
            }
    };
}