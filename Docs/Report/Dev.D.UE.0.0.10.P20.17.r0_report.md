# Dev.D.UE.0.0.10.P20.17.r0 Report

## 1. 结论

P20.17 已把 P20.16 显式 Arc confirmation 接入既有且唯一的 1–9 hotbar 入口。`UseHotbarSlot` 不再无条件走 Straight：新的 controller-local trajectory-aware adapter 先读取 GameMode-owned 唯一 choice 一次；Straight 仍精确调用原 `RouteThrownWeaponHotbarInput`，BallisticArc 则读取一个集中管理的产品 policy、分配新的确定性 logical event ID、捕获 P20.16 intent 并提交既有确认链。

非投掷物在 Straight 与 Arc 两条路径中都保留 `ShouldPassThrough()`，因此仍可继续进入既有治疗物品与普通快捷栏路径。Arc 被产品认领、输入被阻止、consumer/choice/policy 无效、序列耗尽或下游协议异常时全部失败关闭，不会误触发普通物品。

本轮没有新增或修改任何物理按键、InputComponent、InputBindingSettings 或 UI；现有 Hotbar1–9 绑定仍为 9 条。最终验证为新增 adapter `7/0`、P20.16 `6/0`、P20.15 `6/0`、P20.14 `6/0`、composition `5/0`、projection `5/0`、choice `9/0`、旧 InputAdapter `9/0`、InputRestore `101/0`、V2 ranged `22/0`、0.0.10 全量 `907/0`；Editor 与 Game Development 构建均成功。

## 2. 基线与分支

- 基线提交：`dd9860efe442fb07612e06c1f1c9c568d6c63254`（P20.16）；
- 分支：`agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 唯一 hotbar 路由

新增 `Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter`，由 `Ademo_mapPlayerController` 独占。它不拥有 key、UI、World、Actor、item、Run、inventory 或产品生命周期，只负责一次物理 hotbar 事件在当前轨迹选择下的确定性分流与审计。

固定顺序为：slot 1–9 校验、完整本地输入栅栏、GameMode consumer 可用性、一次 choice read；随后只进入一个分支：

- `Straight`：调用原 Straight product route 一次；不读 Arc policy、不生成 event ID、不触发 P20.16；
- `BallisticArc`：消费一个 controller-local ordinal，生成 logical event ID，读取 Arc policy 一次，捕获 P20.16 intent 并调用 P20.16 一次。

`UseHotbarSlot1` 至 `UseHotbarSlot9` 与原 9 条绑定完全未变。`UseHotbarSlot` 对新 adapter 的调用恰好一次，且仍位于 gameplay gate 之后、治疗 item 和 generic quick-slot 之前；函数内对旧 Straight route 的直接调用已为 0。

## 4. Arc event 序列与产品 policy

Arc logical event identity 使用命名空间 `demo_map.ShanmenThrownWeapon.HotbarConfirmationEvent.r1`，由 controller-local `uint64` ordinal 确定性派生。初始值为 1；每个实际进入 Arc 分支的物理调用都消费一个新 ordinal，包括 policy 或协议失败，防止环境变化把同一物理事件升级为隐式重试。

0 与 `MAX_uint64` 为保留/耗尽值，不生成 GUID；到达末值后失败关闭且绝不回绕。Straight 分支不消费序列。

Arc projection policy 只由 `Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::GetCanonical()` 提供：forward `400–1200`、lateral `300`、apex `100–500`。按键函数和 PlayerController hotbar 路由没有第二份数值常量。

## 5. pass-through 与协议边界

结果显式区分 invalid slot、input blocked、consumer unavailable、choice invalid、Straight delegated/protocol rejected、sequence exhausted、Arc policy/intent rejected、Arc delegated/protocol rejected。

Straight 只接受 slot 与当前请求一致且 diagnostic 非空的原 InputAdapter evidence。Arc 只接受 intent-matching 的有效 P20.16 evidence；`ShouldPassThrough()` 只沿有效的 P20.16 → P20.15 → P20.14 → P20.13 链读取既有 `Fdemo_mapShanmenThrownWeaponInputResult`，不复制物品分类逻辑。

这保证 Arc choice 下的非投掷物仍通过原 item classifier 返回 generic hotbar，而 Arc thrown item 的 handling/rejection 仍由既有 product authority 决定。任何无效或不匹配 wrapper 都不会被解释成 pass-through。

## 6. PlayerController 输入栅栏

`RouteThrownWeaponHotbarConfirmationInput` 把既有 `IsGameplayInputAllowed()`、`Gameplay` surface 与 `GameOnly` mode 合并为本地栅栏。栅栏失败时不访问 World/GameMode、不读 choice/policy、不路由 Straight/Arc，也不消费 Arc event ordinal。

本地允许时才解析 auth GameMode。adapter 的首次 choice read 决定本次 Straight/Arc 分支；P20.16 和 P20.15 仍分别重新读取并核对 choice，因此分流后若权威 choice 变化，会由既有 stale-choice fence 拒绝，不会使用过期 Arc 命令。

## 7. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter` 7 项：

