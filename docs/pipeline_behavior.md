# 流水线行为描述

## Fetch1级

该流水级负责从总线获取指令，进行指令预译码，同时生成下一周期的PC，并将新的PC送到总线

在正常状态下（无来自commit流水级的flush信号，非跳转等待状态，无来自fetch2的stall信号，非总线等待状态）：

* 若所有的指令都不是fence.i指令及跳转分支指令（一切影响PC的指令），则令PC = PC + 4 * 4
* 若第i条指令是fence.i指令，则i必须等于0，并且fetch2/decode/rename/commit流水级必须都处于空闲状态，且Store Buffer必须为空，否则该条指令**及**后续的指令都会被放弃，PC = PC + i * 4，若满足上述条件，则该条指令**后续**的指令都会被放弃，PC = PC + i * 4 + 4
* 若第i条指令是跳转分支指令，则按照如下策略进行：
  * 首先对所有预测器（L0_BTB、L1_BTB、bi_modal、bi_mode）发起预测请求
  * 如果是jal指令，则直接预译码计算得出下一地址，如果满足rd_is_link条件，则同时将PC + 4压入RAS（直接跳转处理流程）
  * 如果是jalr指令，则按照如下策略进行：
    * 如果满足rd_is_link条件，则：
      * 如果满足rs1_is_link条件，则：
        * 如果rs1与rd相等，则从L0_BTB获得下一地址，并将PC + i * 4 + 4压入RAS（call处理流程）
        * 否则，从RAS弹出下一地址，并将PC + i * 4 + 4压入RAS（协程上下文切换处理流程）
      * 否则，从L0_BTB获得下一地址，并将PC + i * 4 + 4压入RAS（call处理流程）
    * 否则，按照如下策略进行：
      * 如果rs1_is_link条件满足，则从RAS弹出下一地址（ret处理流程）
      * 否则，从L0_BTB获得下一地址（普通间接跳转处理流程）
  * 如果是条件分支指令，则从bi_modal预测器获取跳转方向，并结合预译码结果计算下一地址，同时将bi_mode预测器当前的global_history填入指令信息中，用于后续的bi_mode预测器GHR修复用
  * 如果不满足以上所有条件，则该分支属于不可预测分支（如mret），进入跳转等待状态，同时PC = PC + i * 4 + 4，同时终止后续的指令处理
  * 如果分支属于可预测分支，且预测器给出的下一地址正好是下一条指令的地址，则直接令PC = PC + i * 4 + 4，否则令PC为预测的下一地址，并终止后续的指令处理
* 遍历所有已预测指令，如果分支属于条件分支，则同时对所有的预测器执行推测更新，推测更新历史记录以提升后续的预测精度（注意，这次的历史记录更新结果不用于本周期预测，而是用于下一周期的预测，因为尽管预测的4条指令中存在多条指令都跳转的情况，但是这种情况应该是极少的，而较多的情况应该是存在1条或2条分支指令，而在存在2条分支指令的情况下，第1条分支指令大概率是不跳的，因此对于第2条分支指令来说，最新的历史记录位一般是0，而这样似乎没有引入太多的信息，根据某些文献显示，引入上一条分支指令的分支结果得到的收益似乎可以忽略）

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

在读入新指令后，对所有指令进行一次筛选过滤，对于其中的所有可预测分支指令，从二级预测器（L1_BTB及bi_mode）中获得预测结果，如果发现预测结果和一级预测器的结果不同，则对一级预测器进行更新，并对所有预测器执行历史记录恢复操作并重新进行推测更新，同时将该预测失败之后的所有指令全部无效化，并向Fetch1传递PC重定向请求，从新的预测下一地址开始取指。

在繁忙状态下（无来自commit流水级的flush信号）：

使用本流水级暂存的剩余指令，如果fetch2_decode_fifo的空间足够放开所有来自fetch1级的指令，则将这些指令全部推入fifo，同时解除流水级的繁忙状态，否则将剩余的指令暂存在本流水级，并标记流水级状态为繁忙状态，同时生成stall反馈信号

收到来自commit流水级的flush信号时：

清除处理器的繁忙状态，并令fetch2_decode_fifo执行flush操作

反馈信号产生：若当前状态是繁忙状态，则产生stall信号

## Decode级

该流水级负责从fetch2_decode_fifo中获得指令，进行译码，并将译码结果送入decode_rename_fifo

这里将Load指令全部分配到LU，Store指令则暂时分配到SDU，在指令发射时，Store指令将会被拆解成SAU和SDU指令

## Rename级

该流水级负责从decode_rename_fifo中获得指令，进行重命名过程，并将重命名结果送入rename_dispatch_port

在正常状态下（无来自commit流水级的flush信号，无来自dispatch的stall信号）：

从decode_rename_fifo取得新指令，若phy_id_free_list中包含空闲物理寄存器，同时rob有空闲项，则执行重命名操作

重命名操作如下：

* 将phy_id_free_list当前的读指针写入rob_item中（物理寄存器分配前读指针，用于中断现场恢复和Store/Load违例恢复，即本条指令和后续指令都需要撤销的场景）
* 获取待重命名寄存器对应的旧物理寄存器编号
* 从phy_id_free_list中取得新的物理寄存器编号
* 将新的映射关系写入Speculative RAT
* 将phy_id_free_list当前的读指针写入rob_item中（物理寄存器分配后读指针，用于异常和分支预测失败现场恢复，即仅后续指令需要撤销的场景）
* 填充rob_item的剩余项

