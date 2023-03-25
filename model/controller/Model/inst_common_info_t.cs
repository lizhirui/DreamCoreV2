using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class inst_common_info_t
    {
        [JsonProperty("id")]
        public ulong id { get; set; }
    }
}
