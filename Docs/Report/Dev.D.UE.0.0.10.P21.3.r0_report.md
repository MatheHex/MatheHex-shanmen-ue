# Dev.D.UE.0.0.10.P21.3.r0 Report

## 1. 结论

P21.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.2 已进入真实 World 的 canonical “练习飞剑”接到固定频率的近身威慑采样链：产品帧从既有 30 Hz Combat Run timeline 读取时钟，每 3 tick 最多执行一次真实 `UBoxComponent` overlap，并把注册实体的接触通过 P6.22 Router 交给既有 P6 Host。该链只产生零效果 `ThreatPresence`，不造成伤害、状态或资源变更。

```text
World threat sampler focused:             1 Success / 0 Fail
Shanmen.0_0_10 full:                   1226 Success / 0 Fail
Required legacy groups:                 123 Success / 0 Fail
Regression coverage:                     PASS (Changed=7 / Rules=3 / Required=64 / Logs=6)
Regression gate self-test:               PASS 429/429
Game + Editor Development:               PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是无头真实 World 的碰撞查询、时序、身份围栏和产品接线，不宣称视觉、手感或最终伤害体验通过。

## 2. 产品采样链

新增 `Fdemo_mapShanmenControlledWeaponWorldThreatSampler`，职责被限制为 Run-scoped cadence owner：

- 只接受 P21.2 已发布、正在 orbit 的 canonical TrainingFlyingSword World lifecycle；
- 读取既有 30 Hz fixed timeline，每 3 tick 形成一个 10 Hz 采样槽；
- 每次 owner pulse 最多执行一个 World overlap，长帧跳过过期槽而不追帧爆发；
- 查询 canonical weapon Actor 的真实 `UBoxComponent`，对象范围为 WorldDynamic 与 Pawn；
- 忽略 weapon/source，自稳定实体注册表解析、过滤并去重接触；
- 以 Run、Timeline、item instance、sequence、scheduled tick 派生确定性 intent ID；
- 委托既有 P6.22 Router 与 P6 Host 提交 `ThreatPresence`。

空 overlap 仍是一份成功、可审计的 NoOp 样本并推进时序。sampler 自身只保存身份、下一采样 tick 与提交水位，不持有 Actor、Timer、damage、effect、inventory 或第二套 combat authority。

## 3. GameMode 自动接线

`Ademo_mapGameMode` 已在 P21.2 World lifecycle 成功后绑定 sampler，在既有 orbit movement 完成后采样。这样碰撞盒使用本帧更新后的实际位置，同时仍由 canonical timeline 决定采样频率。

激活前会拒绝残留 sampler；绑定失败沿用既有 combat-product rollback。正常 Run end、孤儿回收与失败路径均记录已提交样本数并清空 sampler，且不改变原有 Host、Coordinator、World Actor 的权威结束顺序。

## 4. 真实 World 自动化

新增：

```text
Shanmen.0_0_10.Product.ControlledWeaponWorldThreatSampler.CanonicalCadenceOverlapAndFences
```

测试使用启用 scene/physics/trace collision 的真实 `GamePreview UWorld`，贯通 Profile、Code B、cutover、durable Run、Runtime、Coordinator、P21.2 World lifecycle、物理飞剑 `UBox`、M01 注册实体、P6.22 Router 与 P6 Host。

核心断言：

- tick 0 的真实 box overlap 命中已注册 M01，Host 消费一次 `ThreatPresence`；
- 敌方 vitality 与 committed impact 数保持不变，证明样本为零效果；
- 同 tick 重入与 tick 2 均为 NotDue，不能多花 sequence；
- tick 3 的空查询被接受并推进一次可审计样本；
- 从 tick 3 跳至 tick 35 只执行一个查询，下一槽为 tick 36；
- foreign timeline 被拒绝，owner、Router 与 Host 水位不变；
- durable item authority 在全程前后不变。

## 5. 首败与根因修复

首次 Editor 构建以默认 16 并发运行 33 actions，Windows 返回 error 1455，编译器报 C3859/C1076；这是 commit/pagefile 压力，不是源码错误。改用 `-MaxParallelActions=4 -NoUBA` 后 8 actions / 11.46s 成功，未修改 Windows 设置。

首次 focused 自动化为 0/1：tick-zero overlap 未命中。诊断重跑证明 sampler 已正确提交空样本（`Status=Sampled, Raw=0, Routed=0`），根因位于测试 World fixture：`.InitializeScenes(false)` 使 `.CreatePhysicsScene(true)` 与 `.EnableTraceCollision(true)` 无法建立可查询 scene。改为 `.InitializeScenes(true)` 后，同一 focused 测试以 1/0 通过。

自动化启动期间还遇到另一个 worktree 的全局 `Build.bat` mutex。UE 5.8 启动路径在 `ValidatePlatforms` 阶段触发该锁；在 SDK 已验证的前提下使用引擎支持的 `-Multiprocess` 路径后解除等待。只终止了本轮等待进程，没有干预其它任务。

## 6. 回归证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| World threat sampler focused | 1 | 0 | 262,198 | `47AEA77249321A752591A79763A636CF0A935FE78BFE7A1C09A7113F4FC1473B` |
| `Shanmen.0_0_10` full | 1226 | 0 | 1,883,112 | `AF4965CBF1430BF6EBACCD2C3F7F1E7243F97888814B24688522D25EC70B8B4B` |
| `demo_map.V3.Attributes` | 4 | 0 | 263,366 | `3FB851B91339B8976B4EB48F0CA05A235C76362435F6D8CDEDBA296375FE73FE` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 303,924 | `79F58B0CC8EA74692707136440D0969E6D493D3F88D5F7087ECCD8E39999B7EF` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 283,595 | `75F624E29F22746721DE56813FE13691018B80BE3C1D09BB04DB2F06D32FD1A0` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 308,472 | `07AE7EA9557830ABBF59F2DAD28EAA3E555BD5B0574F10D5220766995575EE20` |
| `demo_map.P4.Hotbar` | 7 | 0 | 266,188 | `16948071F24E0C5032D5F1CF27A4665F46091C733DC9E06403F6715EB9AEA266` |

七份最终自动化日志合计 1350/0（focused 与 full 有意重叠）。完整套件从 `2026-09-07 18:30:15.699` 到 `19:55:34.300`，单一 UnrealEditor-Cmd 实例自然清空 1226 项并原生退出 0；Fatal、Unhandled、Assertion、Ensure 为 0。

按改动路径门禁：

- regression self-test：429/429，42,421 bytes，SHA-256 `0D51F0B6F9377FA6FA1411583F007529C8EDA025D58F4F32B9187EC30C527EC6`；
- changed-file gate：`PASS Changed=7 Rules=3 Required=64 Logs=6`，7,474 bytes，SHA-256 `CF1E9398D914D5AF8E4FC028CCC6E5720768B52C3D8602E81AAAD55CC202E97C`。

## 7. 静态审计与构建

- 7 个实现、测试和流程文件，约 `+1117 / -8`；
- 新 sampler 中 `ApplyDamage`、库存事务、`StartPreparedRun`、RNG、Actor Tick 与 Timer 命中 0；
- 唯一 `Reserve` 是 `TArray` 容量预留，不是物品资源预留；
- `git diff --check` 退出码 0，仅有工作树 LF→CRLF 提示；
- 103 个用户原有 untracked 文件保持未暂存；
- 验证结束后本项目无 UnrealEditor-Cmd 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | Failed / native 6 | 33 / 42.86s | `FBC77FB586585D6C56551007BC7CDD1894B83114247DD5417A008F84B7231FDA` |
| Editor bounded retry | Succeeded | 8 / 11.46s | `C15AC20F27AF1B77AC5C505A0E047DB1093BC0BE4DA5B412519ACC985EDEC1C8` |
| Game Development final | Succeeded / native 0 | 32 / 63.40s | `3333777E18548E5761EEBBF9E91692A4021CFF471AC18D01B35AF65BD19C6590` |
| Editor Development final | Succeeded / native 0 | 0 / 1.00s | `2F5042E066B35CDDCA459E42811D0800C0F531E8DF2D3B9F1AE6DB25673928CD` |

最终产物：

- `demo_map.exe`：359,373,824 bytes，SHA-256 `3AE3698A04614E4174508E375CA1E735A9EEEB02EAE0CCC3E3C78FA957916C23`；
- `UnrealEditor-demo_map.dll`：18,533,376 bytes，SHA-256 `CA87D75ADDF8A670D96F5F8C187BEA83EE1EF07DD28DCD3DF7FF18E1CF0AE232`。

## 8. P/F 边界与后续

PASS：canonical TrainingFlyingSword 在真实 World 中按固定 cadence 查询物理 box；接触只解析注册实体；身份和 timeline 不匹配失败关闭；空样本、长帧与重复 pulse 行为确定；P6.22/P6 Host 收到零效果 ThreatPresence；全量、旧回归、门禁和双目标构建通过。

未声明：可视化近身威慑、敌人 AI 反应、伤害、格挡、击退、VFX/音效、输入体验或人工游戏验收。

下一步建议 P21.4 在不新增 Impact/effect 权威的前提下，把既有 `ThreatPresence` 消费成一个边界清楚、可观察的产品行为；本轮未实现该行为。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling/Docs/Report/Dev.D.UE.0.0.10.P21.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-3-training-flying-sword-world-threat-sampling/Docs/Log/Dev.D.UE.0.0.10.P21.3.r0_log.md>
