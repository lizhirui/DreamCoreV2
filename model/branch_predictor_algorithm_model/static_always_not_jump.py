from branch_predictor_base import branch_predictor_base

class static_always_not_jump(branch_predictor_base):
    def get_name(self):
        return "static_always_not_jump"

    def get(self, pc):
        return False

    def update(self, pc, jump, hit):
        pass