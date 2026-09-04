# Dev.D.UE.0.0.10.P20.15.r0 Report

## 1. 结论

P20.15 已在 P20.14 唯一 source-Actor basis route 之前增加 device-independent Arc launch command seam。调用方先冻结 hotbar slot、P20.11 Arc choice state 与 P20.12 projection policy；新适配器按固定顺序检查 gameplay lock、Gameplay input surface 与 GameOnly input mode，随后才解析权威 GameMode、读取当前 choice 一次，并把同一 slot、当前 pawn 与冻结 policy 精确委托给 P20.14。

相同 canonical 输入重放同一 command identity；slot、choice revision/state identity 或 policy identity 变化都会产生不同命令。旧命令面对已变化的权威 choice 会在产品 route 之前失败关闭。适配器不拥有物理按键、UI、轨迹预览、Actor/World、item、Run、inventory 或产品状态，也没有修改现有 hotbar 行为。

最终验证为：Arc launch exact `6/0`、source basis `6/0`、composition `5/0`、projection `5/0`、choice `9/0`、InputAdapter `9/0`、InputRestore `101/0`、V2RangedCompatibility `22/0`、0.0.10 全量 `894/0`。Editor 与 Game Development 构建均成功。changed-file regression gate 为 `PASS Changed=9 Rules=2 Required=16 Logs=9`。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实设备输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`5815956b234edbfaa398dca03e73be0fcc700a6a`（P20.14）；
- 分支：`agent/0.0.10-p20-15-thrown-weapon-arc-launch-input`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 不可变命令契约

新增 `Fdemo_mapShanmenThrownWeaponArcLaunchCommand`。`TryCapture` 只接受 1–9 hotbar slot、有效 BallisticArc choice、已存在 Arc target intent 与有效 projection policy。成功命令保存纯值快照，不保存 UObject 或回调。

command identity 使用命名空间 `demo_map.ShanmenThrownWeapon.ArcLaunchCommand.r1`，由 slot、choice state identity、choice revision 与 policy identity 确定性派生。`IsValid()` 会重新派生并比对 identity，避免任意 GUID 或字段不一致绕过契约；失败 capture 会清空输出，不能复用半成品命令。

## 4. 本地栅栏与惰性权威解析

`Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route` 的顺序固定为：

1. command validity；
2. `IsGameplayInputAllowed()`；
3. `InputSurfaceState == Gameplay`；
4. `InputModeState == GameOnly`；
5. resolve authoritative GameMode route 一次；
6. read current choice state 一次；
7. compare frozen/current choice；
8. delegate P20.14 一次。

前三个本地栅栏拒绝时，GameMode 解析、choice 读取与产品委派均为 0。route 不可用时停止在一次解析；choice 无效或已变化时停止在一次读取。结果显式记录三个次数与完整下游结果，因此提前结束、合法委派和协议异常都可无头审计。

## 5. 状态、重放与协议边界

状态区分 `CommandInvalid`、`GameplayBlocked`、`InputSurfaceBlocked`、`InputModeBlocked`、`ProductRouteUnavailable`、`ChoiceStateInvalid`、`ChoiceStateMismatch`、`Delegated` 与 `ProductProtocolRejected`。

同一有效命令可重复进入相同一次解析、一次读取、一次委派路径；是否被产品接受仍完全读取 P20.14 的 `IsAccepted()`，新层不提升下游拒绝。下游返回无效审计对象时，结果保留一次同步调用事实并升级为 typed protocol rejection。

## 6. PlayerController 接入与权威

`Ademo_mapPlayerController::RouteThrownWeaponArcLaunchCommand` 只提供现有 gameplay/surface/mode 状态，并在本地栅栏通过后惰性解析 `Ademo_mapGameMode`。它从 GameMode 读取唯一 P20.11 choice state，再调用：

`RouteThrownWeaponArcChoiceFromSourceHotbarInput(slot, GetPawn(), policy)`。

因此 source 仍为当前 controller pawn，basis 仍由 P20.14 采样一次，projection 仍由 P20.13/P20.12 完成，item/Run/source/world delivery 仍由既有 InputAdapter/GameMode 产品路由拥有。本轮没有增加 `BindKey`、`InputComponent`、`FKey` 或 UI 入口，也没有改写 `UseHotbarSlot`。

## 7. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter` 6 项：

- `CommandContract`：确定性 identity、slot/choice/policy 参与身份及非法形状失败关闭；
- `LocalGateOrder`：gameplay、surface、mode 固定顺序及零下游工作；
- `ContextFences`：route unavailable、invalid choice、stale choice 的一次性边界；
- `DelegateAndReplay`：精确一次委派、同命令重放与完整 P20.14 evidence；
- `ProductProtocol`：无效下游结果升级为 typed protocol rejection；
- `PlayerControllerBoundary`：transient controller 的 Uninitialized input mode 在 GameMode 解析前拒绝。

