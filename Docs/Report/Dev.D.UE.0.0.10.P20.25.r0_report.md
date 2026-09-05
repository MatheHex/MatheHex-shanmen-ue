# Dev.D.UE.0.0.10.P20.25.r0 Report

## 1. 结论

P20.25 已在 P20.20 interaction read 与 P20.21 stale-safe request 之间补齐 Ballistic Arc 的设备无关编辑合成缝。调用方把同一次权威读取结果连同 target、apex delta 或 clear 操作交给新增合成器，即可得到既有 P20.21 请求；合成器不再定义一份 operation、intent、request、identity、revision、result 或状态协议。

三个入口均先清理复用输出，只接受 `IsProjected()` 的 P20.20 read result，然后各自只调用一次 P20.21 的 target、apex 或 clear 捕获入口。target 规范化、apex 规范化、capability 判断、request identity 与后续 stale fence 均继续由 P20.19–P20.21 独占。本轮新增聚焦自动化 `7/0`，最终 0.0.10 全量由 `955` 增至 `962/0`。

本轮只执行编译、纯值合成自动化和静态检查；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`6f3ed11fc4def20422d36dee5e071c21ee54de4a`（P20.24）；
- 分支：`agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 最薄合成契约

新增 `Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition`，只提供三个无状态静态入口：

- `TryComposeTargetRequest(Read, RawTargetIntent, OutRequest)`；
- `TryComposeApexAdjustmentRequest(Read, RawNormalizedDelta, OutRequest)`；
- `TryComposeTargetClearRequest(Read, OutRequest)`。

每个入口的固定行为只有三步：清空 `OutRequest`、验证同一份 read 已投影、委托现有 P20.21 request capture。它不读取或派生 read-model/request ID，不读取 revision，不执行 coordinator，不提交 choice，不持有 callback，也不返回新的状态对象。

## 4. 既有权威边界复用

P20.25 没有复制下列职责：

- P20.19 继续独占 raw target 与 apex delta 的 canonical intent 捕获；
- P20.20 继续独占当前可见 choice、capability 与 revisionless read-model identity；
- P20.21 继续独占 immutable request identity，以及执行时重新读取并拒绝 stale read model；
- P20.18/P20.11 继续独占 controller route、session 与 authoritative choice state。

因此 P20.25 不是第二个输入协议，也不会让 UI 或设备层绕过 stale fence。它只解决调用方已经持有完整 P20.20 read result、而 P20.21 捕获入口接受 read model 时的单一衔接点。

## 5. 失败关闭与设备中立

source unavailable、invalid choice state、Straight trajectory、Arc 无 target 时 clear、apex 上界继续增加、apex 下界继续减少、零/NaN delta 或零 target 均返回 false，并把复用输出恢复为 invalid request。

生产合成文件不包含 `UWorld`、Actor、Pawn、PlayerController、UObject、timer、input registry、`EKeys` 或 `FKey`；不直接调用 Execute、Route、Submit、Reduce、TryEmit 或 command capture。没有选定鼠标、键盘、手柄或触控语义，也没有修改 PlayerController/HUD。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition` 新增 7 项：

- `TargetRequest`：`(3,4)` 经既有链规范化为 `(0.6,0.8)`，并保存当前 read-model identity；
- `ApexRequest`：`0.75` 保持为 canonical apex intent，并保存当前 read-model identity；
- `ClearRequest`：只有已有 target 的 Arc read 才能合成 clear；
- `CapabilityRejections`：Straight、无 target clear 与 apex 双边界均失败关闭；
- `InvalidPayloadClearsOutput`：零 target、零 delta 与 NaN delta 清理复用输出；
- `InvalidReadClearsOutput`：source unavailable 与 invalid state 在捕获前失败；
- `RevisionNeutrality`：revision 1 与 Arc→Straight→Arc 后 revision 3 的等价可见 choice 产生同一 request。

新组首轮即为 `7/0`，本轮没有自动化失败或修复轮。

## 7. 改动文件回归映射

新增 `ThrownWeaponArcEditingInteractionComposition` 映射规则，要求：新合成组、P20.21 request coordinator、P20.20 interaction port、P20.19 intent adapter 与完整 `Shanmen.0_0_10`。

