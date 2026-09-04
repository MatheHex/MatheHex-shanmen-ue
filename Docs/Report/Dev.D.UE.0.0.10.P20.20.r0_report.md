# Dev.D.UE.0.0.10.P20.20.r0 Report

## 1. 结论

P20.20 已在 P20.19 choice-edit intent 前增加设备/UI 中立的投掷武器选择交互端口。新的 revisionless read model 只投影 P20.11 唯一权威状态中的当前轨迹、规范化 Arc target、apex 调整值与 typed capability mask；不会复制 revision、state identity 或可变状态。

交互端口能从 read model 生成既有 P20.19 intent，但不直接生成 P20.10 revision、路由 controller 或提交 session。`Ademo_mapPlayerController` 新增一次性只读入口：解析 auth GameMode 一次、读取 choice state 一次、投影一次；没有新增物理按键、UI 控件、重试或 fallback 写路径。

聚焦自动化新增 `7/0`，最终全量为 `928/0`。11 份有效自动化日志累计记录 `1128/0`，外网探测为 0。本轮只执行编译与无头自动化；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`f5f3f6ee0867168a9f5aab6a171aae940948392d`（P20.19）；
- 分支：`agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Revisionless Read Model

`Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel` 只包含可呈现的 canonical choice：Straight/BallisticArc、Arc target 是否存在及其单位圆值、`[-1,1]` apex 调整与当前 capability mask。

read-model identity 使用命名空间 `demo_map.ShanmenThrownWeapon.InputChoiceInteractionReadModel.r1`，由这些可见值与 capability 派生，不携带 revision。相同可见状态即使来自 revision 0 与 revision 2，也得到同一 read-model ID；P20.11 state ID 与 revision 仍留在唯一权威层。

投影前强制验证 P20.10 state；Straight 必须无 target 且 apex 为 0，Arc target 继续保留 P20.10 的 canonical unit-disc 规则。无效 state 失败关闭，不生成部分 read model。

## 4. Typed Capability 与 Intent 输出

capability 使用一个 typed bitmask，而非新增一组可变布尔状态：

- Straight 只提供切换 BallisticArc；
- Arc 提供切回 Straight、设置 target、可行方向的 apex 调整；
- 仅有 target 时提供 clear；
- apex 达到 `+1/-1` 时分别关闭继续增加/减少。

capability 表示“当前 canonical choice 下有意义的交互”，不是 gameplay 授权。Run、lifecycle、surface 与 input-mode gate 继续由 P20.18/P20.11 裁决。

四个 emit helper 只调用 P20.19 的 trajectory、target、apex 与 clear factory。非法操作或无 capability 时清空输出并失败关闭；端口内 P20.19 route、P20.11 `.Submit` 与 revision read 均为 0。

## 5. 一次性权威读取

`Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read` 固定顺序为：choice source resolve 一次 → state read 一次 → projection 一次。每个短路点返回 typed status、diagnostic 与精确计数，但结果不保留完整权威 state，因此不会形成第二状态来源。

`Ademo_mapPlayerController::ReadThrownWeaponInputChoiceInteraction` 只提供现有 auth GameMode 的 read seam。无 World/GameMode 时返回 `ChoiceSourceUnavailable`，state read 与 projection 计数均为 0；该入口不改变 input surface/mode，也不路由 intent。

## 6. 自动化覆盖

新增 `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` 7 项：

- `CanonicalProjection`：Straight/Arc canonical 可见值与不同身份；
- `CapabilityTransitions`：轨迹、target 与操作集合转换；
- `RevisionlessIdentity`：不同 revision 的同一可见 choice 得到相同 identity；
- `IntentEmission`：trajectory、target、apex 输出既有 P20.19 intent；
- `SaturationAndClear`：apex 边界与 target clear capability；
- `ReadOrder`：source/read/projection 的一次性短路顺序；
- `CompositionAndPlayerControllerBoundary`：P20.19→P20.10 组合及无 GameMode typed 拒绝。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `choice_interaction_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `D7CB030B98257FAE9B339A651D1BEAE063C0D444BDA1749E3D28FDE28C6E96C5` |
| `choice_intent_final.log` | `Product.ThrownWeaponInputChoiceIntentAdapter` | `7/0` | `5EC5C84D825D1CBB25F883C9ACF5B5B3A7038A03712ADDDE7B6A4F3DE1B59E49` |
| `choice_controller_final.log` | `Product.ThrownWeaponInputChoiceControllerAdapter` | `7/0` | `E199B8A5CBEA957AABD82804F9ED5C2531590E8698F7DAEF6D128242EEE5E6C3` |
| `input_choice_session_final.log` | `Product.ThrownWeaponInputChoiceSession` | `4/0` | `9E6631A7FE69135FE0FB1EC26B67774615BEE3535A84C3B3EE120E8D3F3D09F1` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `30/0` | `5761F85FEF31BECBD7B5569F27FC06F1270E6419530BB0C6C75DEB1B535888E1` |
| `hotbar_confirmation_final.log` | `Product.ThrownWeaponHotbarConfirmationAdapter` | `7/0` | `EBB492F30A08ECE12939391011344DEF3D19783D295BD2AF9CAD27E5B66541DF` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `BBB9942316DC1C1735E33710A03B2762E1CB5AAD8F4083EC7C9BDEDAC96C7F21` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `1AEA1E52A064A6CCD07869925C37645E3E969D456439C30F787A8079F4021160` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `D0E3AEA63D6D615EF19B6B8F13409A6E02267554A878EB319676A9D1A8B027C4` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `67C5AF957E49167E8DFA5492CC3489D5B2680A4796A4E5DB7D7B1241775BF2FE` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `928/0` | `5FC58C307B898DE26F46277DED78A60E8405B0454AB9E4533D22CC07B85C508B` |

