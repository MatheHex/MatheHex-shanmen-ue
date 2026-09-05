# Dev.D.UE.0.0.10.P20.21.r0 Log

## 阶段

- 任务：P20.21 thrown-weapon stale-safe choice interaction request coordinator；
- 基线：`1815a97673312d008928659da669b1ff743a2cf0`；
- 分支：`agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 immutable `Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest`，只冻结 request ID、expected read-model ID 与既有 P20.19 intent。
2. 四个 request factory 全部委托 P20.20 capability/emit helper；非法或当前不可用操作清空输出并失败关闭。
3. request ID 由 expected read-model ID 与 intent ID 确定性派生，不携带 revision/state/session。
4. 新增无状态 coordinator：request validation → current interaction read 1 → read-model identity fence → P20.19 route 最多 1。
5. visible model 改变时返回 typed `StaleReadModel` 且 route 0；没有自动重试。
6. 相同 visible state 跨 revision 时通过 revisionless fence，P20.19 再读取唯一当前 revision 并冻结 command。
7. nested P20.20/P20.19 evidence 与 read/route 计数写入 typed result；合法 downstream rejection 与 protocol mismatch 分离。
8. `Ademo_mapPlayerController` 新增组合入口，只调用现有 current-read 与 intent-route 各一次；未新增按键/UI/第二状态。
9. 新增 7 项 exact 测试；更新 changed-file regression map 与正/反 self-test，总数 `335/335`。

## 首次证据与修正记录

- 最初一次构建命令外壳多出右括号，PowerShell 在 UnrealBuildTool 启动前即拒绝解析；未触达源码、构建器、产物或产品状态。删除该字符后继续同一有界流程。
- 首次实际 Editor 编译：24 actions / native 0 / 57.41 秒，SHA-256 `719CAB95213DB171F0E86111F46E7D4BD67883FE179824F0EDBAF3BB1D8A08B0`；
- `choice_interaction_request_first.log`：7/0 / terminal 1 / fatal 0，SHA-256 `52CF8A03C7F59F863329CFF54D531B79D14CD0CB512EF7B54270D92EBF19B6E4`；
- 首次实际编译与 exact 自动化均通过，没有源码、编译或断言返工；源码此后未修改，因此 final exact 复用同一冻结日志。

## 最终聚焦与全量回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `choice_interaction_request_final.log` | `7/0` | `52CF8A03C7F59F863329CFF54D531B79D14CD0CB512EF7B54270D92EBF19B6E4` |
| `choice_interaction_final.log` | `7/0` | `BFFD74A508020D48DAF458B71C14EF9F95B72257F5C4474DBCDA36A358F2B79E` |
| `choice_intent_final.log` | `7/0` | `0D2B694EA930D68D36CC71DFD1CA5773A52524E3811042D8CEBB0D99DBAD0936` |
| `choice_controller_final.log` | `7/0` | `F7E1A04BDDD2798183B0C20EE1C87C2EBB375C6A4CA7ED89D4BD745EFD73649C` |
| `input_choice_session_final.log` | `4/0` | `0DD2212D7A202D13F7E2C93EC95587D2F8B32B5488E64FFFA82FDDF9FA561F8A` |
| `input_choice_final.log` | `37/0` | `71D07C7A305568AFD51F125C83C159ABAFA9449545613FEE08E90D630F9C5FC8` |
| `hotbar_confirmation_final.log` | `7/0` | `0E76709F1DD4B371641A78E4EF195D0723C5AA74541E5DE8C325E2D2BF650D17` |
| `arc_launch_input_final.log` | `6/0` | `CA46E66D0A44D59B9B5EAC2D1FF7E24CAFD0A821411A152D475C2E24E8A2FDC9` |
| `input_adapter_final.log` | `9/0` | `6E0D285C97AAB837C8617AFF8DD34F17F3B607BEC4643CE55A8BED9D325CFE9D` |
| `legacy_input_restore_final.log` | `101/0` | `E7E94615888A5EFC0C2F33584A38D6FA6461EB3FB91303767D8089B352C1CE1C` |
| `legacy_v2_ranged_final.log` | `22/0` | `897F1419EC1944F8B821F8FEBDD814BDCF9510992FFCB59487A04299477FC38F` |
| `full_0_0_10_final.log` | `935/0` | `C25788DD81E8F7496F508F9D6D6E692C4D118E02180D2AAD10101AB654654C6C` |

日志审计：`PASS Logs=12 RecordedSuccess=1149 Invalid=0 ExternalProbe=0`，SHA-256 `E157DD1A6743386748354790BE121B387DBBA2E73D354E7B66045BF38C9BFE8E`。

全量中的既有 retry/checkpoint/manifest 纯值测试从 651 附近进入长计算窗口；日志、CPU 与进程响应持续推进，最终到 935/0。未重启、未并行重复、未把静默计算误判为失败。

## 流程与静态证据

- regression self-test：`335/335`，SHA-256 `CD34922FD74E235259C4C513CE931292E8C15C16967BB27652CAEFB1170DA6A8`；
- static audit：`PASS ForbiddenProduction=0 RevisionReads=0 DirectSessionSubmit=0 DirectCommandCapture=0 IntentFactoryCalls=4 CoordinatorReads=1 CoordinatorRoutes=1 RetryLoops=0 AddedPhysicalBinding=0 ControllerCurrentReads=1 ControllerIntentRoutes=1`，SHA-256 `C06985F6E6A94973A73D6E693C33AD83B32741779F73E5EE80E46AAF9A7BA05C`；
- changed-file gate：`PASS Changed=9 Rules=2 Required=20 Logs=12`，SHA-256 `158833177A91F22B93283E647FF20F1C0551F85F0236D923C7D9DE1D86C584F9`；
- `git diff --check`：PASS / native 0，SHA-256 `10BAFB137822E90B65D5C497E00038B8979F82FDF16642E24B547AFA7C55FC4C`。

## 最终构建

- Editor：0 actions / native 0 / 1.40 秒，SHA-256 `D9905CE9F60106C250DB72C0F2282FC81F40462A3B9220980C0D259BA921F6C4`；
- Game：23 actions / native 0 / 56.90 秒，SHA-256 `995B2BA5359620EA9D2153EB486F34DC10043BE3FCFEDD30C42559B463F5B3AD`；
- Game artifact：357,531,136 bytes，SHA-256 `32E758DE4E1BED9B8AEACB8236528F497B689DD60B058935C4AA230440D2B505`；
- Editor artifact：16,259,072 bytes，SHA-256 `7D0EF9F7255AEAC09BA3E15BFCEC80A1A9DF95725A7D88112CDE12598F43A61A`。

## P/F

PASS：immutable request、revisionless stale fence、一次 current read、最多一次 P20.19 route、当前 revision 冻结、typed downstream/protocol evidence 与 PlayerController 组合边界。

未验证：物理设备/UI 请求生成、具体交互、预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.22：停止继续堆协议层，直接把现有可配置输入体系中的一个 trajectory-toggle logical event 接到 P20.21；固定为 current read → request capture → one route，不硬编码新键位、不复制 state。Arc target/apex 等待明确的 UI/指针语义。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request/Docs/Report/Dev.D.UE.0.0.10.P20.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-21-thrown-weapon-choice-interaction-request/Docs/Log/Dev.D.UE.0.0.10.P20.21.r0_log.md>
