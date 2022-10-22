using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class commit_feedback_pack
    {
        [JsonProperty("idle")]
        public bool idle { get; set; }
        [JsonProperty("next_handle_rob_id_valid")]
        public bool  next_handle_rob_id_valid { get; set; }
        [JsonProperty("has_exception")]
        public bool has_exception { get; set; }
        [JsonProperty("exception_pc")]
        public uint exception_pc { get; set; }
        [JsonProperty("flush")]
        public bool flush { get; set; }
        [JsonProperty("committed_rob_id")]
        public uint[]? committed_rob_id { get; set; }
        [JsonProperty("committed_rob_id_valid")]
        public bool[]? committed_rob_id_valid { get; set; }
        [JsonProperty("jump_enable")]
        public bool jump_enable { get; set; }
        [JsonProperty("jump")]
        public bool jump { get; set; }
        [JsonProperty("next_pc")]
        public uint next_pc { get; set; }
    }
}
