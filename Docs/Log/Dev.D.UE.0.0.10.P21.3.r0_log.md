# Dev.D.UE.0.0.10.P21.3.r0 Development Log

## 1. 基线与目标

- base：`398f26f73df2409eb2f85b9a24f6362cba5ccc9b`（P21.2 canonical flying-sword World lifecycle）；
- branch：`agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling`；
- 目标：让 canonical TrainingFlyingSword 在真实 World 中按固定 cadence 采样近身接触，并复用 P6.22 Router / P6 Host；
- 边界：只投递零效果 `ThreatPresence`，不建立第二套 damage/effect/item/Run authority，不启动可视产品，不修改 Windows 或 Engine。

## 2. 起始审计与设计选择

P21.2 已创建真实飞剑 Actor，并由 Combat Run 自动负责准入、orbit movement 与 teardown。P6.22 已有 deterministic threat sample intent、registered contact route 和 P6 Host consumption，但产品帧尚未从真实碰撞盒生成该 intent。

本轮新增一个无 UObject 所有权的 Run-scoped sampler。它以现有 30 Hz timeline 为唯一时钟，以 3 tick 为间隔，使用 P21.2 Actor 的 `UBoxComponent` 做 World overlap，再把接触交给现有 stable entity registry 与 P6.22 Router。选择 cadence sampling 而非 overlap delegate 或 Actor Tick，是为了让重放身份、长帧策略和查询上限都可审计。

## 3. 实现

新增：

- `demo_mapShanmenControlledWeaponWorldThreatSampler.h/.cpp`；
- `Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus`；
- `Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult`。

`TryBegin` 固定 exact Run、Timeline 与 canonical item identity。`TrySample` 的顺序为：

1. 校验 owner、World、lifecycle、Coordinator、Host、Router 与 timeline；
2. 判断当前 tick 是否到期以及飞剑是否 orbiting；
3. 读取 canonical `UBoxComponent` 的世界位置、旋转与缩放后 extent；
4. 查询 WorldDynamic + Pawn，忽略 source 与 weapon；
5. 通过 stable entity registry 解析接触，按 entity/component/item index 稳定排序与去重；
6. 由 Run/Timeline/item/sequence/scheduled tick 派生 intent ID；
7. 先在候选 owner/Router/Host 上路由，完整通过后才原子提交状态；
8. 以 observed tick 的下一个 cadence 边界更新水位，跳过过期槽而不追帧。

GameMode 在 P21.2 World lifecycle 成功后启动 sampler，在每帧既有 orbit pump 后调用它；所有 Run 结束、回滚与孤儿清理路径均增加 sampler reset 和样本水位日志。

## 4. 测试实现

新增 `CanonicalCadenceOverlapAndFences`。fixture 的 GamePreview World 启用 scene、physics scene 与 trace collision，完整建立 Profile → Code B/cutover → durable Run/Runtime → Coordinator → P21.2 World Actor → physical box overlap → stable M01 entity → P6.22 Router → P6 Host。

覆盖：tick 0 实际命中、零 vitality/impact 效果、同 tick/tick 2 NotDue、tick 3 空样本、tick 35 长帧仅采一次且 next=tick 36、foreign timeline 无状态突变，以及 durable authority 不变。全量测试由 P21.2 的 1225 精确增加至 1226 项。

## 5. 首次失败证据与修复

### 5.1 构建资源首败

首次 Editor build 使用默认 16 并发、UBA、33 actions。系统 committed memory 已接近上限，多个编译进程返回 Windows 1455；UBT 最终为 `OtherCompilationError`、Build.bat native exit 6。

