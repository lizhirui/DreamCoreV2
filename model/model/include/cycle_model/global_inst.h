/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-15     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"

namespace cycle_model
{
    class global_inst
    {
        public:
            uint64_t branch_num = 0;
            uint64_t branch_predicted = 0;
            uint64_t branch_hit = 0;
            uint64_t branch_miss = 0;
            uint64_t load_num = 0;
            uint64_t replay_num = 0;
            uint64_t replay_load_num = 0;
            uint64_t conflict_load_num = 0;
            
            void branch_num_add()
            {
                branch_num++;
            }
            
            void branch_predicted_add()
            {
                branch_predicted++;
            }
            
            void branch_hit_add()
            {
                branch_hit++;
            }
            
            void branch_miss_add()
            {
                branch_miss++;
            }
            
            void load_num_add()
            {
                load_num++;
            }
            
            void replay_num_add(uint32_t incr)
            {
                replay_num += incr;
            }
            
            void replay_load_num_add()
            {
                replay_load_num++;
            }
            
            void conflict_load_num_add()
            {
                conflict_load_num++;
            }
    };
}