from branch_target_buffer_base import branch_target_buffer_base

class direct_mapped_simple_btb(branch_target_buffer_base):
    def __init__(self, size):
        self.size = size
        self.buffer = [{"valid": False, "next_pc": 0} for i in range(size)]

    def get_name(self):
        return "direct_mapped_simple_btb(" + str(self.size) + ")"

    def get(self, pc):
        item = self.buffer[pc % self.size]

        if item["valid"]:
            return item["next_pc"]
        else:
            return pc + 4

    def update(self, pc, next_pc, is_branch, is_ret):
        if is_branch:
            self.buffer[pc % self.size] = {"valid": True, "next_pc": next_pc}