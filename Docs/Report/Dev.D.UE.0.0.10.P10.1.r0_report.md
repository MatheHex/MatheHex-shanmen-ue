# Dev.D.UE.0.0.10.P10.1.r0 Report

## 1. 结论

P10.1 **PASS**。本阶段在 P10.0 主动闪避窗口之上冻结了第一条方向移动交付契约：产品输入先 capture 为不可变、动作绑定的 planar movement intent；只有同一 P10.0 窗口仍由同一 `FShanmenActionOrchestrator` 判定为 `Active` 时，planner 才能发出 movement-policy request；重复桥接回调由 request ledger 以确定性 RequestId 拒绝。

最终验证为 SpiritEvasionMovement `5/5`、SpiritEvasion（含子组）`11/11`、ActionLifecycle `1/1`、CombatCore `9/9`、CombatRuntime `87/87`、`Shanmen.0_0_10` 全量 `390/390`。六份正式日志合计 `503` 条 success、`0` fail；changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段没有把移动策略偷换成产品数值：request 不包含距离、持续时间、速度、碰撞结果、腾空曲线、无敌帧或 SpiritEnergy。现有 `Fdemo_mapCombatDisplacement` 仍绑定旧 P6 配置中的 WorldStaticSkin，故本阶段没有直接耦合它；后续产品 adapter 应在真实策略与资源权威明确后单向消费 request。

## 2. 功能性

### 2.1 不可变方向意图

`FShanmenSpiritEvasionMovementIntent::TryCapture` 固定：

- canonical action 必须为 `Combat.Action.Spell.SpiritEvasion01`；
- MovementPolicyId 必须非空；
- 输入 X/Y 必须有限且非零；
- Z 只属于未来策略输出，不属于玩家 planar input，capture 时明确丢弃；
- X/Y 规范化为单位方向，并消除负零差异；
- action snapshot、policy 与规范方向成功后只读。

零平面方向、仅垂直方向、空 policy 或外来 action 均失败关闭。

### 2.2 Active-window 请求

P10.0 新增公共只读查询 `FShanmenSpiritEvasionWindow::IsActiveFor`，集中复用原有 exact runtime/action/commit 检查。原有 DefenseLayer projection 也改为调用同一查询，避免移动与防御产生两套窗口判定。

`FShanmenSpiritEvasionMovementPlanner::TryCreateRequest` 同时要求：

- intent 自身有效；
- window receipt 自身有效；
- window 仍对传入 runtime 处于 Active；
- intent action 与 window action 的 Run、Owner、Activation、Source、Item、Definition、Content 与 SourceTags 完整一致。

满足条件后生成只含 intent 与 window receipt 的不可变 request。Recovery、Interrupted、terminal、外来 runtime 或外来 window 均不能继续发出请求；已经发出的 request 仍保留为有效审计证据。

### 2.3 确定性身份与幂等

IntentId 由完整 action 身份、content stamp、规范排序的 source tags、MovementPolicyId 与规范方向的 IEEE double bits 派生。RequestId 由 IntentId 与 P10.0 WindowReceiptId 派生。

因此：

- 同一动作、policy 与同向比例输入可重放相同 IntentId/RequestId；
- policy、方向或 action/content 任一改变都会产生不同身份；
- `FShanmenSpiritEvasionMovementLedger` 首次接收有效 request，拒绝完全相同的重复交付；
- ledger reset 后可开始新的交付作用域，无隐式全局状态。

## 3. 完整性与兼容性

- 复用 P10.0 `FShanmenSpiritEvasionWindow`，不复制窗口状态机；
- 复用 P3 `FShanmenActionOrchestrator`，不增加 clock、timer 或 Tick；
- 只输出 movement-policy request，不访问 CharacterMovement、World 或 collision；
- 不调用旧 `LaunchCharacter`，也不把随机 DodgeChance 冒充主动闪避；
- 不直接调用仍耦合 P6 skin 参数的 `Fdemo_mapCombatDisplacement`；
- 不修改 SpiritShield、formation、controlled/thrown weapon、item 或 0.0.9B 权威；
- opaque policy key 允许后续地面步、腾空或其它产品策略扩展，而不修改纯 intent/request 结构；
- 没有 `demo_map`、UWorld、AActor、ApplyDamage、RNG、Timer 或 Tick callback 依赖。

## 4. 关键不变量

