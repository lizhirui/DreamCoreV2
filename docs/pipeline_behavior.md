# 流水线行为描述

## Fetch1级

该流水级负责从总线获取指令，进行指令预译码，同时生成下一周期的PC，并将新的PC送到总线

在正常状态下（无来自commit流水级的flush信号，非跳转等待状态，无来自fetch2的stall信号，非总线等待状态）：

* 若所有的指令都不是fence.i指令及跳转分支指令（一切影响PC的指令），则令PC = PC + 4 * 4
* 若第i条指令是fence.i指令，则i必须等于0，并且fetch2/decode/rename/commit流水级必须都处于空闲状态，且Store Buffer必须为空，否则该条指令及后续的指令都会被放弃，PC = PC + i * 4
* 若第i条指令是跳转分支指令，则进入跳转等待状态，同时PC = PC + i * 4 + 4

在跳转等待状态下（无来自commit流水级的flush信号）：

若commit反馈包中包含了跳转信息，则解除跳转等待状态，同时若commit_feedback_pack.jump为真，则PC = commit_feedback_pack.next_pc，否则PC保持不变

在总线等待状态及有来自fetch2的stall信号情况下：

在这些情况下，该流水级什么都不做

收到来自commit流水级的flush信号时：

清除跳转等待状态， 若commit_feedback_pack.has_exception为真，则令PC = commit_feedback_pack.exception_pc，否则若commit_feedback_pack.jump_enable为真，则令PC = commit_feedback_pack.next_pc（此时commit_feedback_pack.jump必为真）

未来目标：

在该流水级会加入BTB和RAS，进行最简单的跳转目标预测，同时根据预译码的结果，将分支预测请求送给GShare预测器

## Fetch2级

该流水级目前只负责将从Fetch1级送来的指令推入fetch2_decode_fifo

在正常状态下（无来自commit流水级的flush信号，不在繁忙状态）：

从fetch1_fetch2_port取得新指令，如果fetch2_decode_fifo的空间足够放开所有来自fetch1级的指令，则将这些指令全部推入fifo，否则将剩余的指令暂存在本流水级，并标记流水级状态为繁忙状态，同时生成stall反馈信号

在繁忙状态下（无来自commit流水级的flush信号）：

使用本流水级暂存的剩余指令，如果fetch2_decode_fifo的空间足够放开所有来自fetch1级的指令，则将这些指令全部推入fifo，同时解除流水级的繁忙状态，否则将剩余的指令暂存在本流水级，并标记流水级状态为繁忙状态，同时生成stall反馈信号

收到来自commit流水级的flush信号时：

清除处理器的繁忙状态，并令fetch2_decode_fifo执行flush操作

反馈信号产生：若当前状态是繁忙状态，则产生stall信号

## Decode级

该流水级负责从fetch2_decode_fifo中获得指令，进行译码，并将译码结果送入decode_rename_fifo

## Rename级

该流水级负责从decode_rename_fifo中获得指令，进行重命名过程，并将重命名结果送入rename_dispatch_port

在正常状态下（无来自commit流水级的flush信号，无来自dispatch的stall信号）：

从decode_rename_fifo取得新指令，若phy_id_free_list中包含空闲物理寄存器，同时rob有空闲项，则执行重命名操作

重命名操作如下：

* 将phy_id_free_list当前的读指针写入rob_item中（物理寄存器分配前读指针，用于中断现场恢复）
* 获取待重命名寄存器对应的旧物理寄存器编号
* 从phy_id_free_list中取得新的物理寄存器编号
* 将新的映射关系写入Speculative RAT
* 将phy_id_free_list当前的读指针写入rob_item中（物理寄存器分配后读指针，用于异常和分支预测失败现场恢复）
* 填充rob_item的剩余项

取得新指令的特殊规则：

* 若当前指令为csr/mret指令，分为两种情况：情况一，若该指令位于本次发送包的第一项，则该包中后续都不得包含其它指令；情况二：若该指令不位于本次发送包的第一项，则该包中不得包含该指令及后续的所有指令
* 若当前指令为fence指令，则该包中后续都不得包含LSU指令

在有来自dispatch的stall信号时（无来自commit流水级的flush信号）：

什么也不做

收到来自commit流水级的flush信号时：

向rename_dispatch_port中写入空包

## Dispatch级

该流水级负责从rename_dispatch_port中获得指令，进行分发，将整数指令送入dispatch_integer_issue_port，将LSU指令送入dispatch_lsu_issue_port

在正常状态下（无来自commit流水级的flush信号，不处于指令等待状态或Store Buffer空等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态，可能处于流水级繁忙状态）：

