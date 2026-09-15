# Dev.D.UE.0.0.10.P27.29.r0 Report

## 1. 结论

P27.29 完成一项真实的底层生命周期修复：GameMode 结束 Combat Run 时，剑法逻辑 owner 或表现 owner 身份不匹配，不再把拒绝转换为破坏性清空，也不再继续拆除它们所依赖的共享 Run。

新增测试先在旧生产实现上复现失败，再在修复后通过。剑法表现控制器全组 6/6、完整 `Shanmen.0_0_10` 根组 1,416/1,416、受改动影响的四组旧系统 116/116 最终全部通过；回归映射自检 504/504，最终六文件覆盖门满足 99 个必跑组，Editor/Game 原生构建退出码均为 0。

**本阶段不是整个 0.0.10 底层框架的冻结声明。** 另有结束后清理与 orphan 分支需要单独证明和审计。没有开始 UI、物理输入、正式资产或实际游戏性开发。

## 2. 起点与最小结构缺口

基线为 `b1cef4a39a8ccd6dc09821f1045520a3f6894b53`；该提交仅新增总体报告，产品实现承接 P27.28。当前分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。

`Ademo_mapGameMode::ReleaseCombatProductRun` 的旧实现存在两个相邻问题：

- 表现控制器 `TryEnd` 拒绝后直接 Reset，随后仍尝试逻辑会话、Coordinator 和 Timeline 的释放；返回 false 不代表原 owner 或共享 Run 还在。
- 逻辑剑法会话的有效性/Run 身份检查发生在表现 teardown 之后；检查失败时还会 Reset 逻辑会话，丢失非空观察记录及原回执。

下层已有拒绝保留规则，缺口位于 GameMode 的组合顺序及拒绝处理，不需要新 Authority、新 ID 工厂或恢复协议层。

## 3. 最小修复

1. 将逻辑剑法 Session 的有效性检查前移至表现 teardown 之前。
2. 非空 Session 必须属于当前活动 Coordinator Run；无效或不一致时保留状态并返回 false。
3. 表现控制器 `TryEnd` 失败后立即返回 false，不再 Reset 失败 owner，不再执行后续共享 Run teardown。
4. 通过预检后仍使用既有 TryEnd 和正常释放路径；不增加数据所有者或玩家入口。
5. GameMode 头文件只增加 WITH_DEV_AUTOMATION_TESTS 内的一个 friend，供自动化构造组合状态，不增加运行时公开 API。

生产 GameMode 实现差异为 25 行新增、22 行删除；头文件增加 1 行；测试增加 127 行；回归映射增加 1 行。新增行包含移动代码及注释，不等于新增同等数量的独立逻辑。

## 4. 身份、非零证据与恢复证明

新测试 `Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController.GameModeReleaseRecovery` 包含两组独立情形：

| 注入情形 | 两次结束尝试 | 必须保持 | 校正后 |
|---|---|---|---|
| 正确逻辑 Session + 外来 Run 表现 owner | 两次均拒绝 | 原非零逻辑回执、待处理视觉/音频 Handoff、排队数量、Coordinator 与 Timeline | 正常结束，重复结束成功 |
| 外来 Run 逻辑 Session + 正确表现 owner | 两次均拒绝 | 外来逻辑记录和正确表现记录均不被消费；共享 Run 仍活动 | 正常结束，重复结束成功 |

夹具通过既有剑法动作生成真实的非零观察、两个待处理表现通道和排队 revision，不以默认空值的相等断言代替保存证明。测试核对精确 ReceiptId/HandoffId、观察数量、捕获 revision、排队数量和 Run 身份。

测试使用瞬态 GameMode 组合对象，不启动正式 World/资产/玩家输入接线。故意触发的两次对应错误日志登记为预期，未忽略其他错误。

“校正后”指测试恢复已知的原始正确 owner 快照，再调用同一 GameMode 结束路径；**不是本阶段新增了生产环境自动修复或快照导入功能。**

## 5. 证明范围与限制

该修复保留的是剑法逻辑/表现 owner 及其后续共享 Run、Timeline。释放函数更早的护盾、神识、治疗、闪避/格挡等清理前缀仍可能已完成；本阶段不提供整函数事务回滚，也不声称所有子系统都保持未变。

新测试具体证明两种跨 Run 身份错误。无效 Session 的早期拒绝由源码调整支持，但没有把未单独构造的任意内存损坏情形算成已测试。

本阶段不改变伤害、资源消耗、数值、资产播放语义、物品持久权威或 Formation 发布协议。下层成功释放的既有语义保持不变。

## 6. 自动化证据

