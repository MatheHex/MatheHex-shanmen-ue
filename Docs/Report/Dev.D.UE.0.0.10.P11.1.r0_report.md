# Dev.D.UE.0.0.10.P11.1.r0 Report

## 1. 结论

P11.1 **PASS**。本阶段在 P11.0 唯一普通武器格挡窗口之上建立了精准格挡时序策略：调用方冻结一个外部单调时间线、Active 起点 tick 与 Perfect 结束 tick，CombatRuntime 只对显式 observation 做纯分类，不拥有 Tick、Timer、物理时钟或第二套动作生命周期。

精准区间采用半开语义 `ActiveStartTick <= ObservedTick < PerfectEndTick`。命中该区间时，只投影 `PreventAll / PerfectGuard / DefensePerfectGuard`；从 `PerfectEndTick` 起，只返回 P11.0 原有 `ReduceFraction / Guard / DefenseGuard` 层。公开 receipt 只暴露一个最终 `GetLayer()`，内部普通层证明不会作为第二个可消费层暴露，因此精准与普通格挡在结构上互斥。

正式验证为 WeaponPerfectGuard `6/6`、WeaponGuard `6/6`、ActionLifecycle `1/1`、CombatCore `9/9`、CombatRuntime `99/99`、`Shanmen.0_0_10` 全量 `467/467`，合计 `588` Success、`0` Fail。90-rule mapping、140/140 mapping self-test、changed-file gate、边界扫描、`git diff --check`、Editor Development 与 Game Development 全部通过。

本阶段没有硬编码产品时长、秒数或帧率；测试使用的 `10..15` 只是测试内容。也没有实现格挡方向、命中角度、武器耐久、体力/灵力、产品输入、GAS ability、Actor/World 适配或真实反馈。

## 2. 功能性

### 2.1 外部时间线策略

`FShanmenWeaponPerfectGuardPolicy::TryCapture` 冻结：

- P11.0 的 exact `FShanmenWeaponGuardWindowReceipt`；
- 非空 caller-owned `TimelineId`；
- 非负 `ActiveStartTick`；
- 严格大于起点的 `PerfectEndTick`；
- 非空 `PerfectRuleId`。

所有 tick 都是调用方时间线中的不透明整数。CombatRuntime 只比较顺序，不读取系统时间、不换算秒数、不推定帧率，也不负责验证跨 observation 的单调序列；时间源与采样顺序仍由上层权威拥有。

### 2.2 半开区间与互斥投影

`FShanmenWeaponGuardTimingEvaluator::TryProject` 先要求：

1. P11.0 window 当前仍对同一 action runtime 有效；
2. policy 绑定 window 的 exact WindowId 与 ReceiptId；
3. observation 有效且来自 policy 的 exact TimelineId；
4. observation 不早于 ActiveStartTick；
5. P11.0 普通格挡层仍可由同一 Active runtime 投影。

分类结果只有一个：

- `[start, end)`：生成 `PreventAll / PerfectGuard / DefensePerfectGuard`；
- `[end, +∞)`：原样选择 P11.0 普通 Guard layer；
- pre-start、foreign timeline、foreign window、Recovery 或 Interrupted：失败关闭。

精准层只含 `DefensePerfectGuard`，普通层只含 `DefenseGuard`。精准层不会叠加普通 `0.25` 减伤；100 点匹配伤害直接得到 `PerfectGuarded / PreventedDamage=100 / FinalDamage=0`，并保持守恒。边界 tick `15` 回落普通层后得到测试内容定义的 `25 / 75`。

### 2.3 过滤、来源与身份

精准层继承 P11.0 冻结 definition 的 damage/source/target required/blocked filters，并继续使用冻结武器实例作为 `SourceInstanceId`。没有真实耐久 reservation，所以 `bRequiresCommitOnTrigger=false`；时序成功不能伪装成资源提交成功。

PolicyId 纳入 window、timeline、start/end 与 perfect rule；ObservationId 纳入 timeline 与 tick；perfect LayerId 纳入 policy、observation、普通层与武器来源；最终 receipt 纳入 policy、observation、P11.0 projection、band 与 selected layer。等价冻结输入可重放相同身份，区间或 rule 变化会产生不同身份。

## 3. 完整性与兼容性

- 复用 P11.0 window 与 P3 action orchestrator，没有复制格挡或动作状态机；
- 复用 CombatCore 已有 `PerfectGuard` ordering、`PreventAll`、`DefensePerfectGuard` 与 resolver outcome；
- P11.0 普通 Guard 层的 LayerId、fraction、filters 与 source identity 不变；
- public receipt 只公开最终 selected layer，内部 ordinary provenance 不形成第二消费入口；
- 没有修改 CombatCore enum、tag tree、resolver、Impact 或 action snapshot；
- 没有修改既有 P4/P5/P9/P10 产品和资源权威；
- 没有 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、RNG、Timer、Tick callback 或 GetWorld 依赖。

## 4. 关键不变量

