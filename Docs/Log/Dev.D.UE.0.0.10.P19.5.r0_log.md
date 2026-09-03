# Dev.D.UE.0.0.10.P19.5.r0 Development Log

## 1. 目标与基线

- 基线：`3c4f1cc1b9087cf3b8b4b62d89ccd7d661d1aa09`（P19.4）；
- 分支：`agent/0.0.10-p19-5-divine-sense-product-session`；
- 目标：把 P19.3 Host 与 P19.4 Router 组合成一个绑定 ready Combat Run 的 Product Session，并公开 pointer-free availability；
- 约束：不接物理输入、UI/表现、Actor discovery、持续 Tick/Timer、存档或最终数值定案。

## 2. 设计审计

审计了 P19.3 Product Host、P19.4 Command Router、Combat Run Coordinator，以及现有 SwordQi、ThrownWeapon、WeaponGuard 的 Session/Controller 结构。

P19.4 已经具备以下完整能力：

- Host 与 Router 原子 staging/publish；
- deterministic command / Host / Router / receipt 身份；
- opening → expected → before → after 的资源 snapshot chain；
- exact replay 不读取 World/source/subjects/provider；
- capacity、activation conflict、stale projection 与 rollback。

因此 P19.5 没有复制 pulse、资源或 World observation 逻辑。缺口只在产品生命周期：调用方需要一个明确对象负责把 Host/Router 绑定到当前 Run，控制 begin/end，并在不泄露 authority 的情况下查询“现在是否可发动”。

## 3. Session 状态与 begin

新增 `Empty / Active / Ended` 三态 Session。

`TryBegin` 顺序检查：

1. 本地 Session 状态有效且为空；
2. Combat Run 已 ready；
3. opening snapshot 有效、属于 Run player、channel 为 SpiritEnergy、reservation 为 0；
4. processed capacity 为正；
5. Product Host 从 snapshot current/max/revision 精确打开；
6. Host opening SnapshotId 与输入精确相同；
7. Command Router 从该 Host 创建；
8. SessionId 从 Run/source/opening/Host/Router 完整派生；
9. 候选 Session 通过自身与 Coordinator 一致性验证后一次发布。

active 状态下只有完全相同的 begin 才幂等成功，其它绑定全部失败关闭。active Session 不允许 reset。

## 4. Availability projection

投影只包含值：Session/Run/source/Host/Router IDs、当前 immutable resource snapshot、processed count 与 capacity。

ProjectionId 把完整状态规范化后确定性派生。`HasRouteCapacity` 与 `CanAfford` 不触碰 mutable authority；后者同时验证 cost 本身、SpiritEnergy channel、当前 available amount 与剩余 route slot。

`IsAvailabilityCurrent` 重新捕获当前投影并比较完整身份。首次 route 后旧投影自然 stale；exact replay 前后 ProjectionId 必须相同。

## 5. Command 与 route

Session 的 `TryCaptureCommand` 只把当前 Coordinator registry、action、definition、cost 与调用方显式 Actor batch 交给既有 Router。Session 不遍历 World，也不保存 Actor 指针。

`TryRoute` 先验证 active/valid、ready Run、RunId、player entity 与 command。随后：

1. 捕获 pre-route availability；
2. 复制整个 Session；
3. 在副本的 Router/Host 上 route；
4. rejection 返回 nested proof，前后 availability 使用同一投影；
5. accepted 结果要求副本有效、仍匹配 Coordinator、post projection 有效；
6. first apply 要求 count +1 且 resource receipt 闭合前后 snapshot；
7. replay 要求前后投影完全相同；
8. 仅 first apply 发布 Session 副本，replay 保持原状态。

该结构把 Session、Router 与 Host 的原子边界放在同一个 value commit 中。

## 6. Teardown

首次 end 必须提供 exact SessionId，且原 Combat Run 仍 ready、Run/source 完全匹配。Session 捕获 final resource snapshot，生成 terminal receipt，绑定 opening/final snapshot IDs、Host/Router IDs、count/capacity 与 Run/source。

Ended 状态关闭 availability 与 route。相同 SessionId 的 end replay 返回同一 receipt，不再要求 live Run 读取。reset 只允许 Empty 或 Ended。

## 7. 测试开发

新增四项 exact contract：

- `LifecycleAndAvailability`：foreign owner、exact begin、active reset、capacity、cost 与投影失效；
- `ReplayWithoutLiveInputs`：首次 route 后以 null World/source/subjects 和拒绝 provider exact replay，provider read 与余额均不增加；
- `FencesAndRollback`：foreign Run、invalid command、provider rejection rollback、修复后同 command 恢复、source fence 与 foreign teardown；
- `CapacityAndTeardown`：单 slot、精确余额、capacity exhaustion、wrong SessionId、terminal snapshot chain、end replay、reset 与 Run teardown。

