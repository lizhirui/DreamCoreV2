from branch_predictor_base import branch_predictor_base

class gshare(branch_predictor_base):
    def __init__(self, global_history_length=12, branch_pc_length=13):
        self.pc_p1_addr_width = branch_pc_length
        self.global_history_width = global_history_length
        self.pht_addr_width = self.pc_p1_addr_width
        self.pht_size = 1 << self.pht_addr_width
        self.pc_p1_addr_mask = (1 << self.pc_p1_addr_width) - 1
        self.global_history_mask = (1 << self.global_history_width) - 1
        self.global_history = 0
        self.pht = [0 for x in range(0, self.pht_size)]

    def get_name(self):
        return "gshare(" + str(self.global_history_width) + ", " + str(self.pc_p1_addr_width) + ")"

    def get(self, pc):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1
        return self.pht[pht_addr] >= 2

    def update(self, pc, jump, hit):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1

        if jump:
            self.pht[pht_addr] = min(self.pht[pht_addr] + 1, 3)
        else:
            self.pht[pht_addr] = max(self.pht[pht_addr] - 1, 0)

        self.global_history = ((self.global_history << 1) | (1 if jump else 0)) & self.global_history_mask