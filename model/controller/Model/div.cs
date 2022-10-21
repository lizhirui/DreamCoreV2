﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class div
    {
        [JsonProperty("progress")]
        public uint progress { get; set; }
        [JsonProperty("busy")]
        public bool busy { get; set; }
    }
}
