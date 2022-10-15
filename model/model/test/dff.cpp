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
#include <component/dff.h>
#include "dff.h"

namespace test
{
    namespace dff
    {
        void test()
        {
            component::dff<uint32_t> u_dff(0);

            assert(u_dff.get() == 0);
            u_dff.set(1);
            assert(u_dff.get() == 0);
            assert(u_dff == 0);
            assert(u_dff.get_new() == 1);
            component::dff_base::sync_all();
            assert(u_dff.get() == 1);
            assert(u_dff == 1);
            assert(u_dff.get_new() == 1);
            u_dff.set(2);
            assert(u_dff.get() == 1);
            assert(u_dff == 1);
            assert(u_dff.get_new() == 2);
            component::dff_base::sync_all();
            assert(u_dff.get() == 2);
            assert(u_dff == 2);
            assert(u_dff.get_new() == 2);
        }
    }
}