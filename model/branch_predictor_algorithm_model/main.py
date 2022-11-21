#!/usr/bin/python3
from tqdm import tqdm
from static_always_jump import static_always_jump
from static_always_not_jump import static_always_not_jump
from static_backward_jump_forward_not_jump import static_backward_jump_forward_not_jump
from gshare import gshare
from gshare_with_infinite_pht import gshare_with_infinite_pht
from bimode import bimode
from bimodal import bimodal
from yags import yags
from direct_mapped_simple_btb import direct_mapped_simple_btb
from queue import Queue

#branch_predictor_list = [static_always_jump(), static_always_not_jump(), static_backward_jump_forward_not_jump(), 
                         #gshare(6, 7), gshare(8, 9), gshare(10, 11), gshare(15, 16), gshare(16, 17), gshare(17, 18), gshare(20, 21), 
                         #gshare(7, 7), gshare(9, 9), gshare(11, 11), gshare(16, 16), gshare(17, 17), gshare(18, 18), gshare(21, 21), 
                         #gshare_with_infinite_pht(6), gshare_with_infinite_pht(8), gshare_with_infinite_pht(10), 
                         #gshare_with_infinite_pht(15), gshare_with_infinite_pht(20),
                         #bimode(6, 6, 6), bimode(8, 8, 6), bimode(10, 10, 6), bimode(15, 15, 6), bimode(20, 20, 6),
                         #yags(6, 6, 6, 6), yags(8, 8, 6, 6), yags(10, 10, 6, 6), yags(15, 15, 6, 6), yags(20, 20, 6, 6)]
branch_predictor_list = [bimode(16, 16, 10)]
#branch_predictor_list = [bimodal(4)]

branch_target_buffer_list = [direct_mapped_simple_btb(256)]
branch_predictor_update_delay = 16

dump_path = "../../dump/branch_dump/coremark_10_7_2.txt"
update_dump_compare = False
branch_predictor_update_dump_path = "../../dump/branch_predictor_update_dump/coremark_10_7_2.txt"

print("Reading dump file...")

with open(dump_path, "r") as f:
    dump_txt = f.read()

print("Parsing dump file...")

lines = dump_txt.split('\n')
condition_branch_info = []
branch_info = []
static_branch_set = {}
branch_predictor_update_queue = Queue(0)

for i in tqdm(range(0, len(lines) - 1), colour="blue"):
    line = lines[i]
    arg = line.split(',')

    if len(arg) != 2:
        raise Exception("branch dump file error")

    pc = int(arg[0], 16)
    inst = int(arg[1], 16)

    opcode = inst & 0x7f
    rd = (inst >> 7) & 0x1f
    funct3 = (inst >> 12) & 0x7
    rs1 = (inst >> 15) & 0x1f
    rs2 = (inst >> 20) & 0x1f
    funct7 = (inst >> 25) & 0x7f
    rd_is_link = (rd == 1) or (rd == 5)
    rs1_is_link = (rs1 == 1) or (rs1 == 5)

    next_line = lines[i + 1]
    next_arg = next_line.split(',')

    if next_line.strip() == "":
        continue
    
    if len(next_arg) != 2:
        raise Exception("branch dump file error")

    next_pc = int(next_arg[0], 16)

    if opcode == 0x63:
        if next_pc == pc + 4:
            #[pc, not jump]
            condition_branch_info.append([pc, False])
        else:
            #[pc, jump]
            condition_branch_info.append([pc, True])
    
    if opcode == 0x6f or opcode == 0x67 or opcode == 0x63:
        is_branch = True

        if opcode == 0x67 and not rd_is_link and rs1_is_link:
            is_ret = True
        else:
            is_ret = False
    else:
        is_branch = False
        is_ret = False
    
    branch_info.append([pc, next_pc, is_branch, is_ret])
    static_branch_set[pc] = True

print("Static Branch: " + str(len(static_branch_set.keys())) + ", Dynamic Branch: " + str(len(branch_info)))