- `PolicyAndIdentity`：单一 policy、确定性 ID 与保留序列值；
- `GatesAndChoice`：slot/input/consumer/choice 的固定短路顺序；
- `StraightPassThroughAndHandled`：原 Straight 语义、零 Arc 工作与零序列消费；
- `ArcPassThroughAndHandled`：非投掷物 pass-through、产品认领与两次新 event；
- `ProtocolFailures`：Straight slot mismatch、invalid policy、invalid/mismatched P20.16 evidence；
- `SequenceExhaustion`：最后可用 ordinal、耗尽 sentinel 与不回绕；
- `PlayerControllerBoundary`：Uninitialized mode 在 World consumer 之前阻止。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `hotbar_confirmation_final.log` | `Product.ThrownWeaponHotbarConfirmationAdapter` | `7/0` | `9783AC42DD694B0171A59D3E69D2459C1DA6201579160534C85F40BE3BD650E0` |
| `arc_confirmation_owner_final.log` | `Product.ThrownWeaponArcConfirmationOwner` | `6/0` | `85E68F7D56C8427F80994F5870B9F9BFD6D1B2BF23DAF614815AECFC1D4FD66A` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `ADDDDF39C388BB7DAB7DF82A49069FC7C1583F6108856AB9AB10E0DB59210E11` |
| `source_basis_adapter_final.log` | `Product.ThrownWeaponArcSourceBasisAdapter` | `6/0` | `67FDC54DE6CF00EE0732011D67D2DD837F9B84D8B96D8CBCBF294EC9D076140B` |
| `arc_choice_composition_final.log` | `Product.ThrownWeaponArcChoiceInputComposition` | `5/0` | `F28D8811F7000048ABB873A371329F8509881DE9F442B461FB36A6B9D7C6F419` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `B5DD90FD8D6D2ED10F4F9E850E2338B62834BD61A8662C220153F4B50CB9EC66` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `51A9B0947AE1218AC26F7D2B93633696910A55BC2E99CFBE5EC1A00B755CE190` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `0ACFCA2C66A18B448DACE5643BA3120A3C60436535832DF597AB879DAE810B02` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `91BE6DB93A31ED0708B7944FCC300F8211816660ABE4C5DC50020255B5B9E88A` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `2F019D93401EDF5CA0EFF00B461978BB5B7B890FBC6123D6EC87E6E698BCD687` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `907/0` | `A2446B16AC4A916E5B4EC6F0DDD17DB50DC635FE22C76B497B3FE3CD999BAEF4` |

十一份最终 Unreal 日志合计 `1083/0`，0 no-match、0 fatal/unhandled/ensure，每份恰好一个 RunTests command 与 terminal-success marker；审计 SHA-256：`6172B3F5AD45CCFF9898F8B0DE470E85323B036CF3B01B23F43204CFE28D5905`。全量从 P20.16 的 900 增至 `907`，恰好增加本轮 7 项。

## 8. 首次验证、流程证据与静态门禁

