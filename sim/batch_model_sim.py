#!/bin/python3
#****************************************************************************************
# Copyright lizhirui
#
# SPDX-License-Identifier: Apache-2.0
#
# Change Logs:
# Date           Author       Notes
# 2022-07-20     lizhirui     the first version
# 2022-07-21     lizhirui     add time show support
# 2022-08-24     lizhirui     fixed exit bug and add xcelium support and warning check
#****************************************************************************************
from concurrent.futures import thread
from pathlib import Path as path
import sys
import os
import subprocess
import threading
import math
import argparse
from multiprocessing import cpu_count
import time

def shell(cmd):
    p = subprocess.Popen(cmd, shell = False, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    out = p.stdout.read().decode()

    while p.poll() is None:
        time.sleep(0.5)

    return [p.returncode, out]
    

def file_key(elem):
    return elem[0] + "_" + elem[1]

def error(text):
    print("\033[0;31mError: " + text + "\033[0m")
    exit(1)

def warning(text):
    print("\033[0;34mWarning: " + text + "\033[0m")

def warning_or_error(is_warning, text):
    if is_warning:
        warning(text)
    else:
        error(text)

def red_text(text):
    print("\033[0;31m" + text + "\033[0m")

def green_text(text, end="\n"):
    print("\033[0;32m" + text + "\033[0m", end=end)

def yellow_text(text):
    print("\033[0;33m" + text + "\033[0m")

def to_human_friendly_time(t):
    t = int(t)

    if t < 60:
        return str(t) + "s"
    elif t < 3600:
        return str(t // 60) + "m" + str(t % 60) + "s"
    else:
        return str(t // 3600) + "h" + str(t % 3600 // 60) + "m" + str(t % 60) + "s"

print_lock = threading.Lock()
passed_cnt = 0
failed_cnt = 0
unknown_error_cnt = 0

class dynamic_check_thread(threading.Thread):
    def __init__(self, thread_id, local_task_list):
        threading.Thread.__init__(self)
        self.thread_id = thread_id
        self.local_task_list = local_task_list

    def run(self):
        global passed_cnt, failed_cnt, unknown_error_cnt
        cnt = 0

        for item in self.local_task_list:
            cnt = cnt + 1
            
            start_time = time.time()

            if ".bin" in item[2]:
                exe_type = "bin"
            else:
                exe_type = "elf"

            [exit_code, out] = shell(["../model/model/bin/sim", "--nocontroller", "--notelnet", "--load" + exe_type, os.path.join(item[0], item[1], item[2]), "--breakpoint cycle:" + str(item[3])])

            end_time = time.time()
            elapsed_time = to_human_friendly_time(end_time - start_time)
            case_name = item[1] + ", " + item[2]

            if exit_code == 4:
                print_lock.acquire()
                green_text("[" + str(self.thread_id + 1) + "-" + str(cnt) + "/" + str(len(self.local_task_list)) + ", " + case_name + "]: Test Passed " + elapsed_time)
                passed_cnt += 1
                print_lock.release()
            elif exit_code == 5:
                print_lock.acquire()
                red_text("[" + str(self.thread_id + 1) + "-" + str(cnt) + "/" + str(len(self.local_task_list)) + ", " + case_name + "]: Test Failed " + elapsed_time)
                #print(ret)
                #os._exit(0)
                failed_cnt += 1
                print_lock.release()
            else:
                print_lock.acquire()
                yellow_text("[" + str(self.thread_id + 1) + "-" + str(cnt) + "/" + str(len(self.local_task_list)) + ", " + case_name + "]: Unknown Error[" + str(exit_code) + "] " + elapsed_time)
                print(out)
                #os._exit(0)
                unknown_error_cnt += 1
                print_lock.release()

total_start_time = time.time()

testcase_dir = "../testcase"
parallel_count = 1

parser = argparse.ArgumentParser()
parser.add_argument("-g", "--group", help="test group", choices=["riscv-tests", "benchmark", "all"], required=True)
parser.add_argument("-j", help="parallel count limit, default is 1", nargs="?", type=int, const=0, choices=range(0, cpu_count() + 1), dest="parallel_count")
parser.add_argument("-t", help="testcase name, default is all testcases", type=str, dest="testcase_name")
args = parser.parse_args()

if not args.parallel_count is None:
    if args.parallel_count == 0:
        parallel_count = cpu_count()
    else:
        parallel_count = args.parallel_count

tb_groups = []

if args.group == "all":
    tb_groups.append("base-tests")
    tb_groups.append("riscv-tests")
    tb_groups.append("benchmark")
else:
    tb_groups.append(args.group)

print("Start Time: " + time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))

print("Start to scan testcases...")

original_testcase_list = []
group_testcase_list = {}
testcase_list = []

for group in tb_groups:
    group_root = path(os.path.join(testcase_dir, group))
    group_testcase_list[group] = []
    
    for dir_item in group_root.iterdir():
        if not dir_item.is_dir():
            if dir_item.name == args.testcase_name or args.testcase_name is None:
                if not ".dump" in dir_item.name:
                    original_testcase_list.append([group, dir_item.name])
                    group_testcase_list[group].append(dir_item.name)

original_testcase_list.sort(key = file_key)

for item in original_testcase_list:
    testcase_list.append([testcase_dir, item[0], item[1], 20000000])

print(str(len(original_testcase_list)) + " testcases are found: ")

for group in tb_groups:
    green_text(group + ":", end="")

    for testcase in group_testcase_list[group]:
        print(" " + testcase, end="")

    print()

print(str(len(testcase_list)) + " extended testcases are found!")

if len(testcase_list) == 0:
    error("No any testcase needs to test!")

print("Start to assign tasks...")
actual_parallel_count = min(parallel_count, len(testcase_list))

if actual_parallel_count < parallel_count:
    warning("You request " + str(parallel_count) + " threads, but only " + str(len(testcase_list)) + " testcases need to be tested!")

total_task_num = len(testcase_list)

thread_task_list = [[] for i in range(0, parallel_count)]
thread_list = []

assigned_task_num = 0
cur_thread_id = 0

for i in range(0, total_task_num):
    thread_task_list[cur_thread_id].append(testcase_list[i])
    assigned_task_num += 1
    cur_thread_id = (cur_thread_id + 1) % parallel_count

print("The count of total assigned tasks is " + str(total_task_num) + " , the count of actual assigned tasks is " + str(assigned_task_num))

print("Starting threads...")
print_lock.acquire()

for i in range(0, actual_parallel_count):
    task_thread = dynamic_check_thread(i, thread_task_list[i])
    thread_list.append(task_thread)
    task_thread.start()

print("Waiting threads finish...")
print_lock.release()

for task_thread in thread_list:
    task_thread.join()

print("All thread are finished, " + str(passed_cnt) + " passed, " + str(failed_cnt) + " failed, " + str(unknown_error_cnt) + " unknown error, " + str(total_task_num - passed_cnt - failed_cnt - unknown_error_cnt) + " don't have any results!")

if passed_cnt < total_task_num:
    error("Some testcases are failed")
else:
    green_text("All testcases are passed!")

total_end_time = time.time()
total_elapsed = to_human_friendly_time(total_end_time - total_start_time)
print(total_elapsed)