if update_dump_compare:
    branch_predictor_update_dump_queue = Queue(0)

    print("Reading branch predictor update dump file...")

    with open(branch_predictor_update_dump_path, "r") as f:
        branch_predictor_update_dump_txt = f.read()

    print("Parsing branch predictor update dump file...")
    lines = branch_predictor_update_dump_txt.split('\n')

    for i in tqdm(range(0, len(lines) - 1), colour="blue"):
        line = lines[i]
        arg = line.split(',')

        if len(arg) != 4:
            raise Exception("branch predictor update dump file error")

        pc = int(arg[0], 16)
        bru_jump = arg[1] == "true"
        bru_next_pc = int(arg[2], 16)
        hit = arg[3] == "true"
        branch_predictor_update_dump_queue.put([pc, bru_jump, bru_next_pc, hit])

for branch_predictor in branch_predictor_list:
    print(branch_predictor.get_name() + " is processing...")
    
    hit = 0
    miss = 0
    update_cnt = 0

    if update_dump_compare:
        ref_hit = 0
        ref_miss = 0
        ref_hit_miss = 0
        ref_miss_hit = 0

    for i in tqdm(range(0, len(condition_branch_info)), colour="blue", leave=False):
        pc = condition_branch_info[i][0]
        jump = condition_branch_info[i][1]

        predict = branch_predictor.get(pc)

        if predict == jump:
            hit += 1
        else:
            miss += 1

        #branch_predictor.update(pc, jump, predict == jump)
        branch_predictor_update_queue.put([pc, jump, predict == jump])

        if i >= branch_predictor_update_delay:
            [t_pc, t_jump, t_hit] = branch_predictor_update_queue.get()

            if update_dump_compare:
                [ref_pc, ref_jump, ref_next_pc, ref_t_hit] = branch_predictor_update_dump_queue.get()

                if ref_t_hit:
                    ref_hit += 1
                else:
                    ref_miss += 1

                if ref_t_hit != t_hit:
                    if ref_t_hit:
                        ref_hit_miss += 1
                    else:
                        ref_miss_hit += 1

                if ref_pc != t_pc or ref_jump != t_jump:
                    print("update_cnt: " + str(update_cnt))
                    print("ref_pc: " + hex(ref_pc) + ", t_pc: " + hex(t_pc))
                    print("ref_jump: " + str(ref_jump) + ", t_jump: " + str(t_jump))
                    print("ref_hit: " + str(ref_t_hit) + ", t_hit: " + str(t_hit))
                    raise Exception("branch predictor update dump information compare error")

            branch_predictor.update(t_pc, t_jump, t_hit)
            update_cnt += 1

    print("[" + branch_predictor.get_name() + "] hit: " + str(hit) + ", miss: " + str(miss) + ", total: " + str(hit + miss) + ", hit rate: " + str(hit * 100 / (hit + miss)) + "%" + branch_predictor.get_state_str())
    
    if update_dump_compare:
        print("[" + branch_predictor.get_name() + "] ref_hit: " + str(ref_hit) + ", ref_miss: " + str(ref_miss) + ", total: " + str(ref_hit + ref_miss) + ", hit rate: " + str(ref_hit * 100 / (ref_hit + ref_miss)) + "%" + branch_predictor.get_state_str() + ", ref_hit_miss: " + str(ref_hit_miss) + ", ref_miss_hit: " + str(ref_miss_hit))

for branch_target_buffer in branch_target_buffer_list:
    print(branch_target_buffer.get_name() + " is processing...")
    
    hit = 0
    miss = 0
    ret = 0

    for i in tqdm(range(0, len(branch_info)), colour="blue", leave=False):
        pc = branch_info[i][0]
        next_pc = branch_info[i][1]
        is_branch = branch_info[i][2]
        is_ret = branch_info[i][3]

        if is_ret:
            ret += 1

        predict = branch_target_buffer.get(pc)

        if predict == next_pc:
            hit += 1
        else:
            miss += 1

        branch_target_buffer.update(pc, next_pc, is_branch, is_ret)

    print("[" + branch_target_buffer.get_name() + "] hit: " + str(hit) + ", miss: " + str(miss) + ", ret:" + str(ret) + ", total: " + str(hit + miss) + ", hit rate: " + str(hit * 100 / (hit + miss)) + "%")