using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class Fetch1
    {
        [JsonProperty("pc")]
        public uint pc { get; set; }
        [JsonProperty("jump_wait")]
        public bool jump_wait { get; set; }
    }
}
