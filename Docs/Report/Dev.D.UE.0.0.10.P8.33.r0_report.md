# Dev.D.UE.0.0.10.P8.33.r0 Report

## 1. 结论

P8.33 已把 P8.32 formation influence consumer runtime 接入既有 caller-facing lifecycle command host。`LifecycleCommandHost` 现在内部持有唯一 consumer runtime 值状态，显式承接 native modifier Apply/Remove，并在 `SealAndEnd` 前强制验证 consumer applications 已全部清空。

本阶段结论为 **PASS**：LifecycleCommandHost `5/5`、LifecycleCommandRouter `4/4`、consumer runtime `4/4`、ProductHost `6/6`、legacy Attributes `4/4`、FormationInfluence `85/85`、`Shanmen.0_0_10` 全量 `334/334`；regression gate、映射 JSON、自检 `94/94`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 唯一 consumer runtime 所有权

- `FormationInfluenceLifecycleCommandHost::TryOpen` 同时冻结 ProductHost correlation/ledger identity，并创建唯一 consumer runtime；
- command host 对外提供显式 `TryActivateConsumer` 与 `TryDeactivateConsumer`；
- ProductHost、World、attribute component 和 frozen command 继续由 caller 每次提供；command host 不保存 engine-object pointer；
- 终止验证不能通过构造同 identity 的空 runtime 副本绕过，因为 `TrySubmit` 始终使用 command host 内部持有的同一份 runtime 状态。

### 2.2 teardown fail-closed fence

`SealAndEnd` 的 guarded 路径按以下顺序执行：

1. 校验 lifecycle coordinator 与 ProductHost/correlation/ledger；
2. 调用 exact consumer runtime 的 `CheckTeardownReady`；
3. active application 存在时返回 `ConsumerTeardownRequired`，不 seal、不结束 session、不生成 durable End receipt；
4. runtime drained 后才允许 `TrySealInfluence` 与 `TryEndAndTeardown`；
5. lifecycle result 保存 `bConsumerTeardownChecked` 与完整 nested runtime evidence。

### 2.3 forward recovery 不可降级

World teardown 首次失败时，Host 可能已经 seal 且 session terminal。P8.33 允许同一 terminal ProductHost 仅执行只读 `CheckTeardownReady`，以支持 exact command 的 forward recovery；Apply/Remove 仍拒绝 terminal product。

带 consumer fence 产生的 `EndRejected` durable receipt 只能携带 consumer runtime 继续恢复。改走旧的无 consumer Router API 会返回 `ConsumerTeardownRequired`，不会降级或改写既有收据。成功恢复后 exact replay 只读取 immutable receipt，不再进入产品终止逻辑。

## 3. 权威与兼容性

- ProductHost/Session 仍是 formation 产品生命周期权威；
- influence ledger/execution runtime 仍是 intent、lease 与 acknowledge 权威；
- consumer runtime/bridge/command host 仍只管理显式 native application lifecycle；
- `Udemo_mapAttributeComponent` 仍是 native modifier 权威；
- 新代码不发现 subsystem/component，不自动合成 Remove，不循环、不调度、不后台重试、不持久化、不拥有 ProductHost/World/Actor/component。

既有无 consumer 的 coordinator/router API 保留，供原有 lower-level composition 使用；产品 caller-facing `LifecycleCommandHost` 固定走 guarded 路径，因此旧测试与 API 兼容，同时新增产品终止栅栏。

## 4. 修改范围

更新生产代码：

- `demo_mapShanmenFormationInfluenceConsumerProductRuntime.h/.cpp`；
- `demo_mapShanmenFormationInfluenceLifecycleCoordinator.h/.cpp`；
- `demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h/.cpp`；
- `demo_mapShanmenFormationInfluenceLifecycleCommandHost.h/.cpp`。

更新验证与流程：

