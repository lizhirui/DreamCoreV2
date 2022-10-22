using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class integer_readreg_execute_pack
    {
        [JsonProperty("alu")]
        public integer_readreg_execute_op_info[]? alu { get; set; }
        [JsonProperty("bru")]
        public integer_readreg_execute_op_info[]? bru { get; set; }
        [JsonProperty("csr")]
        public integer_readreg_execute_op_info[]? csr { get; set; }
        [JsonProperty("div")]
        public integer_readreg_execute_op_info[]? div { get; set; }
        [JsonProperty("mul")]
        public integer_readreg_execute_op_info[]? mul { get; set; }
    }
}