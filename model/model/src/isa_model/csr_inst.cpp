/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-26     lizhirui     the first version
 */
#include "common.h"
#include "config.h"
#include "isa_model/component/csrfile.h"
#include "isa_model/component/csr_all.h"
#include "isa_model/csr_inst.h"

namespace isa_model
{
    component::csr::mstatus csr_inst::mstatus;
    component::csr::mie csr_inst::mie;
    component::csr::mtvec csr_inst::mtvec;
    component::csr::mepc csr_inst::mepc;
    component::csr::mcause csr_inst::mcause;
    component::csr::mtval csr_inst::mtval;
    component::csr::mip csr_inst::mip;
    component::csr::finish csr_inst::finish;
    component::csr::mcycle csr_inst::mcycle;
    component::csr::minstret csr_inst::minstret;
    component::csr::mcycleh csr_inst::mcycleh;
    component::csr::minstreth csr_inst::minstreth;
    component::csr::mhpmcounter csr_inst::branchnum("branchnum");
    component::csr::mhpmcounterh csr_inst::branchnumh("branchnumh");
    component::csr::mhpmcounter csr_inst::branchpredicted("branchpredicted");
    component::csr::mhpmcounterh csr_inst::branchpredictedh("branchpredictedh");
    component::csr::mhpmcounter csr_inst::branchhit("branchhit");
    component::csr::mhpmcounterh csr_inst::branchhith("branchhith");
    component::csr::mhpmcounter csr_inst::branchmiss("branchmiss");
    component::csr::mhpmcounterh csr_inst::branchmissh("branchmissh");
}