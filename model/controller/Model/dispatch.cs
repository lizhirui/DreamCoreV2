using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class dispatch
    {
        [JsonProperty("integer_busy")]
        public bool integer_busy { get; set; }
        [JsonProperty("lsu_busy")]
        public bool lsu_busy { get; set; }
        [JsonProperty("busy")]
        public bool busy { get; set; }
        [JsonProperty("is_inst_waiting")]
        public bool is_inst_waiting { get; set; }
        [JsonProperty("inst_waiting_rob_id")]
        public uint inst_waiting_rob_id { get; set; }
        [JsonProperty("is_stbuf_empty_waiting")]
        public bool is_stbuf_empty_waiting { get; set; }
    }
}