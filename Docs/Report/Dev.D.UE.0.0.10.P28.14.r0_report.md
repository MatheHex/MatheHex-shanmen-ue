# Dev.D.UE.0.0.10.P28.14.r0 Report

## 1. 状态与范围

`P28_14_COMPLETE`。基线P28.13 / `478bd1670180fc8933066b7a79c5adfc44b19f9e`，2026-09-19。已闭合实际复现的RequestSettlement逻辑终局后散落物拒绝销毁、原World续清理丢失的问题，完整阶段验证通过；不宣布底层整体冻结。

## 2. 实际反例

Runtime.RequestSettlement先把World物品计为损失，再忽略Actor.Destroy拒绝、移除绑定并压缩Destroyed身份。后续Prepare清空Runtime，Manager的既有终局续接看不到仍存活的散落物，故障解除后也不会再释放它。P28.13的Active World身份保护并未覆盖这一已改变归属的路径。

既有ManagerSettlementContinuation保留直接/延迟持久成功两种原变体，再各加入两单位SpiritDust的真实World投影和ROLE_SimulatedProxy销毁拒绝。首次Red为0 Success / 1 Fail、队列1、原生0、崩溃指标0，12条最终保持/恢复断言失败；原件保留，退出0不等于通过。

## 3. 有界修复

- Runtime在逻辑终局后保留拒绝销毁的Actor绑定及Destroyed身份；损失回执仍为原Run的既有Summary，不重新变成可拾取物品，不新增第二权威。
- 仅当既有Settled状态与Summary的World→Destroyed行、物品定义/数量精确匹配时，不变量才接受该待释放投影；Teardown同时处理这类绑定，全部接受后才压缩墓碑、清空间包追踪。
- Flow不把已成功的持久结算误报为磁盘失败：匹配原终局的World释放拒绝保留durable结果，交给Manager已有World-only续接；直接Flow Start在残留终局投影时先拒绝，不提交新Run。
- Manager的原绑定校验允许同Run的已Settled Runtime；先完成World释放，再完成Runtime Preparation，之后才清原pending。不新增持久schema、恢复记录或公共接口。
- 测试验证成功前缀/持久提交不重复、原身份与损失行保持、故障解除后一次销毁；追加竞争终局与直接Flow Start拒绝检查，没有为这两条追加检查单独执行Red。

## 4. 完成验证

首次修复于17:40:55.157UTC锁定，Editor通过，但WorldLifecycle专项6/1，运行器已按失败停止，未进入Game/完整根。定位到本次新增匹配读取顺序错误：延迟重试的Summary引用PendingShanmenSettlement，清掉该容器之后再比较会丢失匹配。已把匹配判定移到清理之前，保留首次修复失败日志和输入锁，不削弱断言。

修正后于17:44:49.253UTC重新锁定1617输入与103原用户文件，四处源码149新增/17删除；原运行器串行执行完Editor、专项、Game与双完整根，新根于19:02:43.338UTC结束，运行器原生退出0。没有重启正常运行实例、拼接过程计数或以此前阶段执行代替本次验证。

| 验证 | 最终结果 |
|---|---|
| 最终Editor编译 | 4 actions / 8.05秒，原生退出0 |
| WorldLifecycle / ProductFlow专项 | 7/0及5/0，四种终局组合及追加拒绝检查通过 |
| 最终Game编译 | 6 actions / 44.02秒，原生退出0 |
| 完整旧根 / 新根 | 1330/0及1430/0，合计2760成功、0失败，专项不重复计入 |
| 实际改动驱动覆盖门 | PASS，7路径、4规则、9必跑组、2份健康完整根日志 |
| 映射自检 | 独立执行529/529，原生退出0 |

四组成功自动化均精确队列结束、原生退出0、崩溃指标0。覆盖门包含demo_map.CodeB、ItemEconomySchema、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility，以及Shanmen.0_0_10、Shanmen.0_0_10.Items、Shanmen.0_0_10.Items.ProductFlow；不是只按战斗主题选择回归。

首次Red与首次修复失败两套证据均保留；12项原件的路径和SHA-256见[Development Log](../Log/Dev.D.UE.0.0.10.P28.14.r0_log.md)。HTTP超时和引擎诊断未删除，测试通过不等于无警告、性能达标或F阶段验收。19:36:11UTC复核1617锁定输入、103原用户文件、两份保护文档及此前10项完成原件均一致，专项/双根计数、队列和最终进程状态再次核对通过。

## 5. 边界

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。仅Editor/Game编译和UE-Cmd无头契约验证，不启动Editor UI/PIE/Standalone/产品exe，不接物理输入或修改正式资产、地图、玩法、UI，不做Smoke/Cook/Package。

实测为Shanmen cutover后的正常Manager终局，直接与延迟持久成功各保留原/散落物变体；不是全部旧兼容终局、技术激活回滚、空间包部分释放或强制EndPlay都已闭合的证明。FZ-1/2仍开放，不新增通用恢复系统。[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)将最新完整阶段更新为P28.14，但不关闭其余有限审计项。

103原未跟踪文件与两份OverallReadiness用户修改保持。本阶段只交接四源码、索引、本Report/Log七路径；包含这七个文件的Git提交标识阶段基线，Saved原始证据只保留本地，不宣称原件已上传GitHub。
