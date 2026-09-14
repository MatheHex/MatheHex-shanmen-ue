# Dev.D.UE.0.0.10.P27.28.r0 Development Log

## 1. 阶段元数据

- 阶段：P27.28（Formation scatter GameMode composition）
- 父提交：`f369eb131dba0fbc967c8de2e9cdb4535a33091b`
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`
- 目标：把 P27.27 的唯一 scatter publication Lifecycle 接到 GameMode-owned Combat Run 根，并修复 Run release 拒绝时的破坏性 Reset。
- 限制：只做底层框架闭合；不进入物理输入、正式地图、内容资产、玩法数值、UI 或体验开发。

## 2. 起点与结构缺口

P27.27 已实现 Handoff-bound Route、显式 Publish/replay、scatter → product → Coordinator 的顺序 teardown、双检查点恢复和整体 Owner 转移，但 GameMode 只持有 Lifecycle，没有受控发布入口。外部调用若自行提供 World/Route/Command，会绕过 GameMode 唯一组合根。

检查 GameMode 结束路径还发现：`ControlledWeaponRunLifecycle::TryEndRun` 拒绝后会 Reset Coordinator、Formation Lifecycle、Timeline 与多项 Run owner。该路径会摧毁 P27.27 已保存的 scatter/product teardown 检查点，使“前缀成功、最后 release 失败”的状态不可恢复。

## 3. 本阶段文件

1. `Source/demo_map/demo_mapGameMode.h`
   - 声明 `PublishFormationScatterForActiveCombatRun`；
   - 增加仅测试可见的 GameMode 访问友元。
2. `Source/demo_map/demo_mapGameMode.cpp`
   - 实现 GameMode-owned Coordinator/Lifecycle/Handoff/World preflight；
   - 委托唯一 Lifecycle Route 完成 Publish/replay；
   - 扩展成功 release 日志的 scatter 检查点字段；
   - 删除 Run release 拒绝后的破坏性 owner Reset。
3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
   - 将共享 fixture 拆出可复用 `StartEnvironment`；
   - 增加瞬态 GameMode 夹具和受限测试访问器；
   - 新增 3 条 GameMode composition/recovery 自动化。
4. `Scripts/ShanmenRegressionMap.json`
   - 为 `M01GameMode` 增加 13 个 formation scatter 下游证据组。
5. `Docs/Report/Dev.D.UE.0.0.10.P27.28.r0_report.md`
6. `Docs/Log/Dev.D.UE.0.0.10.P27.28.r0_log.md`

实现与测试文件净差异（写文档前）：497 insertions、17 deletions；其中生产 GameMode 头/实现为 112 insertions、16 deletions，测试为 372 insertions、1 deletion，回归映射为 13 insertions。

## 4. 实现顺序

1. 读取 P27.27 Report/Log、当前 Git 状态与 P/F 边界；确认 103 个既有未跟踪用户文件不属于本阶段。
2. 复用 P27.18–P27.27 的 Run/Handoff/Route/Host/Session 链，不新增 Authority 或 ID 工厂。
3. 在 GameMode 增加单一发布组合入口，先验证 Coordinator 与 Lifecycle 活动和有效，再验证三处 Run 身份与 Handoff 有效性，最后借用 GameMode 自身 World。
4. 仅在 preflight 全部通过后打开 Lifecycle 唯一 Route 槽并 Publish；拒绝结果保留 Event、Run、可选 Route 与诊断。
5. 检查 GameMode release 失败路径，删除下层拒绝后的强制 Reset，使 Lifecycle 双检查点、Coordinator 与 Timeline 可供同一路径重试。
6. 在成功 release 日志增加 scatter owner/检查点/重放信息。
7. 增加发布重放、预检/重绑围栏、Coordinator 拒绝恢复三条无头测试。
8. 扩展改动文件驱动回归映射，运行专项、兼容、完整根组、覆盖门和双目标构建。

## 5. 首次失败与修正

首次专项运行：2 Success、1 Fail，原始日志 SHA-256：

`3C0C7EE3044361215ECAB8EAE1B48886A52BDA7D813271DBFE90E106ECE0AF7C`

失败测试为 `CoordinatorRecoveryPreservesOwners`。所有契约断言都已执行到预期恢复路径；失败原因是测试故意注入 Coordinator 身份冲突后，GameMode 正确发出的 `RunReleaseRejected` Error 被 Automation Controller 当作未预期日志。日志精确诊断为 player vitality host 不再拥有预期 Run identity。

修正仅限测试夹具：使用 `AddExpectedError` 精确登记该上下文的一次预期拒绝日志，没有降低断言、过滤其它错误或修改生产路径。修正后 Editor 增量编译成功，最终专项 3/3。

## 6. 专项与兼容测试

| 日志 | 组 | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P27.28_gamemode_scatter_focused.log` | `...FormationRunLifecycle.GameModeScatterPublication` | 3 | 0 | `3E71A18821EBADBB880C209C7DF4A06372C795CF0C9054E753472EA56D10E537` |
| `P27.28_formation_run_lifecycle.log` | `...FormationRunLifecycle` | 13 | 0 | `9E9DA2FBD2C91EFF9DC5D95F20613E8D51487452B3DEE8F188894BE8B1D60D6A` |
| `P27.28_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4 | 0 | `E0164BC50FFF5FE2450F12D57F5E0A14BD82AE8CAA7059812589C1FB90E4DD08` |
| `P27.28_legacy_enemy_skill.log` | `demo_map.EnemySkillFramework` | 44 | 0 | `AC82A823BF35DE6409024ACA04AAD06800C2AF59F9C05D07676B1F84C4D2060C` |
| `P27.28_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | `FE4420EB68F832868995A508D1FEF09D1A977DBC9416CD4A2D9BC267DDDCB68B` |
| `P27.28_legacy_item_armor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `0A07EAD6E15955D56F545C26DECF7F8E9173E2BC85C71AD9F65B5FA5B66C8470` |

专项关键事实：

- 2 个 publication Actor 首次发布，精确 replay 后仍为 2；
- 外来 Run、无效 Actor 类和 class drift 均不替换唯一 Route；
- 首次 release 拒绝后，Actor 已由成功前缀清理为 0；
- Lifecycle 同时保留 scatter 与 product teardown 检查点；
- Coordinator/Timeline 仍持有原 Run，late Publish 失败关闭；
- 身份修复后日志记录 `FormationTeardownReused=1`、`FormationScatterTeardownReused=1`，随后所有 owner 为空。

## 7. 完整根组

- 命令根：`Automation RunTests Shanmen.0_0_10`
- 开始：2026-09-14 03:08:43.986 UTC
- 结束：2026-09-14 04:34:09.166 UTC
- 结果：1,415 Success、0 Fail、原生退出 0
- Fatal / Unhandled Exception / Ensure condition failed：0
- 日志 SHA-256：`75DEE689E683210ACE250292D8086DFC24D8663F6E148BB3513AE4421E42D930`

完整根组比 P27.27 的 1,412 项增加本阶段 3 项，且终止于原生 `Automation Test Queue Empty 1415 tests performed`。

## 8. 回归映射与构建

映射自检：

- `SELF_TEST: PASS 504/504`
- SHA-256：`507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`

改动文件驱动覆盖门：

- `REGRESSION_COVERAGE: PASS Changed=6 Rules=7 Required=83 Logs=7`
- SHA-256：`459B17137B6CA4E6EB29083659B1274095637ACC400227F0752F6D96F3ADA1AE`
- 证据日志：最终专项、FormationRunLifecycle、完整 `Shanmen.0_0_10`、V3 Attributes、EnemySkillFramework、V2RangedCompatibility、ItemUseAndArmor。

Editor：

- 初次真实编译：35 actions，Result Succeeded，总计 98.97 秒；
- 测试夹具修正后增量：4 actions，Result Succeeded，总计 21.55 秒；
- 最终核验：target up to date，Result Succeeded，原生退出 0，总计 1.77 秒；
- 最终日志 SHA-256：`05D12AB89F626C2C0769C5E65185F4CECF22887F006413A8990B3E2F883FAC3B`；
- `UnrealEditor-demo_map.dll`：20,237,312 bytes，SHA-256 `F44A4FE422541E593613FEB0B865AA77A151C171422C6A3FE91F69AA6B83798C`。

Game：

- 34 actions；UBA 41.14 秒，总计 45.56 秒；
- Result Succeeded，原生退出 0；
- 最终日志 SHA-256：`FDAF3E509DE00B7DAB6C405844DFD86A429C9854F7ABB744B93E61F1456AB250`；
- `demo_map.exe`：360,771,584 bytes，SHA-256 `68CAD98BB5D74BB92E2946DAF441192EC42C2941D95BEF076C15C70A341EB2C8`。

卫生：

- Regression Map JSON 解析 PASS；
- 生产新增代码边界扫描无 Tick/Timer、异步、随机 GUID/RNG、Actor 枚举、直接 Spawn/Destroy、Actor transform/owner 写入、物理输入、Inventory/Profile/SaveGame；
- 唯一新增 World 表面是 GameMode 内部一次性 `GetWorld()` 借用；
- `git diff --check`：PASS；
- 精确暂存目标为本节列出的 6 个实现、测试、映射和交接文件；
- 103 个既有未跟踪用户文件保持原样且不得暂存。

## 9. P/F 边界

P 阶段完成：GameMode-owned Run/Handoff/Lifecycle/World preflight，唯一 Route 发布与重放，顺序 teardown，双检查点恢复，以及拒绝后 owner 保留。

F 阶段未执行：没有物理玩家输入、正式地图或内容资产、玩法数值/手感、敌人行为、关卡、UI/体验工作；没有 Unreal Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P27.29 应执行底层框架闭合审计与架构冻结，而非继续添加玩法：建立已批准契约到唯一产品根的索引，审查 Authority/Owner/Run teardown/replay/recovery 是否仍有真实结构缺口，列出债务和明确留给 F 阶段的工作，补齐回归映射与基线文档。若审计无阻塞缺口，则运行最终完整回归与双构建，形成 P 阶段冻结提交并暂停自动化；若发现缺口，只做最小底层修复后再冻结。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-28-formation-scatter-gamemode-composition>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-28-formation-scatter-gamemode-composition/Docs/Report/Dev.D.UE.0.0.10.P27.28.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-28-formation-scatter-gamemode-composition/Docs/Log/Dev.D.UE.0.0.10.P27.28.r0_log.md>
