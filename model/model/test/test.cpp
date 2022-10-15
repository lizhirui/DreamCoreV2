/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#include <common.h>
#include <test.h>
#include "dff.h"

namespace test
{
    void test()
    {
        dff::test();
        std::cout << "Self-Test Passed" << std::endl;
    }
}