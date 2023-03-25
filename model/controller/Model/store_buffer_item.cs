using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class store_buffer_item
    {
        [JsonProperty("inst_common_info")]
        public inst_common_info_t? inst_common_info { get; set; }
        [JsonProperty("enable")]
        public bool[]? enable { get; set; }
        [JsonProperty("committed")]
        public bool[]? committed { get; set; }
        [JsonProperty("rob_id")]
        public uint[]? rob_id { get; set; }
        [JsonProperty("pc")]
        public uint[]? pc { get; set; }
        [JsonProperty("addr")]
        public uint[]? addr { get; set; }
        [JsonProperty("data")]
        public uint[]? data { get; set; }
        [JsonProperty("size")]
        public uint[]? size { get; set; }
    }
}
