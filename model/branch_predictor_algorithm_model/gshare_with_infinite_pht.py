from branch_predictor_base import branch_predictor_base

class gshare_with_infinite_pht(branch_predictor_base):
    def __init__(self, global_history_length=12):
        self.global_history_width = global_history_length
        self.global_history_mask = (1 << self.global_history_width) - 1
        self.global_history = 0
        self.pht = {}

    def get_name(self):
        return "gshare_with_infinite_pht(" + str(self.global_history_width) + ")"

    def get(self, pc):
        pc_p1 = pc >> 2
        pht_addr = self.global_history | (pc_p1 << self.global_history_width)
        return self.pht.get(pht_addr, 0) >= 2

    def update(self, pc, jump, hit):
        pc_p1 = pc >> 2
        pht_addr = self.global_history | (pc_p1 << self.global_history_width)

        if jump:
            self.pht[pht_addr] = min(self.pht.get(pht_addr, 0) + 1, 3)
        else:
            self.pht[pht_addr] = max(self.pht.get(pht_addr, 0) - 1, 0)

        self.global_history = ((self.global_history << 1) | (1 if jump else 0)) & self.global_history_mask

    def get_state_str(self):
        return ", pht_cur_size: " + str(len(self.pht))