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

namespace cycle_model::component
{
    class dff_base
    {
        private:
            static std::unordered_set<dff_base *> dff_list;

        protected:
            virtual void sync() = 0;
            virtual void reset() = 0;

        public:
            dff_base()
            {
                dff_list.insert(this);
            }

            ~dff_base()
            {
                dff_list.erase(this);
            }

            static void sync_all()
            {
                for(auto dff : dff_list)
                {
                    dff->sync();
                }
            }
            
            static void reset_all()
            {
                for(auto dff : dff_list)
                {
                    dff->reset();
                }
            }
    };

    template<typename T>
    class dff : dff_base
    {
        private:
            T init_value;
            T cur_value;
            T new_value;

        protected:
            virtual void sync()
            {
                this->cur_value = this->new_value;
            }
        
            virtual void reset()
            {
                this->cur_value = this->init_value;
                this->new_value = this->init_value;
            }

        public:
            dff()
            {
                this->init_value = T();
                this->cur_value = T();
                this->new_value = T();
            }

            dff(T init_value)
            {
                this->init_value = init_value;
                this->cur_value = init_value;
                this->new_value = init_value;
            }

            dff(const dff<T> &other)
            {
                this->cur_value = other.cur_value;
                this->new_value = other.cur_value;
            }

            void set(T new_value)
            {
                this->new_value = new_value;
            }

            T get()
            {
                return this->cur_value;
            }

            T get_new()
            {
                return this->new_value;
            }
            
            void revert()
            {
                this->new_value = this->cur_value;
            }
            
            bool is_changed()
            {
                return this->cur_value != this->new_value;
            }

            operator T()
            {
                return this->cur_value;
            }
    };
}