如果该指令是一条Load指令，还需要做如下操作：

* 记录Load Queue当前的写指针，存储到ROB中
* 将该写指针作为该Load指令的Load Queue ID，写入发送包中
* 将空白项写入Load Queue中该指令对应的位置

如果该指令是一条Store指令，还需要做如下操作：

* 创建一条新的Store Buffer项，写入当前指令的ROB编号用于后续年龄判断
* 将上述的Store Buffer项号写入发送包中

对于被预测的条件分支指令和Load指令，还需要进行Checkpoint分配操作（前提是Checkpoint Buffer非满，若已满则不分配）：

* 将Speculative RAT的Valid位写入Checkpoint
* 将Free List的旧读指针（本次物理寄存器分配前读指针）和新读指针（本次物理寄存器分配后读指针）
* 将Checkpoint Buffer的旧写指针（即本条Checkpoint写入前的写指针）和新写指针（即本条Checkpoint写入后的写指针）写入Checkpoint
* 将Load Queue的旧写指针（即该Load指令对应的Load Queue项被写入前的Load Queue写指针）写入Checkpoint
* 将Checkpoint写入Checkpoint Buffer
* 在发送包中标记Checkpoint有效，并写入Checkpoint ID

取得新指令的特殊规则：

* 若当前指令为csr/mret指令，分为两种情况：情况一，若该指令位于本次发送包的第一项，则该包中后续都不得包含其它指令；情况二：若该指令不位于本次发送包的第一项，则该包中不得包含该指令及后续的所有指令
* 若当前指令为fence指令，则该包中后续都不得包含LSU指令

在有来自dispatch的stall信号时（无来自commit流水级的flush信号）：

什么也不做

收到来自commit流水级的flush信号时：

向rename_dispatch_port中写入空包

## Dispatch级

该流水级负责从rename_dispatch_port中获得指令，进行分发，将整数指令送入dispatch_integer_issue_port，将LSU指令送入dispatch_lsu_issue_port

在正常状态下（无来自commit流水级的flush信号，不处于指令等待状态或Store Buffer空等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态，可能处于流水级繁忙状态，可能有来自BRU或SAU的flush信号）：

* 若不处于流水级繁忙状态，从rename_dispatch_port中获得指令，否则使用上一个周期获取到的指令，根据指令所属队列分配到integer_issue_pack与lsu_issue_pack中，每个pack的宽度都与rename_dispatch_port宽度一致，以保证送来的指令能够被一次性分发
* 若收到了来自BRU或SAU的flush信号，则丢弃所有收到的新指令，否则执行下面的流程
* 若遇到csr/mret指令，则处理分为两种情况
  * 若integer_issue不处于堵塞状态，且该指令是commit流水级的下一个待处理项，则将该指令送入dispatch_integer_issue_port并置该流水级为指令等待状态，等待该指令的完成后再分发下一个指令包，同时若lsu_issue不处于堵塞状态，则向dispatch_lsu_issue_port送空包，否则保持其不变
  * 若不满足上述情况，则向dispatch_integer_issue_port（仅当integer_issue不处于堵塞状态时，否则保持不变）与dispatch_lsu_issue_port（仅当lsu_issue不处于堵塞状态时，否则保持不变）送空包，置流水级繁忙状态，暂停本轮整数指令与LSU指令分发
* 若遇到fence指令，并且有整数指令要分发但是integer_issue处于阻塞状态，或者有LSU指令要分发但是lsu_issue处于阻塞状态，则向dispatch_integer_issue_port（仅当integer_issue不处于堵塞状态时，否则保持不变）与dispatch_lsu_issue_port（仅当lsu_issue不处于堵塞状态时，否则保持不变）送空包，置流水级繁忙状态，暂停本轮整数指令与LSU指令分发
* 若不满足上述情况，则执行如下流程：
  * 若遇到fence指令，则置该流水级为指令等待状态及Store Buffer空等待状态，等待该指令的完成及Store Buffer为空后再分发下一个指令包
  * 若需要分发整数指令，则分为两种情况：
    * 若integer_issue不处于堵塞状态，则将该指令送入dispatch_integer_issue_port
    * 否则，将integer_issue_pack暂存到hold_integer_issue_pack中，并置整数队列繁忙标志有效
  * 若不需要分发整数指令，但若integer_issue处于堵塞状态，则保持dispatch_integer_issue_port不变，否则将空包送入dispatch_integer_issue_port
  * 若需要分发LSU指令，则分为两种情况：
    * 若lsu_issue不处于堵塞状态，则将该指令送入dispatch_lsu_issue_port
    * 否则，将lsu_issue_pack暂存到hold_lsu_issue_pack中，并置LSU队列繁忙标志有效
  * 若不需要分发LSU指令，但若lsu_issue处于堵塞状态，则保持dispatch_lsu_issue_port不变，否则将空包送入dispatch_lsu_issue_port

在指令等待状态下（无来自commit流水级的flush信号，可能同时处于Store Buffer空等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态）：

