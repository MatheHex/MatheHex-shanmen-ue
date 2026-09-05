# Dev.D.UE.0.0.10.P20.21.r0 Report

## 1. 结论

P20.21 已把 P20.20 revisionless read model 收束为可安全提交的、设备/UI 中立的投掷武器选择交互请求。请求只冻结用户所见的 read-model ID 与一条已经由 P20.20 capability 校验并规范化的 P20.19 intent；不复制 choice state、revision、session 或产品状态。

新的无状态 coordinator 在执行时只重读当前交互投影一次。当前 read-model ID 与请求一致时，才把请求内原始 P20.19 intent 路由一次；不一致时返回 typed `StaleReadModel`，路由次数为 0。相同可见状态即使 revision 已从 0 变为 2，仍会通过 revisionless fence，再由 P20.19 从唯一当前 state 冻结 revision 2 的 P20.10 command，因此不会错误拒绝安全的“离开后恢复”状态，也不会使用旧 revision。

`Ademo_mapPlayerController` 新增这一组合入口，但没有新增按键、UI、World 状态、重试或第二写路径。聚焦自动化新增 `7/0`，最终全量为 `935/0`。12 份有效自动化日志累计记录 `1149/0`，实际外网探测为 0。本轮只执行编译与无头自动化；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`1815a97673312d008928659da669b1ff743a2cf0`（P20.20）；
- 分支：`agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Immutable Interaction Request

`Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest` 只保存三个不可变事实：

- 由 expected read-model ID 与 P20.19 intent ID 确定性派生的 `RequestId`；
- 用户操作时所见的 `ExpectedReadModelId`；
- 已规范化的 P20.19 `Intent`。

trajectory、Arc target、apex 与 clear 四类 capture factory 全部调用 P20.20 emit helper。等价 target（`3,4` 与 `0.6,0.8`）得到同一 intent 与 request identity；当前 capability 不允许的操作会清空输出并失败关闭。请求不携带 revision，不能绕过 P20.19 对唯一当前 state 的重新读取。

## 4. Stale-View Fence

`Execute` 的顺序固定为：

1. 验证 request；无效时 read/route 均为 0；
2. 调用 P20.20 current-read callback 一次；
3. 拒绝无效协议证据或 typed source/state read failure；
4. 比较 current read-model ID 与 expected ID；不等则返回 `StaleReadModel`，route 为 0；
5. 相等时把请求中原始 intent 交给 P20.19 callback 一次；
6. 验证返回 intent evidence 与 request intent 一致，再返回 `Routed`。

coordinator 没有循环或自动重试。当前投影变化后恢复为同一 canonical 可见状态时，revisionless identity 相同；P20.19 会在 fence 后读取当前 revision，因此安全请求可以继续，而不是被历史 revision 误伤。

## 5. Typed Result 与下游拒绝

结果区分 `RequestInvalid`、`InteractionReadRejected`、`InteractionReadProtocolRejected`、`StaleReadModel`、`Routed` 与 `IntentRouteProtocolRejected`，并记录 read/route 精确计数以及嵌套 P20.20/P20.19 evidence。

`Routed` 表示请求已正确到达 P20.19，不等同于产品接受。P20.18 gameplay/surface/mode gate 或 P20.11 Run/lifecycle fence 的合法拒绝仍保留在嵌套 intent result 中，由 `WasRejectedByIntentRoute()` 显式暴露；无效或错 intent 的下游结果则视为协议错误，不能伪装成业务拒绝。

## 6. PlayerController 边界

`RouteThrownWeaponInputChoiceInteractionRequest` 只组合现有两个入口：

- `ReadThrownWeaponInputChoiceInteraction()`：当前投影读取一次；
- `RouteThrownWeaponInputChoiceIntent()`：匹配后路由一次。

无 World/GameMode 的 transient controller 在 current-read 阶段返回 typed source rejection，intent route 为 0。新增 diff 中没有 `BindAction`、`FKey` 或 `EKeys`，不会改变既有物理输入与热栏行为。

## 7. 自动化覆盖

新增 `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` 7 项：

- `RequestCanonicalization`：等价原始值稳定 identity，非法 capability 失败关闭；
- `AllRequestKinds`：五种可见编辑均冻结为既有 P20.19 intent；
- `ReadRejections`：无效 request、缺 source 与无效 state 的一次性短路；
- `StaleReadModelFence`：可见状态变化时 route 为 0；
- `RevisionlessCurrentRoute`：revision 0→2 且可见状态恢复时使用当前 revision 2 路由；
- `ProtocolAndDownstreamRejection`：无效/错 intent evidence 与合法 gameplay 拒绝分离；
- `PlayerControllerBoundary`：无 GameMode 时在 current-read 栅栏失败关闭。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `choice_interaction_request_final.log` | `Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` | `7/0` | `52CF8A03C7F59F863329CFF54D531B79D14CD0CB512EF7B54270D92EBF19B6E4` |
| `choice_interaction_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `BFFD74A508020D48DAF458B71C14EF9F95B72257F5C4474DBCDA36A358F2B79E` |
| `choice_intent_final.log` | `Product.ThrownWeaponInputChoiceIntentAdapter` | `7/0` | `0D2B694EA930D68D36CC71DFD1CA5773A52524E3811042D8CEBB0D99DBAD0936` |
| `choice_controller_final.log` | `Product.ThrownWeaponInputChoiceControllerAdapter` | `7/0` | `F7E1A04BDDD2798183B0C20EE1C87C2EBB375C6A4CA7ED89D4BD745EFD73649C` |
| `input_choice_session_final.log` | `Product.ThrownWeaponInputChoiceSession` | `4/0` | `0DD2212D7A202D13F7E2C93EC95587D2F8B32B5488E64FFFA82FDDF9FA561F8A` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `37/0` | `71D07C7A305568AFD51F125C83C159ABAFA9449545613FEE08E90D630F9C5FC8` |
| `hotbar_confirmation_final.log` | `Product.ThrownWeaponHotbarConfirmationAdapter` | `7/0` | `0E76709F1DD4B371641A78E4EF195D0723C5AA74541E5DE8C325E2D2BF650D17` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `CA46E66D0A44D59B9B5EAC2D1FF7E24CAFD0A821411A152D475C2E24E8A2FDC9` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `6E0D285C97AAB837C8617AFF8DD34F17F3B607BEC4643CE55A8BED9D325CFE9D` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `E7E94615888A5EFC0C2F33584A38D6FA6461EB3FB91303767D8089B352C1CE1C` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `897F1419EC1944F8B821F8FEBDD814BDCF9510992FFCB59487A04299477FC38F` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `935/0` | `C25788DD81E8F7496F508F9D6D6E692C4D118E02180D2AAD10101AB654654C6C` |

