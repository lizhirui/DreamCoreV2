/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-19     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "fifo.h"
#include "dff.h"

namespace cycle_model::component
{
    typedef struct checkpoint_t : public if_print_t
    {
        bool phy_map_table_valid[PHY_REG_NUM];
        uint32_t new_phy_id_free_list_rptr = 0;
        bool new_phy_id_free_list_rstage = false;
        uint32_t new_checkpoint_buffer_wptr = 0;
        bool new_checkpoint_buffer_wstage = false;
        
        void clone_to(checkpoint_t &cp)
        {
            memcpy(cp.phy_map_table_valid, phy_map_table_valid, sizeof(phy_map_table_valid));
            cp.new_phy_id_free_list_rptr = new_phy_id_free_list_rptr;
            cp.new_phy_id_free_list_rstage = new_phy_id_free_list_rstage;
        }
    }checkpoint_t;
}