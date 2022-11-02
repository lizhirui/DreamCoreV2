from branch_predictor_base import branch_predictor_base

class bimode(branch_predictor_base):
    def __init__(self, global_history_length=12, branch_pc_length=13, pht_choice_length=6):
        self.pc_p1_addr_width = branch_pc_length
        self.global_history_width = self.pc_p1_addr_width
        self.pht_addr_width = self.pc_p1_addr_width
        self.pht_size = 1 << self.pht_addr_width
        self.pht_choice_addr_width = pht_choice_length
        self.pht_choice_size = 1 << self.pht_choice_addr_width
        self.pht_choice_addr_mask = (1 << self.pht_choice_addr_width) - 1
        self.pc_p1_addr_mask = (1 << self.pc_p1_addr_width) - 1
        self.global_history_mask = (1 << self.global_history_width) - 1
        self.global_history = 0
        self.pht_left = [0 for x in range(0, self.pht_size)]
        self.pht_right = [0 for x in range(0, self.pht_size)]
        self.pht_choice = [0 for x in range(0, self.pht_choice_size)]

    def get_name(self):
        return "bimode(" + str(self.global_history_width) + ", " + str(self.pc_p1_addr_width) + ", " + str(self.pht_choice_addr_width) + ")"

    def get(self, pc):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1
        pht_choice_addr = (pc >> 2) & self.pht_choice_addr_mask
        return self.pht_left[pht_addr] >= 2 if self.pht_choice[pht_choice_addr] <= 1 else self.pht_right[pht_addr] >= 2

    def update(self, pc, jump, hit):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1
        pht_choice_addr = (pc >> 2) & self.pht_choice_addr_mask
        is_pht_left = self.pht_choice[pht_choice_addr] <= 1

        if is_pht_left:
            if jump:
                self.pht_left[pht_addr] = min(self.pht_left[pht_addr] + 1, 3)
            else:
                self.pht_left[pht_addr] = max(self.pht_left[pht_addr] - 1, 0)
        else:
            if jump:
                self.pht_right[pht_addr] = min(self.pht_right[pht_addr] + 1, 3)
            else:
                self.pht_right[pht_addr] = max(self.pht_right[pht_addr] - 1, 0)

        if not ((self.pht_choice[pht_choice_addr] >= 2) != jump and hit):
            if jump:
                self.pht_choice[pht_choice_addr] = min(self.pht_choice[pht_choice_addr] + 1, 3)
            else:
                self.pht_choice[pht_choice_addr] = max(self.pht_choice[pht_choice_addr] - 1, 0)

        self.global_history = ((self.global_history << 1) | (1 if jump else 0)) & self.global_history_mask