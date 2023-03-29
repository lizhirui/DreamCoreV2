/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-16     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"

namespace cycle_model::pipeline
{
    typedef struct execute_feedback_channel_t : if_print_t
    {
        bool enable = false;
        uint32_t phy_id = 0;
        uint32_t value = 0;
        uint32_t rob_id = 0;
        bool rob_id_stage = false;
        
        virtual json get_json()
        {
            json t;
            t["enable"] = enable;
            t["phy_id"] = phy_id;
            t["value"] = value;
            return t;
        }
    }execute_feedback_channel_t;
    
    typedef struct execute_feedback_pack_t : if_print_t
    {
        execute_feedback_channel_t channel[FEEDBACK_EXECUTE_UNIT_NUM];
        
        virtual json get_json()
        {
            json t = json::array();
            
            for(uint32_t i = 0;i < FEEDBACK_EXECUTE_UNIT_NUM;i++)
            {
                t.push_back(channel[i].get_json());
            }
            
            return t;
        }
    }execute_feedback_pack_t;
}