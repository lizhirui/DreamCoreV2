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

namespace cycle_model::component
{
    typedef struct branch_predictor_info_pack_t : if_print_t
    {
        uint32_t global_history = 0;
        bool predicted = false;
        bool jump = false;
        uint32_t next_pc = 0;
    }branch_predictor_info_pack_t;
    
    class branch_predictor_base
    {
        private:
            static std::unordered_set<branch_predictor_base *> bp_list;
            
        public:
            branch_predictor_base()
            {
                bp_list.insert(this);
            }
            
            ~branch_predictor_base()
            {
                bp_list.erase(this);
            }
            
            static void foreach(std::function<void(branch_predictor_base *)> func)
            {
                for(auto bp : bp_list)
                {
                    func(bp);
                }
            }
            
            static void batch_reset()
            {
                branch_predictor_base::foreach([](branch_predictor_base *bp){bp->reset();});
            }
            
            static void batch_update(uint32_t pc, bool jump, bool hit)
            {
                branch_predictor_base::foreach([pc, jump, hit](branch_predictor_base *bp){bp->update(pc, jump,hit);});
            }
            
            static void batch_speculative_update(uint32_t pc, bool jump)
            {
                branch_predictor_base::foreach([pc, jump](branch_predictor_base *bp){bp->speculative_update(pc, jump);});
            }
            
            static void batch_predict(uint32_t port, uint32_t pc)
            {
                branch_predictor_base::foreach([port, pc](branch_predictor_base *bp){bp->predict(port, pc);});
            }
            
            virtual void reset() = 0;
            virtual void update(uint32_t pc, bool jump, bool hit) = 0;
            virtual void speculative_update(uint32_t pc, bool jump) = 0;
            virtual void predict(uint32_t port, uint32_t pc) = 0;
            virtual uint32_t get_next_pc(uint32_t port) = 0;
            virtual bool is_jump(uint32_t port) = 0;
            virtual void fill_info_pack(branch_predictor_info_pack_t &pack) = 0;
    };
}