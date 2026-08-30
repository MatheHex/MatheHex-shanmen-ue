# Dev.D.UE.0.0.10.P10.4.r0 Report

## 1. 结论

P10.4 **PASS**。本阶段在 P10.3 的确定性 motion session 与唯一 swept mutation seam 之上，加入了 action lifecycle coordinator，并补全 cancel/end/interruption/owner end/execution unavailable 的显式关闭语义。

Coordinator 绑定一个 exact Spirit Evasion window、action snapshot 与 motion session；每一步先验证当前 action runtime，再发放一个到期 segment command、调用窄 execution port、接纳 receipt。它不持有 Actor、World、clock、Timer、Tick、输入或资源余额。生产 execution port 仅在调用期暂存 Character，并继续委托 P10.3 executor；实际位移仍只有 shared `Fdemo_mapCombatDisplacement::MoveCharacterSwept` 一条权威路径。

最终验证为 coordinator `7/7`、motion runtime `7/7`、P10.2 adapter `5/5`、P10.1 movement `5/5`、Spirit Evasion `11/11`、Action Lifecycle `1/1`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、0.0.10 全量 `409/409`。九份正式日志合计 `511` 条 Success、`0` Fail；changed-file gate、mapping self-test、静态边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 可审计 motion termination

P10.3 motion state 增加 `Terminated`，并定义以下显式原因：

- `ExplicitCancel`；
- `ActionEnded`；
- `ActionInterrupted`；
- `OwnerEnded`；
- `ExecutionUnavailable`。

`TryTerminate` 只接受仍为 Active 的 session。它记录 SessionId、待放弃 CommandId、已接纳段数、已解析距离和最后 elapsed，生成可重放的 termination receipt，然后清空 pending command 并原子进入终态。终态 session 不能重复关闭、接纳旧 receipt 或继续发段。

### 2.2 exact action/window coordinator

`TryOpen` 要求 motion plan、Ready preflight、active Spirit Evasion window 和 action runtime 全部匹配。CoordinatorId 由 SessionId 与 WindowReceiptId 确定性派生；`IsValid` 会重新检查该身份及完整 action snapshot。

`TryAdvance` 的顺序为：

1. 验证 coordinator 仍可运行；
2. 比对 exact action identity；
3. 在动作已离开 Active 时先生成 `ActionEnded` 或 `ActionInterrupted` termination；
4. 验证 elapsed 有限且单调不减；
5. 发放最多一个到期 command；
6. 调用窄 execution port；
7. 接纳 exact receipt，并返回 Waiting、SegmentCommitted、Completed 或 Blocked。

错误 action、elapsed rewind 与无效输入不会改变 coordinator。Character 不可用会产生 `OwnerEnded`；movement authority 或 malformed execution 不可用会产生 `ExecutionUnavailable`；两者都先关闭并清除已发放 command，不留下悬空 pending 状态。

### 2.3 产品执行边界

`Idemo_mapShanmenSpiritEvasionSegmentExecutionPort` 是 coordinator 唯一依赖的可变执行能力。Automation 使用 typed fake port 验证 domain lifecycle；生产 `Fdemo_mapShanmenSpiritEvasionCharacterExecutionPort` 只把一次调用委托给 `Fdemo_mapShanmenSpiritEvasionSegmentExecutor::ExecuteSwept`。

Coordinator 类没有 Actor 指针。生产 port 的一个 `ACharacter*` 字段只在调用者构造的短生命周期适配器中存在，不进入 session、command、receipt 或 coordinator 的可重放状态。

## 3. 完整性与兼容性

- 复用 P10.0 action lifecycle/window、P10.1 direction、P10.2 policy/preflight 和 P10.3 scheduler/receipt；
- 不复制 action、window、trajectory、collision 或 displacement 权威；
- 不修改旧 enemy skill、knockback、V2/V3 位移行为；
- action 进入 Recovery、Cancelled、Interrupted 或 owner/executor 消失时，不再留下可继续执行的 active session；
- termination receipt 与 coordinator identity 均由 canonical inputs 重放；
- 没有 World、Timer、Tick、RNG、SpiritEnergy、无敌帧、恢复或输入映射；
- 不修改 formation、weapon、item、profile、CodeB 或 0.0.9B schema。

本阶段尚未把 coordinator 放入真实产品 component/host，也没有把真实输入、owner EndPlay 和逐帧 elapsed 接到该 owner；这些属于 P10.5 产品接线。SpiritEnergy 继续等待唯一 balance/revision/recovery/persistence authority，不在 coordinator 中建立临时余额。

## 4. 关键不变量

