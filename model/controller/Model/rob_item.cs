using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class rob_item
    {
        [JsonProperty("new_phy_reg_id")]
        public uint new_phy_reg_id { get; set; }
        [JsonProperty("old_phy_reg_id")]
        public uint old_phy_reg_id { get; set; }
        [JsonProperty("old_phy_reg_id_valid")]
        public bool old_phy_reg_id_valid { get; set; }
        [JsonProperty("finish")]
        public bool finish { get; set; }
        [JsonProperty("pc")]
        public uint pc { get; set; }
        [JsonProperty("inst_value")]
        public uint inst_value { get; set; }
        [JsonProperty("rd")]
        public uint rd { get; set; }
        [JsonProperty("has_exception")]
        public bool has_exception { get; set; }
        [JsonProperty("exception_id")]
        public uint exception_id { get; set; }
        [JsonProperty("exception_value")]
        public uint exception_value { get; set; }
        [JsonProperty("bru_op")]
        public bool bru_op { get; set; }
        [JsonProperty("bru_jump")]
        public bool bru_jump { get; set; }
        [JsonProperty("bru_next_pc")]
        public uint bru_next_pc { get; set; }
        [JsonProperty("is_mret")]
        public bool is_mret { get; set; }
        [JsonProperty("csr_addr")]
        public uint csr_addr { get; set; }
        [JsonProperty("csr_newvalue")]
        public uint csr_newvalue { get; set; }
        [JsonProperty("csr_newvalue_valid")]
        public bool csr_newvalue_valid { get; set; }
        [JsonProperty("old_phy_id_free_list_rptr")]
        public uint old_phy_id_free_list_rptr { get; set; }
        [JsonProperty("old_phy_id_free_list_rstage")]
        public bool old_phy_id_free_list_rstage { get; set; }
        [JsonProperty("new_phy_id_free_list_rptr")]
        public uint new_phy_id_free_list_rptr { get; set; }
        [JsonProperty("new_phy_id_free_list_rstage")]
        public bool new_phy_id_free_list_rstage { get; set; }
        [JsonProperty("rob_id")]
        public uint rob_id { get; set; }
    }
}
