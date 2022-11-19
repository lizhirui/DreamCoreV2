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

namespace cycle_model::component
{
    class age_compare
    {
        public:
            uint32_t rob_id = 0;
            bool rob_id_stage = false;
            
            age_compare(uint32_t rob_id, bool rob_id_stage)
            {
                this->rob_id = rob_id;
                this->rob_id_stage = rob_id_stage;
            }
            
            bool operator < (const age_compare &other) const
            {
                if(this->rob_id_stage == other.rob_id_stage)
                {
                    return this->rob_id > other.rob_id;
                }
                else
                {
                    return this->rob_id < other.rob_id;
                }
            }
            
            bool operator == (const age_compare &other) const
            {
                return this->rob_id == other.rob_id && this->rob_id_stage == other.rob_id_stage;
            }
            
            bool operator != (const age_compare &other) const
            {
                return this->rob_id != other.rob_id || this->rob_id_stage != other.rob_id_stage;
            }
            
            bool operator > (const age_compare &other) const
            {
                if(this->rob_id_stage == other.rob_id_stage)
                {
                    return this->rob_id < other.rob_id;
                }
                else
                {
                    return this->rob_id > other.rob_id;
                }
            }
            
            bool operator <= (const age_compare &other) const
            {
                return (*this < other) || (*this == other);
            }
            
            bool operator >= (const age_compare &other) const
            {
                return (*this > other) || (*this == other);
            }
    };
}