# Dev.D.UE.0.0.10.P20.18.r0 Log

## 阶段

- 任务：P20.18 thrown-weapon choice controller adapter；
- 基线：`8f8c45ad0f26ea9141accd0855e192850df656b2`；
- 分支：`agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 `demo_mapShanmenThrownWeaponInputChoiceControllerAdapter.h/.cpp`。
2. 新增 typed controller status/result，保留 command、session result、resolution/submission count 与 diagnostic。
3. 固定短路顺序：command validity → gameplay gate → Gameplay surface → GameOnly mode → GameMode/session resolve → one submit。
4. 对 `Applied`、`Replay`、`NoChange`、Run/lifecycle fence 与 reducer rejection 执行 typed protocol 自洽检查。
5. `Ademo_mapPlayerController` 新增 `RouteThrownWeaponInputChoiceCommand`，只委托现有 GameMode `SubmitThrownWeaponInputChoiceCommand`。
6. 新增 7 项 exact 测试，覆盖全部 P20.10 command kind 与 P20.11 结果类别。
7. 更新 changed-file regression map 与正/反 self-test，总数 `329/329`。

## 首次证据

- `editor_build_first.log`：21 actions / native 0 / 42.31s，SHA-256 `642B10C55F27FB93D13CE07B4E6DE2A294EED15C5603526B01A34560301AF79A`；
- `choice_controller_first.log`：7/0 / terminal marker 1 / fatal 0，SHA-256 `94DD722C67EFD2926DF5FFE10BA779F650CF8A8E4A7D871968C9C26D60DCC7DD`；
- 没有源码、编译或测试断言失败与返工。

## 最终聚焦回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `choice_controller_final.log` | `7/0` | `10B0322B04AD411A68241D182690FB1C81992933D7484603FF36037E195A23A7` |
| `input_choice_session_final.log` | `4/0` | `2321DFC04DE45ACAC9310EDE523526D83BFB08F72C5723304E126B4B55C62416` |
| `input_choice_final.log` | `16/0` | `0ECBCBC41E8A4A387A5C2BE19FFF033C6CAF8B62FC0E56EACC4676F41CDF55E9` |
| `hotbar_confirmation_final.log` | `7/0` | `44D99CA924A00B2D7FA6B69BDD8B30A139B5AEDDFDE685BE364D49A2C5981B6B` |
| `arc_launch_input_final.log` | `6/0` | `E29488103E674D62501CEBD33D0F95516CE745130152FA9C386D5BC5C73BD4F9` |
| `input_adapter_final.log` | `9/0` | `ECC65E2E56D3F93889332FE7F81180FDCF19553FBDB235A42C5D857BD61DEF92` |
| `legacy_input_restore_final.log` | `101/0` | `34FA26A92B2DF5A0AB33EB820D1A266B0B2B335FECCA7CD54FA8D9B4891D7F89` |
| `legacy_v2_ranged_final.log` | `22/0` | `55E7B262557DD3244245D139A949D31E658F27586BE56EB374984F2DF90D166D` |
| `full_0_0_10_final.log` | `914/0` | `1C27B8C2CBD63CD608C0A3973D9B0D1574DF20F9B97919B6BD25AC065380E250` |

日志审计：`PASS Logs=9 RecordedSuccess=1086 Invalid=0`，SHA-256 `FAF9B4D6D000DA3E64E839577334D9B3B82D9B1BCEF56BD13DF71BBFD015D1C9`。

## 流程与静态证据

- regression self-test：`329/329`，SHA-256 `0B40D9BEEF9037D58DEE7489C737CFE9069F8965D686C9406A373C31C8026214`；
- static audit：`PASS ForbiddenApi=0 AddedPhysicalBinding=0 ControllerAdapterCalls=1 GameModeSessionSubmitCalls=1 DirectSessionSubmitInAdapter=0`，SHA-256 `D67663EA391C0041DCC3B5495C41AFA2AD08A8066D756CA01CD5B71674E29E84`；
- changed-file gate：`PASS Changed=9 Rules=2 Required=17 Logs=9`，SHA-256 `097944CDA5DA5EB08BDEA1DB84ABB195A15709D4998D5B56920317F14C350B87`；
- staged diff：`PASS / 9 files / native 0`。

## 最终构建

- Editor：0 actions / native 0 / 1.25s，SHA-256 `8EBBB50947D86EC6DD5CB1926BD4FB8A85F2CC8D1A5C23DDB7C0D156F274E0F3`；
- Game：20 actions / native 0 / 51.59s，SHA-256 `78DDE03C672211AE4BEAF48A6E01A77EDD668B00E5BD5B65B3F3F62DF4D75910`；
- Game artifact：357,444,096 bytes，SHA-256 `6D443A72C11F4D572D4F65C85B27283AB95E44ACBE8FF76BB88FFCBA9FD20323`；
- Editor artifact：16,152,576 bytes，SHA-256 `4AF4394B88E1B83796D30E19D2DAF3DEC5D2EF003CF05A5E7C1E8EA5C160AE53`。

## P/F

PASS 范围：冻结 command 通过 PlayerController 现有栅栏，一次解析、一次提交至唯一 GameMode session，typed 重放/no-op/冲突证据可验证。

未验证：物理设备、UI、预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.19：device-independent choice-edit intent capture，一次读取当前 revision，生成 P20.10 command，仅委托 P20.18 controller route，仍不接物理键或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-18-thrown-weapon-choice-controller-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.18.r0_log.md>
