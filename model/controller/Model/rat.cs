using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class rat
    {
        [JsonProperty("value")]
        public uint[]? value { get; set; }
        [JsonProperty("valid")]
        public bool[]? valid { get; set; }
        [JsonProperty("visible")]
        public bool[]? visible { get; set; }
    }
}