1. 一个 coordinator 只能绑定一个 exact action window 与 motion session；
2. CoordinatorId、CommandId、ReceiptId 与 TerminationReceiptId 必须可重新派生；
3. 当前 runtime 的 action snapshot 必须与窗口冻结 action 完全一致；
4. window 失活时必须先关闭 session，不得再移动；
5. elapsed 必须有限且单调不减；
6. 一步最多执行一个 segment command；
7. execution receipt 必须与本步 command exact match；
8. owner/execution failure 必须清除 pending command并留下 termination proof；
9. terminal coordinator 不得再次执行或关闭；
10. coordinator 不保存 Actor、World、clock、input 或 resource state；
11. production movement 只能委托 P10.3 executor；
12. P10.3 executor 只能复用 shared swept displacement authority。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator` 七个测试：

- `BindingAndReplay`：exact window/action/preflight 绑定与确定性 CoordinatorId；
- `WaitAndCommit`：边界前等待、边界时执行一次、相同 sample 不提前发下一段；
- `Completion`：四段完整提交后 Completed，不能额外执行；
- `Blocked`：blocked receipt 记录实际距离并终止；
- `ActionLifecycle`：Active→Recovery 与 interruption 均在移动前关闭；
- `ExplicitAndOwnerEnd`：显式 cancel、owner 消失、movement authority 消失及 pending 清理；
- `InputFences`：foreign action、负 elapsed 和 rewind 均失败关闭且不突变。

P10.3 motion runtime 另增 `TerminationReceipt`，覆盖 pending command 审计、重复关闭拒绝、终态拒绝旧 receipt/新 command 和 deterministic replay。0.0.10 全量由 P10.3 的 `401` 增至 `409`。

## 6. 修改范围

新增：

- `demo_mapShanmenSpiritEvasionActionCoordinator.h/.cpp`；
- `demo_mapShanmenSpiritEvasionActionCoordinatorTests.cpp`。

更新：

- `demo_mapShanmenSpiritEvasionMotionRuntime.h/.cpp`；
- `demo_mapShanmenSpiritEvasionMotionRuntimeTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产、测试与门禁共 `8` 个文件、`1314` insertions、`1` deletion；加入本 Report 与同名 Log 后 exact stage 为 `10` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.4-SpiritEvasionActionCoordinator-final.log` | Product coordinator | 7 | 0 | `42C4DEC2868666FA63B676A954F874F7B2BAC081FB34A6D9F115B15A9A999DDB` |
| `P10.4-SpiritEvasionMotionRuntime-final.log` | Product motion runtime | 7 | 0 | `90BB52802DEDB1A42C378F16985075CAEADBC90825DB578B514248A81F32D486` |
| `P10.4-SpiritEvasionMovementAdapter-final.log` | P10.2 adapter | 5 | 0 | `F5D37A7355742974CF1539083D918F344387637F22F87EC2E24605B52D11C417` |
| `P10.4-SpiritEvasionMovement-final.log` | P10.1 movement | 5 | 0 | `4E875871F83B684674322F598C29D34BF2DD962D1AA16BFC74EFCE3F4B82856F` |
| `P10.4-SpiritEvasion-final.log` | Spirit Evasion window | 11 | 0 | `19E9F9E5CD206351489906F3FD21B464C5B4A52B981421B959C7CAD281E471B0` |
| `P10.4-ActionLifecycle-final.log` | Action lifecycle | 1 | 0 | `C32CE8E38658C362D8B06E0FEAAA2CA81D1810BB26770920CD56EBC72D032FA3` |
| `P10.4-EnemySkillFramework-final.log` | Enemy skills | 44 | 0 | `4BDCEBAE68F0BE713D6B69B8834586DA3DE1AFD3E7439F70201725B4599DA7FC` |
| `P10.4-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `65AFA31F573B40E810BD64B3CC51FF6F17F04E49ED8A0B971BCFD42152B80280` |
| `P10.4-Shanmen-0_0_10-final.log` | 0.0.10 full | 409 | 0 | `27CC4D2878E384CF788FFB7EF957E1977FFDCB6A52697F718C4C21EAFAF4596D` |

所有正式进程原生退出码均为 `0`，九份日志均有 selected queue-empty，并且没有 selected fail、fatal、unhandled 或 ensure。

```text
REGRESSION_MAP_JSON: PASS Rules=81
SELF_TEST: PASS 122/122
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=9 Logs=9
git diff --check: PASS
COORD_EXECUTOR_DELEGATIONS=1
COORD_DIRECT_MUTATION_HITS=0
RUNTIME_SHARED_MOVE_AUTHORITY_CALLS=1
WORLD_CLOCK_TIMER_TICK_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
```

mapping SHA-256：`2A4AB41A98D4F8818C33F21BAB8571BA84810F4C8BB46DF045B6840A3CC6AC48`；self-test SHA-256：`D2AEDB5CE6BF6AE088D411EBEAC2E085FF781115017534E866FC3E5F61E37D01`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 7 / 32.69s | 0 | `879F6982C9525395569BA011C1F1588453D87E5CEB9EC34092AE446F66AEE48A` |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 | `B2304E1EAAE805B5E22BC06F9E505F2BA8CAF9B04F83E448021B7B79725D58D1` |
| Game final | Succeeded | 6 / 28.88s | 0 | `2A5895FC7A9B9B085768A8B4176C3791102029E154C00409CFDD391DC4C8F1F2` |

`UnrealEditor-demo_map.dll`：`12259840` bytes / SHA-256 `0F944FB4E3553944F80343690836E57F3E9BA5651DFF74C56A0BD886058965A6`；`demo_map.exe`：`353736192` bytes / SHA-256 `AD6E1690EB751DD153FCA5970A24BFCBE478378ECA232F7CE36777F1881D878F`。

## 9. 真实异常

没有源码编译失败、Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。候选编译、focused candidate、九组正式 Automation、最终双目标构建均首次通过。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 通过 typed fake execution port 验证 coordinator domain lifecycle，生产 Character port 已编译，但没有在测试 World 中执行真实 Character 碰撞位移。真实输入、逐帧 elapsed、EndPlay、action transition 接线、碰撞连续画面和手感属于 P10.5/F 阶段。

P10.5 建议建立唯一产品 host/component：把当前 action runtime、elapsed、Character execution port 和 cancel/end/destroy 信号路由到本 coordinator；host 只编排现有权威，不复制 motion/session 状态。SpiritEnergy 仍应在其真实资源 authority 落地后再接入。
