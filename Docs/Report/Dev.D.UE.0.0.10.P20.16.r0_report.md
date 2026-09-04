# Dev.D.UE.0.0.10.P20.16.r0 Report

## 1. 结论

P20.16 已在 P20.15 device-independent Arc launch command seam 之前增加 consumer-local 显式确认 owner。调用方用一个有效 logical confirmation event ID、hotbar slot 与 Arc projection policy 捕获不可变 intent；只有显式提交该 intent 时，owner 才读取唯一 current choice 一次、捕获 P20.15 command 一次并调用既有 PlayerController route 一次。

同一 retained event ID 与同一 canonical payload 的重放直接返回冻结结果，current choice 读取与 P20.15 调用均为 0；同一 event ID 携带不同 slot 或 policy 时失败关闭。owner 只保留最近 32 个事件，避免 PlayerController 生命周期内无界增长；超过窗口的旧 ID 不再拥有 replay 语义，因此未来设备层必须为每个新物理确认生成新的 logical event ID。

本轮没有增加或改写物理键、hotbar 行为、UI/preview、Actor/World 产品权威、item/Run/inventory、投射物、碰撞、命中或伤害。最终验证为新 owner `6/0`、P20.15 launch `6/0`、P20.14 source basis `6/0`、composition `5/0`、projection `5/0`、choice `9/0`、既有 InputAdapter `9/0`、InputRestore `101/0`、V2 ranged `22/0`、0.0.10 全量 `900/0`；Editor 与 Game Development 构建均成功。

## 2. 基线与分支

- 基线提交：`62bc761f0bfb059900831c2b40a65bd0279c030e`（P20.15）；
- 分支：`agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 显式确认 intent 契约

新增 `Fdemo_mapShanmenThrownWeaponArcConfirmationIntent`。`TryCapture` 只接受有效 confirmation event ID、1–9 hotbar slot 与有效 Arc choice policy；失败会清空输出，不能复用半成品 intent。

intent identity 使用命名空间 `demo_map.ShanmenThrownWeapon.ArcConfirmationIntent.r1`，由 event ID、slot 与 policy ID 确定性派生。`IsValid()` 会重算并比对 identity；相同 canonical 输入产生相同 identity，任一字段改变都会产生不同 identity。

intent 不包含 current choice。choice 只在明确调用 `Confirm` 时从既有唯一 GameMode choice session 读取，因此创建 intent 本身不会提前冻结过期选择，也不会触发产品路由。

## 4. 有界 owner、重放与冲突

新增 `Fdemo_mapShanmenThrownWeaponArcConfirmationOwner`，由 PlayerController 独占。新事件按固定顺序执行：intent validity、owner invariant、retained replay/conflict、本地确认许可、一次 current choice read、一次 P20.15 command capture、一次 P20.15 route。

每个已处理事件保存 intent、choice、command 与完整 P20.15 result。exact retained replay 返回同一冻结 evidence，并把本次 read/route 计数显式置 0；event identity 冲突不覆盖旧记录。blocked、invalid choice、capture rejection 与 protocol rejection 也会被记录，防止同一物理事件因环境变化被偷偷升级为第二次尝试。

owner 使用 FIFO 窗口，最大 32 项；第 33 个新事件仅淘汰最旧 evidence，不影响当前事件。`Reset()` 回到 canonical empty/valid 状态。该 owner 不实现 retry loop、Run lifecycle 或第二套产品 ledger。

## 5. PlayerController 接入与输入栅栏

`Ademo_mapPlayerController::RouteThrownWeaponArcConfirmation` 是新的设备无关入口。它先组合既有 `IsGameplayInputAllowed()`、`Gameplay` surface 与 `GameOnly` mode；本地不允许时，owner 不访问 World/GameMode、不读 choice、也不调用 P20.15。

允许时，choice reader 从当前 auth GameMode 的 `GetThrownWeaponInputChoiceState()` 读取一次，然后 owner 捕获 P20.15 command。P20.15 仍会在实际调用点重新检查 gameplay/surface/mode、解析 GameMode、核对 frozen/current choice 并委托 P20.14，因此两个读取之间的状态变化仍会被 stale-choice fence 拒绝。

没有改写 `UseHotbarSlot`、`BindKey`、InputComponent 或 InputBindingSettings；本阶段只建立未来 physical/UI consumer 可调用的显式确认边界。

## 6. 状态与协议边界

结果区分 `IntentInvalid`、`OwnerInvalid`、`EventConflict`、`ConfirmationBlocked`、`ChoiceStateInvalid`、`CommandCaptureRejected`、`Routed` 与 `InputProtocolRejected`。每个状态规定严格的 0/1 read 与 route 计数，以及允许存在的 evidence。

只有有效且 command-matching 的 P20.15 result 才能成为 `Routed`。无效下游对象或不同 command 的有效对象都会升级为 typed protocol rejection，同时保留本次确实进行过一次同步调用的事实。`IsAccepted()` 不提升下游结果，只在 P20.15 自身接受时返回 true。

## 7. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner` 6 项：