初始 Editor Development build 原生成功：5 actions，22.57 秒，native 0。随后 exact tests 首轮 4/0，通过后再运行映射要求的九个依赖组。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 4/0 | `33DFC1C1A0519CB335F06A998D608A0D2B69E974921166232D8E010EDF78C3F6` |
| `automation_command_router.log` | 4/0 | `CD238703672FD7469DA245CEC6FF8B9FE72663D00BE4B90F3E0D40E997F5445D` |
| `automation_product_host.log` | 4/0 | `1DA371CD105FECE58CB0C6DE9B3DC3591AA9EDE6E86C042B7A2241FDF72C92BC` |
| `automation_pulse_coordinator.log` | 4/0 | `96277251E1C5455CBD67281E2BC1F9AC8B1AE209F23ACEB86E615A81696E02A1` |
| `automation_world_observation.log` | 4/0 | `2221167B4AA218180F2892A7A73A5875F9995B8C0550DF88913E2EA72BADEE9D` |
| `automation_combat_run_coordinator.log` | 18/0 | `6F2390D8B4D4EEC0D415A60E8AE00A7EF2D4D93437B108691100AEC46094120B` |
| `automation_divine_sense_runtime.log` | 4/0 | `6A126E2233F850050C3DDB6EC93404C581382072190EEE8A9D4D19B9E8D943A4` |
| `automation_action_resource.log` | 7/0 | `F2D1ED280B2C628A18515B2264E5346C05A3E4A8BA360A7F9EFC7CB779CCD361` |
| `automation_action_lifecycle.log` | 1/0 | `DF8EDC3A5E3CB30CB0C5E41CD613144B91F550ECD1E6B90936226EA60A88700A` |
| `automation_world_gameplay.log` | 10/0 | `4FC809FE8D68889029C6AA429BD2AC4BDF5DE60F9AC52C2ECDBE9C65AABFD71E` |
| `automation_shanmen_full.log` | 812/0 | `F11E602505138AB2239A6A17B583FFE6BB21B7C745C89E9A34EBDFB0786F0C13` |

全量从 P19.4 的 808 增加到 812 项。既有 Sword Rhythm durable retry/checkpoint 区段仍有网络探测超时 warning 与长 test tick，但每项最终均为 Success；没有把 warning 删除或伪装成其它结果。

最终审计逐份验证：唯一 canonical command、Success > 0、Fail 0、唯一 queue-empty、native terminal 存在、选定阶段 Critical 0。审计 `PASS Logs=11`，SHA-256 `76178285DB7647975D3AC5FD66F3A8793DE7A0ECD4F4E5F77DB5FD98CF1DF278`。

## 9. 改动路径回归门禁

新增 `DivineSenseProductSession` mapping，要求：

1. `Product.DivineSenseProductSession`；
2. `Product.DivineSenseCommandRouter`；
3. `Product.DivineSenseProductHost`；
4. `Product.DivineSensePulseCoordinator`；
5. `Product.DivineSenseWorldObservation`；
6. `Product.CombatRunCoordinator`；
7. `CombatRuntime.DivineSense`；
8. `CombatRuntime.ActionResource`；
9. `CombatRuntime.ActionLifecycle`；
10. `WorldGameplay`。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
```

- gate SHA-256：`EC1AE1B18895D301ED88C01D8CE89FEA536093BC24C89C0C21E4BAACB5A1D18A`；
- self-test：新增一正一反后 `299/299 PASS`，SHA-256 `CEE14DE5E37CDA1DBF20F92973FBFB14E3EED83787997335D83975FF446A2E25`。

## 10. 静态边界与构建

生产 header/cpp 扫描禁止隐式 Actor discovery、trace/sweep/overlap、spawn/destroy、目标 mutation、random、Timer/Tick、输入绑定、Widget/Viewport 与 player-controller lookup：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`F69B0144C98B4BAC203C0B17F2726727081773F380A109FF20C42DC778624B99`。`git diff --check` 通过。

最终构建：

- Game Development：4 actions / 22.60s / native 0；log SHA-256 `35EA0C845E20E4DA3BFAD4977002E0F57288BED8E6F4AB7C98AF0E7D3B6D0EFB`；
- Editor Development：5 actions / 11.30s / native 0；log SHA-256 `EBBB8A73D05AAB32D533DBE4BE2A2600ABE9E5B9BFF74362B506653823979AC5`。

产物：

- `demo_map.exe`：356,826,112 bytes / `CA5CC5D1B1682EBD24C10EC13EB82B2529F62D8B454D1F690CB9DF4396DC55B9`；
- `UnrealEditor-demo_map.dll`：15,500,800 bytes / `D0D1C071007CE6061986F3A185A3149BCA86D40385712A6BDA28260D706C5843`。

## 11. P/F 与提交边界

本轮只执行 P 阶段源码、无头 Automation、静态扫描和 Development build。没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

计划精确提交 3 个新增源码、2 个回归工具文件、本 Report 与本 Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.5`。

下一阶段建议 P19.6：建立 caller-explicit Divine Sense Product Controller，冻结 definition/cost/capacity 并拥有 P19.5 Session 的 Run 内调用顺序；仍不接物理输入、UI 或隐式 Actor 搜索。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-5-divine-sense-product-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-5-divine-sense-product-session/Docs/Report/Dev.D.UE.0.0.10.P19.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-5-divine-sense-product-session/Docs/Log/Dev.D.UE.0.0.10.P19.5.r0_log.md>