- `demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.33-FormationInfluenceLifecycleCommandHost-final.log` | `FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `0967ABBA5CA4694A8DEE59B9F359E1FDB3DE8EE88385C5A7CFA274B931B3DC3B` |
| `P8.33-FormationInfluenceLifecycleCommandRouter-final.log` | `FormationInfluenceLifecycleCommandRouter` | 4 | 0 | 0 | `6095E36C8BADFE3EA1DAD18BFC0498E2D58F01B889E3CE8867C471A03EE467C6` |
| `P8.33-FormationInfluenceConsumerProductRuntime-final.log` | `FormationInfluenceConsumerProductRuntime` | 4 | 0 | 0 | `873EB5F3EA42ADF0A4BD67FF0F292ECD76E1F05DE531D73D3C8A489C5A86BA40` |
| `P8.33-FormationProductHost-final.log` | `FormationProductHost` | 6 | 0 | 0 | `FBDD6761D95C4DB89BCF2154F67B63570781AA42319FE5C2C88025117D63F3AE` |
| `P8.33-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `F015E362C160E106F1977BE99E739B099CBCB2ACA0749A1371C3AD5956D7041E` |
| `P8.33-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 85 | 0 | 0 | `79DAA77FDF1DD2AA89014923E42912097DE5B17169668C852CFD56630F526D1D` |
| `P8.33-Shanmen-full-final.log` | `Shanmen.0_0_10` | 334 | 0 | 0 | `EE17BF3BDB1302A1031658D23B4768AC8B36B4E4F8B1107CE654643BA2C2C1A4` |

七份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖

新增真实产品链用例覆盖：

- ProductHost intent 通过 execution runtime 形成 active lease；
- lease 通过 consumer projector 生成 frozen Apply/Remove；
- Apply 在真实 `Udemo_mapAttributeComponent` 创建一个 native modifier；
- influence Remove 完成后，native modifier 仍活跃时 End 失败关闭且没有 durable receipt；
- 显式 consumer Remove 后 runtime drained；
- null World 造成 forward `EndRejected` 时保存 consumer readiness evidence；
- terminal ProductHost 使用同 command/runtime 成功恢复；
- 完成后 exact replay 只读；
- guarded End receipt 不能通过无 consumer API 恢复。

## 7. Changed-file regression gate

LifecycleCoordinator、LifecycleCommandRouter、LifecycleCommandHost、ConsumerProductRuntime 与 ProductHost 规则已增加 consumer chain、CombatCore、legacy Attributes 和 full-suite 反向依赖。

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=11 Rules=5 Required=36 Logs=7
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- `git diff --check`：PASS。

## 8. 静态边界

八个生产 header/cpp 扫描：`GetSubsystem=0`、`FindComponent=0`、`TActorIterator=0`、`Tick=0`、`while=0`、`TMap=0`、`AActor=0`、GAS symbols `0`、RNG `0`、SaveGame/ProfileRepository `0`；三个 lifecycle header 的 raw object-pointer member 为 `0`。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / 9 actions / 18.72s | 0 | `3C20F2C9326C7A7F9BD91F672C9194979B4A9884B0BD831640638D1E29BF4979` |
| Game final | Succeeded / 8 actions / 34.05s | 0 | `048633D7BF98E2416A162BB3B40F3E26E913A41C78CEEBD53D5D91FA49A3C999` |

- `UnrealEditor-demo_map.dll`：`12069376` bytes，SHA-256 `92E3FDAE921AF99A836F04BC668EB3726E32FBAFE33E9D9DB587B0F0BC9A0D66`；
- `demo_map.exe`：`353118208` bytes，SHA-256 `408539DBEB9E76B8D0D9890C6336CD1734B3B44C497A76A500A775906A093B25`。

生产代码与 Automation 首轮均通过。回归映射扩展后的第一次 self-test 原生退出码为 `1`，精确原因是 ProductHost/lifecycle 正例 fixture 只提供 full evidence，遗漏新规则明确要求的 `demo_map.V3.Attributes`；补入 attributes fixture 后最终 `94/94`。这不是产品源码、UE 构建或环境故障。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.34 把该 command host 接到现有 formation 产品上层 caller：由 caller 显式提交已解析的 subject/component 与 frozen projection command，并在产品结束前调用同一 command host；继续禁止 World/component 自动发现与隐式批量清理。
