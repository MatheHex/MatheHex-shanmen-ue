# Dev.D.UE.0.0.10.P8.26.r0 Report

## 1. 结论

P8.26 已在 P8.25 deterministic consumer projection／command 之上增加 run-scoped 纯值 application registry。formation influence 的 Apply／Remove 现在拥有可自校验的 active snapshot、success-only command history 与 immutable receipt；重复成功命令稳定重放，暂态 slot collision、missing application 与 stale handle 不污染命令历史，因此后续条件变化后仍可合法重试。

本阶段结论为 **PASS**：registry 专项 `4/4`、consumer projection `3/3`、既有 attributes `3/3`、完整 influence 链 `63/63`、`Shanmen.0_0_10` 全量 `312/312`、changed-file regression gate、自检 `82/82`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 显式 registry scope 与 application identity

- registry 只能通过 `TryCreate(RunId, Content)` 建立；registry ID 由 run/content canonical payload 确定性派生；
- 每个 active application 封存 registry ID、Apply command ID 与完整 P8.25 projection；
- application ID 由 registry、Apply command、modifier handle、lease 与 target attribute 派生；
- command 不属于当前 run/content 时 fail closed，不会进入历史或活动状态。

### 2.2 success-only 幂等回放

- 只有成功的 Apply／Remove 进入 `CompletedCommands`；
- 相同成功 command ID 重放时返回原 receipt，并设置 `bCommandReplayed`，不再次修改状态；
- `ApplicationMissing`、`SlotOccupied`、`StaleHandle` 等暂态拒绝不被缓存；
- 因此先 Remove missing、稍后 Apply 后再次 Remove，或先 slot collision、释放后再次 Apply，均可成功。

### 2.3 slot、handle 与旧 Remove 安全性

- 活动 slot 定义为 `LeaseId + TargetAttributeId`；同一 slot 同时只允许一个 handle；
- 不同 lease 可向同一 attribute 叠加，未建立全局 attribute 单例；
- 未完成的旧 handle Remove 遇到同 slot 新 handle 时返回 `StaleHandle`；
- 已完成的旧 Remove 只重放自己的 immutable receipt，不会触碰后来重新 Apply 的 handle。

### 2.4 可重建一致性与 teardown drain

- `IsConsistent()` 从成功命令历史重放全部 Apply／Remove，逐条复核 status、receipt、application ID 与历史 active count；
- 重建结果必须与当前 active snapshots 一一对应；重复 command/application、双占 slot 或孤立 active state 均使 registry 失效；
- `IsDrained()` 只有在 registry 一致且无 active application 时为真，可作为后续 teardown 门禁。

## 3. 完整性与安全边界

registry 只保存纯值证据，不拥有 Actor、component、attribute math、队列、异步任务、持久化或 Host authority。它不调用 `AddModifier`／`RemoveModifier`，不把 exact rational magnitude 转成 float，也不改变 P8.24 lease 或 ledger 的权威。

