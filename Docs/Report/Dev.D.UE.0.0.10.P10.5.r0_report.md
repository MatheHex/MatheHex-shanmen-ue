# Dev.D.UE.0.0.10.P10.5.r0 Report

## 1. 结论

P10.5 **PASS**。本阶段建立了 Spirit Evasion 的唯一产品 host，把 P10.0 action lifecycle、P10.0 window、P10.1 movement request、P10.2 World preflight、P10.3 motion runtime 与 P10.4 action coordinator 串成一条可审计链路。

Host 独占一个 `FShanmenActionOrchestrator` 和一个 P10.4 coordinator；调用方只提供冻结内容、候选方向、绝对时间样本，以及一次性 preflight/execution port。Host 不复制 motion/session 状态，不保存 Actor、World、Tick、Timer、输入或资源余额，也未引入临时 SpiritEnergy。

最终验证为 ProductHost `7/7`、ActionCoordinator `7/7`、MotionRuntime `7/7`、MovementAdapter `5/5`、SpiritEvasionMovement `5/5`、SpiritEvasion `11/11`、ActionLifecycle `1/1`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、0.0.10 全量 `416/416`。十份正式日志合计 `525` 条 Success、`0` Fail；changed-file gate、mapping self-test、静态边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 唯一产品生命周期 owner

`Fdemo_mapShanmenSpiritEvasionProductHost::TryStart` 事务性组装以下已有权威：

1. 启动 exact action snapshot；
2. 提交 Startup → Active；
3. 打开 exact Spirit Evasion window；
4. 捕获候选方向并生成 movement request；
5. 由 P10.2 adapter 构造 movement plan 并执行只读 World preflight；
6. 由 P10.3 构造 motion plan；
7. 由 P10.4 打开 exact action/motion coordinator。

任何一步失败都不会发布半成品 host。HostId 由 CoordinatorId、ActivationId 与绝对起始时间的 IEEE-754 bits 确定性派生；`IsValid` 会重新派生身份，并核对 coordinator window 与 owned action snapshot。

### 2.2 绝对时间与原子收口

`TryAdvance` 接受产品层绝对时间，拒绝非有限值、早于 start 的样本与时间倒退，然后只把 `NowSeconds - StartTimeSeconds` 交给 P10.4 coordinator。每次调用最多执行一个到期 segment。

正常 motion completion 或 collision block 会把 owned action 从 Active 推进到 Recovery，但不会擅自跳过恢复阶段；只有 `TryFinishRecovery` 才完成 Recovery → Idle/Completed。

显式 cancel、interruption、owner end 与 execution failure 都在候选副本上先关闭 active motion，再把 action 置为 Interrupted，最后原子提交 host。Recovery 中的 cancel/interruption/owner end 只关闭 action；终态重复信号幂等返回 `AlreadyTerminal`。

### 2.3 窄产品端口

`Idemo_mapShanmenSpiritEvasionPreflightPort` 是启动期唯一只读碰撞能力。生产 `Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort` 只在调用期暂存一个 `const ACharacter*`，并委托 P10.2 `PreflightWorldStatic`。

运行期继续复用 P10.4 typed execution port。`TryAdvanceCharacter` 只构造短生命周期 Character execution adapter，再委托 typed `TryAdvance`；实际位移仍由 P10.3 executor 转交 shared `MoveCharacterSwept` 权威。

## 3. 完整性与兼容性

- 复用 P10.0–P10.4 已有 action、window、plan、preflight、session、command、receipt 与 termination 权威；
- Host 字段只包含 HostId、start time、ActionRuntime、Coordinator 与 started bit；
- 不新增第二套 action phase、motion state、collision 或 displacement 系统；
- 不修改旧 enemy skill、knockback、V2/V3 displacement 行为；
- 不修改 formation、weapon、item、profile、CodeB 或 0.0.9B schema；
- 不读取或写入 SpiritEnergy；资源权威仍等待单一 balance/revision/recovery/persistence owner；
- production Character convenience API 已编译，但没有在 P 阶段启动真实 World 或移动真实 Character。

## 4. 关键不变量

