# Dev.D.UE.0.0.10.P12.1.r0 Report

## 1. 结论

P12.1 已完成并通过 P 阶段门禁。

本阶段把 WeaponGuard 私有命名的 30 Hz 时钟收敛为唯一的 Run-bound 产品时间线，并建立了从真实、正常闭合的 BasicSword 产品结果到 P12.0 纯节奏核心的可信桥接。`Fdemo_mapBasicSwordProductExecutionResult` 现在携带协调器实际接受的 immutable action snapshot；节奏宿主不再依赖调用方用 GUID 复述动作身份。

最终结果：

- 通用 Run timeline：`5/5`；
- SwordRhythm product Host：`2/2`；
- CombatRunCoordinator：`17/17`；
- 0.0.10 全量：`549/549`；
- 11 份最终 Automation 日志原始合计 `700 Success / 0 Fail`，按 test identity 去重为 `665`；
- changed-file gate：`Changed=16 / Rules=4 / Required=43 / Logs=11`；
- regression gate self-test：`170/170`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

本阶段没有把 chain count 接入伤害、攻速或其它数值，也没有在缺少正式内容时序来源时把宿主强行安装到在线 GameMode。

## 2. 功能性

### 2.1 唯一 Run 固定时间线

原 `Fdemo_mapShanmenWeaponGuardFixedTimeline` 已替换为 `Fdemo_mapShanmenCombatRunFixedTimeline`：

- 固定速率仍为 30 Hz；
- `TimelineId` 从 RunId 与固定速率确定性派生；
- 每个 tick sample 额外拥有由 TimelineId 与 tick 派生的 deterministic `SampleId`；
- begin、advance、capture、end 均校验结构状态，错误输入原子拒绝；
- 负数、NaN、Infinity、溢出与错误 Run teardown 均不能改变时钟。

GameMode 只推进这一份时钟。既有 WeaponGuard 输入契约通过显式 adapter 从通用 sample 获得同一个 TimelineId 与 tick，因此没有保留第二套 guard clock。

### 2.2 BasicSword 执行事实不再只靠 GUID

`Fdemo_mapBasicSwordProductExecutionResult` 新增完整 `FShanmenCombatActionSnapshot Action`。协调器只有在 action runtime 成功进入 Active 后才写入该快照并消耗 activation sequence。

`IsExecuted()` 现在同时要求：

- `Error == None`，即完整 emission 与 runtime closure 均成功；
- Action snapshot 自身有效；
- ActivationId 有效且等于 Action 中的 ActivationId。

因此合法 miss 仍是可记录的真实动作；未启动、身份被篡改或后续闭合失败的结果不能进入节奏链。

### 2.3 Run-local SwordRhythm 产品宿主

新增 `Fdemo_mapShanmenSwordRhythmProductHost`，由调用方注入 RunId 与冻结的内容定义。它只接受：

- `IsExecuted()` 为真的 BasicSword 产品结果；
- Action.RunId 与宿主 Run 一致；
- 来自同一 Run canonical fixed timeline 的 immutable sample；
- P12.0 pure chain 可接受的 observation。

宿主在副本链上先计算，再整体提交，确保失败不改变现有锚点或层数；exact replay 返回同一 receipt。它不拥有输入、动画、World、Actor、Timer、伤害写入或内容默认值。

集成测试通过真实 `Fdemo_mapCombatRunCoordinator` 连续执行两个合法 miss：首个动作在 tick 0 形成 `Started / 1`，第二个动作在调用方定义的 open tick 形成 `PreciseLinked / 2`。测试没有伪造 action snapshot，也没有用伤害命中作为“动作成功”的替代条件。

## 3. 完整性

新增或强化七个 focused 契约：

1. Run timeline identity、幂等 begin 与精确 teardown；
2. 不同 frame partition 到达相同 30 Hz tick；
3. sub-tick carry；
4. 非法 delta 原子拒绝；
5. immutable sample identity 与 tick 演进；
6. 真实 Coordinator BasicSword → timeline sample → rhythm receipt；
7. 未执行结果、跨 Run action、跨 Run sample、身份篡改与错误 teardown 原子拒绝。

Coordinator 既有 BasicSword 测试同步断言 Action 的 Run、player entity、weapon instance、action definition 与 ActivationId；前置失败必须保留 invalid Action。

## 4. 兼容性与权威边界

- GameMode 仍是 DeltaSeconds 的唯一产品推进点，但对外暴露的是 Run timeline，不再把时钟所有权归给 WeaponGuard；
- WeaponGuard session、enemy attack context 与 physical input 继续复用同一 tick，完整 0.0.10 回归通过；
- timeline deterministic namespace 从 guard 专名迁移为 Run 通用名。该 identity 仅在一次 Run 内瞬态使用，没有写入存档或 schema；
- BasicSword 仍由既有 CombatRunCoordinator、ActionOrchestrator 与 canonical vitality route 执行，没有第二套 sword action 或伤害路径；
- Host 当前是可组合产品契约，尚未安装进 GameMode。原因是项目还没有提交正式 SwordRhythm timing config／animation marker source；本阶段拒绝把测试窗口烧成产品平衡值；
- 没有修改 Impact、damage resolver、vitality、inventory、schema、资源事务、GAS ability 或动画；
- 四个新增生产文件未引用 `AActor`、`UWorld`、`GetWorld`、Timer、frame counter、wall clock、RNG、`ApplyDamage` 或 `TakeDamage`；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