* 将空包送入dispatch_integer_issue_port（仅当integer_issue不处于堵塞状态时，否则保持不变）与dispatch_lsu_issue_port（仅当lsu_issue不处于堵塞状态时，否则保持不变）
* 若等待的指令已退休或等待的指令年龄小于BRU反馈包中的ROB项（来自BRU的flush信号有效时）或小于等于SAU反馈包中的ROB项（来自SAU的flush信号有效时）时，则解除指令等待状态，同时若处于Store Buffer空等待状态且Store Buffer为空，则解除Store Buffer空等待状态
* 若不满足上面的条件，则什么都不做

在Store Buffer空等待状态下（无来自commit流水级的flush信号，不处于指令等待状态，不处于整数队列繁忙等待或LSU队列繁忙等待状态）：

* 将空包送入dispatch_integer_issue_port（仅当integer_issue不处于堵塞状态时，否则保持不变）与dispatch_lsu_issue_port（仅当lsu_issue不处于堵塞状态时，否则保持不变）
* 若Store Buffer为空或引起该等待状态的指令的年龄小于BRU反馈包中的ROB项（来自BRU的flush信号有效时）或小于等于SAU反馈包中的ROB项（来自SAU的flush信号有效时）时，则解除Store Buffer空等待状态，否则什么都不做

**可以注意到，指令等待状态比Store Buffer空等待状态优先级更高，这是因为fence指令前可能存在LSU指令，这些LSU指令执行完之后Store Buffer就会为空，但是此时还没有执行到fence指令，使得fence指令没有成功阻断LSU指令提前fence指令执行，因此需要先等待fence指令退休，再同时或在之后等待Store Buffer为空**

在整数队列繁忙或LSU队列繁忙等待状态下（无来自commit流水级的flush信号，不处于指令等待状态或Store Buffer空等待状态，该状态的包不可能包含csr/mret或fence指令）：

* 分别从hold_integer_issue_pack与hold_lsu_issue_pack中取出保留的指令包
* 若integer_issue处于堵塞状态，则保持dispatch_integer_issue_port不变
* 否则，分为如下两种情况：
  * 若需要分发整数指令，则将该指令送入dispatch_integer_issue_port，并解除整数队列繁忙等待状态
  * 若不需要分发整数指令，则将空包送入dispatch_integer_issue_port
* 若lsu_issue处于堵塞状态，则保持dispatch_lsu_issue_port不变
  * 若需要分发LSU指令，则将该指令送入dispatch_lsu_issue_port，并解除LSU队列繁忙等待状态
  * 若不需要分发LSU指令，则将空包送入dispatch_lsu_issue_port

收到来自commit流水级的flush信号时：

* 将空包送入dispatch_integer_issue_port与dispatch_lsu_issue_port
* 解除指令队列繁忙与LSU队列繁忙等待状态
* 解除流水级繁忙状态
* 清除所有的保留包
* 解除指令等待状态
* 解除Store Buffer空等待状态

收到来自BRU或SAU的flush信号时（二者可能同时有效）：

* 清空hold_integer_issue_pack（当来自BRU的flush信号有效时，乱序流分支预测恢复）或清空hold_integer_issue_pack中年龄小于SAU反馈包中ROB项的指令（当来自SAU的flush信号有效时，乱序流Store/Load违例恢复）
* 清空hold_lsu_issue_pack中年龄小于BRU反馈包中ROB项的指令（当来自BRU的flush信号有效时）或清空hold_lsu_issue_pack（当来自SAU的flush信号有效时）

这里之所以针对hold_integer_issue_pack采取部分年龄筛选策略，是因为同类型的指令都是顺序派发的，而整数指令和LSU指令之间却是乱序派发的

反馈信号产生：若当前状态为整数队列繁忙等待状态、LSU队列繁忙等待状态、流水级繁忙状态、（指令等待状态且没有收到等待指令的退休信号）或（Store Buffer空等待状态且当前Store Buffer不为空），则产生stall信号

## Issue级

### Integer Issue级

该流水级实际分为两级：输入级与输出级

输入级行为：

* 若当前处于繁忙状态，则直接使用之前暂存的指令包，否则从dispatch_integer_issue_port接收新的指令包
* 判断当前发射队列是否已满或空间不足，若已满或空间不足则置繁忙标志有效，同时右对齐保存无法处理的项，留置之后处理，否则置繁忙标志无效
* 如果检测到来自BRU或SAU的flush请求，且当前指令年龄小于（对于BRU）或小于等于（对于SAU）指令的年龄，则丢弃该指令
* 从phy_regfile中读取寄存器的有效状态，同时分析来自execute/wb流水级的反馈包，若其中有一个寄存器匹配某个源寄存器，或该指令符合下述条件，则该源操作数标记为ready状态，否则为not ready状态：
  * 指令无效
  * 指令发生了异常
  * 源操作数不需要映射（为x0或立即数或不存在该源操作数）
* 对于ALU/MUL指令，需要进行仲裁出口分配，这里使用Round-Robin算法进行分配，对于DIV指令，将仲裁出口指定为1，对于其它指令，将仲裁出口指定为0
* 将指令的所属执行单元类型转换为one-hot单独保留用于仲裁
* 单独保存指令的rob_id/rob_id_stage用于年龄计算
* 单独保存指令的rd/rd_valid用于更短路径的唤醒
* 计算每条指令的wakeup_shift，公式为(LATENCY == 1) ? 0 : (1 << (LATENCY - 2))
* 计算执行单元的空闲延迟（即在几个周期后空闲，公式为(LATENCY == 1) ? 0 : (1 << (LATENCY - 2))），避免仲裁时从反馈网络获得该信息造成过大的组合逻辑延迟
* 清零指令每一个操作数的LPV
* 将结果推入发射队列

