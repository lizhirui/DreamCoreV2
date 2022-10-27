using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class integer_issue
    {
        [JsonProperty("busy")]
        public bool busy { get; set; }
        [JsonProperty("hold_rev_pack")]
        public dispatch_issue_op_info[]? hold_rev_pack { get; set; }
        [JsonProperty("next_port_index")]
        public uint next_port_index { get; set; }
        [JsonProperty("issue_q")]
        public ooo_issue_queue<integer_issue_queue_item>? issue_q { get; set; }
        [JsonProperty("alu_idle")]
        public bool[]? alu_idle { get; set; }
        [JsonProperty("bru_idle")]
        public bool[]? bru_idle { get; set; }
        [JsonProperty("csr_idle")]
        public bool[]? csr_idle { get; set; }
        [JsonProperty("div_idle")]
        public bool[]? div_idle { get; set; }
        [JsonProperty("mul_idle")]
        public bool[]? mul_idle { get; set; }
        [JsonProperty("alu_idle_shift")]
        public uint[]? alu_idle_shift { get; set; }
        [JsonProperty("bru_idle_shift")]
        public uint[]? bru_idle_shift { get; set; }
        [JsonProperty("csr_idle_shift")]
        public uint[]? csr_idle_shift { get; set; }
        [JsonProperty("div_idle_shift")]
        public uint[]? div_idle_shift { get; set; }
        [JsonProperty("mul_idle_shift")]
        public uint[]? mul_idle_shift { get; set; }
        [JsonProperty("alu_busy_shift")]
        public uint[]? alu_busy_shift { get; set; }
        [JsonProperty("bru_busy_shift")]
        public uint[]? bru_busy_shift { get; set; }
        [JsonProperty("csr_busy_shift")]
        public uint[]? csr_busy_shift { get; set; }
        [JsonProperty("div_busy_shift")]
        public uint[]? div_busy_shift { get; set; }
        [JsonProperty("mul_busy_shift")]
        public uint[]? mul_busy_shift { get; set; }
        [JsonProperty("wakeup_shift_src1")]
        public uint[]? wakeup_shift_src1 { get; set; }
        [JsonProperty("src1_ready")]
        public bool[]? src1_ready { get; set; }
        [JsonProperty("wakeup_shift_src2")]
        public uint[]? wakeup_shift_src2 { get; set; }
        [JsonProperty("src2_ready")]
        public bool[]? src2_ready { get; set; }
        [JsonProperty("port_index")]
        public int[]? port_index { get; set; }
        [JsonProperty("op_unit_seq")]
        public int[]? op_unit_seq { get; set; }
        [JsonProperty("rob_id")]
        public int[]? rob_id { get; set; }
        [JsonProperty("rob_id_stage")]
        public bool[]? rob_id_stage { get; set; }
        [JsonProperty("wakeup_rd")]
        public uint[]? wakeup_rd { get; set; }
        [JsonProperty("wakeup_rd_valid")]
        public bool[]? wakeup_rd_valid { get; set; }
        [JsonProperty("wakeup_shift")]
        public uint[]? wakeup_shift { get; set; }
        [JsonProperty("new_idle_shift")]
        public uint[]? new_idle_shift { get; set; }
        [JsonProperty("new_busy_shift")]
        public uint[]? new_busy_shift { get; set; }
    }
}