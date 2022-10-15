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

namespace component
{
    namespace slave
    {
        class memory : public slave_base
        {
            private:
                uint8_t *mem;
                
                trace::trace_database tdb;
                
                virtual void _init()
                {
                    mem = new uint8_t[this->size]();
                    memset(mem, 0, this->size);
                }
                
                virtual void _reset()
                {
                
                }
            
            public:
                memory(component::bus *bus_if) : slave_base(bus_if), tdb(TRACE_TCM)
                {
                
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
                
                virtual void _read_instruction(uint32_t addr)
                {
                    bus_if->set_instruction_value((uint32_t *)(mem + addr));
                }
        };
    }
}