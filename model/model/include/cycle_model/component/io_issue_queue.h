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
#include "fifo.h"

namespace component
{
    template<typename T>
    class io_issue_queue : public fifo<T>
    {
        public:
            io_issue_queue(uint32_t size) : fifo<T>(size)
            {
            
            }
    };
}