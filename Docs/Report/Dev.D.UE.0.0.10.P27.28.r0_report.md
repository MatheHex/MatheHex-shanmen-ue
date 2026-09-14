# Dev.D.UE.0.0.10.P27.28.r0 Report

## 1. 结论

P27.28 已把 P27.27 的 Formation scatter-publication Run Lifecycle 接入 `Ademo_mapGameMode` 持有的唯一 Combat Run 组合根。GameMode 现在提供一个受控、无物理输入的发布表面：调用方只能提交已完成的 P27.22 Handoff Evidence 与 Actor 类，不能注入 `UWorld*`、Route ID、Command ID 或另建 publication owner；GameMode 从自身活动 Coordinator、Formation Lifecycle 和 live World 完成预检、唯一 Route 绑定与显式 Publish。

同时修复了 GameMode 结束路径中的结构性重试破坏：当下层 `ControlledWeaponRunLifecycle::TryEndRun` 拒绝共享 Run 释放时，不再清空 Coordinator、Formation 双 teardown 检查点、Timeline 和其它 Run owner。失败保持可诊断且可精确重试；身份恢复后，既有 scatter/product teardown 证明被复用，World 不会二次进入，最终可释放唯一 Run 根。

专项测试 3/3、完整 FormationRunLifecycle 13/13、完整 `Shanmen.0_0_10` 根组 1,415/1,415、四组旧 GameMode 兼容测试 116/116，全部最终为 0 Fail。回归映射自检 504/504；改动文件驱动覆盖门要求的 83 个唯一证据组全部满足；Editor 与 Game 双目标构建成功。本阶段只闭合产品组合、唯一权威与失败恢复，没有进入实际游戏性开发。

## 2. 阶段问题与范围

P27.27 已让 Formation Lifecycle 持有唯一 scatter Route，并把 teardown 顺序固定为 scatter → product → Coordinator release，但 GameMode 尚不能通过它所持有的 Lifecycle 发布已经提交的 Handoff。若由外部调用方自行取得 World、打开 Route 并 Publish，会重新暴露四类结构风险：

- GameMode 的 Coordinator Run 与 Handoff 内嵌 Run 可能漂移；
- 外部调用方可能持有第二个 Route/Command owner；
- World 来源依赖调用方纪律，而非 GameMode 的产品组合边界；
- GameMode 结束失败时，旧的强制 Reset 会销毁 P27.27 为精确重试保存的两个 teardown 检查点。

P27.28 严格限制为解决这些底层问题：增加一个 GameMode-owned publication 组合入口、补齐 preflight/fail-closed 规则、保留拒绝后的可恢复 owner，并扩展回归映射。没有增加输入绑定、地图接线、玩法决策、数值、UI、内容资产或实时调度。

## 3. GameMode 唯一发布组合入口

新增 `PublishFormationScatterForActiveCombatRun`。它只接受：

1. 已提交且完整有效的 P27.22 `FormationScatterWorldPlacementHandoffEvidence`；
2. publication 使用的 Actor 类。

方法按固定顺序失败关闭：

1. GameMode-owned `CombatRunCoordinator` 与 `FormationRunLifecycle` 必须同时活动；
2. Lifecycle 的跨对象不变量必须有效；
3. Coordinator Run ID 必须有效并与 Lifecycle Run ID 完全相同；
4. Handoff 必须有效，其 Deployment → Resource → Plan 链中的 `ActiveRunId` 必须等于 Coordinator Run；
5. GameMode 自身 `GetWorld()` 必须是 live World；
6. 只有全部预检通过后，才由 Lifecycle 打开唯一 Route 槽并显式 Publish。

任何预检失败都会返回带 Run/Event/可选 Route 身份的有效拒绝结果，不占用 Route 槽、不发布 Actor，也不改写 Handoff。精确重复提交沿 P27.25 的命令记录返回同一个 Route/Command 回执；Actor 类或 Handoff 漂移由唯一槽拒绝。

## 4. 共享 Run 释放的检查点恢复

GameMode 的既有 `ReleaseCombatProductRun` 会先调用 Formation Lifecycle，使 scatter Route `End` 与 formation product teardown 保存各自检查点，再由共享 `ControlledWeaponRunLifecycle` 请求 Coordinator release。旧实现若最后一步失败，会无条件 Reset 多个 Run owner，其中包括 Formation Lifecycle 与 Coordinator；这会把已成功的 World cleanup 和 product teardown 证明一起丢失，既无法证明前缀已经完成，也可能迫使重试二次进入 World。

