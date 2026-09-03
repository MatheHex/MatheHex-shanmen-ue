# Dev.D.UE.0.0.10.P19.5.r0 Report

## 1. 结论

P19.5 已完成 Run-scoped Divine Sense Product Session。

新 Session 把 P19.3 Product Host 与 P19.4 Command Router 绑定到一个已准备的 Combat Run，接受一次调用方显式提供的、无 reservation 的 SpiritEnergy 起始快照，并提供 pointer-free、只读的可用性投影。首次 route 仍由既有 Host/Router 完成唯一资源扣减、World evidence 读取、receipt 与 replay ledger；Session 不建立第二套运行时资源权威。

本轮为 P 阶段。没有接物理输入、UI、表现、隐式 Actor 搜索或最终平衡数值，也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 基线与分支

- 基线提交：`3c4f1cc1b9087cf3b8b4b62d89ccd7d661d1aa09`（P19.4）；
- 分支：`agent/0.0.10-p19-5-divine-sense-product-session`；
- 引擎：Unreal Engine 5.8；
- 目标平台：Win64 Development。

## 3. 新增实现

- `Source/demo_map/demo_mapShanmenDivineSenseProductSession.h`：291 行；
- `Source/demo_map/demo_mapShanmenDivineSenseProductSession.cpp`：908 行；
- `Source/demo_map/demo_mapShanmenDivineSenseProductSessionTests.cpp`：623 行。

回归流程同步更新：

- `Scripts/ShanmenRegressionMap.json`：新增 Product Session 的 10 组 required evidence；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与缺失 Run/Router 证据的失败关闭反例。

## 4. Session 生命周期

Session 状态严格为 `Empty / Active / Ended`：

1. `TryBegin` 只接受 ready Combat Run、该 Run 的 player entity、SpiritEnergy channel、零 reservation 与正容量；
2. Host 必须从起始快照的 current/max/revision 精确重建，重建后的 SnapshotId 必须与输入一致；
3. Router 在同一候选状态上创建，SessionId 绑定 Run、source、opening snapshot、HostId 与 RouterId；
4. exact begin 幂等；不同 Run、不同 snapshot 或不同容量不能覆盖 active Session；
5. active Session 禁止 `Reset`，必须先以 exact SessionId 对同一 live Run 执行 `TryEnd`；
6. ended Session 保留 value-only Host/Router proof 与不可变 terminal receipt，不保留 Actor 或 Coordinator 指针；exact end 可重放，随后才允许 reset。

## 5. 只读可用性投影

`Fdemo_mapShanmenDivineSenseAvailabilityProjection` 冻结：

- Session / Run / source / Host / Router 身份；
- 当前 `FShanmenActionResourceSnapshot`；
- 已处理命令数、容量与剩余容量。

ProjectionId 从上述完整状态确定性派生。`CanAfford` 同时检查 SpiritEnergy channel、当前 available amount 与剩余 route capacity。每次首次接受 pulse 后投影身份更新；exact replay 不改变投影。投影不暴露可写 authority，也不持有 UObject 指针。

## 6. 原子 route、回放与失败关闭

`TryRoute` 在任何 live evidence 读取前检查 Session、Coordinator、Run、source 与 command 身份。首次 route 在 Session 副本上调用 P19.4 Router；只有以下条件同时成立才发布副本：

- nested Router/Host proof 有效且被接受；
- Host 与 Router snapshot chain 一致；
- Session 仍与 Combat Run 一致；
- processed count 精确增加 1；
- resource-before / resource-after 与前后 availability snapshot 精确闭合。

Router rejection 返回 `RouteRejected` 和完整 nested proof，前后 availability 必须相同。exact RouteCommandId replay 在 Router ledger 命中后允许空 World、空 source、空 subjects 与拒绝型 provider，返回同一 receipt，不重复读取 live evidence、不重复扣除 SpiritEnergy，也不增加 processed count。

结构化状态区分 Session 非 active、Session invalid、Coordinator 未准备、Run/source 不匹配、command invalid、Router rejection 与 state desynchronization。

## 7. Teardown proof

首次 `TryEnd` 必须面对仍 ready 的同一 Combat Run 与 exact SessionId。terminal receipt 确定性绑定：

- Session / Run / source；
- HostId / RouterId；
- opening / final resource SnapshotId；
- processed count / capacity。

结束后 availability 与 route 均关闭；exact teardown replay 只返回已保留 receipt。Combat Run 可在 Session 正常结束后独立结束。

## 8. 自动化结果

新增 exact tests 4 项：

