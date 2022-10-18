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

该流水级实际分为两级：输入级与输出级

输入级行为：

* 若当前处于繁忙状态，则直接使用之前暂存的指令包，否则从dispatch_integer_issue_port接收新的指令包
* 从Physical Register File中读取寄存器的有效状态，同时分析来自execute/wb流水级的反馈包，若其中有一个寄存器匹配某个源寄存器，或该指令符合下述条件，则该源操作数标记为ready状态，否则为not ready状态：
  * 指令无效
  * 指令发生了异常
  * 源操作数不需要映射（为x0或立即数或不存在该源操作数）
* 对于ALU/MUL指令，需要进行仲裁出口分配，这里使用Round-Robin算法进行分配
* 将指令的所属执行单元类型转换为one-hot单独保留用于仲裁
* 单独保存指令的rob_id/rob_id_stage用于年龄计算
* 单独保存指令的rd/rd_valid用于更短路径的唤醒
* 计算每条指令的wakeup_shift，公式为(LATENCY == 1) ? 0 : (1 << (LATENCY - 1))
* 计算执行单元的繁忙延迟（即在几个周期后繁忙，公式为(LATENCY == 1) ? 0 : (1 << 1)）和空闲延迟（即在几个周期后繁忙，公式为(LATENCY == 1) ? 0 : (1 << (LATENCY - 1))），避免仲裁时从反馈网络获得该信息造成过大的组合逻辑延迟

若从commit流水级收到了flush信号，则会重置所有的状态变量到复位值，同时对发射队列执行flush操作

反馈信号产生：若当前状态为繁忙状态，则产生stall信号

公式推导：

* wakeup_shift公式：
  * 考虑到流水线是如下结构：
  * issue -> readreg -> execute
  * 并且只可能在execute处发生停顿，因此，若流水线不发生停顿（LATENCY = 1的情形），则关联的指令可以直接背靠背发射，不需要提前唤醒，而是直接唤醒即可，因此wakeup_shift = 0，而若流水线发生停顿（LATENCY > 1的情形），则关联的指令只需要相应地延迟LATENCY - 1个周期后发射即可
  * 1 << x 只是二进制码到one-hot编码的变换公式，这里之所以是x而不是x - 1是因为，当该one-hot为1时，就会开始执行唤醒操作，并在下一个周期完成唤醒标志位的变化，而唤醒是提前发射一个周期的，因此，这里的x实际上是当前到唤醒时的周期间隔，而不是当前到发射时的周期间隔
* 繁忙延迟公式：
  * 考虑如下的情形：
  * issue -> readreg -> execute
  * T0 A -> [none] -> [none]
  * T1 B -> A -> [none]
  * T2 C -> B -> A
  * 和上面同样的流水线结构，显然，若执行单元的LATENCY = 1，执行单元也就不存在繁忙的可能性，而若LATENCY > 1，执行单元将会在T2时刻变为繁忙状态，因此将繁忙延迟设置为1，对应的one-hot则为1 << 1
* 空闲延迟公式：
  * 流水线情形和上述情况类似，显然，若执行单元的LATENCY = 1，执行单元也就不存在繁忙的可能性，而若LATENCY > 1，执行单元的繁忙持续时间必然是LATENCY - 1，因此将空闲延迟设置为LATENCY - 1，对应的one-hot则为1 << (LATENCY - 1)

输出级负责利用仲裁算法从发射队列中选出至多两条指令，并送入integer_issue_readreg_port

仲裁算法：

* 该仲裁算法具有两个仲裁出口，出口1负责ALU0/MUL0/CSR/BRU执行单元，出口2负责ALU1/MUL1/DIV执行单元
* 计算每一个执行单元未来的繁忙状态
* 对于每一个仲裁出口，比较每一条指令的ready信号和年龄信息，选出一条已ready的最老的指令，并且该指令所属执行单元未来不能是繁忙的

在完成仲裁后，会将指令的目的寄存器编号广播到整数指令队列和LSU指令队列中，执行提前唤醒操作

提前唤醒设计：

* 在发射一条指令时，将目标寄存器进行广播，队列内的每个有效指令项收到广播后，会检查该指令的源寄存器是否与广播的目标寄存器相同，若相同，则将该指令该操作数对应的wakeup_shift_src设为待发射指令的延迟信息（采用one-hot编码，以实现移位寄存器）
* 每一个wakeup_shift_src在每周期都会右移一位
* 若某一操作数的wakeup_shift_src当前为1，则置对应的src_ready为1

正常唤醒设计：

* 对于某些不支持提前唤醒的指令，如LSU指令，需要在接收到来自execute级的反馈包时，将对应的src_ready置为1

可以注意到上面在输入级的时候，需要同时判断execute/wb流水级的反馈包，而在正常唤醒时，只需要判断execute级的反馈包，这是由于以下原因：

* 在当I1位于输入级时可以观测到如下的当前和未来指令流水状态（其中I1是当前待进入发射队列的指令，I2与I3则比I1的年龄老，且I2已经完成执行输出了正常的反馈包，I3也输出了正常的反馈包）：
  * I1 input -> output -> rf
  * I2 execute -> wb -> commit
  * I3 wb -> commit
* 若I1在输入级不使用execute流水级的反馈包，则input可能需要在下一个周期才被唤醒，从而延迟一个周期发射
* 而在正常唤醒时，若被唤醒的指令是I1指令，则执行唤醒操作的I2指令若当前在wb级，则上一个周期必然在execute级，此时I1指令必然在输入级或发射队列中，在输入级时，已经处理过了来自execute流水级的反馈包，而如果在发射队列中，由于该指令在execute级，所以必然已经唤醒了I1指令，因此没必要处理来自wb流水级的反馈包，这样也减少了反馈电路的复杂度，有助于频率的提升

### LSU Issue级

## Readreg级

### Integer Readreg级

该流水级首先从integer_issue_readreg_port中读入指令包（该指令包的几条指令分别对应相应的发射端口，顺序不可错），负责从Physical Register File/execute feedback/wb feedback读入源操作数，然后发送到相应的执行单元端口（根据指令在指令包中的位置以及指令类型）

如果收到了来自commit流水级的flush请求，则对全部执行单元的handshake_dff执行flush操作

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