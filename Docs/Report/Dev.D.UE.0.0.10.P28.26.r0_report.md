# Dev.D.UE.0.0.10.P28.26.r0 Report

## 1. 状态与范围

`P28_26_PASS`。基线P28.25 / `064e087155d16a2036becc04cf167ce6930305b7`；2026-09-21 UTC。本阶段闭合旧空间包续清理与成员独立重新丢弃的归属隔离。实际反例已复现，最小修复、三组专项、双目标构建、完整新旧根及改动覆盖门均通过。这是有界阶段完成，不是整体冻结或F验收；发布基线由包含本文的Git提交追溯。

## 2. 已复现的结构缺口

Manager.RequestDropPlayerItem的普通库存丢弃路由调用Runtime.DropPlayerItemToWorld。空间包逻辑回收后，一项原投影仍拒绝Destroy，其他成员已经释放并回到库存；这些成员的独立正常丢弃仍是可达操作。旧PendingSpatialRecoveries验证却遍历整份历史成员列表，将独立新World对象误认成旧包待释放对象，归属检查拒绝了这次正常丢弃。

既有PreparedWorldPickupIdentity增加实际三对象夹具：一项真实Destroy拒绝、两项原对象已释放；通过现有带修订的DropIntent重新丢弃已释放成员。首次有效Red为0成功/1失败、精确队列1、原生0、崩溃指标0；最终Expected断言为“Released bundle member can acquire an independent new World projection”。此前首次构建缺少测试头文件、原生6，没有运行测试，单独保留且不算产品Red。

## 3. 修复与验证范围

复用现有SpatialBundleByInstance索引：旧回执的范围检查、释放遍历、待释放不变量和最后一个原成员的收尾，仅处理仍精确归属原BundleId的成员。历史成员列表不是后来独立投影的所有权。新World对象仍由正常World权威及唯一绑定检查验证，不额外增加恢复表、schema、数量权威或API。

测试验证独立新投影不被旧包两次失败重试接管或销毁；原对象最终释放后旧回执消失，新投影与其World物品、非零修订仍保持；之后沿普通拾取恰好回收一次。原携入三单位材料、活动Run和持久快照不变，已有部分生成回滚和终局组合断言仍保留。首次Red在重新丢弃失败后提前清理退出，后续隔离/完成变体没有独立Red；交互范围筛选为同一归属规则的静态一致性修正，没有单独远距离反例。

| 本阶段验证 | 当前实际结果 |
|---|---|
| Editor Development构建 | 原生0 |
| 拾取 / World生命周期 / 产品流程专项 | 1/0、7/0、5/0；队列正常结束、原生0 |
| Game Development构建 | 原生0，仅编译、不启动产品 |
| 完整旧根 | 1330/0、精确队列1330、原生0 |
| 完整0.0.10新根 | 1430/0、精确队列1430、原生0 |
| 五路径改动驱动回归 | PASS：1条规则、3个必跑组、2份本阶段完整日志 |
| 回归映射自检 | 537/537 |

完整新根于14:48:50.0391969UTC正常结束；15:21UTC恢复时核对原始run-state、完整日志及exec29243外层0，未重复启动或拼接中间计数。两套完整根共2760个独立成功用例；13项专项属于完整根子集，不重复计数。所有测试同时核对结果、精确队列、原生退出及崩溃指标。三必跑组为Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar，均使用本阶段健康完整日志。

## 4. 交接与边界

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。交接限定为ItemSubsystem.cpp、既有PreparationAdapterTests、有限索引及本Report/Log共五路径；生产13新增/3删除，测试77新增，注册数不变。两份OverallReadiness用户修改及103原未跟踪文件保持不变，不纳入提交。

[Development Log](../Log/Dev.D.UE.0.0.10.P28.26.r0_log.md)登记12份原始日志路径与SHA，包括首次编译失败及有效Red，原件仅保留本地，不宣称已上传GitHub。[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)更新至P28.26，FZ-1/2仍开放。本阶段不证明所有重入/强制EndPlay变体或跨进程World恢复；不接物理输入、不改正式地图/资产/UI/玩法、不启动Editor UI/PIE/Standalone或产品程序，不做Smoke/Cook/Package。
