using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class execute
    {
        [JsonProperty("div")]
        public div[]? div { get; set; }
        [JsonProperty("lu")]
        public lu[]? lu { get; set; }
    }
}