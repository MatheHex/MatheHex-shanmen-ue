# Dev.D.UE.0.0.10.P28.12.r0 Report

## 1. 状态与目标

`P28_12_COMPLETE`。基线P28.11 / `e01eaea99329e232d05124d9c1a4db8585445338`，2026-09-19。已闭合Profile终局中“战斗已释放，但容器自身拒绝Destroy时仍被宣称World清理成功”的已复现缺口；全部阶段验证完成。这里只完成局部契约，不宣布整体冻结。

## 2. 反例证据

P28.11已修复战斗释放前的容器清理顺序，但DestroyRuntimeContainers本身仍忽略Destroy返回值，先清掉Chests/Corpses与奖励Run上下文，DeactivateProfileWorld随后返回true。终局续接因此丢失原pending，存活宝箱既没有owner，也不会在故障解除后重试释放。

扩展既有ManagerSettlementContinuation的直接/延迟持久成功两变体：恢复飞剑销毁权限后，再令实际非空LootChest为ROLE_SimulatedProxy，触发UE真实DestroyActor拒绝，两次调用原终局重试。修复前注册测试0 Success / 1 Fail，8条保持/恢复断言失败；原生退出0、队列1、崩溃指标0。不是把退出0当测试通过，也不是伪造清理返回值。

## 3. 最小修复

- 私有DestroyRuntimeContainers返回是否已释放；遍历快照，处理EndPlay对数组的移除，只保留真正拒绝的原owner，成功项不再重试。
- 原有Chests、Corpses、SpawnedCodeBNormalContainerTargets共用同一局部处理；仍有拒绝时保留奖励Run/会话及相关归属，待全部接受才清上下文。
- DeactivateProfileWorld接收该结果，容器拒绝时返回false，由既有终局/回滚续接记录保持未完成状态；不新增持久schema、第二份权威或新pending记录。
- M01奖励容器重绑定在旧容器拒绝清理时返回false，避免覆盖旧owner；本阶段不运行正式M01地图。
- 既有测试验证两次容器拒绝期间原身份、pending及启动门不丢失，战斗前缀已释放一次、持久结算不再提交；容器恢复权限后恰好一次销毁。

## 4. 完成验证

03:02:45.951UTC锁定1617输入，仅Manager.cpp/.h及PreparationAdapterTests.cpp三处源码变化；之后保持同一输入，串行完成以下验证。未新增注册测试，也没有重启正常推进中的完整根或拼接中间计数。

| 验证 | 最终结果 |
|---|---|
| 最终Editor编译 | 27 actions / 133.82秒，原生退出0 |
| WorldLifecycle专项 / ProductFlow专项 | 7/0及5/0；新容器拒绝/恢复断言由Red失败转为成功 |
| 最终Game编译 | 26 actions / 146.68秒，原生退出0 |
| 完整旧根 / 新根 | 1330/0及1430/0，合计2760通过、0失败；不重复计入专项 |
| 实际改动驱动覆盖门 | PASS，6路径、2规则、7必跑组、2份完整根日志 |
| 映射自检 | 独立执行529/529，原生退出0 |

四组成功自动化均有精确队列结束、原生退出0和崩溃指标0；最终新根于04:28:54.506UTC结束。覆盖门命中ProductRunItemUse与ItemProductAdapters，必跑组为demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility及Shanmen.0_0_10、Shanmen.0_0_10.Items，不按本轮主题裁掉旧产品回归。

首次RedBuild成功日志与RedProof失败日志完整保留，全部原件路径和SHA-256见Development Log。HTTP超时、SDK和大Tick等原始警告没有被删去；上述结果不是无警告、性能达标或F阶段验收。05:04:43UTC复核1617输入、103原用户文件和两份保护文档未变，git diff --check通过。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。仅编译与无头契约验证，不启动Editor UI/PIE/Standalone或产品exe，不操作正式资产、物理输入、UI/玩法、Smoke/Cook/Package。

实测故障对象为LootChest，其他容器数组共用处理不等于其所有生产调用点均已专项验证；M01重绑定为静态路由检查。整个World的清理、散落物、灵石、其他敌人Destroy及强制EndPlay仍需分别核对，不宣称跨进程原子恢复或已完成FZ-1/2。[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)更新最新完成阶段为P28.12，保留剩余已批准契约审计，不扩展为通用恢复系统。

本阶段交接范围严格为三处源码、索引、本Report和[Development Log](../Log/Dev.D.UE.0.0.10.P28.12.r0_log.md)六路径；103份原未跟踪用户文件与两份OverallReadiness用户修改不动。包含这些文件的Git提交标识阶段基线；原始Saved证据保留本地，不宣称已上传GitHub。
