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

typedef std::string (*socket_cmd_handler)(std::vector<std::string> args);
void register_socket_cmd(std::string name, socket_cmd_handler handler);
void network_command_init();
