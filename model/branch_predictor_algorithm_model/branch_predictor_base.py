class branch_predictor_base:
    def get_name(self):
        raise Exception("get_name method not implemented")

    def get(self, pc):
        raise Exception("get method not implemented")

    def update(self, pc, jump, hit):
        raise Exception("update method not implemented")

    def get_state_str(self):
        return ""