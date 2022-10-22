using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class integer_issue_output_feedback_pack
    {
        [JsonProperty("wakeup_valid")]
        public bool[]? wakeup_valid { get; set; }
        [JsonProperty("wakeup_rd")]
        public uint[]? wakeup_rd { get; set; }
        [JsonProperty("wakeup_shift")]
        public uint[]? wakeup_shift { get; set; }
    }
}