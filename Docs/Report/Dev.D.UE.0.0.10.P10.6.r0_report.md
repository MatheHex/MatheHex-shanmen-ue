# Dev.D.UE.0.0.10.P10.6.r0 Report

## 1. 结论

P10.6 **PASS**。本阶段在 P10.5 唯一 Spirit Evasion ProductHost 之上建立了一个薄 `UActorComponent` 产品生命周期桥。组件只读取 owner Character 与 World time，把 start、Tick advance、cancel、interruption、owner end、Recovery finish 全部委托给 P10.5；没有复制 action phase、motion state、collision 或 displacement 权威。

组件默认停 Tick，只在 host Active 时启用；motion completion/block 进入 Recovery 后立即停 Tick，EndPlay 将未终止 host 路由为 owner end。输入/content/resource owner 必须提供已冻结 action、definition、policy、trajectory 与方向，组件不生成临时身份、不绑定按键、不维护 SpiritEnergy。

最终验证为 Component `7/7`、ProductHost `7/7`、ActionCoordinator `7/7`、MotionRuntime `7/7`、MovementAdapter `5/5`、SpiritEvasionMovement `5/5`、SpiritEvasion `11/11`、ActionLifecycle `1/1`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、0.0.10 全量 `423/423`。十一份正式日志合计 `539` 条 Success、`0` Fail；changed-file gate、mapping self-test、静态边界扫描、`git diff --check`、Editor Development 和 Game Development 均通过。

## 2. 功能性

### 2.1 薄 UE 生命周期桥

`Udemo_mapShanmenSpiritEvasionComponent` 是 `BlueprintSpawnableComponent`，但核心入口保持 typed C++：

- `TryStart` 从真实 owner 取得 `ACharacter`，从 World 取得绝对 game time，再用一次性 preflight port 启动 P10.5 host；
- `TickComponent` 只在 host Active 时调用 `TryAdvanceCharacter`；
- `TryCancel`、`TryInterrupt`、`TryOwnerEnd` 与 `TryFinishRecovery` 原样路由 host；
- `EndPlay` 对仍非终态的 host 执行 owner-end，再关闭 Tick；
- `GetHost` 与 `GetLastStep` 只提供审计观察，不建立新的可变权威。

组件字段只有一个 ProductHost 和最近一次 step proof；活动、恢复、终态和是否需要 Tick 都从 Host 派生。

### 2.2 Tick 与时间边界

构造时 `bCanEverTick=true`、`bStartWithTickEnabled=false`、TickGroup 为 `TG_PrePhysics`。每次状态操作后统一调用 `RefreshTickEnabled`：

- Host Active：启用 execution Tick；
- Host Recovery/terminal/not-ready：停 Tick。

Tick 只读取 `World->GetTimeSeconds()`，转换仍由 P10.5 host 完成。owner 不再是 Character、World 不可用或 World teardown 时，不尝试移动，直接路由 `TryOwnerEnd`。

### 2.3 启动与复用

活动或 Recovery 中的组件拒绝第二次 start，且不会执行第二次 preflight。终态组件可接收新的冻结 action，候选 host 只有完整启动成功后才替换旧终态 host；失败不会损坏已发布状态。

`TryStartAtForAutomation` 与 `TryAdvanceAtForAutomation` 仅在 `WITH_DEV_AUTOMATION_TESTS` 下存在，用 fake ports 验证委托契约，不进入 Shipping 产品接口。

## 3. 完整性与兼容性

- 复用 P10.5 ProductHost 作为唯一 action/motion owner；
- 复用 P10.2 production preflight adapter 与 P10.4 production execution port；
- 实际位移继续只经过 P10.3 executor 和 shared `MoveCharacterSwept`；
- 不新增 `bActive`、ActionPhase、MotionState、distance accumulator 或 collision result；
- 不修改 PlayerController、GameMode、SkillComponent 或旧 KnockbackComponent；
- 不绑定 Enhanced Input，不抢占现有按键或输入消费；
- 不修改 enemy skill、V2/V3 displacement、formation、weapon、item、profile、CodeB 或 schema；
- 不读取或写入 SpiritEnergy；
- 组件可编译挂载，但本阶段没有把它自动安装到玩家 Pawn，也没有真实产品输入入口。

## 4. 关键不变量

