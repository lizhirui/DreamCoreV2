using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class dispatch_feedback_pack
    {
        [JsonProperty("stall")]
        public bool stall { get; set; }
    }
}