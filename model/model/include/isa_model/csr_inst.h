/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-26     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "isa_model/component/csrfile.h"
#include "isa_model/component/csr_all.h"

namespace isa_model
{
    class csr_inst
    {
        public:
            static component::csr::mstatus mstatus;
            static component::csr::mie mie;
            static component::csr::mtvec mtvec;
            static component::csr::mepc mepc;
            static component::csr::mcause mcause;
            static component::csr::mtval mtval;
            static component::csr::mip mip;
            static component::csr::finish finish;
            static component::csr::mcycle mcycle;
            static component::csr::minstret minstret;
            static component::csr::mcycleh mcycleh;
            static component::csr::minstreth minstreth;
            static component::csr::mhpmcounter branchnum;
            static component::csr::mhpmcounterh branchnumh;
            static component::csr::mhpmcounter branchpredicted;
            static component::csr::mhpmcounterh branchpredictedh;
            static component::csr::mhpmcounter branchhit;
            static component::csr::mhpmcounterh branchhith;
            static component::csr::mhpmcounter branchmiss;
            static component::csr::mhpmcounterh branchmissh;
    };
}