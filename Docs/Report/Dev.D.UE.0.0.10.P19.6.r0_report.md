# Dev.D.UE.0.0.10.P19.6.r0 Report

## 1. 结论

P19.6 已完成 caller-explicit Divine Sense Product Controller。

新 Controller 在一个 Combat Run 开始时冻结 Divine Sense definition、SpiritEnergy cost 与 pulse capacity，并独占一个 P19.5 Product Session。上游只提交带稳定 IntentId 的 device-independent Intent 与显式 Actor 集合；Controller 把每个 Intent 永久绑定到唯一 P19.4 route command，随后仍由既有 Session / Router / Host 完成资源扣减、World evidence、receipt 与 replay。它没有建立第二套资源、扫描或回放权威。

首次 World/provider route 被拒绝时，已捕获 command 会以有界记录保留，可在同一 Intent 下修复重试；已接受结果的 exact replay 不读取 World、Actor 或 provider，也不重复扣除 SpiritEnergy。Controller、Session 与 intent journal 共用同一 pulse capacity，因此拒绝路径不能无限增长。

本轮为 P 阶段。没有接物理输入、UI、表现、隐式 Actor 搜索或最终平衡数值，也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 基线与分支

- 基线提交：`648d3574bc5afef4454f57a8107bfbdb584d4fab`（P19.5）；
- 分支：`agent/0.0.10-p19-6-divine-sense-product-controller`；
- 引擎：Unreal Engine 5.8；
- 目标平台：Win64 Development。

## 3. 新增实现

- `Source/demo_map/demo_mapShanmenDivineSenseProductController.h`：331 行；
- `Source/demo_map/demo_mapShanmenDivineSenseProductController.cpp`：1,049 行；
- `Source/demo_map/demo_mapShanmenDivineSenseProductControllerTests.cpp`：648 行。

回归流程同步更新：

- `Scripts/ShanmenRegressionMap.json`：新增 Product Controller 的 11 组 required evidence；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与缺少 config / Session / Run 证据的失败关闭反例。

## 4. 冻结配置、Intent 与生命周期

`Fdemo_mapShanmenDivineSenseProductConfig` 是不可变值对象，冻结 canonical Divine Sense definition、SpiritEnergy cost 与正 pulse capacity。ConfigId 从三者完整确定性派生；非 canonical action、非 SpiritEnergy channel 或非正容量全部拒绝。

`Fdemo_mapShanmenDivineSenseProductIntent` 保存调用方稳定 IntentId、不可变 action snapshot、scan ordinal 与 subject budget，不保存 Actor 指针。Action 必须属于当前 Run/player，并使用 canonical Divine Sense action definition。

Controller 生命周期严格为 `Empty / Active / Ended`：

1. `TryBegin` 只接受 ready Combat Run、有效 opening SpiritEnergy snapshot 与有效 frozen config；
2. P19.5 Session 以同一 capacity 打开，ControllerId 绑定 Run、source、config、Session 与 opening snapshot；
3. exact begin 幂等，不同 config、snapshot 或 Run 不能覆盖 active Controller；
4. active 状态禁止 reset，必须先以 exact ControllerId 显式 teardown；
5. ended 状态保留 immutable end receipt，exact end 可重放，随后才允许 reset。

## 5. 可用性与有界 Intent journal

`Fdemo_mapShanmenDivineSenseProductAvailability` 是 pointer-free 只读投影，冻结：

- Controller 与 config identity；
- P19.5 Session availability；
- 已捕获 Intent 数与剩余 Intent capacity。

AvailabilityId 从 Controller、config、Session projection 与 captured count 完整派生。新 Intent 只有在 Controller journal 与 Session route capacity 都有空间、且当前 SpiritEnergy 足够时才可捕获。

command capture 成功后，即使首次 route 因 World/provider evidence 被拒绝，该 Intent 与 command 仍占用一个有界 slot。这样重试复用原始策略与身份，同时把 rejected-command memory 上限锁定为 config capacity。command capture 本身失败则不发布 journal 记录。

## 6. 原子提交、冲突与回放

`TrySubmit` 在任何 live evidence 读取前检查 Controller/Coordinator/Run/source/Intent，并先捕获 availability：

- 新 Intent：只从 frozen config 与 Intent 捕获一次 P19.4 command，再在 Controller 副本上 route；
- 同 IntentId + 同 payload：只复用已冻结 command；
- 同 IntentId + 不同 payload：返回 `IntentIdConflict`，不读取 provider、不改资源与 journal；
- 超出 capacity：返回 `IntentCapacityExceeded`，不重新捕获 command；
- route rejection：保留 command 与 nested typed proof，Session 资源和 processed count 不变；
- 修复后 retry：同一 command 可首次成功，不生成新 identity；
- accepted exact replay：允许 null World、null source、空 subjects 与拒绝 provider，返回同一 receipt，不触碰 live evidence。

每次候选状态只有在 Controller、Session、availability 与 nested route proof 全部自洽时才一次发布；否则以 `StateDesynchronized` 失败关闭。

## 7. Teardown proof

首次 `TryEnd` 要求 exact ControllerId，并委托 P19.5 Session 在同一 live Run 中结束。Controller end receipt 确定性绑定：

- Controller / config / Run / source identity；
- P19.5 Session end receipt identity；
- captured Intent count。

结束后 availability 与 submit 均关闭。exact teardown replay 返回保留的相同 receipt，不重新结束 Session；reset 只清理 Empty 或 Ended 状态。

