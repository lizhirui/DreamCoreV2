using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class fetch2_decode_op_info
    {
        [JsonProperty("enable")]
        public bool enable { get; set; }
        [JsonProperty("pc")]
        public uint pc { get; set; }
        [JsonProperty("value")]
        public uint value { get; set; }
        [JsonProperty("has_exception")]
        public bool has_exception { get; set; }
        [JsonProperty("exception_id")]
        public string? exception_id { get; set; }
        [JsonProperty("exception_value")]
        public uint exception_value { get; set; }
    }
}