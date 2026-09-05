# Dev.D.UE.0.0.10.P20.26.r0 Report

## 1. 结论

P20.26 已把 P20.25 的 Ballistic Arc target、apex adjustment 与 target clear 三种设备无关合成接入现有 `Ademo_mapPlayerController`。三个公开入口共享同一条最薄流程：先取得一份 P20.20 interaction read，调用一次对应的 P20.25 compose，再把所得 request 交给一次既有 P20.21 stale-safe route。

本轮没有定义第二套 intent、request、result、identity、revision 或状态协议，也没有绑定鼠标、键盘、手柄、触控或 UI 控件。新增聚焦自动化 `7/0`，完整 0.0.10 由 `962` 增至 `969/0`；八组聚焦、依赖、全量与旧系统回归合计 `1127/0`。

本轮仅执行编译、无头自动化与静态检查；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`f6a2ba33da6df4e198452bab2a0ce17ad4a666db`（P20.25）；
- 分支：`agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. PlayerController 最薄入口

`Ademo_mapPlayerController` 新增三个普通 C++ 调用入口：

- `RouteThrownWeaponArcTargetInteraction(RawTargetIntent)`；
- `RouteThrownWeaponArcApexAdjustmentInteraction(RawNormalizedDelta)`；
- `RouteThrownWeaponArcTargetClearInteraction()`。

三者只负责把调用参数交给一个共享的内部 helper。合成器依赖只在 `.cpp` 中引入，没有泄漏到公开控制器头文件；调用方继续只看见现有 P20.21 request result。

## 4. 固定调用顺序与失败关闭

共享 helper 每次固定执行：

1. 一次初始 `ReadThrownWeaponInputChoiceInteraction()`；
2. 一次由调用方选定的 P20.25 compose；
3. 一次 `RouteThrownWeaponInputChoiceInteractionRequest(Request)`。

合成成功时，P20.21 route 按冻结契约再读一次当前 interaction model，比较 request 中的 expected read-model identity，然后至多路由一次 P20.19 intent。因此成功结果中的 `InteractionReadCount=1` 是 stale fence 的第二次读取；helper 的初始读取不被重复计入 P20.21 结果对象。

合成失败时，helper 仍只调用现有 P20.21 route 一次，但传入明确的 invalid request。P20.21 返回既有 typed `RequestInvalid`，其下游 read/intent-route 计数均为 0；不存在 choice 重读、写入、循环或重试。

## 5. 权威与设备边界

- P20.20 继续独占可见 choice、capability 与 revisionless read-model identity；
- P20.25 继续独占 target/apex/clear 到既有 P20.21 request 的合成；
- P20.21 继续独占 stale model 复查与至多一次 intent route；
- P20.19/P20.18/P20.11 继续独占 intent、command、controller/session 与 authoritative choice state；
- 既有 PlayerController gameplay-input fence 继续决定锁定期间是否接受写入。

新增生产代码没有直接访问 GameMode choice state、session submit、reducer、command capture 或 ID/revision 工厂。没有新增 input registry action、按键绑定、轴、pointer、设备分发、timer 或 retry。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute` 新增 7 项真实 transient World/PlayerController/GameMode 组合自动化：

- `TargetFreshReadRoundTrip`：`(3,4)` 规范化为 `(0.6,0.8)`；重复同值从新的 read-model identity 开始，并保持 revision 不变；
- `ApexRoundTrip`：`+0.75` 经完整控制器链写入一次；
- `ClearRoundTrip`：已有 target 时 clear 经完整链清除一次；
- `CapabilityRejections`：Straight 的三种 Arc 操作与 Arc 无 target clear 均返回 `RequestInvalid`，且下游计数为 0；
- `ApexBoundary`：`+1.0` 可达上界，继续 `+0.25` 失败关闭，`-0.25` 可重新进入范围；
- `InvalidPayloadRejections`：零 target、零 delta 与 NaN delta 均失败关闭且不改权威状态；
- `InputLock`：有效 request 到达现有 gameplay fence 后被 typed intent-route rejection，权威状态不变。

测试没有发送合成键、鼠标、手柄或触控事件；world fixture 只建立既有控制器与 GameMode 组合边界。

## 7. 改动文件回归映射

PlayerController 规则新增 P20.26 controller route 与 P20.25 composition 依赖；新增测试文件有独立规则，要求 controller route、composition、request coordinator、interaction port、intent adapter、controller adapter、choice session、choice reducer 与完整 0.0.10。

