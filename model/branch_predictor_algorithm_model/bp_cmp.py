#!/usr/bin/python3
from tqdm import tqdm
import os

class Object(object):
    pass

dump_path_1 = "../../dump/branch_predictor_dump/coremark_10_7_2_no_checkpoint.txt"
dump_path_2 = "../../dump/branch_predictor_dump/coremark_10_7_2_checkpoint.txt"
log_path = "./log/bp_cmp"
global_history_length = 16

print("Reading dump file 1...")

with open(dump_path_1, "r") as f:
    dump_txt = f.read()

print("Parsing dump file 1...")
lines = dump_txt.split('\n')
bp_info_1 = []

for i in tqdm(range(0, len(lines)), colour="blue"):
    line = lines[i]
    arg = line.split(',')

    if line.strip() == "":
        continue

    if len(arg) != 8:
        raise Exception("branch predictor dump file error")

    obj = Object()
    obj.pc = int(arg[0], 16)
    obj.inst = int(arg[1], 16)
    obj.global_history_retired = int(arg[2], 16)
    obj.global_history = int(arg[3], 16)
    obj.pht_value = int(arg[4])
    obj.bru_jump = arg[5] == "true"
    obj.bru_next_pc = int(arg[6], 16)
    obj.hit = arg[7] == "true"
    bp_info_1.append(obj)

print("Reading dump file 2...")

with open(dump_path_2, "r") as f:
    dump_txt = f.read()

print("Parsing dump file 2...")
lines = dump_txt.split('\n')
bp_info_2 = []

for i in tqdm(range(0, len(lines)), colour="blue"):
    line = lines[i]
    arg = line.split(',')

    if line.strip() == "":
        continue

    if len(arg) != 8:
        raise Exception("branch predictor dump file error")

    obj = Object()
    obj.pc = int(arg[0], 16)
    obj.inst = int(arg[1], 16)
    obj.global_history_retired = int(arg[2], 16)
    obj.global_history = int(arg[3], 16)
    obj.pht_value = int(arg[4])
    obj.bru_jump = arg[5] == "true"
    obj.bru_next_pc = int(arg[6], 16)
    obj.hit = arg[7] == "true"
    bp_info_2.append(obj)

print("file 1 count: " + str(len(bp_info_1)))
print("file 2 count: " + str(len(bp_info_2)))

print("Comparing dump file 1 and 2...")

f1_hit_f2_miss = 0
f1_hit_f2_miss_list = []
f1_miss_f2_hit = 0
f1_miss_f2_hit_list = []
f1_history_error = 0
f2_history_error = 0
cmp_cnt = 0

for i in tqdm(range(0, min(len(bp_info_1), len(bp_info_2))), colour="blue"):
    obj_1 = bp_info_1[i]
    obj_2 = bp_info_2[i]

    if obj_1.pc != obj_2.pc:
        print("pc not equal")
        break

    if obj_1.inst != obj_2.inst:
        print("inst not equal")
        break

    if obj_1.bru_jump != obj_2.bru_jump:
        print("bru_jump not equal")
        break

    if obj_1.bru_next_pc != obj_2.bru_next_pc:
        print("bru_next_pc not equal")
        break

    if obj_1.global_history_retired != obj_2.global_history_retired:
        print("global_history_retired not equal")
        break

    if obj_1.global_history != obj_1.global_history_retired:
        f1_history_error += 1

    if obj_2.global_history != obj_2.global_history_retired:
        f2_history_error += 1

    cmp_cnt += 1

    if obj_1.hit != obj_2.hit:
        if obj_1.hit:
            f1_hit_f2_miss += 1
            f1_hit_f2_miss_list.append([i, obj_1, obj_2])
        else:
            f1_miss_f2_hit += 1
            f1_miss_f2_hit_list.append([i, obj_1, obj_2])

print("cmp_cnt: " + str(cmp_cnt))
print("f1_hit_f2_miss: " + str(f1_hit_f2_miss))
print("f1_miss_f2_hit: " + str(f1_miss_f2_hit))
print("f1_history_error: " + str(f1_history_error))
print("f2_history_error: " + str(f2_history_error))

with open(os.path.join(log_path, "f1_hit_f2_miss.txt"), "w") as f:
    print("id,pc,correct_global_history,f1_global_history,f1_pht_value,f2_global_history,f2_pht_value", file=f)

    for i, obj_1, obj_2 in f1_hit_f2_miss_list:
        print(str(i) + "," + hex(obj_1.pc) + "," + bin(obj_1.global_history_retired).replace("0b", "").zfill(global_history_length) + "," + 
        bin(obj_1.global_history).replace("0b", "").zfill(global_history_length) + "," + str(obj_1.pht_value) + 
        ("(T)" if obj_1.pht_value >= 2 else "(F)") + "," + bin(obj_2.global_history).replace("0b", "").zfill(global_history_length) + "," + 
        str(obj_2.pht_value) + ("(T)" if obj_2.pht_value >= 2 else "(F)"), file=f)
