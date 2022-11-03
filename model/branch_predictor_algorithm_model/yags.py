from branch_predictor_base import branch_predictor_base

class yags(branch_predictor_base):
    def __init__(self, global_history_length=12, branch_pc_length=13, pht_choice_length=6, tag_width=6):
        self.pc_p1_addr_width = branch_pc_length
        self.global_history_width = global_history_length
        self.pht_addr_width = self.pc_p1_addr_width
        self.pht_size = 1 << self.pht_addr_width
        self.pht_choice_addr_width = pht_choice_length
        self.pht_choice_size = 1 << self.pht_choice_addr_width
        self.pht_choice_addr_mask = (1 << self.pht_choice_addr_width) - 1
        self.pc_p1_addr_mask = (1 << self.pc_p1_addr_width) - 1
        self.global_history_mask = (1 << self.global_history_width) - 1
        self.global_history = 0
        self.tag_width = tag_width
        self.tag_mask = (1 << self.tag_width) - 1
        self.cache_t = [{"tag": 0, "valid": False, "count": 0} for x in range(0, self.pht_size)]
        self.cache_nt = [{"tag": 0, "valid": False, "count": 0} for x in range(0, self.pht_size)]
        self.pht_choice = [0 for x in range(0, self.pht_choice_size)]

    def get_name(self):
        return "yags(" + str(self.global_history_width) + ", " + str(self.pc_p1_addr_width) + ", " + str(self.pht_choice_addr_width) + ", " + str(self.tag_width) + ")"

    def get(self, pc):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1
        pht_choice_addr = (pc >> 2) & self.pht_choice_addr_mask
        predict_taken = self.pht_choice[pht_choice_addr] >= 2
        pc_tag = (pc >> 2) & self.tag_mask

        if predict_taken:
            cache_hit = self.cache_nt[pht_addr]["valid"] and self.cache_nt[pht_addr]["tag"] == pc_tag
            cache_taken = self.cache_nt[pht_addr]["count"] >= 2
        else:
            cache_hit = self.cache_t[pht_addr]["valid"] and self.cache_t[pht_addr]["tag"] == pc_tag
            cache_taken = self.cache_t[pht_addr]["count"] >= 2

        return cache_taken if cache_hit else predict_taken

    def update(self, pc, jump, hit):
        pc_p1 = (pc >> 2) & self.pc_p1_addr_mask
        pht_addr = self.global_history ^ pc_p1
        pht_choice_addr = (pc >> 2) & self.pht_choice_addr_mask
        predict_taken = self.pht_choice[pht_choice_addr] >= 2
        pc_tag = (pc >> 2) & self.tag_mask

        if predict_taken:
            cache_hit = self.cache_nt[pht_addr]["valid"] and self.cache_nt[pht_addr]["tag"] == pc_tag
            cache_taken = self.cache_nt[pht_addr]["count"] >= 2
        else:
            cache_hit = self.cache_t[pht_addr]["valid"] and self.cache_t[pht_addr]["tag"] == pc_tag
            cache_taken = self.cache_t[pht_addr]["count"] >= 2

        cache_t_update = False
        cache_nt_update = False

        if (self.pht_choice[pht_choice_addr] >= 2) != jump:
            if jump:
                cache_t_update = True
            else:
                cache_nt_update = True

        if cache_hit:
            if predict_taken:
                cache_nt_update = True
            else:
                cache_t_update = True

        if cache_t_update:
            count_origin = self.cache_t[pht_addr]["count"] if self.cache_t[pht_addr]["valid"] and self.cache_t[pht_addr]["tag"] == pc_tag else 3

            if jump:
                count = min(count_origin + 1, 3)
            else:
                count = max(count_origin - 1, 0)

            self.cache_t[pht_addr] = {"tag": pc_tag, "valid": True, "count": count}

        if cache_nt_update:
            count_origin = self.cache_nt[pht_addr]["count"] if self.cache_nt[pht_addr]["valid"] and self.cache_nt[pht_addr]["tag"] == pc_tag else 0

            if jump:
                count = min(count_origin + 1, 3)
            else:
                count = max(count_origin - 1, 0)

            self.cache_nt[pht_addr] = {"tag": pc_tag, "valid": True, "count": count}

        if not ((self.pht_choice[pht_choice_addr] >= 2) != jump and hit):
            if jump:
                self.pht_choice[pht_choice_addr] = min(self.pht_choice[pht_choice_addr] + 1, 3)
            else:
                self.pht_choice[pht_choice_addr] = max(self.pht_choice[pht_choice_addr] - 1, 0)

        self.global_history = ((self.global_history << 1) | (1 if jump else 0)) & self.global_history_mask