using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using DreamCoreV2_model_controller.Model;
using Newtonsoft.Json;

namespace DreamCoreV2_model_controller
{
    public partial class PipelineStatusWindow : Window
    {
        private static PipelineStatusWindow? instance = null;
        private MainWindow mainWindow;
        private Model.PipelineStatus? pipelineStatus = null;
        
        struct PipelineStatusHistoryItem
        {
            public int cpuCycle;
            public string pipelineStatusString;
        };

        private FixedSizeBuffer<PipelineStatusHistoryItem> pipelineStatusHistory = new(100);
        private int curHistoryIndex = 0;

        private PipelineStatusWindow(MainWindow mainWindow)
        {
            InitializeComponent();
            this.mainWindow = mainWindow;
            this.mainWindow.PipelineStatusReceivedEvent += MainWindow_PipelineStatusReceived;
            this.mainWindow.PipelineStatusResetEvent += MainWindow_PipelineStatusResetEvent;

            if(int.TryParse(Config.Get("PipelineStatus_Left", ""), out var left) && int.TryParse(Config.Get("PipelineStatus_Top", ""), out var top))
            {
                Left = left;
                Top = top;
            }
        }

        private void MainWindow_PipelineStatusResetEvent()
        {
            pipelineStatusHistory.Clear();
        }

        ~PipelineStatusWindow()
        {
            mainWindow.PipelineStatusReceivedEvent -= MainWindow_PipelineStatusReceived;
            mainWindow.PipelineStatusResetEvent -= MainWindow_PipelineStatusResetEvent;
        }

        public static PipelineStatusWindow CreateInstance(MainWindow mainWindow)
        {
            if(instance == null)
            {
                instance = new PipelineStatusWindow(mainWindow);
                instance.Show();
            }

            return instance;
        }

        private void MainWindow_PipelineStatusReceived(string str, Model.PipelineStatus PipelineStatus)
        {
            var item_exist = false;

            if(pipelineStatusHistory.Count() > 0)
            {
                var item = pipelineStatusHistory.Get(pipelineStatusHistory.Count() - 1);

                if(item.cpuCycle == Global.cpuCycle.Value && item.pipelineStatusString == str)
                {
                    item_exist = true;        
                }
            }

            if(!item_exist)
            {
                pipelineStatusHistory.Push(new PipelineStatusHistoryItem{cpuCycle = Global.cpuCycle.Value, pipelineStatusString = str});
            }

            curHistoryIndex = pipelineStatusHistory.Count() - 1;
            loadHistory(true);
            pipelineStatus = PipelineStatus;
            updateInstruction();
            refreshDisplay();
        }

        private void loadHistory(bool headerOnly = false)
        {
            var item = pipelineStatusHistory.Get(curHistoryIndex);
            label_Cycle.Content = "Cycle: " + item.cpuCycle;
            label_History.Content = "History: " + ("" + (curHistoryIndex + 1)).PadLeft(2, '0') + "/" + ("" + pipelineStatusHistory.Count()).PadLeft(2, '0') + ((curHistoryIndex >= (pipelineStatusHistory.Count() - 1)) ? "(Current)" : "(Old)    ");
            label_History_Prev.Background = (curHistoryIndex > 0) ? Brushes.Green : Brushes.Red;
            label_History_Next.Background = (curHistoryIndex < (pipelineStatusHistory.Count() - 1)) ? Brushes.Green : Brushes.Red;

            if(!headerOnly)
            {
                pipelineStatus = JsonConvert.DeserializeObject<Model.PipelineStatus>(item.pipelineStatusString);
                updateInstruction();
                refreshDisplay();
            }
        }

        private string getInstructionText(uint Value, uint PC, bool last_uop, string? uop_name)
        {
            var result = RISC_V_Disassembler.Disassemble(BitConverter.GetBytes(Value), PC);
            return (!last_uop ? "<" + (uop_name == null ? "null" : uop_name) + ">" : "") + (((result != null) && (result.Length > 0)) ? (result[0].mnemonic + " " + result[0].op_str) : "<Invalid>");
        }

