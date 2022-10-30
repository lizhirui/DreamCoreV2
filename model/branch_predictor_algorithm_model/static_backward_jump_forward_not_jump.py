from branch_predictor_base import branch_predictor_base

class static_backward_jump_forward_not_jump(branch_predictor_base):
    def __init__(self):
        self.last_pc = 0

    def get_name(self):
        return "backward_jump_forward_not_jump"

    def get(self, pc):
        ret = True if pc < self.last_pc else False
        self.last_pc = pc
        return ret

    def update(self, pc, jump, hit):
        pass