日志审计：`PASS Logs=11 RecordedSuccess=1128 Invalid=0 ExternalProbe=0`，SHA-256 `B7B9943FA12A0239C68C48D1A9A0F5F8DBBCA0366362BC98954CC9011BE4FE3F`。

## 8. 流程、静态边界与构建

- regression self-test：`333/333`，SHA-256 `A1D5C5D4C186419A0F2AD7925F18DF21E29DD563E26C901EA49C8E6D0F2CF37F`；
- static audit：`PASS ForbiddenProduction=0 RevisionReads=0 DirectSessionSubmit=0 DirectIntentRoute=0 IntentFactoryCalls=4 AddedPhysicalBinding=0 ControllerChoiceReads=1 ControllerPortReads=1`，SHA-256 `64F0F64BFA374A5A7F575585FEAA75DD832E2CE772E4938F4A77273D7BCA7B13`；
- changed-file regression gate：`PASS Changed=9 Rules=2 Required=19 Logs=11`，SHA-256 `CC21105D56616C7C7CC81A2BF8E257A88E2ACE51B6C3A147AA0075582021C9F6`；
- `git diff --check`：PASS / native 0，SHA-256 `0E0E5C876FB4EDCBCCAD8B4AB94F5C8486F820BAC2EDF430FE8DF81D2746D5C7`；
- 首次 Editor 编译：23 actions / native 0 / 55.05 秒；随后 exact test 首次即为 `7/0`，无源码、编译或断言返工；
- final Editor：0 actions / native 0 / 1.15 秒，SHA-256 `40C73EBF3A06128DD715F756E75C77AE5CDDBCD94F1B391BEC41D756439B2C1F`；
- final Game：22 actions / native 0 / 57.03 秒，SHA-256 `139FC90764F679438A2799B8F0D0C4C9FE1C4F9A9B2BD7812CDE412392F3DC2F`。

## 9. 产物与 P/F 边界

- `Binaries/Win64/demo_map.exe`：357,501,440 bytes，SHA-256 `B4FECFDE7801F701E0DE1CD2DFE0406878679F4DFFC754B172CB1AF7AFAEC507`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,223,232 bytes，SHA-256 `EA3E2AAFCC651A0A277700716885D6A4458BDCCB1F5C05448164DA2AE45FD8C2`。

PASS 范围：唯一 choice state 的 revisionless 投影、typed capability、稳定 read-model identity、一次 source/read/projection、四类 P20.19 intent 输出、边界饱和与 typed 缺源拒绝。

未验证：具体键鼠/手柄/触控或 UI 控件、交互 view 的并发陈旧检测、预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为产品运行验收。

## 10. 下一步与 GitHub

建议 P20.21 增加 device/UI-neutral interaction request coordinator：冻结 read-model ID 与请求操作，重新读取一次当前投影并显式拒绝 stale view，再只输出并路由一条 P20.19 intent。它仍不绑定具体设备/控件，也不拥有第二状态或自动重试。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port/Docs/Report/Dev.D.UE.0.0.10.P20.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port/Docs/Log/Dev.D.UE.0.0.10.P20.20.r0_log.md>
