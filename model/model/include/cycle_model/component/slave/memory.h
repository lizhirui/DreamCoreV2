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

namespace cycle_model::component::slave
{
    class memory : public slave_base
    {
        private:
            uint8_t *mem = nullptr;
            
            trace::trace_database tdb;
            
            virtual void _init()
            {
                mem = new uint8_t[this->size]();
                memset(mem, 0, this->size);
            }
            
            virtual void _reset()
            {
                memset(mem, 0, this->size);
            }
        
        public:
            memory(component::bus *bus_if) : slave_base(bus_if), tdb(TRACE_TCM)
            {
                this->memory::_reset();
            }
            
            ~memory()
            {
                delete[] mem;
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
                mem[addr] = value;
            }
            
            virtual void _write16(uint32_t addr, uint16_t value)
            {
                *(uint16_t *)(mem + addr) = value;
            }
            
            virtual void _write32(uint32_t addr, uint32_t value)
            {
                *(uint32_t *)(mem + addr) = value;
            }
            
            virtual void _read8(uint32_t addr)
            {
                bus_if->set_data_value(mem[addr]);
            }
            
            virtual void _read16(uint32_t addr)
            {
                bus_if->set_data_value(*(uint16_t *)(mem + addr));
            }
            
            virtual void _read32(uint32_t addr)
            {
                bus_if->set_data_value(*(uint32_t *)(mem + addr));
            }
            
            virtual uint8_t _read8_sys(uint32_t addr)
            {
                return mem[addr];
            }
            
            virtual uint16_t _read16_sys(uint32_t addr)
            {
                return *(uint16_t *)(mem + addr);
            }
            
            virtual uint32_t _read32_sys(uint32_t addr)
            {
                return *(uint32_t *)(mem + addr);
            }
            
            virtual void _read_instruction(uint32_t addr)
            {
                bus_if->set_instruction_value((uint32_t *)(mem + addr));
            }
    };
}