最终日志：

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `3F2B331901CCB0DE26A48364F5836A7FAB6C9A35D8D5A3F4B8B8E515CBE3AF1D` |
| `source_basis_adapter_final.log` | `Product.ThrownWeaponArcSourceBasisAdapter` | `6/0` | `FEF0412C392CB7F24E5F87D1037D48C3A29B8700F48B4EB621F71B91A77B4F04` |
| `arc_choice_composition_final.log` | `Product.ThrownWeaponArcChoiceInputComposition` | `5/0` | `D197D8CCDC614FA65FB6CEE97762FD8F839B962301099E0E219D75BD0DAD0455` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `84899D2A8F7B987ABCD2F84689A5716F8D4C6F195CD9E67FDFBDBA49C464B20F` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `77D986316AC631FA950586CA98950DA9B5DF6E36879BF5947ABC70E44D0BEBA3` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `B3A431467A2E18261ACA69E53977658929AFC5FFB27CEC3131AF417EBAC9CAAB` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `C98C295D0288D1853BD214EB2C4F384E49E365E705EFDC17C3654F4E3EEA6791` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `3AF46393C59E0276E122296B65A37C318A6CCB2D4E320D6620F314EBA6DF0091` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `894/0` | `F2E0CEFB04F3BE9A6E4F601EA7282A092A9E009D5FF00DD5977C1E1DE5897CCD` |

九份最终日志合计 `1057/0`，0 no-match、0 fatal/unhandled/ensure；审计 SHA-256：`00827E5187EE9F72B14A621B6664FEDCC3ED52F4FA6B34AC649565D4C3D2611B`。0.0.10 全量从 P20.14 的 888 增至 `894`，恰好增加本轮 6 项。

## 8. 首次验证、门禁与静态边界

首次 Editor 构建直接通过：18 actions / native 0 / 47.91 秒。首次 Arc launch exact 直接为 `6/0`、无 fatal/unhandled/ensure，SHA-256 `05570928A69B0BA9D691FF3BE4C2115A542887E867F1E9D9E62BAF60FA4807B0`；没有源码或测试断言修复。

回归映射新增 Arc launch rule，要求本轮 exact、P20.14 source、P20.13 composition、P20.12 projection、P20.11 choice、既有 InputAdapter 与 0.0.10 full；PlayerCombatController rule 同步加入本轮 exact。流程自测新增 broad evidence 正例和无关 item evidence 必须失败的反例，最终 `323/323`，SHA-256 `27B4C31C4B4F5DECC5AA083963F158C33DD9D55F9FF95C4BB3E9CD9B8FE0D33B`。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=16 Logs=9
```

- gate SHA-256：`01B1B781979EA823508EC8CF95EDBE8A9E820B75F0CA9606F710A7D55DAAB763`；
- adapter production 边界：387 行扫描 direct World/Actor、spawn、trace/sweep、device binding、库存/物品权威、伤害、RNG 与产品 begin/commit API，0 forbidden matches；
- PlayerController 35 条新增行只在既有 controller 世界边界惰性解析 GameMode/pawn，未增加物理绑定；
- boundary/placeholder/JSON 审计 SHA-256：`5B56FE799EC83EF358C788711DB7C9364F0EE99074F296CFB7121F1910C73EAF`；
- `git diff --cached --check`：精确暂存本轮 9 个文件后 `PASS / native 0`，证据 SHA-256 `C0297E0DB69BC4079A82B13C8735416999C4518B83F1780973319E551A1F631B`；
- 长期未跟踪文件未纳入暂存、提交或推送。

## 9. 构建与产物

- final Editor：`0 actions / native 0 / 1.59 秒`，SHA-256 `22F603357209EAD4F51A72D13A71DC0A306FD5086504C9B3BDE9AFE05A06E8AD`；
- final Game：`0 actions / native 0 / 1.08 秒`，SHA-256 `6C565A2D73DE1FBC3EB257A1595518E1B0769DEB1B108CD265D6F14C6A7D95B3`。

产物：

- `Binaries/Win64/demo_map.exe`：`357347840` bytes，SHA-256 `95811883E66560B34EB98F38E1EDAA05509974C5893F4CFCEE63619839F1B4E0`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`16039424` bytes，SHA-256 `EA98E979DC1A3FA674C77682F8D02795BAEC185156E82D4819AE3DD3F14AF7D9`。

headless 日志中的 `generate_204` 网络探测超时与 controller large-delta 信息属于既有环境噪声；进程保持响应，各测试 terminal 与原生退出码以最终日志为准。

## 10. P/F 边界与下一步

P20.15 证明的是：“一个冻结且 device-independent 的 Arc launch command 能在既有 PlayerController 边界先经过 gameplay/surface/mode 栅栏，再核对唯一当前 choice，并把 slot、current pawn 与 policy 精确委托给 P20.14；本地拒绝、stale choice、合法重放和协议异常均可审计且失败关闭。”

它没有证明物理键鼠/手柄绑定、真实焦点事件、UI/轨迹预览、真实注册 source 的成功产品 launch、Editor/PIE/Standalone、真实输入、world trace、碰撞、命中、库存扣减或伤害。`894/0` 不能描述为产品运行验收。

建议 P20.16 增加 consumer-owned Arc confirmation intent/command owner：只在显式确认意图时，从唯一 current choice 与 policy 捕获本轮命令并交给 PlayerController；仍不绑定具体设备、不增加预览或第二套产品路由。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input/Docs/Report/Dev.D.UE.0.0.10.P20.15.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-15-thrown-weapon-arc-launch-input/Docs/Log/Dev.D.UE.0.0.10.P20.15.r0_log.md>
