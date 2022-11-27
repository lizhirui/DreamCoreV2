using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class lsu_readreg_execute_pack
    {
        [JsonProperty("lu")]
        public lsu_readreg_execute_op_info[]? lu { get; set; }
        [JsonProperty("sau")]
        public lsu_readreg_execute_op_info[]? sau { get; set; }
        [JsonProperty("sdu")]
        public lsu_readreg_execute_op_info[]? sdu { get; set; }
    }
}