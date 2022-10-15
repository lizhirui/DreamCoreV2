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
#include "csrfile.h"
#include "csr_all.h"

namespace component
{
    class interrupt_interface : public if_reset_t
    {
        private:
            bool meip = false;
            bool msip = false;
            bool mtip = false;
            dff<bool> mei_ack;
            dff<bool> msi_ack;
            dff<bool> mti_ack;
            
            csrfile *csr_file;
            
            trace::trace_database tdb;
        
        public:
            interrupt_interface(csrfile *csr_file) : mei_ack(false), msi_ack(false), mti_ack(false), tdb(TRACE_INTERRUPT_INTERFACE)
            {
                this->csr_file = csr_file;
            }
            
            virtual void reset()
            {
                mei_ack = false;
                msi_ack = false;
                mti_ack = false;
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
            
            bool get_cause(riscv_interrupt_t *cause)
            {
                csr::mie mie;
                csr::mstatus mstatus;
                mie.load(csr_file->read_sys(CSR_MIE));
                mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                
                if(!mstatus.get_mie())
                {
                    return false;
                }
                
                if(meip && mie.get_meie())
                {
                    *cause = riscv_interrupt_t::machine_external;
                }
                else if(msip && mie.get_msie())
                {
                    *cause = riscv_interrupt_t::machine_software;
                }
                else if(mtip && mie.get_mtie())
                {
                    *cause = riscv_interrupt_t::machine_timer;
                }
                else
                {
                    return false;
                }
                
                return true;
            }
            
            bool has_interrupt()
            {
                csr::mie mie;
                csr::mstatus mstatus;
                mie.load(csr_file->read_sys(CSR_MIE));
                mstatus.load(csr_file->read_sys(CSR_MSTATUS));
                return mstatus.get_mie() && ((meip && mie.get_meie()) || (msip && mie.get_msie()) || (mtip && mie.get_mtie()));
            }
            
            void set_pending(riscv_interrupt_t cause, bool pending)
            {
                switch(cause)
                {
                    case riscv_interrupt_t::machine_external:
                        meip = pending;
                        break;
                    
                    case riscv_interrupt_t::machine_software:
                        msip = pending;
                        break;
                    
                    case riscv_interrupt_t::machine_timer:
                        mtip = pending;
                        break;
                        
                    default:
                        assert(0);
                        break;
                }
            }
            
            void set_ack(riscv_interrupt_t cause)
            {
                switch(cause)
                {
                    case riscv_interrupt_t::machine_external:
                        mei_ack.set(true);
                        break;
                    
                    case riscv_interrupt_t::machine_software:
                        msi_ack.set(true);
                        break;
                    
                    case riscv_interrupt_t::machine_timer:
                        mti_ack.set(true);
                        break;
                        
                    default:
                        assert(false);
                        break;
                }
            }
            
            bool get_ack(riscv_interrupt_t cause)
            {
                switch(cause)
                {
                    case riscv_interrupt_t::machine_external:
                        return mei_ack;
                    
                    case riscv_interrupt_t::machine_software:
                        return msi_ack;
                    
                    case riscv_interrupt_t::machine_timer:
                        return mti_ack;
                    
                    default:
                        return false;
                }
            }
            
            void run()
            {
                mei_ack.set(false);
                msi_ack.set(false);
                mti_ack.set(false);
                csr::mip mip;
                mip.load(csr_file->read_sys(CSR_MIP));
                mip.set_meip(meip);
                mip.set_msip(msip);
                mip.set_mtip(mtip);
                csr_file->write_sys(CSR_MIP, mip.get_value());
            }
    };
}