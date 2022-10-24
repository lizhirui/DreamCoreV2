class branch_target_buffer_base:
    def get_name(self):
        raise Exception("get_name method not implemented")

    def get(self, pc):
        raise Exception("get method not implemented")

    def update(self, pc, next_pc, is_branch, is_ret):
        raise Exception("update method not implemented")