* 若不处于流水级繁忙状态，从rename_dispatch_port中获得指令，否则使用上一个周期获取到的指令，根据指令所属队列分配到integer_issue_pack与lsu_issue_pack中，每个pack的宽度都与rename_dispatch_port宽度一致，以保证送来的指令能够被一次性分发
* 若遇到csr/mret指令，则处理分为两种情况
  * 若integer_issue不处于堵塞状态，且该指令是commit流水级的下一个待处理项，则将该指令送入dispatch_integer_issue_port并置该流水级为指令等待状态，等待该指令的完成后再分发下一个指令包
  * 若不满足上述情况，则向dispatch_integer_issue_port与dispatch_lsu_issue_port送空包，置流水级繁忙状态，暂停本轮整数指令与LSU指令分发
* 若遇到fence指令，并且有整数指令要分发但是integer_issue处于阻塞状态，或者有LSU指令要分发但是lsu_issue处于阻塞状态，则向dispatch_integer_issue_port与dispatch_lsu_issue_port送空包，置流水级繁忙状态，暂停本轮整数指令与LSU指令分发
* 若不满足上述情况，则执行如下流程：
  * 若遇到fence指令，则置该流水级为指令等待状态及Store Buffer空等待状态，等待该指令的完成及Store Buffer为空后再分发下一个指令包
  * 若需要分发整数指令，则分为两种情况：
    * 若integer_issue不处于堵塞状态，则将该指令送入dispatch_integer_issue_port
    * 否则，将integer_issue_pack暂存到hold_integer_issue_pack中，并置整数队列繁忙标志有效
  * 若不需要分发整数指令，则将空包送入dispatch_integer_issue_port
  * 若需要分发LSU指令，则分为两种情况：
    * 若lsu_issue不处于堵塞状态，则将该指令送入dispatch_lsu_issue_port
    * 否则，将lsu_issue_pack暂存到hold_lsu_issue_pack中，并置LSU队列繁忙标志有效
  * 若不需要分发LSU指令，则将空包送入dispatch_lsu_issue_port

在指令等待状态下（无来自commit流水级的flush信号，可能同时处于Store Buffer空等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态）：

* 将空包送入dispatch_integer_issue_port与dispatch_lsu_issue_port
* 若等待的指令已经成为commit流水级的下一个待处理项，则解除指令等待状态，同时若处于Store Buffer空等待状态且Store Buffer为空，则解除Store Buffer空等待状态
* 若等待的指令仍没有成为commit流水级的下一个待处理项，则什么都不做

在Store Buffer空等待状态下（无来自commit流水级的flush信号，不处于指令等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态）：

* 将空包送入dispatch_integer_issue_port与dispatch_lsu_issue_port
* 若Store Buffer为空，则解除Store Buffer空等待状态，否则什么都不做

**可以注意到，指令等待状态比Store Buffer空等待状态优先级更高，这是因为fence指令前可能存在LSU指令，这些LSU指令执行完之后Store Buffer就会为空，但是此时还没有执行到fence指令，使得fence指令没有成功阻断LSU指令提前fence指令执行，因此需要先等待fence指令退休，再同时或在之后等待Store Buffer为空**

在整数队列繁忙或LSU队列繁忙等待状态下（无来自commit流水级的flush信号，不处于指令等待状态或Store Buffer空等待状态，该状态的包不可能包含csr/mret或fence指令）：

* 分别从hold_integer_issue_pack与hold_lsu_issue_pack中取出保留的指令包
* 若需要分发整数指令，则分为两种情况：
    * 若integer_issue不处于堵塞状态，则将该指令送入dispatch_integer_issue_port
    * 否则，将integer_issue_pack暂存到hold_integer_issue_pack中，并置整数队列繁忙标志有效
  * 若不需要分发整数指令，则将空包送入dispatch_integer_issue_port
  * 若需要分发LSU指令，则分为两种情况：
    * 若lsu_issue不处于堵塞状态，则将该指令送入dispatch_lsu_issue_port
    * 否则，将lsu_issue_pack暂存到hold_lsu_issue_pack中，并置LSU队列繁忙标志有效
  * 若不需要分发LSU指令，则将空包送入dispatch_lsu_issue_port

收到来自commit流水级的flush信号时：

* 将空包送入dispatch_integer_issue_port与dispatch_lsu_issue_port
* 解除指令队列繁忙与LSU队列繁忙等待状态
* 解除流水级繁忙状态
* 清除所有的保留包
* 解除指令等待状态
* 解除Store Buffer空等待状态

反馈信号产生：若当前状态为整数队列繁忙等待状态或LSU队列繁忙等待状态或流水级繁忙状态，则产生stall信号

## Issue级

### Integer Issue级

### LSU Issue级

## Readreg级

### Integer Readreg级

### LSU Readreg级

## Execute级

### ALU

### BRU

### CSR

### DIV

### LSU

### MUL

## WB级

## Commit级