using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class WBFeedback
    {
        [JsonProperty("enable")]
        public bool Enable { get; set; }
        [JsonProperty("phy_id")]
        public uint PhyID { get; set; }
        [JsonProperty("value")]
        public uint Value { get; set; }
    }
}
