# Dev.D.UE.0.0.10.P28.13.r0 Report

## 1. 状态与范围

`P28_13_COMPLETE`。基线P28.12 / `94a383d5b6b1f0df6f93edf79edc9bea3a3386a4`，2026-09-19。本阶段已闭合实际复现的Manager去激活→Runtime.TeardownWorld散落物销毁拒绝路径，完整阶段验证通过；不宣布底层整体冻结。

## 2. 真实反例

TeardownWorld原先在Actor.Destroy前移除绑定，不检查销毁返回值，随后把物品置为Destroyed、清ActiveWorld。Manager也无条件丢弃InitialWorldItems并返回清理完成。飞剑/敌人释放已成功，但真实两单位SpiritDust投影拒绝销毁时，原身份与owner因此丢失，故障解除后也不能再清理该存活Actor。

在既有ManagerDeactivationRetention测试中追加两次真实ROLE_SimulatedProxy销毁拒绝，而非模拟返回值；首次Red为0 Success / 1 Fail、队列1、原生退出0、崩溃指标0，共5条断言失败（两次完成确认、两次原身份/owner保持及最终一次释放）。首次日志保留，退出0不等于测试通过。

## 3. 最小修复

- TeardownWorld返回bool；按身份快照销毁，拒绝时保留原Actor、物品数量/World归属及ActiveWorld，全部完成后才清空间包追踪。
- Manager接收释放结果，未完成时保留自身World引用与活动状态，不进入后续局部清理；沿现有入口可继续清理，不引入新的pending或持久权威。
- BeginWorld不再覆盖拒绝释放的旧World；World物品创建、空间包丢弃及三个非Smoke Manager绑定入口检查结果。整备清理和自动化重置也不越过拒绝结果清空原权威。
- 既有测试增加跨World清理、重新绑定和物品创建的拒绝检查；恢复原角色后仍要求一次释放、原Run与持久快照不变。注册测试总数不增加。

## 4. 完成验证

15:05:24.554UTC锁定1617输入与103原未跟踪文件；四处源码75新增/22删除。原运行器串行完成以下验证，最终新根于16:21:15.906UTC结束；没有重启正常运行实例、拼接过程计数或使用前阶段日志代替本阶段执行。

| 验证 | 最终结果 |
|---|---|
| 最终Editor编译 | 48 actions / 179.28秒，原生退出0 |
| WorldLifecycle专项 / ProductFlow专项 | 7/0及5/0；核心散落物拒绝/恢复断言由Red转绿，追加跨World拒绝检查通过 |
| 最终Game编译 | 47 actions / 189.31秒，原生退出0 |
| 完整旧根 / 新根 | 1330/0及1430/0，合计2760通过、0失败；专项不重复计入 |
| 实际改动驱动覆盖门 | PASS，7路径、2规则、7必跑组、2份完整根日志 |
| 映射自检 | 独立执行529/529，原生退出0 |

四组成功自动化均有精确队列结束、原生退出0、崩溃指标0。覆盖门命中ProductRunItemUse及ItemProductAdapters，必跑组为demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility及Shanmen.0_0_10、Shanmen.0_0_10.Items，不按本轮主题遗漏旧产品回归。

首次RedBuild（4 actions / 19.06秒、原生0）和RedProof失败原件均保留；10项完成证据的原件路径和SHA-256见[Development Log](../Log/Dev.D.UE.0.0.10.P28.13.r0_log.md)。HTTP generate_204超时与引擎诊断未删除，测试通过不等于无警告、性能达标或F验收。16:56:28UTC复核1617锁定输入、103原用户文件、两份保护文档及此前8项完成原件均一致，git diff --check通过。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。本轮无正式地图/资产、物理输入、UI或玩法开发；仅双目标编译与UE-Cmd无头验证，不启动Editor UI/PIE/Standalone/产品exe，不做Smoke/Cook/Package。

本轮实测是持久Run仍活动时的Manager直接去激活与散落物释放；不冒充RequestSettlement先改变物品归属后的终局路径、空间包部分释放、其他销毁调用点或强制EndPlay都已闭合。BeginWorld的非Smoke调用方及Preparation保护为静态路由检查，未做正式地图激活。FZ-1/2仍开放，不新增通用恢复系统。

103原未跟踪用户文件与两份OverallReadiness用户修改保持。本阶段交接范围严格为四处源码、[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)、本Report/Log七路径；索引将最新完成阶段更新为P28.13，FZ-1/2仍开放。包含这七个文件的Git提交标识阶段基线，Saved原件只在本地保留，不宣称已上传GitHub。
