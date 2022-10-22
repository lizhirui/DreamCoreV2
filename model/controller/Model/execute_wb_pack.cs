using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class execute_wb_pack
    {
        [JsonProperty("alu")]
        public execute_wb_op_info[]? alu;
        [JsonProperty("bru")]
        public execute_wb_op_info[]? bru;
        [JsonProperty("csr")]
        public execute_wb_op_info[]? csr;
        [JsonProperty("div")]
        public execute_wb_op_info[]? div;
        [JsonProperty("mul")]
        public execute_wb_op_info[]? mul;
        [JsonProperty("lsu")]
        public execute_wb_op_info[]? lsu;
    }
}