        #pragma warning disable CS8602
        private void updateInstruction()
        {
            if(pipelineStatus != null)
            {
                for(var i = 0;i < pipelineStatus.fetch1_fetch2.Length;i++)
                {
                    var item = pipelineStatus.fetch1_fetch2[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, true, "");
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.fetch2_decode.Length;i++)
                {
                    var item = pipelineStatus.fetch2_decode[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, true, "");
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.decode_rename.Length;i++)
                {
                    var item = pipelineStatus.decode_rename[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.rename_dispatch.Length;i++)
                {
                    var item = pipelineStatus.rename_dispatch[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.dispatch_integer_issue.Length;i++)
                {
                    var item = pipelineStatus.dispatch_integer_issue[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.dispatch_lsu_issue.Length;i++)
                {
                    var item = pipelineStatus.dispatch_lsu_issue[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_issue.hold_rev_pack.Length;i++)
                {
                    var item = pipelineStatus.integer_issue.hold_rev_pack[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_issue.hold_rev_pack.Length;i++)
                {
                    var item = pipelineStatus.lsu_issue.hold_rev_pack[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_issue_readreg.Length;i++)
                {
                    var item = pipelineStatus.integer_issue_readreg[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_issue_readreg.Length;i++)
                {
                    var item = pipelineStatus.lsu_issue_readreg[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.alu.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.alu[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.bru.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.bru[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.csr.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.csr[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.div.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.div[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.mul.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.mul[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.lu.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.lu[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.sau.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.sau[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.sdu.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.sdu[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_wb.alu.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.alu[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_wb.bru.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.bru[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_wb.csr.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.csr[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_wb.div.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.div[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_wb.mul.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.mul[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }
                
                for(var i = 0;i < pipelineStatus.execute_wb.lu.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.lu[i];
                    
                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }
                
                for(var i = 0;i < pipelineStatus.execute_commit.alu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.alu[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.bru.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.bru[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.csr.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.csr[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.div.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.div[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.mul.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.mul[i];

                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.lu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.lu[i];
                    
                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.sau.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.sau[i];
                    
                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.execute_commit.sdu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.sdu[i];
                    
                    if(item.enable)
                    {
                        item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                    }
                    else
                    {
                        item.Instruction = "<Empty>";
                    }
                }

                for(var i = 0;i < pipelineStatus.rob.Length;i++)
                {
                    var item = pipelineStatus.rob[i];
                    item.Instruction = getInstructionText(item.inst_value, item.pc, item.last_uop, item.sub_op);
                }

                for(var i = 0;i < pipelineStatus.integer_issue.issue_q.value.Length;i++)
                {
                    if(pipelineStatus.integer_issue.issue_q.valid[i])
                    {
                        var item = pipelineStatus.integer_issue.issue_q.value[i];

                        if(item.enable)
                        {
                            item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                        }
                        else
                        {
                            item.Instruction = "<Empty>";
                        }
                    }
                }

                for(var i = 0;i < pipelineStatus.lsu_issue.issue_q.value.Length;i++)
                {
                    if(pipelineStatus.lsu_issue.issue_q.valid[i])
                    {
                        var item = pipelineStatus.lsu_issue.issue_q.value[i];

                        if(item.enable)
                        {
                            item.Instruction = getInstructionText(item.value, item.pc, item.last_uop, item.sub_op);
                        }
                        else
                        {
                            item.Instruction = "<Empty>";
                        }
                    }
                }
            }
        }
        #pragma warning restore CS8602

        private string getDisplayText(string instruction, uint pc, bool has_rob, uint rob_id, inst_common_info_t? inst_common_info)
        {
            if(instruction == "<Empty>")
            {
                return instruction;
            }
            else
            {
                return ((inst_common_info != null) ? string.Format("[{0:X8}]", inst_common_info.id) : "") + (has_rob ? rob_id + "," : "") + "0x" + string.Format("{0:X8}", pc) + ":" + instruction;
            }
        }

        #pragma warning disable CS8602
        private void refreshDisplay()
        {
            if(pipelineStatus != null)
            {
                label_Fetch1_PC.Content = "PC = 0x" + string.Format("{0:X8}", pipelineStatus.fetch1.pc);
                label_Fetch1_Jump.Background = pipelineStatus.fetch1.jump_wait ? Brushes.Green : Brushes.Red;
                label_Fetch2_Busy.Background = pipelineStatus.fetch2.busy ? Brushes.Green : Brushes.Red;
                label_Fetch2_Feedback_Idle.Background = pipelineStatus.fetch2_feedback_pack.idle ? Brushes.Green : Brushes.Red;
                label_Decode_Feedback_Idle.Background = pipelineStatus.decode_feedback_pack.idle ? Brushes.Green : Brushes.Red;
                label_Dispatch_IntBusy.Background = pipelineStatus.dispatch.integer_busy ? Brushes.Green : Brushes.Red;
                label_Dispatch_LSUBusy.Background = pipelineStatus.dispatch.lsu_busy ? Brushes.Green : Brushes.Red;
                label_Dispatch_Busy.Background = pipelineStatus.dispatch.busy ? Brushes.Green : Brushes.Red;
                label_Dispatch_InstWait.Background = pipelineStatus.dispatch.is_inst_waiting ? Brushes.Green : Brushes.Red;
                label_Dispatch_InstWait.Content = "InstWait: " + pipelineStatus.dispatch.inst_waiting_rob_id;
                label_Dispatch_StbufWait.Background = pipelineStatus.dispatch.is_stbuf_empty_waiting ? Brushes.Green: Brushes.Red;
                label_Dispatch_Feedback_Stall.Background = pipelineStatus.dispatch_feedback_pack.stall ? Brushes.Green : Brushes.Red;
                label_IntIssue_Busy.Background = pipelineStatus.integer_issue.busy ? Brushes.Green : Brushes.Red;
                label_IntIssue_ALU0.Background = pipelineStatus.integer_issue.alu_idle[0] ? Brushes.Green : Brushes.Red;
                label_IntIssue_ALU0.Content = "ALU0: " + Global.Onehot2Binary(pipelineStatus.integer_issue.alu_idle[0] ? 0 : pipelineStatus.integer_issue.alu_idle_shift[0]);
                label_IntIssue_ALU1.Background = pipelineStatus.integer_issue.alu_idle[1] ? Brushes.Green : Brushes.Red;
                label_IntIssue_ALU1.Content = "ALU1: " + Global.Onehot2Binary(pipelineStatus.integer_issue.alu_idle[1] ? 0 : pipelineStatus.integer_issue.alu_idle_shift[1]);
                label_IntIssue_BRU.Background = pipelineStatus.integer_issue.bru_idle[0] ? Brushes.Green : Brushes.Red;
                label_IntIssue_BRU.Content = "BRU: " + Global.Onehot2Binary(pipelineStatus.integer_issue.bru_idle[0] ? 0 : pipelineStatus.integer_issue.bru_idle_shift[0]);
                label_IntIssue_CSR.Background = pipelineStatus.integer_issue.csr_idle[0] ? Brushes.Green : Brushes.Red;
                label_IntIssue_CSR.Content = "CSR: " + Global.Onehot2Binary(pipelineStatus.integer_issue.csr_idle[0] ? 0 : pipelineStatus.integer_issue.csr_idle_shift[0]);
                label_IntIssue_DIV.Background = pipelineStatus.integer_issue.div_idle[0] ? Brushes.Green : Brushes.Red;
                label_IntIssue_DIV.Content = "DIV: " + Global.Onehot2Binary(pipelineStatus.integer_issue.div_idle[0] ? 0 : pipelineStatus.integer_issue.div_idle_shift[0]);
                label_IntIssue_MUL0.Background = pipelineStatus.integer_issue.mul_idle[0] ? Brushes.Green : Brushes.Red;
                label_IntIssue_MUL0.Content = "MUL0: " + Global.Onehot2Binary(pipelineStatus.integer_issue.mul_idle[0] ? 0 : pipelineStatus.integer_issue.mul_idle_shift[0]);
                label_IntIssue_MUL1.Background = pipelineStatus.integer_issue.mul_idle[1] ? Brushes.Green : Brushes.Red;
                label_IntIssue_MUL1.Content = "MUL1: " + Global.Onehot2Binary(pipelineStatus.integer_issue.mul_idle[1] ? 0 : pipelineStatus.integer_issue.mul_idle_shift[1]);
                label_IntIssue_NextPortIndex.Content = "NextPortIndex: " + pipelineStatus.integer_issue.next_port_index;
                label_IntIssue_Feedback_Stall.Background = pipelineStatus.integer_issue_feedback_pack.stall ? Brushes.Green : Brushes.Red;
                label_IntIssue_OutFeedback.Background = Brushes.Red;

                for(var i = 0;i < pipelineStatus.integer_issue_output_feedback_pack.wakeup_valid.Length;i++)
                {
                    if(pipelineStatus.integer_issue_output_feedback_pack.wakeup_valid[i])
                    {
                        label_IntIssue_OutFeedback.Background = Brushes.Green;
                        break;
                    }
                }

                label_LSUIssue_Busy.Background = pipelineStatus.lsu_issue.busy ? Brushes.Green : Brushes.Red;
                label_LSUReadreg_Busy.Background = pipelineStatus.lsu_readreg.busy ? Brushes.Green : Brushes.Red;
                label_LSUReadreg_Feedback_Stall.Background = pipelineStatus.lsu_readreg_feedback_pack.stall ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_Idle.Background = pipelineStatus.commit_feedback_pack.idle ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_Next.Background = pipelineStatus.commit_feedback_pack.next_handle_rob_id_valid ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_Next.Content = "Next: " + pipelineStatus.commit_feedback_pack.next_handle_rob_id;
                label_Commit_Feedback_EXCPC.Background = pipelineStatus.commit_feedback_pack.has_exception ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_EXCPC.Content = "EXCPC: 0x" + string.Format("{0:X8}", pipelineStatus.commit_feedback_pack.exception_pc);
                label_Commit_Feedback_Flush.Background = pipelineStatus.commit_feedback_pack.flush ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_JumpEN.Background = pipelineStatus.commit_feedback_pack.jump_enable ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_JumpPC.Background = pipelineStatus.commit_feedback_pack.jump ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_JumpPC.Content = "JumpPC: 0x" + string.Format("{0:X8}", pipelineStatus.commit_feedback_pack.jump_next_pc);
                label_Commit_Feedback_I0.Background = pipelineStatus.commit_feedback_pack.committed_rob_id_valid[0] ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_I0.Content = "I0: " + pipelineStatus.commit_feedback_pack.committed_rob_id[0];
                label_Commit_Feedback_I1.Background = pipelineStatus.commit_feedback_pack.committed_rob_id_valid[1] ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_I1.Content = "I1: " + pipelineStatus.commit_feedback_pack.committed_rob_id[1];
                label_Commit_Feedback_I2.Background = pipelineStatus.commit_feedback_pack.committed_rob_id_valid[2] ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_I2.Content = "I2: " + pipelineStatus.commit_feedback_pack.committed_rob_id[2];
                label_Commit_Feedback_I3.Background = pipelineStatus.commit_feedback_pack.committed_rob_id_valid[3] ? Brushes.Green : Brushes.Red;
                label_Commit_Feedback_I3.Content = "I3: " + pipelineStatus.commit_feedback_pack.committed_rob_id[3];
                label_DIV_Busy.Background = pipelineStatus.execute.div[0].busy ? Brushes.Green : Brushes.Red;
                label_LU_L2_Stall.Background = pipelineStatus.execute.lu[0].l2_stall ? Brushes.Green : Brushes.Red;
                label_LU_L2_Addr.Content = "Addr: 0x" + string.Format("{0:X8}", pipelineStatus.execute.lu[0].l2_addr);
                
                listView_Fetch1_Fetch2.Items.Clear();

                for(var i = 0;i < pipelineStatus.fetch1_fetch2.Length;i++)
                {
                    var item = pipelineStatus.fetch1_fetch2[i];
                    listView_Fetch1_Fetch2.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, false, 0, item.inst_common_info)});
                }

                listView_Fetch2_Decode.Items.Clear();

                for(var i = 0;i < pipelineStatus.fetch2_decode.Length;i++)
                {
                    var item = pipelineStatus.fetch2_decode[i];
                    listView_Fetch2_Decode.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, false, 0, item.inst_common_info)});
                }

                listView_Decode_Rename.Items.Clear();

                for(var i = 0;i < pipelineStatus.decode_rename.Length;i++)
                {
                    var item = pipelineStatus.decode_rename[i];
                    listView_Decode_Rename.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, false, 0, item.inst_common_info)});
                }

                listView_Rename_Dispatch.Items.Clear();

                for(var i = 0;i < pipelineStatus.rename_dispatch.Length;i++)
                {
                    var item = pipelineStatus.rename_dispatch[i];
                    listView_Rename_Dispatch.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Dispatch_Integer_Issue.Items.Clear();

                for(var i = 0;i < pipelineStatus.dispatch_integer_issue.Length;i++)
                {
                    var item = pipelineStatus.dispatch_integer_issue[i];
                    listView_Dispatch_Integer_Issue.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Dispatch_LSU_Issue.Items.Clear();

                for(var i = 0;i < pipelineStatus.dispatch_lsu_issue.Length;i++)
                {
                    var item = pipelineStatus.dispatch_lsu_issue[i];
                    listView_Dispatch_LSU_Issue.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Integer_Issue_Input.Items.Clear();

                for(var i = 0;i < pipelineStatus.integer_issue.hold_rev_pack.Length;i++)
                {
                    var item = pipelineStatus.integer_issue.hold_rev_pack[i];
                    listView_Integer_Issue_Input.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_LSU_Issue_Input.Items.Clear();

                for(var i = 0;i < pipelineStatus.lsu_issue.hold_rev_pack.Length;i++)
                {
                    var item = pipelineStatus.lsu_issue.hold_rev_pack[i];
                   listView_LSU_Issue_Input.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Integer_Issue_Readreg.Items.Clear();

                for(var i = 0;i < pipelineStatus.integer_issue_readreg.Length;i++)
                {
                    var item = pipelineStatus.integer_issue_readreg[i];
                    listView_Integer_Issue_Readreg.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_LSU_Issue_Readreg.Items.Clear();

                for(var i = 0;i < pipelineStatus.lsu_issue_readreg.Length;i++)
                {
                    var item = pipelineStatus.lsu_issue_readreg[i];
                    listView_LSU_Issue_Readreg.Items.Add(new{Highlight = false, Value = getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Integer_Readreg_Execute.Items.Clear();

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.alu.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.alu[i];
                    listView_Integer_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.integer_readreg_execute.alu, ID = i, Value = "ALU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.bru.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.bru[i];
                    listView_Integer_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.integer_readreg_execute.bru, ID = i, Value = "BRU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.csr.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.csr[i];
                    listView_Integer_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.integer_readreg_execute.csr, ID = i, Value = "CSR" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.div.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.div[i];
                    listView_Integer_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.integer_readreg_execute.div, ID = i, Value = "DIV" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.integer_readreg_execute.mul.Length;i++)
                {
                    var item = pipelineStatus.integer_readreg_execute.mul[i];
                    listView_Integer_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.integer_readreg_execute.mul, ID = i, Value = "MUL" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_LSU_Readreg_Execute.Items.Clear();

                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.lu.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.lu[i];
                    listView_LSU_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.lsu_readreg_execute.lu, ID = i, Value = "LU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.sau.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.sau[i];
                    listView_LSU_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.lsu_readreg_execute.sau, ID = i, Value = "SAU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.lsu_readreg_execute.sdu.Length;i++)
                {
                    var item = pipelineStatus.lsu_readreg_execute.sdu[i];
                    listView_LSU_Readreg_Execute.Items.Add(new{Highlight = false, List = pipelineStatus.lsu_readreg_execute.sdu, ID = i, Value = "SDU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_Execute_WB.Items.Clear();

                for(var i = 0;i < pipelineStatus.execute_wb.alu.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.alu[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.alu, ID = i, Value = "ALU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                for(var i = 0;i < pipelineStatus.execute_wb.bru.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.bru[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.bru, ID = i, Value = "BRU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                for(var i = 0;i < pipelineStatus.execute_wb.csr.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.csr[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.csr, ID = i, Value = "CSR" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_wb.div.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.div[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.div, ID = i, Value = "DIV" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_wb.mul.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.mul[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.mul, ID = i, Value = "MUL" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_wb.lu.Length;i++)
                {
                    var item = pipelineStatus.execute_wb.lu[i];
                    listView_Execute_WB.Items.Add(new{Highlight = false, List = pipelineStatus.execute_wb.lu, ID = i, Value = "LU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                listView_Execute_Commit.Items.Clear();
                
                for(var i = 0;i < pipelineStatus.execute_commit.alu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.alu[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.alu, ID = i, Value = "ALU" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                for(var i = 0;i < pipelineStatus.execute_commit.bru.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.bru[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.bru, ID = i, Value = "BRU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }
                
                for(var i = 0;i < pipelineStatus.execute_commit.csr.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.csr[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.csr, ID = i, Value = "CSR" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_commit.div.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.div[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.div, ID = i, Value = "DIV" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_commit.mul.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.mul[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.mul, ID = i, Value = "MUL" + i + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_commit.lu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.lu[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.lu, ID = i, Value = "LU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_commit.sau.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.sau[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.sau, ID = i, Value = "SAU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                for(var i = 0;i < pipelineStatus.execute_commit.sdu.Length;i++)
                {
                    var item = pipelineStatus.execute_commit.sdu[i];
                    listView_Execute_Commit.Items.Add(new{Highlight = false, List = pipelineStatus.execute_commit.sdu, ID = i, Value = "SDU" + ": " + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                }

                listView_ROB.Items.Clear();

                for(var i = 0;i < pipelineStatus.rob.Length;i++)
                {
                    var item = pipelineStatus.rob[i];
                    listView_ROB.Items.Add(new{Highlight = false, Value = item.rob_id + ":" + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info) + "(" + (item.finish ? "Finished" : "Unfinish") + ")"});
                }
                
                listView_Integer_Issue_Queue.Items.Clear();

                for(var i = 0;i < pipelineStatus.integer_issue.issue_q.value.Length;i++)
                {
                    if(pipelineStatus.integer_issue.issue_q.valid[i])
                    {
                        var item = pipelineStatus.integer_issue.issue_q.value[i];
                        var ready_string = "<";

                        if(pipelineStatus.integer_issue.src1_ready[i])
                        {
                            ready_string += "ready";
                        }
                        else
                        {
                            ready_string += Global.Onehot2Binary(pipelineStatus.integer_issue.wakeup_shift_src1[i]);
                        }

                        ready_string += ",";

                        if(pipelineStatus.integer_issue.src2_ready[i])
                        {
                            ready_string += "ready";
                        }
                        else
                        {
                            ready_string += Global.Onehot2Binary(pipelineStatus.integer_issue.wakeup_shift_src2[i]);
                        }

                        ready_string += ">";
                        listView_Integer_Issue_Queue.Items.Add(new{Highlight = false, ID = i, Value = i + ": " + ready_string + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                    }
                }

                listView_LSU_Issue_Queue.Items.Clear();

                for(var i = 0;i < pipelineStatus.lsu_issue.issue_q.value.Length;i++)
                {
                    if(pipelineStatus.lsu_issue.issue_q.valid[i])
                    {
                        var item = pipelineStatus.lsu_issue.issue_q.value[i];
                        var ready_string = "<";

                        if(pipelineStatus.lsu_issue.src1_ready[i])
                        {
                            ready_string += "ready";
                        }
                        else
                        {
                            ready_string += Global.Onehot2Binary(pipelineStatus.lsu_issue.wakeup_shift_src1[i]);
                        }
                        
                        ready_string += ",";

                        if(pipelineStatus.lsu_issue.src2_ready[i])
                        {
                            ready_string += "ready";
                        }
                        else
                        {
                            ready_string += Global.Onehot2Binary(pipelineStatus.lsu_issue.wakeup_shift_src2[i]);
                        }

                        ready_string += ">";
                        listView_LSU_Issue_Queue.Items.Add(new{Highlight = false, ID = i, Value = i + ": " + ready_string + getDisplayText(item.Instruction, item.pc, true, item.rob_id, item.inst_common_info)});
                    }
                }
                
                {
                    var cur_index = listView_RAT.SelectedIndex;
                    listView_RAT.Items.Clear();

                    for(var i = 0;i < pipelineStatus.retire_rat.valid.Length;i++)
                    {
                        listView_RAT.Items.Add(new{Highlight = false, Value = i + ": x" + pipelineStatus.speculative_rat.value[i] + (pipelineStatus.speculative_rat.valid[i] ? "<valid>" : "<invalid>") + ", x" +
                                                                              pipelineStatus.retire_rat.value[i] + (pipelineStatus.retire_rat.valid[i] ? "<valid>" : "<invalid>") + ", " +
                                                                              "0x" + string.Format("{0:X8}", pipelineStatus.phy_regfile.value[i]) + (pipelineStatus.phy_regfile.valid[i] ? "<valid>" : "<invalid>")});
                    }

                    if(cur_index < pipelineStatus.retire_rat.valid.Length)
                    {
                        listView_RAT.SelectedIndex = cur_index;
                    }
                }
            }
        }
        #pragma warning restore CS8602

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Maximized;
        }
        private void Window_Closed(object sender, EventArgs e)
        {
            PipelineStatusWindow.instance = null;
        }
        private void Window_LocationChanged(object sender, EventArgs e)
        {
            Config.Set("PipelineStatus_Left", "" + Left);
            Config.Set("PipelineStatus_Top", "" + Top);
        }

        #pragma warning disable CS8602
        private void HighlightROBItem(uint robID, bool highlight)
        {
            try
            {
                for(var i = 0;i < listView_ROB.Items.Count;i++)
                {
                    var item = (dynamic)listView_ROB.Items[i];

                    if(item.Highlight ^ ((pipelineStatus.rob[i].rob_id == robID) && highlight))
                    {
                        listView_ROB.Items[i] = new{Highlight = (pipelineStatus.rob[i].rob_id == robID) && highlight, Value = item.Value};
                    }
                }
            }
            catch
            {
                
            }
        }

        private void DisplayDetail(object? obj)
        {
            #pragma warning disable CS8601
            #pragma warning disable CS8625
            objectInTreeView_Detail.ObjectToVisualize = null;
            objectInTreeView_Detail.ObjectToVisualize = obj;
            #pragma warning restore CS8601
            #pragma warning restore CS8625
        }

        private void listView_Fetch1_Fetch2_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;
            HighlightROBItem(0, false);

            try
            {
                if(id >= 0)
                {
                    DisplayDetail(pipelineStatus.fetch1_fetch2[id]);
                }
                else
                {
                    DisplayDetail(null);
                }
            }
            catch
            {
                DisplayDetail(null);
            }
        }

        private void listView_Fetch2_Decode_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;
            HighlightROBItem(0, false);

            try
            {
                if(id >= 0)
                {
                    DisplayDetail(pipelineStatus.fetch2_decode[id]);
                }
                else
                {
                    DisplayDetail(null);
                }
            }
            catch
            {
                DisplayDetail(null);
            }
        }

        private void listView_Decode_Rename_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;
            HighlightROBItem(0, false);

            try
            {
                if(id >= 0)
                {
                    DisplayDetail(pipelineStatus.decode_rename[id]);
                }
                else
                {
                    DisplayDetail(null);
                }
            }
            catch
            {
                DisplayDetail(null);
            }
        }

        private void listView_Rename_Dispatch_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.rename_dispatch[id].rob_id, true);
                    DisplayDetail(pipelineStatus.rename_dispatch[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Dispatch_Integer_Issue_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.dispatch_integer_issue[id].rob_id, true);
                    DisplayDetail(pipelineStatus.dispatch_integer_issue[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Dispatch_LSU_Issue_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.dispatch_lsu_issue[id].rob_id, true);
                    DisplayDetail(pipelineStatus.dispatch_lsu_issue[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Integer_Issue_Input_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.integer_issue.hold_rev_pack[id].rob_id, true);
                    DisplayDetail(pipelineStatus.integer_issue.hold_rev_pack[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_LSU_Issue_Input_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.lsu_issue.hold_rev_pack[id].rob_id, true);
                    DisplayDetail(pipelineStatus.lsu_issue.hold_rev_pack[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Integer_Issue_Readreg_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;


            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.integer_issue_readreg[id].rob_id, true);
                    DisplayDetail(pipelineStatus.integer_issue_readreg[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_LSU_Issue_Readreg_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    HighlightROBItem(pipelineStatus.lsu_issue_readreg[id].rob_id, true);
                    DisplayDetail(pipelineStatus.lsu_issue_readreg[id]);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Integer_Readreg_Execute_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = ((dynamic)list.Items[id]).List[((dynamic)list.Items[id]).ID] as integer_readreg_execute_op_info;
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_LSU_Readreg_Execute_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = ((dynamic)list.Items[id]).List[((dynamic)list.Items[id]).ID] as lsu_readreg_execute_op_info;
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Execute_WB_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = ((dynamic)list.Items[id]).List[((dynamic)list.Items[id]).ID] as execute_wb_op_info;
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_Execute_Commit_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = ((dynamic)list.Items[id]).List[((dynamic)list.Items[id]).ID] as execute_commit_op_info;
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_ROB_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                var list = sender as ListView;
                var id = list.SelectedIndex;

                if(id >= 0)
                {
                    var item = pipelineStatus.rob[id];
                    DisplayDetail(item);
                }
            }
            catch
            {
                DisplayDetail(null);
            }
        }

        private void listView_Integer_Issue_Queue_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = pipelineStatus.integer_issue.issue_q.value[((dynamic)list.Items[id]).ID];
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }

        private void listView_LSU_Issue_Queue_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var list = sender as ListView;
            var id = list.SelectedIndex;

            try
            {
                if(id >= 0)
                {
                    var item = pipelineStatus.lsu_issue.issue_q.value[((dynamic)list.Items[id]).ID];
                    HighlightROBItem(item.rob_id, true);
                    DisplayDetail(item);
                }
                else
                {
                    HighlightROBItem(0, false);
                    DisplayDetail(null);
                }
            }
            catch
            {
                HighlightROBItem(0, false);
                DisplayDetail(null);
            }
        }
        #pragma warning restore CS8602

        private void label_History_Prev_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if(curHistoryIndex > 0)
            {
                curHistoryIndex--;
                loadHistory();
            }
        }

        private void label_History_Next_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if(curHistoryIndex < (pipelineStatusHistory.Count() - 1))
            {
                curHistoryIndex++;
                loadHistory();
            }
        }

        private void Left_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            if(curHistoryIndex > 0)
            {
                curHistoryIndex--;
                loadHistory();
            }
        }

        private void Right_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            if(curHistoryIndex < (pipelineStatusHistory.Count() - 1))
            {
                curHistoryIndex++;
                loadHistory();
            }
        }
    }
}