新生产文件静态扫描：`UWorld/AActor/UObject = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`Timer/Async/RNG = 0`、`float/double = 0`、`Spawn/ApplyDamage/SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`AddModifier/RemoveModifier/Fdemo_mapModifierSpec = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRegistry.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRegistry.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

文档加入前共 `1050` 行新增：registry header `176`、implementation `604`、tests `240`、mapping `14`、self-test `16`。既有产品实现未被机械改写。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.26-FormationInfluenceConsumerRegistry-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | observed | `2AAA4F27DD3CEB4F5B31620E1B7670DD257E8C9AA813D7B61141E7EF0447C2C8` |
| `P8.26-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | observed | `5E7E091DDA4545736160B96123A7B238A896C64BA02B5B900C914071769CF43E` |
| `P8.26-Attributes-final.log` | `demo_map.V3.Attributes` | 3 | 0 | 0 | observed | `F9E72261931CB3FB94AC3B2A4E855C6B79BDFB49EBC200F730B9B4394C1231C5` |
| `P8.26-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 63 | 0 | 0 | observed | `0A0C4A5F5C380545AE150953F5B14DFF2F7EB67D4E0AAC21731430D5FB621130` |
| `P8.26-Shanmen-full-final.log` | `Shanmen.0_0_10` | 312 | 0 | 0 | observed | `21B2AF002E818ABE91E0348B4DE9D6357A03A1F1C016730326A72846F1A3FD3B` |

五份日志 fatal／unhandled／ensure 均为 `0`。

新增四项专项 case：

1. `ApplyRemoveReplay`：Apply／Remove 各自只转移一次状态，重复命令返回同一 receipt；
2. `SlotCollisionRetry`：slot collision 与 stale handle 不入历史，精确释放后第二 projection 可重试；
3. `StaleRemoveAfterReapply`：已完成旧 Remove 重放不会删除新 handle；
4. `ScopeMissingAndDrain`：跨 scope、missing、invalid command fail closed，成功生命周期控制 drain。

## 6. 首轮与审查结果

首次 Editor 编译即成功（`5 actions / 28.94s / exit 0`）；registry 首轮 `4/4`，projection `3/3`，attributes `3/3`，influence 由 `59` 增至 `63/63`，全量由 `308` 增至 `312/312`。没有源码编译失败或 Automation case 失败。

提交前审查补强了 `SlotCollisionRetry`：除 slot collision 外，显式执行未完成的 stale Remove，并确认它不写历史。随后重新编译并重跑全部最终 Automation。一次 regression gate 的外层 `pwsh -File` 数组传参方式返回退出码 `1`；这是命令包装错误，改为在当前 PowerShell 进程直接调用同一脚本后通过，不属于源码、构建或测试失败。

## 7. Changed-file regression gate

新增 `FormationInfluenceConsumerRegistry` path rule，并把 registry 组加入既有 projection test 文件的重叠规则。修改 registry 或共享 projection tests 时，必须提供 registry、projection、evaluation binding、modifier evaluator、lease executor、legacy attributes 与 CombatCore 七组证据。

```text
REGRESSION_MAP_JSON: PASS Rules=61
SELF_TEST: PASS 82/82
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=7 Logs=5
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=7 Logs=5
```

- mapping SHA-256：`2093F23B708644CA7B3B60C107774AA1362D61B964174130C95F473D39C728ED`；
- self-test SHA-256：`9BC49FB1B22B90EE98481AF089EF6DC3AE26E825C2C293D8FBA064666CF9500F`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 5 actions / 28.94s | 0 | `AFFB3A20DB56FE6DDABB2021E77C9165FC1D80D726FFC2053BE884DBC3A50BC7` |
| Editor final | Succeeded / 4 actions / 5.98s | 0 | `6121109584F3B93C6FB00958D1229F57F0D6C5CB4E6A5AF5A7E3372F1569DBCC` |
| Game final | Succeeded / 4 actions / 24.19s | 0 | `4F8D00DAB45AA71D79FC1AD20586BDDFE6C19420F8E72CB94B3300A7484305F2` |

- `UnrealEditor-demo_map.dll`：`11870208` bytes，SHA-256 `4B40677B3C16941F073ACB90D2057E54F813CC64A4D78E3681E8AF0551037A72`；
- `demo_map.exe`：`352957440` bytes，SHA-256 `CA308580EC5C1AE23D060E13422C999D775CB8697F4F503ADD4D0FB8FC8F6316`。

## 9. 兼容性与工作区保护

- P8.23 evaluator／receipt、P8.24 binding／lease、P8.25 projection／command wire shape 均未修改；
- registry 使用既有 deterministic `Fdemo_mapModifierHandle`，但不调用旧 attribute component；
- 失败命令不写 durable history，避免 deterministic ID 将后续合法重试永久“毒化”；
- 所有 query 先清空输出，失败时不泄露旧 snapshot／result；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；本阶段只 exact-stage 上述七个文件。

`git diff --check` 与最终 `git diff --cached --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.27 增加窄 consumer application port／adapter：只把 registry 已接受的 Apply／Remove 翻译到既有 attribute modifier seam，并以 exact handle 回传 acknowledgement；不得建立第二套 attribute authority，fixed-point 到旧 float 边界的转换与失败补偿必须集中在 adapter。
