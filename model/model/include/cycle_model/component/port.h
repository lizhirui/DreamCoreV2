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
#include "dff.h"

namespace cycle_model::component
{
    template<typename T>
    class port : public if_print_t, public if_reset_t
    {
        private:
            dff<T> value;
            T init_value;

        public:
            port(T init_value) : value(init_value)
            {
                this->init_value = init_value;
            }

            virtual void reset()
            {
                value.set(init_value);
            }

            void set(T value)
            {
                this->value.set(value);
            }

            T get()
            {
                return this->value.get();
            }

            virtual void print(std::string indent)
            {
                auto ret = this->value.get();
                auto if_print = dynamic_cast<if_print_t *>(&ret);
                if_print->print(indent);
            }

            virtual json get_json()
            {
                auto ret = this->value.get();
                auto if_print = dynamic_cast<if_print_t *>(&ret);
                return if_print->get_json();
            }
    };
}