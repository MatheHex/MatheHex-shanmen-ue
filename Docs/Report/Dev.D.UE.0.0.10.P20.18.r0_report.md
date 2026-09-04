# Dev.D.UE.0.0.10.P20.18.r0 Report

## 1. 结论

P20.18 已把 P20.10 的不可变投掷武器选择命令接入唯一的 PlayerController 逻辑输入边界。新的 `Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter` 先检查命令、gameplay gate、`Gameplay` surface 与 `GameOnly` mode，再解析权威 GameMode 一次、向 P20.11 唯一 session 提交一次。

适配层不保存第二份 choice state，不调用旧 `TryConfigureThrownWeaponTrajectory`，也不新增按键、InputComponent、UI、World/Actor 或产品生命周期。`Applied` / `Replay` / `NoChange` 以及 Run 冻结、生命周期冲突、revision mismatch 和 mode mismatch 的 typed 证据都原样保留；返回结构与命令不自洽时转为本地 protocol rejection。

聚焦自动化新增 `7/0`，最终全量与构建结果见下文。本轮仅执行编译与无头自动化，未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`8f8c45ad0f26ea9141accd0855e192850df656b2`（P20.17）；
- 分支：`agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Controller 输入边界

`Ademo_mapPlayerController::RouteThrownWeaponInputChoiceCommand` 是新的唯一 controller 入口。它接受已由 P20.10 冻结的 command，使用现有 `IsGameplayInputAllowed()`、surface 和 mode 栅栏；本地拒绝时不访问 World/GameMode，也不调用 session。

只有本地条件都满足时才解析 auth GameMode，并且只调用已有 `SubmitThrownWeaponInputChoiceCommand` 一次。GameMode 内的 `ThrownWeaponInputChoiceSession` 仍是唯一可变 state 权威；新 adapter 没有新增字段、缓存或 fallback 写路径。

## 4. 命令、重放与冲突证据

结果保留原 command，因此 `CommandId`、`ExpectedRevision`、kind 与 canonical payload 可审计。一次合格调用还保留 P20.11 的完整 `SessionResult`：

- `Applied/Reduced`：state revision 精确为 expected + 1，last-command ID 与本命令一致；
- `Replay/Replay`：同一 command 在 expected + 1 state 上不二次应用；
- `NoChange/NoChange`：新命令的 expected revision 与当前 state 一致，但 canonical 值未变；
- `CombatRunActive` / `ProductLifecycleNotEmpty`：保留 `Reduced` 候选与 session fence 原因；
- `ReductionRejected`：保留 revision exhausted/mismatch、mode mismatch 或 state rejection。

本地 `SessionProtocolRejected` 只表示回调返回了缺失或自相矛盾的 typed 证据，不会将它误报为产品接受。

## 5. 设备与物理绑定边界

本轮没有新增任何物理按键、鼠标、手柄映射或 UI callback。新 adapter 是 device-independent 命令接缝，未来设备/UI 层只需生成 P20.10 command 并调用这个入口，不应直接访问 GameMode session。

静态边界扫描确认：新 production adapter 中 World/Actor/UI/物理绑定/随机/旧 trajectory 配置 API 命中 0；PlayerController 新增物理绑定 0；controller adapter route 1；GameMode session submit 1；adapter 内直接 session `.Submit` 0。

## 6. 自动化覆盖

新增 `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter` 7 项：

- `GateOrder`：非法 command 与三层本地 gate 固定短路；
- `SessionAvailability`：GameMode/session 不可用时只解析一次、提交 0 次；
- `ApplyReplayNoChange`：首次应用、精确重放与新鲜 no-op 原样保留；
- `AllCommandKinds`：trajectory、target、apex、clear 和恢复 Straight 全部共用一个 session；
- `SessionRejections`：Run/lifecycle fence、stale revision 与 mode mismatch；
- `SessionProtocol`：缺失或自相矛盾的回调证据失败关闭；
- `PlayerControllerBoundary`：未初始化 input mode 在 GameMode 解析前拒绝。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `choice_controller_final.log` | `Product.ThrownWeaponInputChoiceControllerAdapter` | `7/0` | `10B0322B04AD411A68241D182690FB1C81992933D7484603FF36037E195A23A7` |
| `input_choice_session_final.log` | `Product.ThrownWeaponInputChoiceSession` | `4/0` | `2321DFC04DE45ACAC9310EDE523526D83BFB08F72C5723304E126B4B55C62416` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `16/0` | `0ECBCBC41E8A4A387A5C2BE19FFF033C6CAF8B62FC0E56EACC4676F41CDF55E9` |
| `hotbar_confirmation_final.log` | `Product.ThrownWeaponHotbarConfirmationAdapter` | `7/0` | `44D99CA924A00B2D7FA6B69BDD8B30A139B5AEDDFDE685BE364D49A2C5981B6B` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `E29488103E674D62501CEBD33D0F95516CE745130152FA9C386D5BC5C73BD4F9` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `ECC65E2E56D3F93889332FE7F81180FDCF19553FBDB235A42C5D857BD61DEF92` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `34FA26A92B2DF5A0AB33EB820D1A266B0B2B335FECCA7CD54FA8D9B4891D7F89` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `55E7B262557DD3244245D139A949D31E658F27586BE56EB374984F2DF90D166D` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `914/0` | `1C27B8C2CBD63CD608C0A3973D9B0D1574DF20F9B97919B6BD25AC065380E250` |

有效日志审计：`PASS Logs=9 RecordedSuccess=1086 Invalid=0`，SHA-256 `FAF9B4D6D000DA3E64E839577334D9B3B82D9B1BCEF56BD13DF71BBFD015D1C9`。流程映射自测从 327 增至 `329/329`，SHA-256 `0B40D9BEEF9037D58DEE7489C737CFE9069F8965D686C9406A373C31C8026214`。

## 8. 首次验证、回归门禁与静态审计

首次 Editor 构建直接通过：21 actions / native 0 / 42.31 秒，SHA-256 `642B10C55F27FB93D13CE07B4E6DE2A294EED15C5603526B01A34560301AF79A`。首次新 adapter exact 直接为 `7/0`，SHA-256 `94DD722C67EFD2926DF5FFE10BA779F650CF8A8E4A7D871968C9C26D60DCC7DD`；没有源码或断言返工。

静态审计：`PASS ForbiddenApi=0 AddedPhysicalBinding=0 ControllerAdapterCalls=1 GameModeSessionSubmitCalls=1 DirectSessionSubmitInAdapter=0`，SHA-256 `D67663EA391C0041DCC3B5495C41AFA2AD08A8066D756CA01CD5B71674E29E84`。

最终 changed-file regression gate：`PASS Changed=9 Rules=2 Required=17 Logs=9`，SHA-256 `097944CDA5DA5EB08BDEA1DB84ABB195A15709D4998D5B56920317F14C350B87`。暂存区格式检查：`PASS / 9 files / native 0`。

## 9. 构建与产物

- final Editor：0 actions / native 0 / 1.25 秒，SHA-256 `8EBBB50947D86EC6DD5CB1926BD4FB8A85F2CC8D1A5C23DDB7C0D156F274E0F3`；
- final Game：20 actions / native 0 / 51.59 秒，SHA-256 `78DDE03C672211AE4BEAF48A6E01A77EDD668B00E5BD5B65B3F3F62DF4D75910`；
- `Binaries/Win64/demo_map.exe`：357,444,096 bytes，SHA-256 `6D443A72C11F4D572D4F65C85B27283AB95E44ACBE8FF76BB88FFCBA9FD20323`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,152,576 bytes，SHA-256 `4AF4394B88E1B83796D30E19D2DAF3DEC5D2EF003CF05A5E7C1E8EA5C160AE53`。

## 10. P/F 边界与下一步

P20.18 证明的是：一个已冻结、设备无关的 P20.10 choice command 可以通过现有 PlayerController 栅栏，精确提交到唯一 P20.11 GameMode session，并且完整保留应用、重放、no-op 与冲突证据。

它没有证明任何物理设备/UI 已生成 command，也没有证明预览、真实投掷、World trace、碰撞、命中、库存扣减或伤害。无头自动化与 Development 构建不能描述为产品运行验收。

建议 P20.19 增加 device-independent choice-edit intent capture：从唯一当前 state 读取 revision 一次，将 trajectory/target/apex/clear 的逻辑意图冻结为 P20.10 command，再只调用本轮 controller 入口；仍不新增物理绑定或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.18.r0_log.md>