- `LifecycleAndAvailability`；
- `ReplayWithoutLiveInputs`；
- `FencesAndRollback`；
- `CapacityAndTeardown`。

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseProductSession` | 4/0 | `33DFC1C1A0519CB335F06A998D608A0D2B69E974921166232D8E010EDF78C3F6` |
| `automation_command_router.log` | `Product.DivineSenseCommandRouter` | 4/0 | `CD238703672FD7469DA245CEC6FF8B9FE72663D00BE4B90F3E0D40E997F5445D` |
| `automation_product_host.log` | `Product.DivineSenseProductHost` | 4/0 | `1DA371CD105FECE58CB0C6DE9B3DC3591AA9EDE6E86C042B7A2241FDF72C92BC` |
| `automation_pulse_coordinator.log` | `Product.DivineSensePulseCoordinator` | 4/0 | `96277251E1C5455CBD67281E2BC1F9AC8B1AE209F23ACEB86E615A81696E02A1` |
| `automation_world_observation.log` | `Product.DivineSenseWorldObservation` | 4/0 | `2221167B4AA218180F2892A7A73A5875F9995B8C0550DF88913E2EA72BADEE9D` |
| `automation_combat_run_coordinator.log` | `Product.CombatRunCoordinator` | 18/0 | `6F2390D8B4D4EEC0D415A60E8AE00A7EF2D4D93437B108691100AEC46094120B` |
| `automation_divine_sense_runtime.log` | `CombatRuntime.DivineSense` | 4/0 | `6A126E2233F850050C3DDB6EC93404C581382072190EEE8A9D4D19B9E8D943A4` |
| `automation_action_resource.log` | `CombatRuntime.ActionResource` | 7/0 | `F2D1ED280B2C628A18515B2264E5346C05A3E4A8BA360A7F9EFC7CB779CCD361` |
| `automation_action_lifecycle.log` | `CombatRuntime.ActionLifecycle` | 1/0 | `DF8EDC3A5E3CB30CB0C5E41CD613144B91F550ECD1E6B90936226EA60A88700A` |
| `automation_world_gameplay.log` | `WorldGameplay` | 10/0 | `4FC809FE8D68889029C6AA429BD2AC4BDF5DE60F9AC52C2ECDBE9C65AABFD71E` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 812/0 | `F11E602505138AB2239A6A17B583FFE6BB21B7C745C89E9A34EBDFB0786F0C13` |

11 份日志各有且仅有一个 canonical RunTests command 与一个 queue-empty marker；选定命令后的 Fatal、Unhandled、Assert、Ensure 均为 0。证据审计 SHA-256：`76178285DB7647975D3AC5FD66F3A8793DE7A0ECD4F4E5F77DB5FD98CF1DF278`。

## 9. 门禁、边界与构建

改动路径门禁：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
```

- gate SHA-256：`EC1AE1B18895D301ED88C01D8CE89FEA536093BC24C89C0C21E4BAACB5A1D18A`；
- self-test：`299/299 PASS`，SHA-256 `CEE14DE5E37CDA1DBF20F92973FBFB14E3EED83787997335D83975FF446A2E25`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `F69B0144C98B4BAC203C0B17F2726727081773F380A109FF20C42DC778624B99`；
- `git diff --check`：PASS。

构建：

- Editor initial：5 actions / 22.57s / native 0，SHA-256 `F3CE2CE05AB2EF3EBC6B4C6CDF1AD49EB66DDAD09CA0F57FCB86592ABECF8077`；
- Game final：4 actions / 22.60s / native 0，SHA-256 `35EA0C845E20E4DA3BFAD4977002E0F57288BED8E6F4AB7C98AF0E7D3B6D0EFB`；
- Editor final：5 actions / 11.30s / native 0，SHA-256 `EBBB8A73D05AAB32D533DBE4BE2A2600ABE9E5B9BFF74362B506653823979AC5`。

产物：

- `demo_map.exe`：356,826,112 bytes，SHA-256 `CA5CC5D1B1682EBD24C10EC13EB82B2529F62D8B454D1F690CB9DF4396DC55B9`；
- `UnrealEditor-demo_map.dll`：15,500,800 bytes，SHA-256 `D0D1C071007CE6061986F3A185A3149BCA86D40385712A6BDA28260D706C5843`。

## 10. P/F 边界与下一步

本轮只完成 C++ Session、确定性身份、只读 availability、typed teardown、无头自动化、静态边界审计与 Development 构建。没有运行 Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P19.6 建议建立 caller-explicit Divine Sense Product Controller：在 Run 开始时冻结 definition、cost 与 Session capacity，在 Run 内只通过 P19.5 Session capture/route，在 Run 结束前显式 teardown；仍不接物理输入、UI 或 Actor discovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-5-divine-sense-product-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-5-divine-sense-product-session/Docs/Report/Dev.D.UE.0.0.10.P19.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-5-divine-sense-product-session/Docs/Log/Dev.D.UE.0.0.10.P19.5.r0_log.md>
