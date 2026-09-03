# Dev.D.UE.0.0.10.P19.6.r0 Development Log

## 1. 目标与基线

- 基线：`648d3574bc5afef4454f57a8107bfbdb584d4fab`（P19.5）；
- 分支：`agent/0.0.10-p19-6-divine-sense-product-controller`；
- 目标：建立 caller-explicit Divine Sense Product Controller，冻结 definition/cost/capacity，拥有一个 Run-scoped P19.5 Session，并把 stable Intent 原子路由到既有 P19.4 command；
- 约束：不接物理输入、UI/表现、隐式 Actor discovery、持续 Tick/Timer、存档或最终数值定案。

## 2. 设计审计

审计了 P19.3 Product Host、P19.4 Command Router、P19.5 Product Session，以及现有 Spirit Evasion、Weapon Guard 与 Sword Rhythm 的 product authority / controller 结构。

P19.5 已经拥有完整的资源与 World transaction：

- Host/Router 的原子 staging 与 publish；
- immutable SpiritEnergy snapshot chain；
- deterministic command、receipt 与 replay ledger；
- exact replay 的 live-evidence bypass；
- Run/source/capacity 与 teardown proof。

因此 P19.6 没有复制扫描、资源或 World observation 逻辑。剩余缺口是产品调用顺序与身份冻结：上游需要一个 Run owner 固定 definition/cost/capacity，把同一 caller Intent 永久绑定到同一 command，并在首次 route 暂时失败时保留可恢复状态。

## 3. Config、Intent 与 Controller begin

新增 immutable `Fdemo_mapShanmenDivineSenseProductConfig`：只有 canonical Divine Sense definition、SpiritEnergy cost 与正 pulse capacity 才能捕获；ConfigId 从三者确定性派生。

新增 device-independent `Fdemo_mapShanmenDivineSenseProductIntent`：保存 caller-stable IntentId、action snapshot、scan ordinal 与 subject budget。它不持有 Actor，且 action owner/source/run/action definition 必须自洽。

Controller `TryBegin` 在候选副本上：

1. 验证本地 Empty 状态、ready Combat Run、opening resource 与 config；
2. 以 config capacity 打开唯一 P19.5 Session；
3. 从 Run/source/config/Session/opening snapshot 派生 ControllerId；
4. 验证 Session 与 Coordinator 一致后一次发布。

active exact begin 幂等；任何不同 binding 失败关闭。active Controller 不能 reset。

## 4. Availability 与容量

Controller availability 只包含值对象：Controller/config identity、P19.5 Session projection、captured Intent count 与剩余 capacity。AvailabilityId 绑定全部字段，旧投影在首次 command capture 或首次 accepted route 后自然失效。

Controller journal 与 Session processed-command authority 使用同一个 config capacity。`CanCaptureNewIntent` 同时要求：

- captured count 小于 capacity；
- Session 仍有 route capacity；
- 当前 SpiritEnergy 能支付 frozen cost。

这使 accepted、pending-retry 与剩余可用量在同一个 bounded projection 中可审计。

## 5. Intent capture、route 与 replay

新 Intent 的处理顺序：

1. 在 live evidence 读取前检查 Controller、Coordinator、Run、source 与 Intent；
2. 捕获 pre-submit availability；
3. 在 Controller 副本上把 frozen config + Intent + 显式 Actor batch 交给 Session 捕获 command；
4. command capture 成功后先写入候选 journal，再 route；
5. 只有候选 Controller、Session、route proof 与 post availability 全部有效时才发布副本。

route rejection 保留 frozen command 与 nested rejection proof，但不扣资源、不增加 Session processed count。之后同 Intent 只复用该 command；provider 修复后可以首次 accepted。已 accepted 的同 Intent exact replay 允许 null World/source、空 subjects 与拒绝 provider，返回原 receipt 且不增加调用次数、余额变更或 processed count。

同 IntentId 的不同 payload 在 provider 前返回 `IntentIdConflict`；新 Intent 超容量返回 `IntentCapacityExceeded`；command capture 失败不占 journal slot。

## 6. Teardown

首次 end 必须提供 exact ControllerId，并由 P19.5 Session 完成实际 teardown。Controller receipt 绑定 Controller/config/Run/source、Session end receipt 与 captured count。

Ended 状态不再提供 availability 或 submit。exact end replay 返回相同 immutable receipt，不读取新的产品状态；reset 只允许 Empty/Ended。

## 7. 测试开发

新增四项 exact contract：

- `ConfigLifecycleAndAvailability`：stable config identity、SpiritEnergy fence、exact begin、config freeze、active reset 与双重 capacity projection；
- `FrozenReplayWithoutLiveInputs`：首次 apply 后用 null World/source、空 subjects 与拒绝 provider exact replay，命令/receipt/余额/调用次数不变；
- `ConflictRollbackAndRecovery`：provider rejection 保留 bounded command、Intent conflict 在 evidence 前拒绝、同 command 修复恢复与 foreign Run fence；
- `CapacityAndTeardown`：单 slot、精确余额、capacity exhaustion、wrong ControllerId、terminal receipt、end replay、reset 与 Run teardown。