映射正/反自测新增 2 项，总数从 `343` 增至 `345/345`。最终 changed-file gate 对 3 个新增生产/测试路径求规则并集，结果为 `Changed=3 Rules=1 Required=5 Logs=5`；每个要求组都有成功日志，无未映射生产路径。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_editing_interaction_composition_final.log` | `Product.ThrownWeaponArcEditingInteractionComposition` | `7/0` | `B15A8925AF2C1E56E0837733BEC243718785A19B099C6D10556056080613ADE8` |
| `interaction_request_coordinator_final.log` | `Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` | `7/0` | `79F9761A443ACC4373A63536395B146205769C69D6668F6B4E7EFCE40F0654EF` |
| `interaction_port_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `3F724E8B7F3B7AEF8FF62C637CBA92074E505ECCE75DBE0135D0C0428DD56FB2` |
| `intent_adapter_final.log` | `Product.ThrownWeaponInputChoiceIntentAdapter` | `7/0` | `095902D76C545E6C921CBCCC74D5406574C4C75A0D96C47E1355A55475C2E031` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `962/0` | `AD9185ED9AC0EAD31989A402FF0C8AA103C3852FE4B2CD369EE2558EE7AD9A1B` |

最终日志审计：`PASS Logs=5 RecordedSuccess=990 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `DDFD0AF2AE05D63E50D3C0957AD8E98545196B74E76706154AA485724AE3E2BF`。

## 9. 流程、静态边界与构建

- regression self-test：`345/345`，SHA-256 `2EC1A0BFF1820CB7990E0E0E92E1C33FA26D6099361D598AAC79AA4FBDFDB0B5`；
- static audit：`PASS ExactTests=7 ForbiddenRuntime=0 PhysicalDevices=0 ExtraIdentity=0 RouteOrMutation=0 ComposeMethods=3 ReadGuards=3 Delegations=3 OutputClears=3`，SHA-256 `5B57076F468E13226E0F0B11263B142842F199E681E5F352BC12A0BEAFAE9274`；
- changed-file gate：`PASS Changed=3 Rules=1 Required=5 Logs=5`，SHA-256 `0EB2DB443B1C907C8A239CA478005B0587549D30D7FD436AA4C96BFB0980C359`；
- `git diff --cached --check`：PASS / native 0 / 7 个精确暂存文件，SHA-256 `FF49A1FBB409F33D781BCE03C4A012D3E11D1F9F871102F9A0E0BBF303E8EC81`；
- final Editor：0 actions / native 0 / 1.07 秒，SHA-256 `3BDA91ED8541F64352B56CAD4557D7945316934D228D3354F2DFBB1BDC75E3CF`；
- final Game：4 actions / native 0 / 23.19 秒，SHA-256 `4C14B09966A921F84F594A88F5DBDB83CF9AE8C00743CC577542081440BA406A`。

产物：

- `Binaries/Win64/demo_map.exe`：357,615,104 bytes，SHA-256 `5CD9023BAF11E51ED39AE9C2BA978954D96BA1AAB7EB98FEF9864604F49CE2FF`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,363,008 bytes，SHA-256 `EB26A6BB956DCF81CAE0F4BDA69D69BAF513C64AE3DCC97B5FEF862CB35D9D82`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：同一 P20.20 read result 到三种既有 P20.21 stale-safe Arc request 的最薄合成、payload/capability/read 失败清理、revision-neutral request 重放，以及由新增路径自动推导的完整回归。

未验证：任何真实 UI/设备输入、请求执行、PlayerController 接入、choice state 修改、轨迹线/落点预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为可见产品验收。

建议 P20.26 在现有 PlayerController 上增加三个设备无关调用入口：每次只读一次 P20.20、调用一次本轮合成器、再走一次既有 P20.21 route；仍不绑定具体按键或指针，以便下一阶段单独决定键鼠、手柄与触控映射。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition/Docs/Report/Dev.D.UE.0.0.10.P20.25.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-25-thrown-weapon-arc-editing-interaction-composition/Docs/Log/Dev.D.UE.0.0.10.P20.25.r0_log.md>
