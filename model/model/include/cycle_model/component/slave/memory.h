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
            uint32_t data_read_latency = 0;
            uint32_t *read_data_value;
            bool *read_data_valid;
            uint32_t data_write_latency = 0;
            uint32_t *write_data_addr;
            uint32_t *write_data_value;
            uint32_t *write_data_size;
            bool *write_data_valid;
            
            trace::trace_database tdb;
            
            virtual void _init()
            {
                mem = new uint8_t[this->size]();
                memset(mem, 0, this->size);
            }
            
            virtual void _reset()
            {
                memset(mem, 0, this->size);
                memset(read_data_valid, 0, data_read_latency + 1);
                memset(write_data_valid, 0, data_read_latency + 1);
            }
        
        public:
            memory(component::bus *bus_if, uint32_t data_read_latency, uint32_t data_write_latency) : slave_base(bus_if), tdb(TRACE_TCM)
            {
                this->data_read_latency = data_read_latency;
                this->data_write_latency = data_write_latency;
                read_data_value = new uint32_t[data_read_latency + 1];
                read_data_valid = new bool[data_read_latency + 1];
                write_data_addr = new uint32_t[data_write_latency + 1];
                write_data_size = new uint32_t[data_write_latency + 1];
                write_data_value = new uint32_t[data_write_latency + 1];
                write_data_valid = new bool[data_write_latency + 1];
                this->memory::_reset();
            }
            
            ~memory()
            {
                delete[] mem;
                delete[] read_data_value;
                delete[] read_data_valid;
                delete[] write_data_addr;
                delete[] write_data_size;
                delete[] write_data_value;
                delete[] write_data_valid;
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
                write_data_valid[0] = true;
                write_data_addr[0] = addr;
                write_data_size[0] = 1;
                write_data_value[0] = value;
            }
            
            virtual void _write16(uint32_t addr, uint16_t value)
            {
                write_data_valid[0] = true;
                write_data_addr[0] = addr;
                write_data_size[0] = 2;
                write_data_value[0] = value;
            }
            
            virtual void _write32(uint32_t addr, uint32_t value)
            {
                write_data_valid[0] = true;
                write_data_addr[0] = addr;
                write_data_size[0] = 4;
                write_data_value[0] = value;
            }
            
            virtual void _write8_sys(uint32_t addr, uint8_t value)
            {
                mem[addr] = value;
            }
            
            virtual void _write16_sys(uint32_t addr, uint16_t value)
            {
                *(uint16_t *)(mem + addr) = value;
            }
            
            virtual void _write32_sys(uint32_t addr, uint32_t value)
            {
                *(uint32_t *)(mem + addr) = value;
            }
            
            virtual void _read8(uint32_t addr)
            {
                //bus_if->set_data_value(mem[addr]);
                read_data_valid[0] = true;
                read_data_value[0] = mem[addr];
            }
            
            virtual void _read16(uint32_t addr)
            {
                //bus_if->set_data_value(*(uint16_t *)(mem + addr));
                read_data_valid[0] = true;
                read_data_value[0] = *(uint16_t *)(mem + addr);
            }
            
            virtual void _read32(uint32_t addr)
            {
                //bus_if->set_data_value(*(uint32_t *)(mem + addr));
                read_data_valid[0] = true;
                read_data_value[0] = *(uint32_t *)(mem + addr);
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
            
            void run_pre()
            {
            
            }
            
            void run_post()
            {
                if(read_data_valid[data_read_latency])
                {
                    bus_if->set_data_value(read_data_value[data_read_latency]);
                }
                
                if(data_read_latency > 0)
                {
                    for(uint32_t i = data_read_latency;i >= 1;i--)
                    {
                        read_data_value[i] = read_data_value[i - 1];
                        read_data_valid[i] = read_data_valid[i - 1];
                    }
                }
    
                read_data_valid[0] = false;
                
                if(write_data_valid[data_write_latency])
                {
                    switch(write_data_size[data_write_latency])
                    {
                        case 1:
                            mem[write_data_addr[data_write_latency]] = write_data_value[data_write_latency];
                            bus_if->set_data_write_ack(bus_errno_t::none);
                            break;
                            
                        case 2:
                            *(uint16_t *)(mem + write_data_addr[data_write_latency]) = write_data_value[data_write_latency];
                            bus_if->set_data_write_ack(bus_errno_t::none);
                            break;
                            
                        case 4:
                            *(uint32_t *)(mem + write_data_addr[data_write_latency]) = write_data_value[data_write_latency];
                            bus_if->set_data_write_ack(bus_errno_t::none);
                            break;
                            
                        default:
                            verify_only(false);
                            break;
                    }
                }
                
                if(data_write_latency > 0)
                {
                    for(uint32_t i = data_write_latency;i >= 1;i--)
                    {
                        write_data_addr[i] = write_data_addr[i - 1];
                        write_data_size[i] = write_data_size[i - 1];
                        write_data_value[i] = write_data_value[i - 1];
                        write_data_valid[i] = write_data_valid[i - 1];
                    }
                }
                
                write_data_valid[0] = false;
            }
    };
}