若从commit流水级收到了flush信号，则会重置所有的状态变量到复位值，同时对发射队列执行flush操作

反馈信号产生：若当前状态为繁忙状态，则产生stall信号

公式推导：

* wakeup_shift公式：
  * 考虑到流水线是如下结构：
  * issue -> readreg -> execute
  * 并且只可能在execute处发生停顿，因此，若流水线不发生停顿（LATENCY = 1的情形），则关联的指令可以直接背靠背发射，不需要提前唤醒，而是直接唤醒即可，因此wakeup_shift = 0，而若流水线发生停顿（LATENCY > 1的情形），则关联的指令只需要相应地延迟LATENCY - 1个周期后发射即可，式中之所以是LATENCY - 2，是因为第一个周期设置延迟唤醒时间，第二~N个周期检测延迟唤醒时间并最后执行唤醒，下一个周期指令才会被发射，因此需要排除掉唤醒检测的这一周期。
  * 1 << x 只是二进制码到one-hot编码的变换公式，这里之所以是x而不是x - 1是因为，当该one-hot为1时，就会开始执行唤醒操作，并在下一个周期完成唤醒标志位的变化，而唤醒是提前发射一个周期的，因此，这里的x实际上是当前到唤醒时的周期间隔，而不是当前到发射时的周期间隔
* 空闲延迟公式：
  * 考虑如下的情形：
  * issue -> readreg -> execute -> wb
  * T0 A -> [none] -> [none] -> [none]
  * T1 [none] -> A -> [none] -> [none]
  * T2 [none] -> [none] -> A -> [none]
  * T3 [none] -> [none] -> A -> [none]
  * T4 B -> [none] -> A -> [none]
  * T5 [none] -> B -> A -> [none]
  * T6 [none] -> [none] -> B -> A
  * 和上面同样的流水线结构，显然，若执行单元的LATENCY = 1，执行单元也就不存在繁忙的可能性，而若LATENCY > 1（示例为4），后级流水线从下一时刻开始就不能送入新指令了，否则会造成指令堵在readreg流水级，这是不允许的
  * 但是，可以观察到，在T4时刻，issue流水级就可以向该指令单元流水线发射新的指令了，因此指令单元的繁忙时间段其实是[T1,T3]
  * issue流水级在T0时刻假如设置延迟为x，则T1时刻可以观测到x，T2时刻可以观测到x-1，T3时刻可以观测到x-2，如果要确保T4时刻可以发射出指令，那么T4时刻必须观测到功能单元是空闲的，那么在T3时刻就必须设置指令执行单元是空闲的，为了做到这一点，延迟到这个流水级必须递减为1，也就是说x-2=1，即x = 3，而这与预期的LATENCY值（4）正好差1，因此，空闲延迟应当设置为LATENCY - 1，对应的one-hot则为1 << (LATENCY - 1)

输出级行为：

* 负责利用仲裁算法从发射队列中选出至多两条指令，并送入integer_issue_readreg_port
* 在选择时，如果遇到来自LU的replay请求，且当前正在仲裁的指令处于replay依赖路径上（即不依赖任何一条未完成的LU指令的结果），则本次不发射该指令
* 在发射时，如果该指令不处于任何一条replay依赖路径上，则直接将该指令从发射队列中剔除，否则暂不剔除并标记为已发射状态
* 发射出的指令的LPV为两个源操作数的LPV的或运算右移一位的结果（因为该结果将会在下一个周期被下一级流水线观测到，所以需要右移一位表示下一周期的状态）

仲裁算法：

* 该仲裁算法具有两个仲裁出口，出口1负责ALU0/MUL0/CSR/BRU执行单元，出口2负责ALU1/MUL1/DIV执行单元
* 计算每一个执行单元未来的繁忙状态
* 对于每一个仲裁出口，比较每一条指令的ready信号和年龄信息，选出一条已ready的最老的指令，并且该指令所属执行单元未来不能是繁忙的

在完成仲裁后，会将指令的目的寄存器编号广播到整数指令队列和LSU指令队列中，执行提前唤醒操作，同时传递LPV值，用于标记replay依赖路径

提前唤醒设计：

* 在发射一条指令时，将目标寄存器进行广播，队列内的每个有效指令项收到广播后，会检查该指令的源寄存器是否与广播的目标寄存器相同，若相同，则将该指令该操作数对应的wakeup_shift_src设为待发射指令的延迟信息（采用one-hot编码，以实现移位寄存器）
* 广播来源为integer_issue或lsu_issue
* 每一个wakeup_shift_src在每周期都会右移一位
* 若某一操作数的wakeup_shift_src当前为1，则置对应的src_ready为1

正常唤醒设计：

* 在接收到来自execute级的反馈包时，将对应的指令的src_ready置为1

可以注意到上面在输入级的时候，需要同时判断execute/wb流水级的反馈包，而在正常唤醒时，只需要判断execute级的反馈包，这是由于以下原因：

* 在当I1位于输入级时可以观测到如下的当前和未来指令流水状态（其中I1是当前待进入发射队列的指令，I2与I3则比I1的年龄老，且I2已经完成执行输出了正常的反馈包，I3也输出了正常的反馈包）：
  * I1 input -> output -> rf
  * I2 execute -> wb -> commit
  * I3 wb -> commit
