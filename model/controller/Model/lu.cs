using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class lu
    {
        [JsonProperty("l2_stall")]
        public bool l2_stall { get; set; }
        [JsonProperty("l2_addr")]
        public uint l2_addr { get; set; }
        [JsonProperty("l2_rev_pack")]
        public lsu_readreg_execute_op_info? l2_rev_pack { get; set; }
    }
}