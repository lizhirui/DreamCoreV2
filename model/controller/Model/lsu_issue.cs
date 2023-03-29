using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class lsu_issue
    {
        [JsonProperty("busy")]
        public bool busy { get; set; }
        [JsonProperty("hold_rev_pack")]
        public dispatch_issue_op_info[]? hold_rev_pack { get; set; }
        [JsonProperty("issue_q")]
        public ooo_issue_queue<lsu_issue_queue_item>? issue_q { get; set; }
        [JsonProperty("wakeup_shift_src1")]
        public uint[]? wakeup_shift_src1 { get; set; }
        [JsonProperty("src1_ready")]
        public bool[]? src1_ready { get; set; }
        [JsonProperty("wakeup_shift_src2")]
        public uint[]? wakeup_shift_src2 { get; set; }
        [JsonProperty("src2_ready")]
        public bool[]? src2_ready { get; set; }
        [JsonProperty("cur_lpv")]
        public uint[]? cur_lpv { get; set; }
        [JsonProperty("src1_lpv")]
        public uint[]? src1_lpv { get; set; }
        [JsonProperty("src2_lpv")]
        public uint[]? src2_lpv { get; set; }
    }
}