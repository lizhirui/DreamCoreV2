/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-13     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "branch_predictor/l0_btb.h"
#include "branch_predictor/l1_btb.h"
#include "branch_predictor/bi_modal.h"
#include "branch_predictor/bi_mode.h"
#include "ras.h"

namespace cycle_model::component
{
    class branch_predictor_set
    {
        public:
            branch_predictor::l0_btb l0_btb;
            branch_predictor::l1_btb l1_btb;
            branch_predictor::bi_modal bi_modal;
            branch_predictor::bi_mode bi_mode;
            component::ras main_ras;
            
            branch_predictor_set() : main_ras(RAS_SIZE)
            {
            
            }
    };
}