映射正/反自测新增 2 项，总数由 `345` 增至 `347/347`。最终 changed-file gate 对 5 个生产/测试/流程路径求规则并集：`Changed=5 Rules=2 Required=23 Logs=8`。完整 0.0.10 覆盖其子组，另有新入口及四个直接上游聚焦日志，并显式补跑 `demo_map.InputRestore` 与 `demo_map.V2RangedCompatibility`。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_editing_controller_route_final.log` | `Product.ThrownWeaponArcEditingControllerRoute` | `7/0` | `CDA322F714B0FCE19D6A224907BD72EC81B92C03CB48A3A391F70841E8303453` |
| `arc_editing_interaction_composition_final.log` | `Product.ThrownWeaponArcEditingInteractionComposition` | `7/0` | `74968D1B6DB9AEDCE6AB1D5450EB8A1C6D590307FA905C228505C834053D7C5F` |
| `interaction_request_coordinator_final.log` | `Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` | `7/0` | `020EF33898F2721AB0DE0FF583D8C77AF6AE87BE157455185720D5C8643BDC86` |
| `interaction_port_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `3D1B648BAF5FB2803E4403EF0AF2AA26E528D4AE671B79AE6FE82BE12088CD46` |
| `intent_adapter_final.log` | `Product.ThrownWeaponInputChoiceIntentAdapter` | `7/0` | `DE882C5983CC595773B09963DA21A544E42DC0F7597857C6A9BE40BC18C2EB3B` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `969/0` | `9161A0037A3E718E3FB68DD35083ED15B2848AB6C10661B1D133570DC8F5D7B5` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `6ED011C3C926E625D77CBC3D4CA1168E0C67417E70EC10323D44A1D1FFEA5B3F` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `0F499DAF3906E2935004F60B766ECAC63B3FE0165FCF3DD763C5650EE30BBEA2` |

日志审计：`PASS Logs=8 RecordedSuccess=1127 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `71ADC4D7C7D5F7324323AEC580333D4C987D9BC4520C87FA9560CAAA308852E7`。

完整 0.0.10 使用一个连续进程；在既有 retry/checkpoint/journal/codec 区间持续核验成功数、末项、CPU 与进程响应，没有重启或并行重复。最终末项为 `WorldGameplay.SweepAdapter`。

## 9. 流程、静态边界与构建

- regression self-test：`347/347`，SHA-256 `E4F3561C70453C0787184BE52EFB31A67510D084804935B56193143DAC9354F5`；
- static audit：`PASS ExactTests=7 PublicEntries=3 Definitions=3 InitialReads=1 RequestRoutes=1 Delegations=3 InvalidFallbacks=1 PhysicalDevices=0 PhysicalDispatchTests=0 NewProtocol=0 DirectAuthority=0 LoopsOrRetries=0`，SHA-256 `16D8C3C3838445509242721BD4547BE969A6BB9E564E5B015C9216D0076DF969`；
- changed-file gate：`PASS Changed=5 Rules=2 Required=23 Logs=8`，SHA-256 `2232D6DA2C081A0EF9EBB5F2D439293A954EF5C63D607D7CDCDB2ABED68BA9AA`；
- 首次 Editor：26 actions / native 0 / 32.08 秒，SHA-256 `8997E415F280F5E2A0282E9140D8695962CACDE3CB6677349598BCA657CD0575`；
- final Editor：26 actions / native 0 / 22.57 秒，SHA-256 `9108D0F060AA85719552843FFB4376A9F00539618E8B0739FCDC662734677F69`；
- final Game：25 actions / native 0 / 30.07 秒，SHA-256 `2195D71A51BA4C9B629283ED073B8F6B51C7D89932F1F12D80C6DAAF578FBC6B`。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,387,584 bytes，SHA-256 `5B5B32E71C27E0BA4BE3E420906017815C368223765C41FEAA3E9F46E2912B64`；
- `Binaries/Win64/demo_map.exe`：357,636,096 bytes，SHA-256 `522EF6B1662EB6C22CAA91823B6D23F6AD4BD214AB1DE5714C1DB61F8BE5725B`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：三个设备无关 PlayerController Arc 编辑入口、固定 read→compose→stale-safe route 顺序、typed composition failure、fresh-read no-op、apex 边界、input-lock 下游拒绝、完整 0.0.10 与受影响旧控制器回归。

未验证：任何真实 UI/物理设备映射、轨迹线/落点编辑控件、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为可见产品验收。

建议 P20.27 单独确定并实现第一种可重映射的 Arc 编辑设备策略：优先复用现有顶视角 pointer/aim 语义提供 target intent，再为 apex 增减与 clear 定义不冲突的 action；只调用本轮三个入口，不绕过 P20.20–P20.21，也不把设备状态写入 choice authority。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route/Docs/Report/Dev.D.UE.0.0.10.P20.26.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route/Docs/Log/Dev.D.UE.0.0.10.P20.26.r0_log.md>