P27.28 删除了这段拒绝后的破坏性 Reset。现在：

- 下层拒绝日志和精确诊断仍然保留；
- Formation Lifecycle、scatter Route、两个 teardown 检查点、Coordinator 和 Timeline 保持原 owner；
- teardown 开始后的 late Publish 继续失败关闭；
- 修复 Coordinator 身份后可调用同一 GameMode 结束路径；
- scatter 与 product teardown 均以检查点重放，只有尚未成功的共享 Run release 再执行；
- 成功后再由既有路径顺序清空 Timeline、Route、Lifecycle 与 Coordinator。

成功释放日志新增 `FormationHadScatter`、`FormationScatterTeardownReused` 与 `FormationScatterTeardownStatus`，使首次 teardown 和检查点恢复可以从原生日志区分。

## 5. 权威、身份与所有权边界

本阶段没有新增 ID 生成器、全局 Registry 或第二套 Authority。Run、Handoff、Route、Command、Session、Deployment 与资源证据仍沿 P27.18–P27.27 的确定性链派生并由下层验证。GameMode 只负责组合既有 owner：

- Combat Run 真值仍由唯一 `CombatRunCoordinator` 持有；
- Formation 产品与可选 scatter Route 仍由唯一 `FormationRunLifecycle` 持有；
- World 只在调用期间从 GameMode 借用，不被新接口保存或转交给调用方；
- Handoff 只读，不重算资源、不修改库存、不改写 placement；
- publication replay 由既有 Host/Session 命令记录负责，不由 GameMode 建第二份账本；
- release 拒绝不会转换成“成功清空”，也不会摧毁恢复所需的 owner。

因此本阶段闭合的是组合入口，而不是新增玩法执行通道。

## 6. P27.28 专项证明

三条瞬态无头 World 自动化覆盖：

1. `PublishReplayAndRelease`：GameMode 发布完整 2-Actor 批次；精确重放保持同一 Route/Command 且不增生；GameMode 结束路径按顺序清理 World、Formation、Timeline 与 Run。
2. `PreflightAndRebindFences`：未活动 GameMode、外来 Run Handoff、无效 Actor 类都在占用 Route 槽前失败；首次成功后 Actor 类漂移被拒；精确重放仍返回原命令；直接 teardown 不留下 publication owner。
3. `CoordinatorRecoveryPreservesOwners`：故意让 Player vitality host 丢失预期 Run 身份；首次 release 在 Coordinator 阶段拒绝，同时保留活动 Lifecycle、scatter/product 双检查点、Coordinator 与 Timeline，Actor 已按前缀清理且 late Publish 被围栏；身份修复后只复用双检查点并完成唯一 Run 释放。

首轮专项为 2 Success、1 Fail。唯一失败不是产品断言失败，而是测试故意触发的 `RunReleaseRejected` Error 尚未登记为预期日志。原始失败日志完整保留；仅在测试夹具中增加一次精确 `AddExpectedError` 后重新编译，复跑为 3 Success、0 Fail。成功日志明确记录 `FormationTeardownReused=1` 与 `FormationScatterTeardownReused=1`。

## 7. 自动化与回归证据

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| 首次专项（保留失败证据） | 2 | 1 | `3C0C7EE3044361215ECAB8EAE1B48886A52BDA7D813271DBFE90E106ECE0AF7C` |
| GameModeScatterPublication 最终专项 | 3 | 0 | `3E71A18821EBADBB880C209C7DF4A06372C795CF0C9054E753472EA56D10E537` |
| FormationRunLifecycle 全组 | 13 | 0 | `9E9DA2FBD2C91EFF9DC5D95F20613E8D51487452B3DEE8F188894BE8B1D60D6A` |
| `demo_map.V3.Attributes` | 4 | 0 | `E0164BC50FFF5FE2450F12D57F5E0A14BD82AE8CAA7059812589C1FB90E4DD08` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `AC82A823BF35DE6409024ACA04AAD06800C2AF59F9C05D07676B1F84C4D2060C` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `FE4420EB68F832868995A508D1FEF09D1A977DBC9416CD4A2D9BC267DDDCB68B` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `0A07EAD6E15955D56F545C26DECF7F8E9173E2BC85C71AD9F65B5FA5B66C8470` |
| 完整 `Shanmen.0_0_10` | 1,415 | 0 | `75DEE689E683210ACE250292D8086DFC24D8663F6E148BB3513AE4421E42D930` |