| 范围 | Success | Fail | 原始日志 SHA-256 |
|---|---:|---:|---|
| 旧实现复现（预期失败） | 0 | 1 | `2CCAF1E6B2D9F2CA5FE0E72968912B8F2C8E8C491A93446F1F5200D4309B7504` |
| SwordRhythm 控制器全组 | 6 | 0 | `011186149D462D1BE0CBA8615F6ACDE0A2D92A8F53E0D061A228E7AA2EA8719B` |
| demo_map.V3.Attributes | 4 | 0 | `E2FE0F981267D58C1082A36F4A34DBA8B375FF65AC1A985F55FB2E457934E1DE` |
| demo_map.EnemySkillFramework | 44 | 0 | `818F2E9ECD336233A3C321153D0EA4BA9E34FEDBD9A49D8F53A0BDE88C326425` |
| demo_map.V2RangedCompatibility | 22 | 0 | `DA05F4C4EE0481597C4AF9A0F6B1E5A6923EF3F99667E992E8CE40DBC58B3FAE` |
| demo_map.ItemUseAndArmor | 46 | 0 | `67A6FCC2FC2DC7DBB3ACDEBE5952192652D856131E670F70C5C7BA68E1D1AAE2` |
| Shanmen.0_0_10 全根 | 1416 | 0 | `87CB5E8499AFF448ACB38C296AFC54341A200311B21BC8623845A0CAE486B9F5` |

复现日志的原生退出码为 0，但 Automation 明确报告 1 Fail，且多个非零状态保留和再次释放断言失败；这是有效的失败复现，不是环境故障或成功结果。原始失败日志保留，未用于通过覆盖门。

最终专项和完整根组的成功、失败、终止标记及原生退出分别核验。专项是根组的子集，不能相加宣称独立测试数量。完整旧 demo_map 根未重跑；本轮运行的是映射要求的四个旧组。

完整根组测试记录：2026-09-14 22:47:36.060 UTC 至 23:51:10.156 UTC，约 1 小时 3 分 34 秒；队列明确记录 1,416 项完成，原生退出 0，Fail/Fatal/Unhandled Exception/Ensure 均为 0。相较 P27.28 增加 1 个注册测试。

## 7. 回归映射与双目标构建

- M01GameMode 规则增加整个 SwordRhythmEffectCuePresentationRunController 组，避免修改 GameMode 却仅运行本轮单测。
- 映射自检：`SELF_TEST: PASS 504/504`，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。
- 最终范围：`REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=99 Logs=5`；覆盖的是本阶段四个实现/映射文件和两份交接文档。99 是唯一必跑组，不是新增测试数。
- 最终覆盖日志 SHA-256：`5BA9A4C640FC5B81D34207A4CC4B1E0C0FAB2AC777005FA4C489AEF7BAC33EEC`。
- 最终 Editor：target up to date，0.87 秒，原生退出 0；日志 SHA-256 `3652C4DC11CE51091E8B75271BBD725DAC122DD170E8CB9A201D6CC52F945135`。
- 最终 Game：35 actions，159.10 秒，UBA 156.62 秒，原生退出 0；日志 SHA-256 `9E814B06ACDE94B67C26F76A0015410AB7F123DDD289D672C1497C83EB0D38B2`。
- 完整验证期间四个实现/映射文件按 SHA-256 锁定，结束后重新核对，未发生测试中途源码漂移。
- `git diff --check` 通过；具体原日志目录、源码哈希和复跑入口见 [Development Log](../Log/Dev.D.UE.0.0.10.P27.29.r0_log.md)。

构建与自动化通过并不代表实际产品启动或帧率/内存验收完成。

## 8. 未冻结事项：下一次只做有证据的最小审计

只读检查仍发现 GameMode 的相邻分支值得继续验证：

1. `ControlledWeaponWorldReleaseRejected`：逻辑 Run 已结束后，World 清理拒绝仍触发 Reset；下层 Reset 又尝试销毁 Actor 并清除 owner。需要检查失败重试与已完成前缀的含义。
2. `RunTimelineReleaseRejected`：Coordinator 结束后 Timeline 拒绝，旧分支仍 Reset；随后重试会遇到 inactive Coordinator。不能只删除 Reset 而不处理结束顺序。
3. `OrphanedControlledWeaponState`：无活动 Coordinator 时仍会广泛清空非空 owner。需要区分合法残留、错误身份和不可恢复数据。

上述为已定位的代码风险，不宣称已在真实游戏中发生，也未在本阶段修复。下一阶段应先构造非零状态的失败测试，确认真实缺口再作最小修复；不得借此无边界增加恢复包装层、玩法或新 Authority。

## 9. P/F 边界

仅进行 P 阶段源码、测试、映射、编译与 UnrealEditor-Cmd 无头验证。无物理玩家输入，无正式地图/内容资产更改，无数值、手感、敌人行为、关卡或 UI/体验开发；未启动 Editor UI、PIE、Standalone 或产品 exe，未做截图、Smoke、Cook、Package。

回归组中包含既有 PhysicalInput 等命名契约测试，不代表接入或驱动真实玩家设备。自动化保持用于底层收尾，不因本项修复完成而提前暂停。

## 10. 交接

本阶段提交范围仅四个实现/测试/映射文件及本 Report、Development Log；103 个既有未跟踪用户文件保留，不纳入提交。原始 Saved 验证产物本地保留，GitHub 文档提供其路径与原字节哈希，不伪称完整日志目录已上传。

- [Development Log](../Log/Dev.D.UE.0.0.10.P27.29.r0_log.md)
- [总体就绪度报告](Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)
- [当前分支](https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-28-formation-scatter-gamemode-composition)

阶段状态：P27.29 修复与验证完成，框架冻结审计继续；不是可玩版本或 F 阶段验收。
