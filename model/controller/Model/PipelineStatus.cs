using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller.Model
{
    [Serializable]
    public class PipelineStatus
    {
        [JsonProperty("fetch1")]
        public fetch1? fetch1 { get; set; }
        [JsonProperty("fetch1_fetch2")]
        public fetch1_fetch2_op_info[]? fetch1_fetch2 { get; set; }
        [JsonProperty("fetch2")]
        public fetch2? fetch2 { get; set; }
        [JsonProperty("fetch2_decode")]
        public fetch2_decode_op_info[]? fetch2_decode { get; set; }
        [JsonProperty("decode_rename")]
        public decode_rename_op_info[]? decode_rename { get; set; }
        [JsonProperty("rename_dispatch")]
        public rename_dispatch_op_info[]? rename_dispatch { get; set; }
        [JsonProperty("dispatch")]
        public dispatch? dispatch { get; set; }
        [JsonProperty("dispatch_integer_issue")]
        public dispatch_issue_op_info[]? dispatch_integer_issue { get; set; }
        [JsonProperty("dispatch_lsu_issue")]
        public dispatch_issue_op_info[]? dispatch_lsu_issue { get; set; }
        [JsonProperty("integer_issue")]
        public integer_issue? integer_issue { get; set; }
        [JsonProperty("lsu_issue")]
        public lsu_issue? lsu_issue { get; set; }
        [JsonProperty("integer_issue_readreg")]
        public integer_issue_readreg_op_info[]? integer_issue_readreg { get; set; }
        [JsonProperty("lsu_issue_readreg")]
        public lsu_issue_readreg_op_info[]? lsu_issue_readreg { get; set; }
        [JsonProperty("lsu_readreg")]
        public lsu_readreg? lsu_readreg { get; set; }
        [JsonProperty("integer_readreg_execute")]
        public integer_readreg_execute_pack? integer_readreg_execute { get; set; }
        [JsonProperty("lsu_readreg_execute")]
        public lsu_readreg_execute_op_info[]? lsu_readreg_execute { get; set; }
        [JsonProperty("execute")]
        public execute? execute { get; set; }
        [JsonProperty("execute_wb")]
        public execute_wb_pack? execute_wb { get; set; }
        [JsonProperty("execute_commit")]
        public execute_commit_pack? execute_commit { get; set; }
        [JsonProperty("fetch2_feedback_pack")]
        public fetch2_feedback_pack? fetch2_feedback_pack { get; set; }
        [JsonProperty("decode_feedback_pack")]
        public decode_feedback_pack? decode_feedback_pack { get; set; }
        [JsonProperty("rename_feedback_pack")]
        public rename_feedback_pack? rename_feedback_pack { get; set; }
        [JsonProperty("dispatch_feedback_pack")]
        public dispatch_feedback_pack? dispatch_feedback_pack { get; set; }
        [JsonProperty("integer_issue_output_feedback_pack")]
        public integer_issue_output_feedback_pack? integer_issue_output_feedback_pack { get; set; }
        [JsonProperty("integer_issue_feedback_pack")]
        public integer_issue_feedback_pack? integer_issue_feedback_pack { get; set; }
        [JsonProperty("lsu_issue_feedback_pack")]
        public lsu_issue_feedback_pack? lsu_issue_feedback_pack { get; set; }
        [JsonProperty("lsu_readreg_feedback_pack")]
        public lsu_readreg_feedback_pack? lsu_readreg_feedback_pack { get; set; }
        [JsonProperty("execute_feedback_pack")]
        public execute_feedback_channel[]? execute_feedback_pack { get; set; }
        [JsonProperty("wb_feedback_pack")]
        public wb_feedback_channel[]? wb_feedback_pack { get; set; }
        [JsonProperty("commit_feedback_pack")]
        public commit_feedback_pack? commit_feedback_pack { get; set; }
        [JsonProperty("rob")]
        public rob_item[]? rob { get; set; }
        [JsonProperty("speculative_rat")]
        public rat? speculative_rat { get; set; }
        [JsonProperty("retire_rat")]
        public rat? retire_rat { get; set; }
        [JsonProperty("phy_regfile")]
        public regfile? phy_regfile { get; set; }
        [JsonProperty("phy_id_free_list")]
        public uint[]? phy_id_free_list { get; set; }
        [JsonProperty("store_buffer")]
        public store_buffer_item[]? store_buffer { get; set; }
    }
}