| Log | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P21.3.r0_editor_initial.log` | Failed / 33 actions / 42.86s | 5,621 | `FBC77FB586585D6C56551007BC7CDD1894B83114247DD5417A008F84B7231FDA` |
| `P21.3.r0_editor_retry.log` | Succeeded / 8 actions / 11.46s | 2,602 | `C15AC20F27AF1B77AC5C505A0E047DB1093BC0BE4DA5B412519ACC985EDEC1C8` |

修复只约束本轮构建为 `-MaxParallelActions=4 -NoUBA`，没有扩大 pagefile 或修改系统。

### 5.2 自动化启动锁等待

首次启动 focused 测试时，UE 的 `ValidatePlatforms` 启动路径等待另一个 worktree 正在持有的全局 `Build.bat` mutex。保留等待日志 `P21.3.r0_focused_sdk_lock_wait.log`：85,344 bytes，SHA-256 `B79C628A7EDCA387639BBAA55AEEE617C634161E82BE501950FD49695BDEAC2F`。

检查 UE 5.8 `TargetPlatformManagerModule.cpp` 后，在 SDK 已验证前提下采用引擎支持的 `-Multiprocess` 路径。仅终止本轮等待实例，没有结束锁持有者或其它任务。

### 5.3 真实 overlap 首败

首次 focused 测试 0/1，tick-zero overlap 未命中：

- `P21.3.r0_focused_first.log`：263,230 bytes，SHA-256 `98B819C0C667B046EFE45C4A384BF550043DFA985B9222F08A3328717D4D2E81`；
- 诊断重编译：4 actions / 24.31s，SHA-256 `2A58BFFC161590D03568D8E71274E0E33C0030BB4F58FE9DBBFC8441DEF65774`；
- `P21.3.r0_focused_diagnostic.log`：262,178 bytes，SHA-256 `FFC44470370E572EE6EF0BB950DAC72BFF2D97558E26D483E1FA88F8FBE3EEE5`。

诊断值为 `Status=Sampled, Raw=0, Routed=0, RouteAccepted=1`，排除了 cadence、Router 与 Host 拒绝。根因是 fixture 使用 `.InitializeScenes(false)`：即使同时请求 `.CreatePhysicsScene(true)` 与 `.EnableTraceCollision(true)`，也没有初始化可查询 scene。

将 fixture 改为 `.InitializeScenes(true)` 后重编译 4 actions / 17.21s（SHA-256 `0F5B740FA7E7F1E30640A57F852D94CAE1FBEC6843F499499D8316C9A6A6C8C4`），focused retry 1/0：262,198 bytes，SHA-256 `47AEA77249321A752591A79763A636CF0A935FE78BFE7A1C09A7113F4FC1473B`。

## 6. 全量与旧回归

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.3.r0_full.log` | `Shanmen.0_0_10` | 1226/0 | 1,883,112 | `AF4965CBF1430BF6EBACCD2C3F7F1E7243F97888814B24688522D25EC70B8B4B` |
| `P21.3.r0_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 263,366 | `3FB851B91339B8976B4EB48F0CA05A235C76362435F6D8CDEDBA296375FE73FE` |
| `P21.3.r0_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | 303,924 | `79F58B0CC8EA74692707136440D0969E6D493D3F88D5F7087ECCD8E39999B7EF` |
| `P21.3.r0_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 283,595 | `75F624E29F22746721DE56813FE13691018B80BE3C1D09BB04DB2F06D32FD1A0` |
| `P21.3.r0_item_use.log` | `demo_map.ItemUseAndArmor` | 46/0 | 308,472 | `07AE7EA9557830ABBF59F2DAD28EAA3E555BD5B0574F10D5220766995575EE20` |
| `P21.3.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 266,188 | `16948071F24E0C5032D5F1CF27A4665F46091C733DC9E06403F6715EB9AEA266` |

最终测试证据合计 1350/0（focused 与 full 重叠）。full 从 `18:30:15.699` 至 `19:55:34.300`，自然收到 `Automation Test Queue Empty 1226 tests performed`，原生退出 0。五组旧回归合计 123/0；所有最终日志 Fatal、Unhandled、Assertion 与 Ensure 均为 0。

## 7. Regression map

为两个新 sampler 文件登记 `ControlledWeaponWorldThreatSampler` rule，要求 sampler、P6.22 Router、P6 Host、P21.2 World lifecycle、fixed timeline、Coordinator、WorldGameplay 与 CombatRuntime 证据。

自测新增 full-suite 正向覆盖与 router-only 必须失败两项，并补齐 self-test fixture 中 P6.22 Router 日志变量：

- self-test：`PASS 429/429`；42,421 bytes；SHA-256 `0D51F0B6F9377FA6FA1411583F007529C8EDA025D58F4F32B9187EC30C527EC6`；
- gate：`PASS Changed=7 Rules=3 Required=64 Logs=6`；7,474 bytes；SHA-256 `CF1E9398D914D5AF8E4FC028CCC6E5720768B52C3D8602E81AAAD55CC202E97C`。

## 8. 静态与最终构建

- 5 tracked changed + 2 owned new production files，约 `+1117 / -8`；
- sampler forbidden scan：damage application、item resource transaction、Run creation、RNG、Actor Tick、Timer 均为 0；
- `Resolved.Reserve(Overlaps.Num())` 仅预分配本地数组容量；
- `git diff --check` exit 0；
- residual project UnrealEditor-Cmd process 0。

| Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---:|---|
| Game final | Succeeded / native 0 | 32 / 63.40s | 4,269 | `3333777E18548E5761EEBBF9E91692A4021CFF471AC18D01B35AF65BD19C6590` |
| Editor final | Succeeded / native 0 | 0 / 1.00s | 1,035 | `2F5042E066B35CDDCA459E42811D0800C0F531E8DF2D3B9F1AE6DB25673928CD` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,373,824 bytes / SHA-256 `3AE3698A04614E4174508E375CA1E735A9EEEB02EAE0CCC3E3C78FA957916C23`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,533,376 bytes / SHA-256 `CA87D75ADDF8A670D96F5F8C187BEA83EE1EF07DD28DCD3DF7FF18E1CF0AE232`。

## 9. 提交边界

计划提交 7 个实现、测试和流程文件、本 Report 与本 Development Log，共 9 个文件。103 份用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.3` raw logs 不入 Git。

未修改 Content、地图、项目资源、配置、Engine、Windows、save schema、inventory authority、Run authority、P6 orbit 数学或 Impact/effect resolver。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling/Docs/Report/Dev.D.UE.0.0.10.P21.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling/Docs/Log/Dev.D.UE.0.0.10.P21.3.r0_log.md>