不含本 Report/Log，共 16 个路径，`1,076` additions / `450` deletions：

- 新增通用 timeline 的 header、implementation 与 5 条测试；
- 删除旧 WeaponGuard 专名 timeline 的三个对应文件；
- 新增 SwordRhythm product Host 的 header、implementation 与 2 条集成测试；
- 修改 CombatRunCoordinator 的 result、写入点与 BasicSword 测试；
- 修改 GameMode 的 timeline 所有权、guard sample adapter、日志与 teardown；
- 更新 changed-file regression map 与正／负 self-test。

## 6. 测试覆盖

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatRunFixedTimeline` | 5 | 0 | 1 | `1FFD783DDBBDEE1BAEC60150486613198FBFEA21265152596F8716ABF6C4D024` |
| `Shanmen.0_0_10.Product.SwordRhythmProductHost` | 2 | 0 | 1 | `6904CBB6DE9CDF1C385DD5F9D65DC5FED556BDCE1D56121E0FC4EFF1F24BD83D` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 1 | `581361F36F46BD0E96091493EB302C8E4CBE311652C256025A45E38FA4C26C6B` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 6 | 0 | 1 | `01C2D1C26740A690B6BB78B3DA294FBEC0904551AE736F4A0FD3E680CB24A590` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | 1 | `87DB81D114040391C8C6E6076C43538C76A70EE4E79C7DC0A9B96C70CDE267C9` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `2AB7C527F4BDFCCA518E99AEC6F68E9124D33843BEFA1FF00E262F3621B10D76` |
| `Shanmen.0_0_10` | 549 | 0 | 1 | `3750905BB1C5F66E0B754E3F000474FE5BFB85152C77A38F2198B00E211BC060` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `A799FA13C539AA4E91D48DE5168F287EF028384F9127554E1C99AE3CF01B58EB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `6B155383D5FD50CEB79A89AE4816E5852D9884CFB0992BA43F6C581C7BFB2382` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `4F8726E5721364DA822E86A376652A249499368B03970E7ABDCFA7F064DAE8AC` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `42EAF304FEA6572E0295BE4FC08714C594076AC55AC78754E1298BEBB0E0DD8E` |

每份最终日志均只有一个 canonical `Automation RunTests <group>` 命令、一个原生 queue-empty / TEST COMPLETE 终止事实、Fail 0，且无 Fatal、Unhandled Exception、Assertion 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=16 Rules=4 Required=43 Logs=11
SELF_TEST: PASS 170/170
BOUNDARY_SCAN: PASS ProductionLines=529 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

最终 changed-file gate 日志 SHA-256：`D9F70AFFB279BFAE34EFCD2F622AA1F5ED8532BC9A3BA348FD5C483F8E3EE66D`。Self-test 日志 SHA-256：`E05A99B08A35898B39CECB6D114B9330E3ECCEC70F0960A714EA96C89818491B`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor final | Succeeded | 4 / 10.01s | 0 | `DE53D723EC1DD4BAC2F9B2D80612E0932C24156979A1E285D9FC5426ED284FE0` |
| Game final | Succeeded | 3 / 11.23s | 0 | `3138B16E09444D464BE9E160840794463910A747860516912BC4771B3BE2568D` |

首次修复后的完整 Editor 编译执行 `56` actions / `153.60s` 并成功；完整 Game 编译执行 `71` actions / `215.98s` 并成功。最终 4/3 action 构建用于覆盖最后一处诊断文本调整。

最终产物：

- `UnrealEditor-demo_map.dll`：12,958,720 bytes，SHA-256 `F5EF045E4C6257A8CA770EE76907270EE922281F0AEBB9C375265020C1BCB388`；
- `demo_map.exe`：354,507,776 bytes，SHA-256 `DC4190EDC8D74FB84CC080D98CC933B3C35885743C903E49AB618294D7073027`。

Win64 SDK `10.0.22621.0` 为 VALID；没有 C3859、C1076、系统代码 1455 或其它内存／页面文件错误。

## 9. 真实异常与修复

首次 Editor 构建暴露了一处真实源码依赖错误：旧 timeline header 曾经传递包含 `demo_mapShanmenWeaponGuardInputAdapter.h`；替换为通用 header 后，GameMode 声明无法识别 guard sample type，出现 `C3646 / C2059 / C2238`。确认根因后中止该轮编译，launcher 原生退出 `1`；随后在 GameMode 明确包含实际依赖。首次失败日志 SHA-256：`652AC687F298B906B9A8013417B030F6AF8A009EFBBBF60220D2594117045F68`。修复后 Editor 与 Game 均成功。

门禁工具首次误由 Windows PowerShell 5 执行，因现有 validator 使用 PowerShell 7 pipeline continuation 而发生 parser error；改用项目标准 `pwsh 7.6.4` 后 self-test `170/170`。另一次跨进程传递 hashtable 的命令封装失败，改为同一 `pwsh` 进程直接 splat 后 gate 通过。两者属于证据命令调用错误，不是产品测试失败，也没有放宽门禁。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、确定性产品契约、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.2 应先建立正式、可版本化的 SwordRhythm 产品配置来源，再把 Host 安装到 GameMode Run 生命周期，并在 `ExecuteM01PlayerBasicSwordSweep()` 返回 `IsExecuted()` 后捕获同一 Run timeline sample。动画 marker 与真实输入验证仍留给后续 P/F 阶段；在设计明确前继续禁止把 chain count 转成伤害倍率。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-1-run-timeline-rhythm-host>
