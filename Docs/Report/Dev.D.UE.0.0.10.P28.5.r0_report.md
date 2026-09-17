# Dev.D.UE.0.0.10.P28.5.r0 Report

## 1. 状态与范围

`P28_5_VERIFIED`。最小修复、双构建、两组专项各5/0、完整旧根1330/0、完整新根1428/0及改动映射门均通过。本阶段从 P28.4 / `ab703477136baccf9b972bbe31e96dfe3002ecf8` 继续有限 FZ-2，修复的问题是：GameMode 已保留释放失败状态，但原 void 去激活接口不能把拒绝传回 Manager，后者仍清理自身投影和 Runtime World 物品。

本轮范围是 Manager.DeactivateProfileWorld → GameMode.DeactivateV3MissionContentForPreparation 的拒绝传播和清理次序，不建立新恢复服务、第二份权威、持久 schema 或玩法系统。

## 2. 反例设计及边界

新增 ManagerDeactivationRetention 测试复用实际隔离 Profile/cutover/Run、真实飞剑实例和非零时间线。瞬态 World 通过 SetGameMode 建立真实 AuthGameMode，Manager 使用正常 World 查找取得它；测试友元只绑定已有 owned 对象和状态，不替换生产调用。

世界包含真实两单位材料和敌人。通过既有测试采用的引擎 Actor 销毁拒绝条件，连续两次调用 Manager 去激活，检查物品/绑定/活动状态、原 Run 和已完成的清理前缀；恢复销毁条件后检查各投影只清理一次、再次调用为空操作，持久权威未被结算或替换。RedProof 实际0/1：两次 Manager 投影保留断言和一次恢复清理复合断言失败；前两条证明外层过早清理，不能把第三条直接说成销毁次数错误。GameMode 原 owner/已完成前缀及持久 Run 保持断言未失败。

首次修复后专项4/1；拆开复合断言后，唯独“FindInstance 为 null”失败，其余销毁次数、绑定和状态复位通过。源码 DestroyWorld 本就保留 Destroyed 墓碑，等待 Run 压缩，不直接删除身份。修正新增测试为检查 Destroyed、原数量2、Owner/Container/Slot 均空、World 集合不含该实例、权威不变量成立；没有修改原物品权威、弱化生产校验或添加删除旁路。最终专项通过，首次失败与诊断日志全部保留。

没有 BeginPlay、正式 M01、物理输入或玩家流程。直接去激活端口的保持不能证明更高层回滚/终局已经正确确认，也不证明所有 Actor 销毁和容器关闭具备全世界原子性。

## 3. 最小修复与验证

- GameMode 去激活返回 bool：下层 Release 拒绝返回 false，原成功清理完成后返回 true；不改变下层退役/恢复模型。
- Manager 在自己的焦点、拾取物、容器或 World 物品清理之前检查该结果，false 时直接返回。保留 Manager 原 void API；不把这一层保护伪称为所有调用者已收到最终完成确认。
- 既有 GameMode 专项补充拒绝/恢复/空清理的返回值断言。新增 Manager 专项通过实际 AuthGameMode 覆盖外层顺序，而非用回调模拟返回值。
- Red Editor：51 actions / 238.05秒 / native0；RedProof：0/1、队列1/native0、崩溃指标0。原失败不是源码崩溃，native0也不等于测试通过。
- 最终 Editor：4 actions / 8.91秒 / SUCCEEDED/native0；Game：50 actions / 217.26秒 / SUCCEEDED/native0。只编译，不运行产品exe。
- ControlledWeaponWorldLifecycle 5/0、ProductFlow 5/0，均精确队列5/native0且崩溃指标0。完整旧根1330/0、新根1428/0，各有精确队列结束、native0和崩溃指标0。新根17:15:17至18:45:05UTC，约89分48秒，同一完整运行，不拼接部分结果。
- 期间新增测试曾误调 ItemSubsystem 私有校验函数，Editor 报 C2248/native6；改用已有公开权威不变量及公开绑定计数，不扩大生产可见性。保留该源码错误日志，不归为环境故障。
- 八个阶段路径（五源码、索引、Report/Log）实际映射 PASS Changed=8 / Rules=3 / Required=84 / Logs=2；命中 M01GameMode / ProductRunItemUse / ItemProductAdapters，使用本阶段完整新旧根，不以专项代替全范围。
- 映射检查器自检本轮执行517/517通过；交付时复核原件与不变输入，不冒称又跑一次自检。两组专项包含在完整新根内，不重复累计。
- 保留103个既有用户文件及两份 OverallReadiness 未提交修改；只有本阶段五源码、索引和 Report/Log 进入阶段提交。

## 4. 未关闭事项

上层 RollbackPreparedProfileRunFor0909B 的“Runtime 已回滚、World 尚未释放”确认和重试语义、一般激活失败/终局调用及强制 EndPlay 仍是既有 FZ-2 审计项。本阶段不以局部保留宣称这些全部已解，也不为猜测场景新增恢复包装层。FZ-1 的剩余权威入口和 FZ-3 最终冻结仍待完成。

严格保持 P/F 边界：无 Editor UI、PIE、Standalone、产品 exe、正式内容资产修改、UI/玩法开发、截图、Smoke、Cook 或 Package。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.5.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
