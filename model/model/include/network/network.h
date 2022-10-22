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

void set_recv_thread_stop(bool value);
bool get_recv_thread_stopped();
void set_program_stop(bool value);
bool get_program_stop();
bool get_server_thread_stopped();
bool get_charfifo_thread_stopped();
charfifo_send_fifo_t *get_charfifo_send_fifo();
charfifo_rev_fifo_t *get_charfifo_rev_fifo();
void send_cmd(std::string prefix, std::string cmd, std::string arg);
void debug_event_handle();
void network_init();
