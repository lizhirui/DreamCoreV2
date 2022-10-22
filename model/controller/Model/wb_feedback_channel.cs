using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class wb_feedback_channel
    {
        [JsonProperty("enable")]
        public bool enable { get; set; }
        [JsonProperty("phy_id")]
        public uint phy_id { get; set; }
        [JsonProperty("value")]
        public uint value { get; set; }
    }
}