* 若I1在输入级不使用execute流水级的反馈包，则input可能需要在下一个周期才被唤醒，从而延迟一个周期发射
* 而在正常唤醒时，若被唤醒的指令是I1指令，则执行唤醒操作的I2指令若当前在wb级，则上一个周期必然在execute级，此时I1指令必然在输入级或发射队列中，在输入级时，已经处理过了来自execute流水级的反馈包，而如果在发射队列中，由于该指令在execute级，所以必然已经唤醒了I1指令，因此没必要处理来自wb流水级的反馈包，这样也减少了反馈电路的复杂度，有助于频率的提升

在唤醒阶段：

* 如果检测到来自BRU或SAU的flush请求，且发射队列中的某条指令年龄小于（对于BRU）或小于等于（对于SAU）反馈包指令的年龄，则将该指令踢出队列
* 如果检测到某条指令已发射，则根据情况执行如下操作：
  * 如果来自LU的replay请求有效，且当前指令处于该replay依赖路径上，则将该指令重新标记为未发射状态，并将对应处于依赖链上的操作数的ready状态置为无效
  * 否则，若该指令的LPV小于等于1，则将该指令从发射队列中剔除（说明该指令已经不处于有风险的replay依赖路径上了）
* 每条指令每一个操作数的LPV都需要右移一位

### LSU Issue级

该流水级实际分为两级：输入级与输出级

输入级行为：

* 若当前处于繁忙状态，则直接使用之前暂存的指令包，否则从dispatch_lsu_issue_port接收新的指令包
* 判断当前发射队列是否已满或空间不足，若已满或空间不足则置繁忙标志有效，同时右对齐保存无法处理的项，留置之后处理，否则置繁忙标志无效
* 如果检测到来自BRU或SAU的flush请求，且当前指令年龄小于（对于BRU）或小于等于（对于SAU）指令的年龄，则丢弃该指令
* 从phy_regfile中读取寄存器的有效状态，同时分析来自execute/wb流水级的反馈包，若其中有一个寄存器匹配某个源寄存器，或该指令符合下述条件，则该源操作数标记为ready状态，否则为not ready状态：
  * 指令无效
  * 指令发生了异常
  * 源操作数不需要映射（为x0或立即数或不存在该源操作数）
* 计算每条指令的wakeup_shift，公式为(LATENCY == 1) ? 0 : (1 << (LATENCY - 1))
* 清零指令每一个操作数的LPV
* 若该指令为LU指令，则同时向cur_lpv和lpv字段写入INIT_LPV，否则写入0，其中lpv作为当前指令的初始LPV，cur_lpv则是当前指令当前的LPV，用于replay操作支持
* 将结果推入发射队列

若从commit流水级收到了flush信号，则会重置所有的状态变量到复位值，同时对发射队列执行flush操作

反馈信号产生：若当前状态为繁忙状态，则产生stall信号

输出级行为：

* 若当前收到了lsu_readreg发来的stall信号，则不改变lsu_issue_readreg_port，否则若fifo头部的指令ready，直接输出fifo头部的指令，并送入lsu_issue_readreg_port，否则送入空包
* 在选择时，如果遇到来自LU的replay请求，且当前正在仲裁的指令处于replay依赖路径上（即不依赖任何一条未完成的LU指令的结果），则本次不发射该指令
* 在发射时，如果该指令不处于任何一条replay依赖路径上且当前正在发射该指令的最后一条微指令，则直接将该指令从发射队列中剔除，否则暂不剔除并标记为已发射状态（对于最后一条微指令）及对应的微指令发射状态（对于SAU和SDU指令）
* 发射出的指令的LPV为两个源操作数的LPV的或运算右移一位的结果（因为该结果将会在下一个周期被下一级流水线观测到，所以需要右移一位表示下一周期的状态）
* 如果当前发射的是Store指令，则根据操作数的ready情况同时发射地址计算和数据填充微指令（即SAU和SDU指令）或发送SAU或SDU微指令，并根据发送情况更新该指令的微指令状态。
* 还需将该指令的lpv字段填充到cur_lpv字段，用于replay支持
* 反馈包中的LPV的计算公式有所不同，应为各个操作数的LPV的或运算结果右移一位之后再或上当前指令自己的LPV值，这是因为该指令可能是replay依赖链的源头

提前唤醒设计：

* 每周期从integer_issue及lsu_issue接收广播，队列内的每个有效指令项收到广播后，会检查该指令的源寄存器是否与广播的目标寄存器相同，若相同，则将该指令该操作数对应的wakeup_shift_src设为待发射指令的延迟信息（采用one-hot编码，以实现移位寄存器）
* 每一个wakeup_shift_src在每周期都会右移一位
* 若某一操作数的wakeup_shift_src当前为1，则置对应的src_ready为1

正常唤醒设计：

* 需要在接收到来自execute级的反馈包时，将对应指令对应源操作数的src_ready置为1

可以注意到上面在输入级的时候，需要同时判断execute/wb流水级的反馈包，而在正常唤醒时，只需要判断execute级的反馈包，这是由于以下原因：

* 在当I1位于输入级时可以观测到如下的当前和未来指令流水状态（其中I1是当前待进入发射队列的指令，I2与I3则比I1的年龄老，且I2已经完成执行输出了正常的反馈包，I3也输出了正常的反馈包）：
  * I1 input -> output -> rf
  * I2 execute -> wb -> commit
  * I3 wb -> commit