- `IntentContract`：确定性 identity、event/slot/policy 参与身份与非法输入失败关闭；
- `CaptureGates`：invalid、local block、invalid choice、non-Arc choice 的固定短路顺序；
- `RouteReplayConflict`：新事件精确一次 read/route、exact replay 零工作、identity conflict 失败关闭；
- `InputProtocol`：无效及 mismatched P20.15 evidence 均拒绝；
- `BoundedRetention`：32 项窗口、淘汰语义与 reset；
- `PlayerControllerBoundary`：Uninitialized input mode 在 World 访问前拒绝，exact replay 零工作。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_confirmation_owner_final.log` | `Product.ThrownWeaponArcConfirmationOwner` | `6/0` | `F016AEC624CC463433351B92702FDF666FD9F3E0B3F0D05A8EAFE30C4B0BFFC2` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `35DBF20FFF00716014FF8485BC9C8F94A23B44693B0DD19DD34CACA81A7F8FC3` |
| `source_basis_adapter_final.log` | `Product.ThrownWeaponArcSourceBasisAdapter` | `6/0` | `C076AA5A870021874EACCEA9179D8B8E9C0E8C95A53E5D0BF07C5362DE83EA6A` |
| `arc_choice_composition_final.log` | `Product.ThrownWeaponArcChoiceInputComposition` | `5/0` | `F23666F93BAC01C5B72D1225E2F707AB84A018D362367A07718E70EAAFC638A1` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `F3578AA86019F199EE4FE2A65C6E879181CCE939F2057687AC3E233755D1B2CA` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `49776AC72B682DBA331E28F3134FC9876B1450CC279CD7C4BAAC0AD4AA096439` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `C4ED2C477657C3F906AE95C159ADE75226AF70D08A8B098D9125C64B5103458E` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `144DC69D56715F12FD14B6F830DE60A808527FC92955D0019598A41611B94E44` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `5CEFAE4B91C93300F8AAF7ECC5483BA0EF3446840E64670A75F26ED9DE5011E1` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `900/0` | `86AA4946714C5CF946441CC8337D3CCBA1966339EC8C5368594D0D7EE3863770` |

十份最终 Unreal 日志合计 `1069/0`，0 no-match、0 fatal/unhandled/ensure、每份恰好一个 RunTests command 与 terminal-success marker；审计 SHA-256：`271F5CC6058DE9ED49678430F093EB5886A401897D68288EEFC52510A1C456D4`。全量从 P20.15 的 894 增至 `900`，恰好增加本轮 6 项。

## 8. 首次验证、流程修正与静态门禁

首次 Editor 构建直接通过：19 actions / native 0 / 48.41 秒。首次 owner exact 直接为 `6/0`，SHA-256 `A47510C819B256DB4023EAD4D94188EE7D122B63852A1BD2B6C9380875EB31A4`；没有源码或测试断言修复。

回归映射新增 confirmation-owner rule，要求本轮 exact、P20.15、P20.14、composition、projection、choice、旧 InputAdapter 与 full；PlayerCombatController rule 同步加入本轮 exact。流程自测新增 broad evidence 正例与 unrelated item evidence 反例，最终 `325/325`，SHA-256 `8F03C6C132E866624F7709568A44B1F000BBAFFB24FC80C27EBF898533BD3DE3`。

首次日志审计错误地使用 `*_final.log`，把非 Unreal 的 `regression_selftest_final.log` 也纳入并得到 `Invalid=1`；该失败证据保存在 `log_audit_first.log`，SHA-256 `6F05C4145DFD46A774D9D84A004B6CDF2E7A56CC9553B962B66249AD62A0FEA4`。改为与回归门禁相同的十个明确输入后，审计通过；没有重跑或修饰任何产品测试日志。

静态审计为 `PASS`：owner production 2 文件对 UWorld/AActor、GetWorld、FKey/BindKey/InputComponent、UI、spawn/trace/sweep、inventory、damage 与 RNG 扫描 0 matches；PlayerController 33 条新增行对 physical binding 扫描 0 matches；SHA-256 `3D74C5D313B612693625613EEC731D45F1760934AF84FF81CE41B5434C0972E2`。

纳入 Report/Log 后的最终 changed-file gate 为 `PASS Changed=9 Rules=2 Required=17 Logs=10`，SHA-256 `BCDD47E7708D1FA92A903497C2E954349B694E415269FD177C854981C92ABF43`。最终 staged diff check 为 `PASS / 9 files / native 0`，SHA-256 `50F5B0B9A040CADE1E73434DC6E0EE6FDC9CA98C7AFA0DAFF0F2619DB5F2D629`。

## 9. 构建与产物

- final Editor：`0 actions / native 0 / 1.27 秒`，SHA-256 `1A2656DED555CE54633E2055B9B01CAB7F875BAE0E5D953A6998921ACB68F43A`；
- final Game：`18 actions / native 0 / 50.37 秒`，SHA-256 `33BF8DB307CC5B644042D34B1203C88623068F6EDF587EB4AEF630A1101B1447`；
- `Binaries/Win64/demo_map.exe`：`357389312` bytes，SHA-256 `A108028D768A2BF39C83EBF49E153DF6B08AB1CB716C6BE991947782227A4061`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`16083456` bytes，SHA-256 `2968B1C5FE2A2B254C8267638AEEB109C933CEEC95B8C6DBB083B20A179AE11C`。

本轮只执行编译与无头自动化。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。全量日志中的 `generate_204` 超时和 AutomationController large-delta 是既有环境噪声；队列持续推进并最终 `900/0`、native 0。

## 10. P/F 边界与下一步

P20.16 证明的是：“一个显式、设备无关的 Arc confirmation event 能在 consumer-local owner 中被确定性识别，在本地 input context 允许时读取唯一 choice 一次、捕获 P20.15 command 并调用一次；最近事件重放、identity conflict、capture failure 与下游协议异常均可审计且失败关闭。”

它没有证明物理 hotbar 输入已经进入该 route、event ID 已由真实输入生成、Arc policy 已有产品配置权威、UI/preview、真实注册 source 的成功 launch、Editor/PIE/Standalone、真实输入、world trace、碰撞、库存扣减或伤害。`900/0` 不能描述为产品运行验收。

建议 P20.17 增加 trajectory-aware hotbar confirmation adapter：复用现有 1–9 hotbar binding，在 Arc choice 下由唯一 controller 序列生成新的 logical event ID、从单一产品 policy 捕获 P20.16 intent 并提交；Straight 与非投掷物仍严格保持现有 pass-through 路径。不新增第二套按键或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner/Docs/Report/Dev.D.UE.0.0.10.P20.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-16-thrown-weapon-arc-confirmation-owner/Docs/Log/Dev.D.UE.0.0.10.P20.16.r0_log.md>
