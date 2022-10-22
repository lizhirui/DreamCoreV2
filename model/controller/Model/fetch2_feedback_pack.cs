using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class fetch2_feedback_pack
    {
        [JsonProperty("idle")]
        public bool idle { get; set; }
        [JsonProperty("stall")]
        public bool stall { get; set; }
    }
}