* 若I1在输入级不使用execute流水级的反馈包，则input可能需要在下一个周期才被唤醒，从而延迟一个周期发射
* 而在正常唤醒时，若被唤醒的指令是I1指令，则执行唤醒操作的I2指令若当前在wb级，则上一个周期必然在execute级，此时I1指令必然在输入级或发射队列中，在输入级时，已经处理过了来自execute流水级的反馈包，而如果在发射队列中，由于该指令在execute级，所以必然已经唤醒了I1指令，因此没必要处理来自wb流水级的反馈包，这样也减少了反馈电路的复杂度，有助于频率的提升

在唤醒阶段：

* 如果检测到来自BRU或SAU的flush请求，且发射队列中的某条指令年龄小于（对于BRU）或小于等于（对于SAU）反馈包指令的年龄，则将该指令踢出队列
* 如果检测到某条指令已发射，则根据情况执行如下操作：
  * 如果来自LU的replay请求有效，且当前指令处于该replay依赖路径上，则将该指令重新标记为未发射状态，并将对应处于依赖链上的操作数的ready状态置为无效
  * 否则，若该指令的LPV小于等于1，则将该指令从发射队列中剔除（说明该指令已经不处于有风险的replay依赖路径上了）
* 每条指令每一个操作数的LPV都需要右移一位
* 每条指令的lpv字段值也需要右移一位

## Readreg级

### Integer Readreg级

该流水级首先从integer_issue_readreg_port中读入指令包（该指令包的几条指令分别对应相应的发射端口，顺序不可错），负责从phy_regfile/execute feedback/wb feedback读入源操作数，然后发送到相应的执行单元端口（根据指令在指令包中的位置以及指令类型）

如果收到了来自commit流水级的flush请求，则对全部执行单元的handshake_dff执行flush操作

如果收到了来自BRU或SAU的flush请求，则对于所有的输入端口跳过对应指令，对于所有的输出端口抛弃对应指令，仅当对应指令的年龄小于（对于BAU）或小于等于（对于SAU）反馈包指令的年龄

如果收到了来自LU的replay请求，则对于所有的输入端口都跳过处于该replay依赖路径上的指令，由于操作数依赖关系原因，在发生replay时，指令最晚只能出现在Readreg的输入级，因此后续流水级包括Readreg的输出级不需要再检测来自LU的replay请求

### LSU Readreg级

该流水级按照以下流程执行：
* 若当前流水级处于繁忙状态，则直接获取上一个周期的保存的指令包，然后解除流水线的繁忙状态，否则从lsu_issue_readreg_port中读入指令包
* 从phy_regfile/execute feedback/wb feedback读入源操作数
* 发送到相应的执行单元端口，若执行单元端口堵塞，则置流水线状态为繁忙状态（这部分暂未实现，理论上堵塞情况在发射级就已经知晓，因此这里应当不会发生堵塞）

如果收到了来自commit流水级的flush请求，则对LSU的handshake_dff执行flush操作

如果收到了来自BRU或SAU的flush请求，则对于所有的输入端口跳过对应指令，对于所有的输出端口抛弃对应指令，仅当对应指令的年龄小于（对于BAU）或小于等于（对于SAU）反馈包指令的年龄

如果收到了来自LU的replay请求，则对于所有的输入端口都跳过处于该replay依赖路径上的指令，由于操作数依赖关系原因，在发生replay时，指令最晚只能出现在Readreg的输入级，因此后续流水级包括Readreg的输出级不需要再检测来自LU的replay请求

## Execute级

### ALU

该流水级负责处理ALU类指令，包括add/and/auipc/ebreak/ecall/fence/fence.i/lui/or/sll/slt/sltu/sra/srl/sub/xor指令及其可能存在的立即数版本

步骤如下：
* 若readreg_alu_hdff不为空且没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 从readreg_alu_hdff读入一条指令
* 根据指令类型计算相应的结果，送入alu_wb_port

如果收到了来自BRU或SAU的flush请求，且收到的指令的年龄小于（对于BAU）或小于等于（对于SAU）反馈包指令的年龄，则直接丢弃该指令

反馈信号生成：若当前指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包

### BRU

该流水级负责处理BRU类指令，包括beq/bge/bgeu/blt/bltu/bne/jal/jalr/mret指令

步骤如下：
* 若readreg_bru_hdff不为空且没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 从readreg_bru_hdff读入一条指令
* 根据指令类型计算相应的结果，送入bru_wb_port

如果收到了来自SAU的flush请求，且收到的指令的年龄小于等于反馈包指令的年龄，则直接丢弃该指令

如果检测到当前分支预测是失败的，且拥有Checkpoint，则直接进行乱序流中的分支预测快速现场恢复

反馈信号生成：

* 数据反馈：若当前指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包
* 分支预测状态反馈：若当前检测到分支预测是失败的，且该指令拥有Checkpoint，则生成flush请求，填充当前分支指令的rob年龄信息和下一地址信息，用于乱序流分支预测快速现场恢复

### CSR

该流水级负责处理CSR类指令，包括csrrc/csrrs/csrrw指令