1. 只有 canonical SpiritEvasion action 可以 capture movement intent；
2. policy identity 必须显式且非空；
3. input contract 只接受有限、非零的平面方向；
4. capture 后方向固定为 XY 单位向量且 Z 为零；
5. intent 绑定完整冻结 action，而不只绑定 ActivationId；
6. P10.0 action window 是 movement request 的唯一生命期权威；
7. 只有 exact Active runtime 可以发出 request；
8. Recovery、Interrupted 与 terminal 状态拒绝新 request；
9. 已发 request 是不可变审计工件，不随 runtime 后续 phase 失效；
10. request 不携带距离、时长、碰撞或资源数值；
11. 等价规范输入生成等价 identity；
12. ledger 以 RequestId 保证重复产品回调幂等。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement` 五个测试：

- `IntentCapture`：规范方向、Z 丢弃与非法输入失败关闭；
- `ActiveWindowRequest`：exact active window 成功、外来 runtime/window 拒绝；
- `LifecycleBoundary`：Active 成功，Recovery/Interrupted 立即关闭；
- `DeterministicReplay`：比例方向重放及 policy/direction/action identity 分离；
- `IdempotencyLedger`：首次接收、重复拒绝、不同 action 接收与 reset。

同时执行 P10.0 SpiritEvasion、ActionLifecycle、CombatCore、CombatRuntime 宽回归和 0.0.10 全量回归。全量由 P10.0 的 `385` 增至 `390`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasionMovement.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasionMovement.cpp`；
- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasion.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasion.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritEvasionMovementTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现为 `7` 个文件、`747` insertions、`5` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `9` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.1-SpiritEvasionMovement-final.log` | `SpiritEvasionMovement` | 5 | 0 | `BB70229073298FBFF8217C14E6E10AC62C66C00CE4BA643B6CA52B146A1ED2D9` |
| `P10.1-SpiritEvasion-final.log` | `SpiritEvasion` | 11 | 0 | `B732D297425B5A259ADA0C6D099109628FE52CE48DD6AF2C497ACEC3DB74EE3A` |
| `P10.1-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `6C0F5E84951B60157340492CDDF425965DEB77CA4A280CAC222320220BC3F9E1` |
| `P10.1-CombatCore-final.log` | `CombatCore` | 9 | 0 | `AFA44465E60F7C68DC00DEE49871B27A57097C94A122B1BD389457E65EF11FA0` |
| `P10.1-CombatRuntime-final.log` | `CombatRuntime` | 87 | 0 | `8012A8BE4D934BB26F39833527312350856D629F7CEA05535CF2F419C3992F29` |
| `P10.1-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 390 | 0 | `A3C9F9A8AC88CD8C0B5433C74EA6A6C01909863DE6C54B86E9E311BD60510153` |

每份日志均有一个目标 RunTests 命令、一个结构化 queue-empty、selected fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=77
SELF_TEST: PASS 116/116
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=5 Logs=6
git diff --check: PASS
BOUNDARY_SCAN: PASS hits=0 (world/actor/damage/RNG/timer/tick-callback)
```

- mapping SHA-256：`EB3AE5705D07D035A349187B6D59AB97CD9E4A82B0B313985D55B578D6748CE4`；
- self-test SHA-256：`B5043A11E17658053C83479ABE549F5876EDCB109A288F43CC9B4E93D2453622`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 8 / 44.52s | 0 | `D3A64B2D0BEA888F16B234614DFDF4061B36796140C3332F18D886F7F6D00055` |
| Editor final | Succeeded, target up to date | 0 / 0.92s | 0 | `06E0A8922BA704EE654B600C190A4A21FF890524036D7027B48594FE959B278D` |
| Game final | Succeeded | 7 / 31.37s | 0 | `B946BC78A491417A13DE24374DF07B26152FAE02BD8C4C8009D3604338998246` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1299968` bytes，SHA-256 `054A9C499C19B0BCF11B7D729C26EA39DD84B07B293D88323AD6F83E85CEFDB9`；
- `demo_map.exe`：`353633280` bytes，SHA-256 `93F9CA6DBB28184F91B85E948A88FF7309F46C6AC1EFF01299354096CDAC8504`。

## 9. 真实异常

没有源码、UHT、Automation、changed-file gate 或构建失败。首次调用 changed-file gate 时，外层命令把 PowerShell 数组经新的 `pwsh -File` 进程展开成了位置参数，脚本在执行门禁前以 `A positional parameter cannot be found` 退出；改为在当前 PowerShell 进程直接调用同一脚本后，原参数、映射与六份日志全部通过。该事件是调用包装错误，不是测试失败，也没有被描述为源码或环境故障。

候选与正式 Automation 启动日志仍包含 UE 5.8 自带 UnifiedError 基线 `Condition failed` 诊断；所有选中测试随后逐项 Success，结构化 queue-empty 存在，selected fail/fatal/unhandled/ensure 均为 `0`，进程原生退出 `0`。

没有发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值契约、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P10.2 建议建立产品侧 movement-policy adapter：从权威配置解析距离/轨迹与 collision policy，并以单向方式消费 P10.1 request；在接入前先把 WorldStaticSkin 从旧 P6 enemy skill config 中拆为通用位移策略。SpiritEnergy 仍须等待真实 balance owner、revision、恢复与持久化契约，不能由移动 adapter 临时创建 float 账本。
