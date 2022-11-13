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
#include "../branch_predictor_base.h"

namespace cycle_model::component::branch_predictor
{
    class bi_mode : public component::branch_predictor_base
    {
        public:
            virtual void reset()
            {
            
            }
            
            virtual void update(uint32_t pc, bool jump, bool hit)
            {
            
            }
            
            virtual void speculative_update(uint32_t pc, bool jump)
            {
            
            }
        
            virtual void predict(uint32_t port, uint32_t pc)
            {
            
            }
            
            virtual uint32_t get_next_pc(uint32_t port)
            {
                return 0;
            }
            
            virtual bool is_jump(uint32_t port)
            {
                return true;
            }
            
            virtual void fill_info_pack(branch_predictor_info_pack_t &pack)
            {
            
            }
    };
}