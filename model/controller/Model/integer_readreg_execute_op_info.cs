using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class integer_readreg_execute_op_info
    {
        [JsonProperty("enable")]
        public bool enable { get; set; }
        [JsonProperty("value")]
        public uint value { get; set; }
        [JsonProperty("valid")]
        public uint valid { get; set; }
        [JsonProperty("last_uop")]
        public bool last_uop { get; set; }
        [JsonProperty("rob_id")]
        public uint rob_id { get; set; }
        [JsonProperty("pc")]
        public uint pc { get; set; }
        [JsonProperty("imm")]
        public uint imm { get; set; }
        [JsonProperty("has_exception")]
        public bool has_exception { get; set; }
        [JsonProperty("exception_id")]
        public string? exception_id { get; set; }
        [JsonProperty("exception_value")]
        public uint exception_value { get; set; }
        [JsonProperty("rs1")]
        public uint rs1 { get; set; }
        [JsonProperty("arg1_src")]
        public string? arg1_src { get; set; }
        [JsonProperty("rs1_need_map")]
        public bool rs1_need_map { get; set; }
        [JsonProperty("rs1_phy")]
        public uint rs1_phy { get; set; }
        [JsonProperty("src1_value")]
        public uint src1_value { get; set; }
        [JsonProperty("rs2")]
        public uint rs2 { get; set; }
        [JsonProperty("arg2_src")]
        public string? arg2_src { get; set; }
        [JsonProperty("rs2_need_map")]
        public bool rs2_need_map { get; set; }
        [JsonProperty("rs2_phy")]
        public uint rs2_phy { get; set; }
        [JsonProperty("src2_value")]
        public uint src2_value { get; set; }
        [JsonProperty("rd")]
        public uint rd { get; set; }
        [JsonProperty("rd_enable")]
        public bool rd_enable { get; set; }
        [JsonProperty("need_rename")]
        public bool need_rename { get; set; }
        [JsonProperty("rd_phy")]
        public uint rd_phy { get; set; }
        [JsonProperty("csr")]
        public uint csr { get; set; }
        [JsonProperty("op")]
        public string? op { get; set; }
        [JsonProperty("op_unit")]
        public string? op_unit { get; set; }
        [JsonProperty("sub_op")]
        public string? sub_op { get; set; }
    }
}