最终专项从 2026-09-14 03:02:53.740 UTC 运行至 03:03:19.528 UTC；完整 FormationRunLifecycle 从 03:03:48.247 至 03:04:37.789 UTC。完整根组从 03:08:43.986 运行至 04:34:09.166 UTC，持续约 1 小时 25 分 25 秒，原生退出 0；1,415 项全部 Success，Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

## 8. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 的既有 `M01GameMode` 规则新增 13 个 formation scatter 下游组：RunRoute、CommandHost、Session、Publication、Handoff、WorldDelivery、Deployment/Resource/Plan/Batch 与 Mastery authorization/adapter。GameMode 改动因此不能只靠本阶段专项证明；它必须同时通过完整 0.0.10 根与四组受影响的旧系统兼容测试。

- 映射自检：`SELF_TEST: PASS 504/504`，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=6 Rules=7 Required=83 Logs=7`，SHA-256 `459B17137B6CA4E6EB29083659B1274095637ACC400227F0752F6D96F3ADA1AE`；
- 初次 Editor 编译：35 actions，Result Succeeded，98.97 秒，日志 SHA-256 `8013A51F5568E57AC10517BA073B10115D8274B963C824276075B497AB07EB5E`；
- 登记预期日志后的 Editor 增量编译：4 actions，Result Succeeded，21.55 秒，日志 SHA-256 `6D050918178EE73E2BFA541E78294CBA933E6E13EF75CAD015713A0E8D127636`；
- 最终 Editor：target up to date，Result Succeeded，原生退出 0，1.77 秒；日志 SHA-256 `05D12AB89F626C2C0769C5E65185F4CECF22887F006413A8990B3E2F883FAC3B`；
- `UnrealEditor-demo_map.dll`：20,237,312 bytes，SHA-256 `F44A4FE422541E593613FEB0B865AA77A151C171422C6A3FE91F69AA6B83798C`；
- Game：34 actions，Result Succeeded，原生退出 0；UBA 41.14 秒，总计 45.56 秒；日志 SHA-256 `FDAF3E509DE00B7DAB6C405844DFD86A429C9854F7ABB744B93E61F1456AB250`；
- `demo_map.exe`：360,771,584 bytes，SHA-256 `68CAD98BB5D74BB92E2946DAF441192EC42C2941D95BEF076C15C70A341EB2C8`。

生产代码新增行静态扫描未发现 Tick/Timer、异步、随机 GUID/RNG、Actor 枚举、直接 Spawn/Destroy、Actor transform/owner 写入、物理输入或 Inventory/Profile/SaveGame 访问。唯一新增 World 表面是 GameMode 内部一次性借用 `UWorld* const World = GetWorld()`；测试夹具中的 Actor spawn 仅用于瞬态无头证明。Regression Map JSON 解析、`git diff --check` 与阶段差异检查均通过。

## 9. P/F 边界

P 阶段完成：已提交 Handoff → GameMode preflight → Lifecycle 唯一 Route 槽 → 显式 Publish/replay → scatter teardown 检查点 → product teardown 检查点 → Coordinator release；身份不一致、Actor 类漂移和 release 拒绝均失败关闭，失败前缀可恢复且不会二次进入 World。

F 阶段未执行：没有接入物理玩家输入，没有修改正式地图或内容资产，没有设计或调试玩法数值、手感、敌人行为、关卡、UI 表现或玩家体验；没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P27.28 已闭合 P27.27 指出的最后一个 formation scatter 产品组合入口。下一阶段不应继续增加玩法层能力；建议 P27.29 转入“底层框架闭合审计与冻结”：逐项核对已批准 0.0.10 契约是否都能从唯一产品根端到端表达，确认没有第二 Authority/Owner、无不可恢复 teardown、无未映射生产路径，整理仍属于 F 阶段的清单与已知债务，补齐索引/边界文档，并以最终完整回归和双目标构建形成可追溯 P 基线。只有审计发现会迫使 F 阶段返工的真实结构缺口，才允许再做一个最小 P 修复。

基线提交：`f369eb131dba0fbc967c8de2e9cdb4535a33091b`（P27.27）。分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-28-formation-scatter-gamemode-composition>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-28-formation-scatter-gamemode-composition/Docs/Report/Dev.D.UE.0.0.10.P27.28.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-28-formation-scatter-gamemode-composition/Docs/Log/Dev.D.UE.0.0.10.P27.28.r0_log.md>