步骤如下：
* 若readreg_csr_hdff不为空且没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 从readreg_csr_hdff读入一条指令
* 检查目标寄存器是否需要重命名，若需要重命名，则对csrfile执行读取操作，若读取失败，则会产生illegal_instruction异常
* 若源操作数不为x0寄存器，则表明需要执行写操作，但是为了保证乱序处理器的一致性，写操作将会推迟到指令退休时进行，这里仅做写权限检查，若写权限检查不通过，同样会产生illegal_instruction异常
* 根据指令类型计算相应的结果，送入csr_wb_port

反馈信号生成：若当前指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包

### DIV

该流水级负责处理DIV类指令，包括div/divu/rem/remu指令

步骤如下：
* 若没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 从readreg_div_hdff读入一条指令
* 若流水级当前处于繁忙状态，则继续等待，直到完整的DIV_LATENCY周期结束为止输出结果到div_wb_port
* 若流水级当前不处于繁忙状态，则若readreg_div_hdff为空，直接返回空包，否则初始化除法器开始计算

如果收到了来自BRU或SAU的flush请求，且收到的指令的年龄小于（对于BAU）或小于等于（对于SAU）反馈包指令的年龄，则直接丢弃该指令

反馈信号生成：若当前指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包

### MUL

该流水级负责处理MUL类指令，包括mul/mulh/mulhsu/mulhu指令

步骤如下：
* 若readreg_mul_hdff不为空且没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 从readreg_mul_hdff读入一条指令
* 根据指令类型计算相应的结果，送入mul_wb_port

如果收到了来自BRU或SAU的flush请求，且收到的指令的年龄小于（对于BAU）或小于等于（对于SAU）反馈包指令的年龄，则直接丢弃该指令

反馈信号生成：若当前指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包

### LU

该流水级负责处理Load指令，包括lb/lbu/lh/lhu/lw指令，且实际分为两个流水级

第一个流水级的行为如下：

* 若没有从commit流水级收到flush信号，则按照如下流程走，否则清零l2_addr与l2_rev_pack（即送到第二级流水线的数据）
* 若第二级流水级处于堵塞状态，则什么都不做，否则按照如下流程走
* 若readreg_lsu_hdff为空，则清零l2_addr与l2_rev_pack，否则按照如下流程走
* 从readreg_lsu_hdff读入一条指令
* 若收到来自BRU或SAU的flush请求，且当前的指令年龄小于（对于BRU）或小于等于（对于SAU）反馈包中的指令年龄，则丢弃该指令，否则继续往下走
* 计算有效地址
* 检查地址的对齐情况，若地址不对齐，则产生load_address_misaligned异常
* 将地址送到总线与Store Buffer
* 将指令送到下一流水级

第二个流水级的行为如下：

* 若没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包
* 若收到来自BRU或SAU的flush请求，且l2_rev_pack中的指令指令年龄小于（对于BRU）或小于等于（对于SAU）反馈包中的指令年龄，则丢弃该指令，否则继续往下走
* 若从Store Buffer接收到了冲突信号，则说明发生了Store/Load违例（即该Load指令本应在Store指令之后执行，但是对应的Store指令的数据还没有准备好），因此需要执行replay操作
* 分析l2_rev_pack的指令，若没有产生异常，则按如下流程走，否则将数据包直接发送到lsu_wb_port
* 若总线未返回数据，则发送stall信号，在本周期流水线暂停什么都不做，否则继续如下流程
* 从总线取得结果，并从Store Buffer获得反馈信息，合并结果后将结果填入send_pack并发送到lsu_wb_port

反馈信号生成：

* 数据反馈：若第二级流水线的指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包，否则反馈包为无效包
* replay反馈：若第二流水级产生了replay请求，则在replay反馈包中置replay信号有效

### SAU

该流水级负责处理Store指令的地址计算，包括sb/sh/sw指令

行为如下：

* 若没有从commit流水级收到flush信号，则按照如下流程走，否则什么也不做
* 若收到来自BRU的flush请求，且当前的指令年龄小于反馈包中的指令年龄，则丢弃该指令，否则继续往下走
* 对收到的指令进行地址计算，如果该指令没有发生异常，则：
  * 将地址填充到Store Buffer中
  * 检查地址是否对齐，若未对齐，则生成未对齐异常
  * 检查Load Queue中是否存在与该Store指令地址重叠且年龄小于该Store指令的表项，若存在，则发生Store/Load违例（即该Load指令本应在Store指令之后执行，依赖Store的数据，但却在Store之前执行，这时候Load获取到的数据是错误的），将违例标志写入Load Queue以便对应Load指令退休时进行现场还原，若对应Load指令拥有Checkpoint，则直接使用该Checkpoint进行乱序流快速现场恢复，减少违例的代价

反馈信号生成：若当前发现Store/Load违例，则生成flush请求，并将当前指令的ROB年龄信息和PC填入，用于乱序流快速现场恢复

### SDU

该流水级负责处理Store指令的数据填充，包括sb/sh/sw指令

行为如下：

* 若没有从commit流水级收到flush信号，则按照如下流程走，否则什么也不做
* 若收到来自BRU或SAU的flush请求，且当前的指令年龄小于（对于BRU）或小于等于（对于SAU）反馈包中的指令年龄，则丢弃该指令，否则继续往下走
* 将数据填充到Store Buffer中

## WB级

该流水级负责phy_regfile的写回与反馈，步骤如下：

