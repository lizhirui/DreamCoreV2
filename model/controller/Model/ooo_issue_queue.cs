using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class ooo_issue_queue<T>
    {
        [JsonProperty("value")]
        public T[]? value { get; set; }
        [JsonProperty("valid")]
        public bool[]? valid { get; set; }
        [JsonProperty("rptr")]
        public uint rptr { get; set; }
        [JsonProperty("rstage")]
        public bool rstage { get; set; }
        [JsonProperty("wptr")]
        public uint wptr { get; set; }
        [JsonProperty("wstage")]
        public bool wstage { get; set; }
    }
}