1. 一个 host 只拥有一个 exact action runtime 与一个 exact coordinator；
2. HostId 必须能由 canonical inputs 重新派生；
3. host-owned action 必须与 coordinator window 冻结 action 完全一致；
4. start time 与每个 absolute sample 必须有限且不倒退；
5. 一次 advance 最多提交一个 motion segment；
6. normal completion/block 必须先进入 Recovery；
7. Recovery 只有显式 finish 才能完成；
8. cancel/interruption/owner end/execution failure 必须同步关闭 action 与 active motion；
9. terminal host 不得再次移动或改变 termination proof；
10. host 不保存 Actor、World、component、clock、Timer、Tick、input 或 resource balance；
11. production preflight 只能委托 P10.2 WorldStatic adapter；
12. production movement 只能委托 P10.4/P10.3/shared swept authority。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionProductHost` 七个测试：

- `StartAndReplay`：完整启动链、deterministic HostId、preflight rejection 与空 Character preflight；
- `CompletionLifecycle`：绝对时间等待、四段提交、Recovery 与显式完成；
- `BlockedRecovery`：blocked receipt 进入 Recovery，再显式完成；
- `CancelAndInterrupt`：active cancel/interruption、termination reason 与终态幂等；
- `OwnerEnd`：Active 和 Recovery 两条 owner teardown 路径；
- `ExecutionFailure`：fake execution unavailable 与空 production Character；
- `TimeFences`：start 前、NaN 与 rewind 样本均被拒绝且状态不突变。

0.0.10 全量由 P10.4 的 `409` 增至 `416`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionProductHost.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductHostTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产、测试与门禁共 `5` 个文件、`1517` insertions、`0` deletions；加入本 Report 与同名 Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.5-SpiritEvasionProductHost-final.log` | Product host | 7 | 0 | `4C1FE60CC9D470878CCC0AE9F860D0329F8C031662DD83FB28EA4AE6E2731724` |
| `P10.5-SpiritEvasionActionCoordinator-final.log` | P10.4 coordinator | 7 | 0 | `A33C03323F84DFAC108D594AD93C916E7826C2EE52F6EC2EA77BA41D43FEE9F3` |
| `P10.5-SpiritEvasionMotionRuntime-final.log` | P10.3 motion runtime | 7 | 0 | `4DD34A180D0696341F2CD00FCD1E3D6A92D0699D483B5B8003FA17932FF613EB` |
| `P10.5-SpiritEvasionMovementAdapter-final.log` | P10.2 adapter | 5 | 0 | `EA737127AA30DD9FC4C98DA2092506B720DC2E1959A2F611576FA888D1ACD90B` |
| `P10.5-SpiritEvasionMovement-final.log` | P10.1 movement | 5 | 0 | `CC448B591D0B6E3F9BB3B0D006498C100CE0AD165ADA549A591B1E9B15F6E205` |
| `P10.5-SpiritEvasion-final.log` | Spirit Evasion window | 11 | 0 | `0304016BC4DA9FAB01DE24B9E08EC3CD0FD5ED087A7EBCB625BBBB2F4DEEF2D8` |
| `P10.5-ActionLifecycle-final.log` | Action lifecycle | 1 | 0 | `A6BF5B86073612BA56E57E2720081D8A7F9C2D80805BE082F1B1B2D7C323C160` |
| `P10.5-EnemySkillFramework-final.log` | Enemy skills | 44 | 0 | `A9CA679DF34E005578D48107385B95C29DCDAFCB7032B01371963CF10A504588` |
| `P10.5-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `A37461D63B78CADB0481C3E818729874CD45414DF09ED096914097F070DAE574` |
| `P10.5-Shanmen-0_0_10-final.log` | 0.0.10 full | 416 | 0 | `EA53E6B7801BA837EA4998FB0AE677ECA54240EBF01B9E5C7A7AE21B171AB8BB` |

所有正式进程原生退出码均为 `0`，每份日志均有 selected queue-empty，并且没有 selected fail、fatal、unhandled 或 ensure。

```text
REGRESSION_MAP_JSON: PASS Rules=82
SELF_TEST: PASS 124/124
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
git diff --check: PASS
DIRECT_MUTATION_HITS=0
WORLD_CLOCK_TIMER_TICK_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
ACharacter pointer occurrences=6（仅接口/短生命周期 adapter/convenience signatures；host fields=0）
PRODUCTION_DELEGATIONS=2（preflight + typed advance）
```

mapping SHA-256：`B306DC9BDC8FFFD05A425F7F907359326BC1D7A63A3A96EF58DBA7F30DCD9B08`；self-test SHA-256：`D0162B313C0F70D3FFF892875D192DC718C926BCEF9B030F62E818F3B222C3A9`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor first candidate | Failed：test API compatibility | 5 / 21.73s | 6 | `659BAFDE665A5B19A37C4145AFB2CD0773410463214C55020E57FF12710B3543` |
| Editor corrected candidate | Succeeded | 4 / 7.28s | 0 | `AACD62CC8DF101AF61A56F73E41A678BEAF65C8DB445217D8F31E390C984C169` |
| Editor convenience candidate | Succeeded | 5 / 8.58s | 0 | `5858AFB6661242700C5A6BB6FC75730358778BC7CC5BBF1B699C9AEF1F46AED7` |
| Editor final | Succeeded, up to date | 0 / 0.90s | 0 | `BB62E1C21ED46DC0A161CE97AF7E293916EF87B4659BE86E7CB39F2E1A90BE08` |
| Game final | Succeeded | 4 / 23.46s | 0 | `90EB6C4C2B749DC7917EAABF8D21B5B81D691DAEA26BFB75CFC67318591FB335` |

`UnrealEditor-demo_map.dll`：`12303360` bytes / SHA-256 `397C9A6EEA2BFFBFD5EB192ECA8B409FB72C0EBBFD43F9B944E99580B7CE6145`；`demo_map.exe`：`353776640` bytes / SHA-256 `1E24D0E65260D330A52054792BC4B3541C2CFD93AEEA7ACC94711D75E2533ACC`。

## 9. 真实异常

首次 Editor candidate 的原生退出码为 `6`，UBT 结果为 `OtherCompilationError`。失败仅位于新测试：UE 5.8 的 `TNumericLimits<double>` 没有 `QuietNaN()`，产生 C2039/C3861；生产 host 没有编译错误。

修正为标准库 `std::numeric_limits<double>::quiet_NaN()` 后，Editor candidate 原生退出 `0`。随后新增 production Character convenience API 后又进行一次必要 candidate 编译并退出 `0`。没有 C3859、C1076、系统代码 1455、外层超时、Automation failure 或门禁失败；首次失败未被描述为环境/内存问题。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 通过 fake preflight/execution ports 验证完整 domain lifecycle；production Character adapters 已编译，但没有在测试 World 中执行真实 collision preflight 或 swept movement。

下一阶段建议建立薄 `UActorComponent`/输入桥：仅从真实 owner 提供 action snapshot、policy/content、absolute time、Character 和 cancel/end/destroy 信号，再委托本 host。该桥不得复制 action/motion 状态；真实碰撞连续画面、手感与输入验收留在 F 阶段，SpiritEnergy 仍等待其唯一资源 authority。