首次 Editor 构建直接通过：20 actions / native 0 / 42.60 秒，SHA-256 `BD635C52769272798EF4D751A7735EC75B80D4F2E3F132C066AA00286160DB59`。首次 exact 直接为 `7/0`，SHA-256 `2C306A42D942C97BBBC9B98A42800E587BA9E00726A49161DAB72BF0940B6C34`；没有源码或测试断言返工。

回归映射新增 hotbar-confirmation rule，并把 exact 加入 PlayerCombatController；正例与 unrelated-item 反例进入流程自测，最终 `327/327`，SHA-256 `C80FD0D0D48CFF7656B696E447F8DD9DD27DEA69DF0D9D51C06BC31F729D47C8`。

静态审计为 `PASS`：新 adapter 的 World/Actor/device binding/UI/product mutation/RNG 禁止 API 0；PlayerController 新增物理绑定 0；既有 hotbar binding 9；`UseHotbarSlot` 新 adapter 1、旧直接 route 0；controller adapter 内 Straight 1、Arc confirmation 1、policy source 1。SHA-256：`0C31E59927EA6322ECC4E498F214D625B1178A557EF55E823E4FA97729745443`。

预 Report changed-file gate：`PASS Changed=8 Rules=3 Required=18 Logs=11`，SHA-256 `57F0F881EF576FEB2BBC65E953CD95DB9418B4D3E0559F8020C76B9611A888DD`。最终 changed-file gate：`PASS Changed=10 Rules=3 Required=18 Logs=11`，SHA-256 `1D9B7939B7882D1CF05A61EA04940506AC71E9B2C0A685C7F302C932E43B5145`。暂存区格式检查：`PASS / 10 files / native 0`。

## 9. 构建与产物

- final Editor：`0 actions / native 0 / 1.06 秒`，SHA-256 `81911D08EB5EEA9DB2CFEF1F98F2AFC9AC483581733F4DA21E57C2448F750728`；
- final Game：`19 actions / native 0 / 47.53 秒`，SHA-256 `382588645CC8F9E4A178A6AF10E46E50063F3D246013E0D1F10D78DD6F61A552`；
- `Binaries/Win64/demo_map.exe`：`357420544` bytes，SHA-256 `D85FA0139D68467C83DA761965159F4E2B64D0085B8FB29938AEE069613C1361`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`16123392` bytes，SHA-256 `4E74DB0CB6346A6C2EA9E45DC33B9ADA096C7ADCB4284D2281FAD2B4A60C5F60`。

首次 Game 构建命令把项目目录误写成不存在的 `Dev.D.UE.0.0.B`，以 `OtherCompilationError` / native 6 在 0.06 秒失败；原始证据保存在 `game_build_first.log`，SHA-256 `73DB85A5B60619E36EA811532042C5BEF791B295AE26A3DA35BA4972C2C89CA0`。只修正命令路径后即得到上述成功构建，没有改源码或测试。

本轮只执行编译与无头自动化。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。全量中的 `generate_204` timeout 属既有环境噪声；队列持续推进并最终 `907/0`、native 0。

## 10. P/F 边界与下一步

P20.17 证明的是：“现有 1–9 hotbar 入口能依据唯一 current choice，在 Straight 下保持原路由，在 Arc 下生成新 logical event、读取单一 policy 并走 P20.16 唯一确认链；非投掷物 pass-through、输入栅栏、序列耗尽和协议异常均可审计且失败关闭。”

它没有证明真实用户已能编辑 trajectory/target/apex choice、UI/preview、真实输入、真实注册 source 的成功 Arc launch、Editor/PIE/Standalone、world trace、碰撞、命中、库存扣减或伤害。`907/0` 不能描述为产品运行验收。

建议 P20.18 增加 device-independent choice-edit controller adapter：用既有 PlayerController 输入栅栏把冻结的 P20.10 command 提交给唯一 GameMode P20.11 session，保留 expected-revision/replay/conflict evidence；仍不新增物理键或 UI。这样后续 UI/设备层只需调用一个入口，不会绕过 Run/lifecycle freeze。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.17.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.17.r0_log.md>