1. 组件只持有一个 P10.5 ProductHost；
2. 组件不得保存第二份 action 或 motion 状态；
3. 启动参数必须来自外部已冻结 authority；
4. 非 Character owner、无 World 或 teardown World 必须 fail closed；
5. Active/Recovery 中不得覆盖当前 host；
6. 候选启动失败不得改变已发布 host；
7. Tick 只能在 Host Active 时需要；
8. Tick 只能读取绝对 World time 并委托 P10.5；
9. Recovery 与 terminal 必须关闭 Tick；
10. EndPlay 必须路由 owner-end；
11. 组件不得直接调用任何位移 mutation API；
12. 组件不得绑定输入、生成 RNG 身份或维护资源余额。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionComponent` 七个测试：

- `OwnerFence`：默认停 Tick、ownerless production start fail closed；
- `StartAndBusyFence`：成功启动、活动期需要 Tick、第二次 start 不运行 preflight 且不覆盖 HostId；
- `CompletionLifecycle`：四段委托、Recovery 停 Tick、显式 finish 与 last-step observation；
- `BlockedRestart`：blocked Recovery、显式完成、终态组件以新身份重启；
- `CancellationSignals`：cancel/interruption 原样路由并终止；
- `OwnerTeardown`：Active 与 Recovery 两条 owner-end 路径；
- `FailureAndTimeFences`：preflight rejection、rewind 拒绝与 execution unavailable 收口。

0.0.10 全量由 P10.5 的 `416` 增至 `423`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionComponent.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionComponent.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionComponentTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产、测试与门禁共 `5` 个文件、`847` insertions、`0` deletions；加入本 Report 与同名 Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 和用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.6-SpiritEvasionComponent-final.log` | Product component | 7 | 0 | `4C64C6C7EC6B6D1654F4F54DA34880D07CDE97FFC1C16B46B80736C6054196D4` |
| `P10.6-SpiritEvasionProductHost-final.log` | P10.5 host | 7 | 0 | `7FB4CE76AC701D54B232D33E2176F5187E77049FCF4158C2FFCB172BCA4395FB` |
| `P10.6-SpiritEvasionActionCoordinator-final.log` | P10.4 coordinator | 7 | 0 | `18F5EB9347CAFB88BA58EB66FE2B71E37F6ADF971BAAA579EEC72A417FCA13BC` |
| `P10.6-SpiritEvasionMotionRuntime-final.log` | P10.3 motion runtime | 7 | 0 | `52901E0A4ACD50D433CB410758AE124BAF1548443A90BE98BDB754882F940DC1` |
| `P10.6-SpiritEvasionMovementAdapter-final.log` | P10.2 adapter | 5 | 0 | `B5297CCFF029DA3135DA099CCDAB00AE3F74A2C2CC008724B6B35E12935E95C5` |
| `P10.6-SpiritEvasionMovement-final.log` | P10.1 movement | 5 | 0 | `ADF397AE3C9BC747F7E80A824A5772FEAB7F579E97450F2D6D93BFF4FC0F7651` |
| `P10.6-SpiritEvasion-final.log` | Spirit Evasion window | 11 | 0 | `96211F75720A6123E92067BAD665F11A4C705D98B0C218CD1E6052ECEA4D68A5` |
| `P10.6-ActionLifecycle-final.log` | Action lifecycle | 1 | 0 | `71A5CE9A96AD8093A3F217C89AD87B5E0CF5447F15E06845B2DB1DE473BD23E7` |
| `P10.6-EnemySkillFramework-final.log` | Enemy skills | 44 | 0 | `679AE7AF2F2E30FCAE49788991703F929A3E823B165CC7F864B4790E6D953ADB` |
| `P10.6-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `6D16F030B9799E8C9AFC645510AEEB1CDD0087E67C5FC984872C147B547B8E10` |
| `P10.6-Shanmen-0_0_10-final.log` | 0.0.10 full | 423 | 0 | `1A043C4226912AAF9AFF63761D409AE218FCB263026064E64B913BCC1F9E207B` |

所有正式进程原生退出码均为 `0`，每份日志均有 selected queue-empty，并且没有 selected fail、fatal、unhandled 或 ensure。

```text
REGRESSION_MAP_JSON: PASS Rules=83
SELF_TEST: PASS 126/126
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
OWNED_HOST_FIELDS=1
DUPLICATE_STATE_FIELDS=0
WORLD_TIME_READS=4
TICK_ENABLE_CALLS=2
OWNER_END_ROUTES=3
```

mapping SHA-256：`E61A24D3E1AC008905FFE6948544783400202BC8ED6214A2699BBD8970CFC27E`；self-test SHA-256：`9BC304EBC5176EFCD46F4C06973EB9EACD9C08CFA2FF53097C6D24E618DE75F8`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 6 / 48.06s | 0 | `CECAE5E419D2D0D6EAC62F7F945B9DD5107EB194FAA1B6B562CE84CDFD506471` |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 | `01DD8F198BDAE4CC3C3C2BC5D3219C4A41D853F8B327AB6F591F31F362AD4B6B` |
| Game final | Succeeded | 5 / 33.21s | 0 | `5EF94BCF37F1667829ADAD1BCB2A79C387997E9CDA4F00E8684D906424DD3CCE` |

focused candidate：`7/7`、原生退出 `0`、SHA-256 `9A6C4A569DD2E709AD40AFA5713A7E30E26DBE1CD1309D968C9BC2E52264580A`。

`UnrealEditor-demo_map.dll`：`12345344` bytes / SHA-256 `0CA997F11DEB5B6F37040D25DD9FF921642D59BA879D1E63F6D20192A735D5A9`；`demo_map.exe`：`353809920` bytes / SHA-256 `94527874817C768E7FA2AE679ACE2D3AF6BBA86CAB33655DD37571572F7C46AF`。

## 9. 真实异常

没有源码编译失败、Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。候选编译、focused candidate、十一组正式 Automation、最终双目标构建均首次通过，没有以重试掩盖失败。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 使用显式时间和 fake ports 验证 component-to-host 委托；production Tick、真实 Character preflight 和 swept movement只完成编译，没有在测试 World 中运行。组件尚未自动安装到玩家 Pawn，也没有绑定输入。

下一阶段建议建立唯一 installation/command route：GameMode 只确保玩家 Pawn 上恰好一个本组件，PlayerController 或既有输入 owner 只提交 typed start/cancel command；冻结 action/content/policy/trajectory 必须来自现有 authority，不能在输入层制造。真实输入、碰撞连续画面与手感留在 F 阶段，SpiritEnergy 继续等待其唯一资源 authority。
