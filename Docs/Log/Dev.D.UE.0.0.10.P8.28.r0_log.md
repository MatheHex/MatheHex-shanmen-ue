# Dev.D.UE.0.0.10.P8.28.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.28.r0`；
- 基线：`4bc877515cb6c00ecd79599bdff3242d39234c26`（P8.27）；
- 分支：`agent/0.0.10-p8-28-formation-influence-consumer-coordinator`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

为 P8.26 registry 与 P8.27 native adapter 增加 component-bound application coordinator：冻结 subject/component、明确跨层顺序、原子提交 registry/native evidence、允许 native rejection 后重试、阻止历史 Apply replay 复活已 Remove modifier，并给 teardown 提供 drain 门禁。不得建立第二套 application 或 attribute authority。

## 设计记录

### candidate-before-commit

直接先提交 registry、再调用 native，会在 native rejection 时留下分裂状态。最终实现复制 coordinator，在候选 registry 上执行 command；native acknowledgement 成功后，才把 transaction receipt 和候选整体提交。失败只丢弃候选，不需要 pending mirror。

### immutable replay fence

coordinator completed history 保存 command 与原成功 result。重复 command 返回同 receipt 的 `TransactionReplayed`，不再次调用 native。该 fence 位于 registry/adapter 之前，因此旧 Apply 在后续 Remove 已完成后也不能复活 modifier。

### replay-aware compensation

提交前审查发现 native replay 没有 mutation，不能执行 inverse compensation。补偿最终只在 `NativeResult.bComponentMutated=true` 时执行；ApplyReplayed/RemoveReplayed 直接视为无需补偿。

### one authority per concern

active application 只由 registry 保存，native modifier 只由 attribute component 保存，coordinator 只保存 completed delivery evidence。没有 TMap、pending queue、mirrored active list 或额外 float conversion。

## 执行序列

1. 审查 P8.26 registry mutation/replay 与 P8.27 adapter/native rejection 语义。
2. 选择候选 registry + native sync + whole-candidate commit 顺序。
3. 新增 component-bound coordinator、transaction status/result/receipt 与 consistency checks。
4. 实现 native rejection 丢弃候选、completed command replay、history collision fence 与 drain query。
5. 增加 AtomicApplyRemove、NativeFailureRetry、HistoricalReplayFence、BindingAndEvidenceFence 四个 case。
6. regression map 增至 `64` 条，自检由 `86/86` 增至 `88/88`。
7. 首次 Editor：`5 actions / 22.64s / exit 0`；首轮及扩展 Automation 全绿。
8. 提交前审查修正 replay compensation，并让专项覆盖 ApplyReplayed/RemoveReplayed。
9. review Editor：`5 actions / 14.52s / exit 0`；最终七组 Automation 全绿。
10. changed-file gate：产品/流程改动 `Changed=5 / Rules=2 / Required=10 / Logs=7`；加入 Report/Log 后 `Changed=7 / Rules=2 / Required=10 / Logs=7`。
11. 最终 Editor up-to-date 与 Game `4 actions` 均成功；完成静态边界与 diff 检查。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.28-FormationInfluenceConsumerApplicationCoordinator-final.log` | 4 | 0 | 0 | `B756E0CB68255EA5359CE9BD8D4A26CB1111AE33274B766637BA791B31B5D28F` |
| `P8.28-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `8B12F583F496D46C0F3D756E7757EE2ED3A488034DC54E4976FEFD09FC5FBBDA` |
| `P8.28-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `EE7B9B28D61F880916FFF00168466E05CF13C0E486096BCB4B43988E4AC883BD` |
| `P8.28-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `11587E304064E17CC0A3F6C66540A3E148A89D79B2A3CF8008788A047A1AFD3D` |
| `P8.28-Attributes-final.log` | 4 | 0 | 0 | `F18FC1ADF7FDB833CAE065D4A2697A0366B4CABD532BE4E91E889E3B4C86E3D1` |
| `P8.28-FormationInfluence-final.log` | 71 | 0 | 0 | `3A9FCDB4DB44BA63838094DCC068AFF6FCC58371B0B6C9EF94FD4FB713F2983B` |
| `P8.28-Shanmen-full-final.log` | 320 | 0 | 0 | `50B13B971C2F0AE9202FAA8A1473B41A647D250D8C48233C30F7F8E15D7B9F07` |

全部日志都有一个 RunTests command、一个 queue-empty、Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=64
SELF_TEST: PASS 88/88
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=10 Logs=7
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=10 Logs=7
```

- mapping SHA-256：`5715E917F4049551905C0EB221240F80AB76E6B39EB540C39E2842B756DE57EC`；
- self-test SHA-256：`551A3E30044FF86FAA8EE30FCA0593772E776037BA9F7E1A80B84F34F6B38823`。

## 静态边界

```text
UWorld/AActor = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
RNG = 0
SaveGame/ProfileRepository = 0
Tick/while = 0
TMap = 0
TArray<Fdemo_mapActiveModifier> = 0
```

## 构建证据

- Editor initial：`5 actions / 22.64s / exit 0`，SHA-256 `2AA63C29E0F830A9FCB6A1D75F15FC5DB110DA702AB9F85909554A5D7B36691E`；
- Editor review correction：`5 actions / 14.52s / exit 0`，SHA-256 `B4B9FE77710BB3CB379F4465A431B4D5D536745ECB6234FE775C04F1D9C6AC9F`；
- Editor final：`0 actions / 0.91s / exit 0`，SHA-256 `BD448C22AD7ECB2C4AB744CEE78B7E9B8F28B7259D799B80A04BCA257E44A901`；
- Game final：`4 actions / 21.79s / exit 0`，SHA-256 `0EA8E3A4E186D454C356586AE102BFCA00D34C359290FFD861531991206D7EE0`；
- `UnrealEditor-demo_map.dll`：`11931648` bytes，SHA-256 `24AF1835597BE96986C0FF417E54B921D33BD70B04C5536DEF93C475C73AA18D`；
- `demo_map.exe`：`353008640` bytes，SHA-256 `E888A9BFAF0593C1609CCA164DD3A7B51DAE96FFAB78C9C5E7E649BB00A38378`。

## 真实异常记录

产品源码、Editor/Game 构建与 Automation 均无失败。提交前审查主动发现 replay compensation 风险并在最终证据前修正；首次成功日志保留为开发轨迹，不作为最终状态的替代。没有把审查风险描述成已发生的产品故障。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.29 可增加窄 consumer command host/resolver，把产品生命周期取得的 exact subject component 与 projection command 路由到 coordinator，并回传 transaction result；不得复制 registry、native modifier 或 coordinator history。