* 若没有从commit流水级收到flush信号，则按照如下流程走，否则直接发送空包到wb_commit_port
* 从每一个execute_wb_port读入一条指令，执行如下流程：
  * 若收到了来自BRU或SAU的flush请求，且该指令的年龄小于（对于BRU）或小于等于（对于SAU）反馈包中的指令年龄，则丢弃该指令
  * 若指令有效、没有产生异常且需要重命名，则执行phy_regfile的写回操作，并将写回的数据送到反馈通道上
  * 将指令送到wb_commit_port

反馈信号生成：若某条指令有效且需要重命名（这说明存在有效的rd寄存器，且不为x0）且没有发生异常，则将rd_phy和结果填入反馈包对应位置，否则反馈包对应位置为无效项

## Commit级

该流水级负责指令的完成、退休和中断异常处理，分为输入级和输出级

输入级步骤如下：
* 若输出级产生了flush请求，则输入级什么都不做，否则执行如下流程
* 从wb_commit_port读入一个指令包
* 遍历其中的每一条指令，凡是有效项，都在ROB中标记为完成

输出级步骤如下：

* 若rob为空，则什么都不做，否则按照如下流程走
* 从rob顶端读出一项，置为rob_item，并在反馈包中标记下一个待处理ROB为该项
* 若检测到中断发生，且ROB当前项为最后一条微指令或发生了异常，则执行如下序列：
  * 将rob_item.pc写入CSR_MEPC
  * 将0写入CSR_MTVAL
  * 将中断ID与0x80000000的或写入CSR_MCAUSE
  * 将CSR_MSTATUS的MIE标志位复制到MPIE标志位，并将MIE标志位置0
  * 向中断接口传递ACK信号，表示中断已处理
  * 产生flush反馈，跳转地址设为CSR_MTVEC
  * 将Retire RAT整体复制到Speculative RAT
  * 对ROB执行flush操作
  * 将Retire RAT的Valid标志位整体复制到phy_regfile的Valid标志位
  * 将rob_item对应的phy_id_free_list修改前的读指针恢复到phy_id_free_list
  * 对Load Queue执行flush操作
  * 对Checkpoint Buffer执行flush操作
* 否则：
  * 准备退休最多COMMIT_WIDTH指令，开始从ROB顶端遍历
  * 从ROB顶端读入一项，置为rob_item
  * 若rob_item.finish为真，表明指令已完成执行，准备退休，否则终止下述流程
  * 在反馈包标记待处理ROB为下一项（如果存在，若不存在，则标记为不存在下一项）
  * 在反馈包中标记该项已退休
  * 若该指令发生异常，则执行如下流程
    * 产生flush反馈，跳转地址设为CSR_MTVEC
    * 将Retire RAT整体复制到Speculative RAT
    * 对ROB执行flush操作
    * 将Retire RAT的Valid标志位整体复制到phy_regfile的Valid标志位
    * 将rob_item对应的phy_id_free_list修改前的读指针恢复到phy_id_free_list
    * 对Load Queue执行flush操作
    * 对Checkpoint Buffer执行flush操作
    * 标记一条指令已完成提交
    * 终止后续的指令处理
  * 若该指令未发生异常，则执行如下流程
    * 将ROB的顶端项弹出
    * 若该指令被分配了Load Queue项，则检查其中的冲突标志是否被标记，若被标记且未使用Checkpoint，则执行如下操作，同时不再执行下面的操作，否则将对应的Load Queue项释放，且若使用了Checkpoint，则释放对应的Checkpoint项，并继续往下走：
      * 产生flush反馈，跳转地址设为当前指令的地址
      * 将Retire RAT整体复制到Speculative RAT
      * 对ROB执行flush操作
      * 将Retire RAT的Valid标志位整体复制到phy_regfile的Valid标志位
      * 将rob_item对应的phy_id_free_list修改前的读指针恢复到phy_id_free_list
      * 对Load Queue执行flush操作
      * 对Checkpoint Buffer执行flush操作
    * 若该指令发生了重命名，则从Speculative RAT中释放旧物理寄存器，并在Retire RAT中提交新的物理寄存器映射，同时在Retire RAT中移除旧的物理寄存器映射，并在phy_regfile中标记旧的物理寄存器无效
    * 标记一条指令已完成提交
    * 若该指令需要对CSR执行写操作，则向csr_file发出写请求
    * 若该指令是BRU指令，则执行如下流程：
      * 自增branch_num性能计数器
      * 若该指令是mret指令，则将CSR_MSTATUS寄存器的MPIE位覆盖到MIE位上
      * 若该指令是可预测分支指令，则：
        * 若预测正确，则自增branch_hit性能计数器，并更新bi_mode和L1_BTB预测器
        * 若预测错误，则执行如下序列：
          * 自增branch_miss性能计数器
          * 更新bi_mode和L1_BTB预测器
          * 若该指令未使用Checkpoint，则继续执行如下序列：
            * 产生jump反馈请求，跳转地址为该指令的跳转目标
            * 产生flush反馈
            * 将Retire RAT整体复制到Speculative RAT
            * 对ROB执行flush操作
            * 将Retire RAT的Valid标志位整体复制到phy_regfile的Valid标志位
            * 将rob_item对应的phy_id_free_list修改前的读指针恢复到phy_id_free_list
            * 对Checkpoint Buffer执行flush操作
        * 若该指令使用了Checkpoint，则释放对应的Checkpoint项
      * 否则，产生jump反馈请求，跳转地址为该指令的跳转目标
      * 终止后续的指令处理（一次只退休一条bru指令）