初始 Editor Development build 原生成功：5 actions，8.44 秒，native 0。exact tests 4/0 后，依 changed-file mapping 运行十个依赖组，最后执行完整 0.0.10 suite。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 4/0 | `F2223240E215C6B3C92DDAB3D992289BCEA0950AE1D51FEC9774E70D2BE92D64` |
| `automation_session.log` | 4/0 | `E48C8093D270A7B0CBD82C808D88FC39F801E5C9A22DBCCC13B2F9AACBDE5901` |
| `automation_command_router.log` | 4/0 | `B3532EFB9E1CB0A09EFFC36CBF228A6929195BFF60984B427D87C92F356D90F6` |
| `automation_product_host.log` | 4/0 | `3D1887FB763E37AD00CE38C41751AFA6E8A44D1DD5A9A490B2CC86B69E297384` |
| `automation_pulse_coordinator.log` | 4/0 | `A93FB7DAFE60902D3ECB42AC58F1078F82DE627BB2A90337A77657059DCF7F28` |
| `automation_world_observation.log` | 4/0 | `1A2C07E3F9895B3BC26E19E79D6C46344A1EE445C07392ECC27770E0F161C80D` |
| `automation_combat_run_coordinator.log` | 18/0 | `821C5016E4E4B778E93AB52ACD94E8F7CDB7334334C0A075059B9EE752A07A5F` |
| `automation_divine_sense_runtime.log` | 4/0 | `BBB4CEEB8C895BE83062261EF47A87E0E3EC9743290BEF28123F7CA1A2190320` |
| `automation_action_resource.log` | 7/0 | `6865CD2D61FDC80190A5F3ED497F091457477D8EA9DEBA1E7FCC85B2C55CEA38` |
| `automation_action_lifecycle.log` | 1/0 | `5D0EF967FFAA406EF2039F4EA1743D7C3BC9CF0C615A82C0EA71D602893555B5` |
| `automation_world_gameplay.log` | 10/0 | `C44139AC0D96E20B4C8040FDAC0F82C3068838887C715EC3CC27C4D455C489B0` |
| `automation_shanmen_full.log` | 816/0 | `1F371271E9F93114A85E8AC693F7C0891A4549FBD2524722109198D13B610875` |

全量由 P19.5 的 812 增加到 816。既有 Sword Rhythm durable retry/checkpoint 区段仍有长 test tick，但每项最终均为 Success；没有删除或改写原始日志。

最终审计逐份验证：唯一 canonical command、精确 Success 数、Fail 0、唯一 queue-empty、native terminal 存在、选定阶段 Critical 0。审计 `PASS Logs=12`，SHA-256 `FC4599DF852A34AB2EADA7567185F44D4DBB27448D415E574A493BBDAD92CB81`。

## 9. 改动路径回归门禁

新增 `DivineSenseProductController` mapping，要求：

1. `Product.DivineSenseProductController`；
2. `Product.DivineSenseProductSession`；
3. `Product.DivineSenseCommandRouter`；
4. `Product.DivineSenseProductHost`；
5. `Product.DivineSensePulseCoordinator`；
6. `Product.DivineSenseWorldObservation`；
7. `Product.CombatRunCoordinator`；
8. `CombatRuntime.DivineSense`；
9. `CombatRuntime.ActionResource`；
10. `CombatRuntime.ActionLifecycle`；
11. `WorldGameplay`。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11
```

- gate SHA-256：`44FE9BDCBCBFC96EA0B7BA8E34B1EE2D357615505F54205972D5812BB0C35345`；
- self-test：新增一正一反后 `301/301 PASS`，SHA-256 `D4EC5F5410ECC71684047F956848E714BBBFBDB321D3F708229D6C1B9C118E36`。

## 10. 静态边界、构建与异常记录

生产 header/cpp 扫描禁止隐式 Actor discovery、trace/sweep/overlap、spawn/destroy、目标 mutation、random、Timer/Tick、输入绑定、Widget/Viewport 与 player-controller lookup：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`F69B0144C98B4BAC203C0B17F2726727081773F380A109FF20C42DC778624B99`。`git diff --check` 通过。

构建：

- Editor initial：5 actions / 8.44s / native 0；log SHA-256 `FD4BCCA208AA2C498BDBEF2314162AF6C894A548D82C778925CAFAFB0958C7D0`；
- Game final：4 actions / 22.86s / native 0；log SHA-256 `A43D14ED92AA791B25920A02D19AFF9B009D24D47065B17D84D992CBE0502B5E`；
- Editor final：up to date / 0 actions / 0.98s / native 0；log SHA-256 `97FCCF862714E5DA9FA2F48799C88D273C8B1747042FBB3ED71CF62C12CCF17E`。

产物：

- `demo_map.exe`：356,884,992 bytes / `7BB15D23CE4DBB1DE0DC1FFB0B12FD2F5A572F06853E18AADB783F89154AFCB0`；
- `UnrealEditor-demo_map.dll`：15,570,432 bytes / `E15A62A88731096A00C3B4576AD7392C8B37E65DCB7915CBC2C12F7829D115CC`。

没有源码编译失败、Automation failure、changed-file gate failure、UBT 非零退出或外层超时。回归 self-test 的第一次人工启动误用了旧 Windows PowerShell parser，脚本在加载阶段即被拒绝；改用项目配置的 `pwsh` 后正式结果为 301/301。该事件没有执行测试、没有修改源码，也不属于产品或门禁失败。

## 11. P/F 与提交边界

本轮只执行 P 阶段源码、无头 Automation、静态扫描和 Development build。没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

产品、测试与流程本体为 3 个新增源码和 2 个回归工具文件，共 2,077 行新增；加入本 Report 与本 Log 后精确提交 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.6`。

下一阶段建议 P19.7：建立 canonical Divine Sense Product Authority，由唯一内容来源提供 config、由 Combat Run 分配 activation identity，再生成本 Controller 所需的 device-independent Intent；仍不接物理输入、UI 或隐式 Actor 搜索。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-6-divine-sense-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-6-divine-sense-product-controller/Docs/Report/Dev.D.UE.0.0.10.P19.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-6-divine-sense-product-controller/Docs/Log/Dev.D.UE.0.0.10.P19.6.r0_log.md>