## 8. 自动化结果

新增 exact tests 4 项：

- `ConfigLifecycleAndAvailability`；
- `FrozenReplayWithoutLiveInputs`；
- `ConflictRollbackAndRecovery`；
- `CapacityAndTeardown`。

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseProductController` | 4/0 | `F2223240E215C6B3C92DDAB3D992289BCEA0950AE1D51FEC9774E70D2BE92D64` |
| `automation_session.log` | `Product.DivineSenseProductSession` | 4/0 | `E48C8093D270A7B0CBD82C808D88FC39F801E5C9A22DBCCC13B2F9AACBDE5901` |
| `automation_command_router.log` | `Product.DivineSenseCommandRouter` | 4/0 | `B3532EFB9E1CB0A09EFFC36CBF228A6929195BFF60984B427D87C92F356D90F6` |
| `automation_product_host.log` | `Product.DivineSenseProductHost` | 4/0 | `3D1887FB763E37AD00CE38C41751AFA6E8A44D1DD5A9A490B2CC86B69E297384` |
| `automation_pulse_coordinator.log` | `Product.DivineSensePulseCoordinator` | 4/0 | `A93FB7DAFE60902D3ECB42AC58F1078F82DE627BB2A90337A77657059DCF7F28` |
| `automation_world_observation.log` | `Product.DivineSenseWorldObservation` | 4/0 | `1A2C07E3F9895B3BC26E19E79D6C46344A1EE445C07392ECC27770E0F161C80D` |
| `automation_combat_run_coordinator.log` | `Product.CombatRunCoordinator` | 18/0 | `821C5016E4E4B778E93AB52ACD94E8F7CDB7334334C0A075059B9EE752A07A5F` |
| `automation_divine_sense_runtime.log` | `CombatRuntime.DivineSense` | 4/0 | `BBB4CEEB8C895BE83062261EF47A87E0E3EC9743290BEF28123F7CA1A2190320` |
| `automation_action_resource.log` | `CombatRuntime.ActionResource` | 7/0 | `6865CD2D61FDC80190A5F3ED497F091457477D8EA9DEBA1E7FCC85B2C55CEA38` |
| `automation_action_lifecycle.log` | `CombatRuntime.ActionLifecycle` | 1/0 | `5D0EF967FFAA406EF2039F4EA1743D7C3BC9CF0C615A82C0EA71D602893555B5` |
| `automation_world_gameplay.log` | `WorldGameplay` | 10/0 | `C44139AC0D96E20B4C8040FDAC0F82C3068838887C715EC3CC27C4D455C489B0` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 816/0 | `1F371271E9F93114A85E8AC693F7C0891A4549FBD2524722109198D13B610875` |

全量从 P19.5 的 812 增加到 816。12 份日志各有且仅有一个 canonical RunTests command 与一个 queue-empty marker；选定命令后的 Fatal、Unhandled、Assert、Ensure 均为 0。证据审计 `PASS Logs=12`，SHA-256 `FC4599DF852A34AB2EADA7567185F44D4DBB27448D415E574A493BBDAD92CB81`。

## 9. 门禁、边界与构建

改动路径门禁：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11
```

- gate SHA-256：`44FE9BDCBCBFC96EA0B7BA8E34B1EE2D357615505F54205972D5812BB0C35345`；
- self-test：`301/301 PASS`，SHA-256 `D4EC5F5410ECC71684047F956848E714BBBFBDB321D3F708229D6C1B9C118E36`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `F69B0144C98B4BAC203C0B17F2726727081773F380A109FF20C42DC778624B99`；
- `git diff --check`：PASS。

构建：

- Editor initial：5 actions / 8.44s / native 0，SHA-256 `FD4BCCA208AA2C498BDBEF2314162AF6C894A548D82C778925CAFAFB0958C7D0`；
- Game final：4 actions / 22.86s / native 0，SHA-256 `A43D14ED92AA791B25920A02D19AFF9B009D24D47065B17D84D992CBE0502B5E`；
- Editor final：up to date / 0 actions / 0.98s / native 0，SHA-256 `97FCCF862714E5DA9FA2F48799C88D273C8B1747042FBB3ED71CF62C12CCF17E`。

产物：

- `demo_map.exe`：356,884,992 bytes，SHA-256 `7BB15D23CE4DBB1DE0DC1FFB0B12FD2F5A572F06853E18AADB783F89154AFCB0`；
- `UnrealEditor-demo_map.dll`：15,570,432 bytes，SHA-256 `E15A62A88731096A00C3B4576AD7392C8B37E65DCB7915CBC2C12F7829D115CC`。

## 10. P/F 边界与下一步

本轮只完成 C++ Controller、确定性身份、有界 Intent journal、只读 availability、typed teardown、无头自动化、静态边界审计与 Development 构建。没有运行 Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P19.7 建议建立 canonical Divine Sense Product Authority：由唯一内容来源冻结 config，由 Combat Run 分配 activation identity，再生成 P19.6 所需的 device-independent Intent；输入层不得自选 action/config/identity。该阶段仍不接物理按键、UI、隐式 Actor discovery 或最终扫描数值。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-6-divine-sense-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-6-divine-sense-product-controller/Docs/Report/Dev.D.UE.0.0.10.P19.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-6-divine-sense-product-controller/Docs/Log/Dev.D.UE.0.0.10.P19.6.r0_log.md>
