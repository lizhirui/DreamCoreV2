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
#include "csrfile.h"
#include "csr_all.h"

namespace isa_model::component
{
    class interrupt_interface : public if_reset_t
    {
        private:
            bool meip = false;
            bool msip = false;
            bool mtip = false;
            
            csrfile *csr_file;
        
        public:
            interrupt_interface(csrfile *csr_file)
            {
                this->csr_file = csr_file;
                this->interrupt_interface::reset();
            }
            
            virtual void reset()
            {
            
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
                        verify_only(0);
                        break;
                }
            }
            
            void run()
            {
                csr::mip mip;
                mip.load(csr_file->read_sys(CSR_MIP));
                mip.set_meip(meip);
                mip.set_msip(msip);
                mip.set_mtip(mtip);
                csr_file->write_sys(CSR_MIP, mip.get_value());
            }
    };
}