日志审计：`PASS Logs=12 RecordedSuccess=1149 Invalid=0 ExternalProbe=0`，SHA-256 `E157DD1A6743386748354790BE121B387DBBA2E73D354E7B66045BF38C9BFE8E`。

## 9. 流程、静态边界与构建

- regression self-test：`335/335`，SHA-256 `CD34922FD74E235259C4C513CE931292E8C15C16967BB27652CAEFB1170DA6A8`；
- static audit：`PASS ForbiddenProduction=0 RevisionReads=0 DirectSessionSubmit=0 DirectCommandCapture=0 IntentFactoryCalls=4 CoordinatorReads=1 CoordinatorRoutes=1 RetryLoops=0 AddedPhysicalBinding=0 ControllerCurrentReads=1 ControllerIntentRoutes=1`，SHA-256 `C06985F6E6A94973A73D6E693C33AD83B32741779F73E5EE80E46AAF9A7BA05C`；
- changed-file regression gate：`PASS Changed=9 Rules=2 Required=20 Logs=12`，SHA-256 `158833177A91F22B93283E647FF20F1C0551F85F0236D923C7D9DE1D86C584F9`；
- `git diff --check`：PASS / native 0，SHA-256 `10BAFB137822E90B65D5C497E00038B8979F82FDF16642E24B547AFA7C55FC4C`；
- 首次 Editor 编译：24 actions / native 0 / 57.41 秒，SHA-256 `719CAB95213DB171F0E86111F46E7D4BD67883FE179824F0EDBAF3BB1D8A08B0`；
- exact test 首次即为 `7/0`，且源码此后未改；final exact 复用同一份冻结日志，SHA-256 相同；
- final Editor：0 actions / native 0 / 1.40 秒，SHA-256 `D9905CE9F60106C250DB72C0F2282FC81F40462A3B9220980C0D259BA921F6C4`；
- final Game：23 actions / native 0 / 56.90 秒，SHA-256 `995B2BA5359620EA9D2153EB486F34DC10043BE3FCFEDD30C42559B463F5B3AD`。

产物：

- `Binaries/Win64/demo_map.exe`：357,531,136 bytes，SHA-256 `32E758DE4E1BED9B8AEACB8236528F497B689DD60B058935C4AA230440D2B505`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,259,072 bytes，SHA-256 `7D0EF9F7255AEAC09BA3E15BFCEC80A1A9DF95725A7D88112CDE12598F43A61A`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：基于 visible read-model identity 的 immutable request、一次 current projection 重读、typed stale rejection、相同可见状态跨 revision 的安全继续、一次 P20.19 route、下游拒绝与协议异常分离，以及 PlayerController 组合边界。

未验证：真实键鼠/手柄/触控或 UI 产生 request、具体交互布局、轨迹预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为产品运行验收。

建议 P20.22 不再增加中间协议层，直接做最薄的产品输入接线：复用现有可配置输入体系提供一个 trajectory-toggle logical event，按“读当前 model → 捕获 P20.21 request → 路由一次”完成 Straight/Arc 切换；不硬编码新键位、不复制 state，target/apex 继续等待明确的指针/UI 语义。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request/Docs/Report/Dev.D.UE.0.0.10.P20.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request/Docs/Log/Dev.D.UE.0.0.10.P20.21.r0_log.md>