1. Timing policy 必须绑定一个有效 P11.0 exact guard window；
2. TimelineId 必须有效，start 必须非负，end 必须严格大于 start；
3. ObservationId 由 timeline 与 tick 确定性派生；
4. observation timeline 必须与 policy 完全一致；
5. pre-start observation 失败关闭；
6. `[start,end)` 严格属于 Perfect，`end` 本身属于 Ordinary；
7. action runtime 仍是唯一窗口生命期权威；
8. Perfect 与 Ordinary 每次只选择一个公开 layer；
9. Perfect 固定为 `PreventAll / PerfectGuard / DefensePerfectGuard`；
10. Ordinary 原样复用 `ReduceFraction / Guard / DefenseGuard`；
11. 两种 layer tags 不交叠；
12. 两种路径完整继承 P11.0 tag filters 与武器 source identity；
13. 无资源 reservation 时不声明 trigger commit；
14. resolver 的 RawDamage、PreventedDamage 与 FinalDamage 保持守恒；
15. 等价输入重放相同 identity，interval/rule 变化产生不同 identity。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` 六个测试：

- `PolicyContract`：window/timeline/tick/rule capture 与非法输入拒绝；
- `PerfectBandProjection`：start、end-1、互斥 tags、PreventAll 与 100 点守恒；
- `OrdinaryBoundary`：exact end tick 回落 P11.0 原层与 25/75 结果；
- `TimelineBinding`：pre-start、foreign timeline 与 foreign window 拒绝；
- `ActionPhaseBoundary`：Recovery 与 Interrupted 拒绝；
- `DeterministicReplay`：等价重放稳定，interval/rule 变化产生新身份。

同时执行 P11.0 WeaponGuard、ActionLifecycle、CombatCore、CombatRuntime 宽回归与 0.0.10 全量回归。CombatRuntime 由 `93` 增至 `99`，全量由 `461` 增至 `467`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenWeaponPerfectGuard.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenWeaponPerfectGuard.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponPerfectGuardTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现与门禁为 `5 files / +993 / -0`；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt、Report、交接文档与用户文件均未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P11.1-WeaponPerfectGuard-final.log` | `WeaponPerfectGuard` | 6 | 0 | `B7BD207CFF48D2692C1FE0B26D2FED4E89C3638815B0370F9F96011A334BCC99` |
| `P11.1-WeaponGuard-final.log` | `WeaponGuard` | 6 | 0 | `C3A805B8BB706D07834ECBCE91D79A33624A1E23686F9D0771EA83877337CD17` |
| `P11.1-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `DC7613811431FC2E679FE9DE4CE650724267AE218934CCE9B35E8F81E76E81B0` |
| `P11.1-CombatCore-final.log` | `CombatCore` | 9 | 0 | `AF764B4E30D4FF80DC206D7D5DE52BB34D2A382AC23DF421F4952BBFE2976C1E` |
| `P11.1-CombatRuntime-final.log` | `CombatRuntime` | 99 | 0 | `E293207951DF9E34AADF30BE95653B114DF50427D50D927E83BB5DC4DC8FA8AF` |
| `P11.1-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 467 | 0 | `2E282C74EE1104A094347D6BE1F70883922B449C2E179A48F7ADE1DB4D3E004A` |

每份日志都包含唯一目标 RunTests、terminal queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=90
SELF_TEST: PASS 140/140
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=6
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- mapping SHA-256：`7AB7A538E145E576B661E93AC44969344FC94AA5F98C496ECDF03FF545CC44A1`；
- self-test SHA-256：`7BC6C4873E00BA9995E85B5F4EC526EEDD87B6BED72FEA3B757C204070172100`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 6 / 22.53s | 0 | `31293F0894E51C9687ED86259F40F547BA415654AC22C5FA5C94D0045635E11A` |
| Game Development | Succeeded | 5 / 28.75s | 0 | `E8C841E99850CD0EA9F71AD9FBE00D473318AD531B81FEC3D71E34F147ECCFAC` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1424896` bytes，SHA-256 `23373EB70ED7FC61BCAA16A3C9FA664C3F2C206D1610C8E40D2D7859E3F4F166`；
- `demo_map.exe`：`354039296` bytes，SHA-256 `745249F251C8D215E6567934142BB65A1D23FBEC84E6BD2AD1385026909705B0`。

## 9. 真实异常

最终源码的 UHT、C++、六组 Automation、changed-file gate、Editor 与 Game 构建均首轮成功，没有源码、测试、内存、页面文件、外层超时或 Win64 SDK 失败。

首版验证后进行了一次主动 API 收紧：移除 public ordinary-projection getter，避免调用方把内部普通证明与最终 selected layer 同时消费；随后按最终源码重新执行 Editor、六组 Automation 与 Game 构建。该轮没有失败，也没有放宽测试或门禁。

Automation 启动日志包含 UE 5.8 自带 `UE::UnifiedErrorTest` 初始化自检噪声，以及非 Win64 平台 SDK 无效信息；selected tests 全部 Success、queue 正常清空、Win64 VALID、进程退出 `0`，不属于产品故障。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值契约、代码审查、无头 Automation、静态/路径门禁、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

后续可以在不改写 P11.0/P11.1 的前提下增加格挡方向/夹角政策，或通过现有 durable item authority 接入真实武器耐久 prepare/commit/cancel；二者都不应让 timing evaluator 自己拥有世界、输入或资源权威。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-1-perfect-guard-timing/Docs/Report/Dev.D.UE.0.0.10.P11.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-1-perfect-guard-timing/Docs/Log/Dev.D.UE.0.0.10.P11.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-1-perfect-guard-timing>
