# Dev.D.UE.0.0.10.P12.8.r0 Report

## 1. 结论

P12.8 已完成并通过 P 阶段门禁。

本阶段把 P12.7 的 SwordRhythm typed contribution seam 接入现有产品事实边界：成功启动的 SpiritEvasion 从其唯一 Component/Host 投影 immutable defense receipt；六类真实敌方攻击仅在最终执行成功且 WeaponGuard timing 为 `Perfect` 时登记 contribution；每个被 Session 接受的 BasicSword 都生成稳定、可验证的 evaluator input，并可选携带此前绑定的 contribution receipt。

最终结果：

- 沿用唯一 `Fdemo_mapShanmenSwordRhythmProductSession` 与既有 contribution ledger，没有第二套状态或时钟；
- SpiritEvasion 的 active projection 通过只读 Host/Component 出口进入现有 Session；
- Perfect WeaponGuard 在六类敌方攻击的最终 execution receipt 后进入现有 Session；
- 新增 immutable `FShanmenSwordRhythmEvaluationInput`，只携带 rhythm/binding evidence；
- evaluator input 自校验 deterministic ID、目标 observation、Run、Owner 与 timeline；
- 路由层仍不定义 strength、倍率、叠加、衰减、属性或 damage policy；
- focused `26/26`，0.0.10 全量 `562/562`；
- 10 份 Automation 日志原始合计 `704 Success / 0 Fail`；
- changed-file gate：`Changed=14 / Rules=6 / Required=48 / Logs=10`；
- regression gate self-test：`182/182`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建最终全部通过。

## 2. 功能性

### 2.1 SpiritEvasion live contribution

`Fdemo_mapShanmenSpiritEvasionProductHost` 新增只读 `TryProjectDefenseLayer()`，仅在 Host 仍处于 active 状态时，使用其 canonical Coordinator window 与 ActionRuntime 生成 projection。Component 只转发其实际 owned Host，不缓存、不复制、不另建权威。

`Ademo_mapGameMode::RouteSpiritEvasionStartIntent()` 在既有 ProductRoute 返回 accepted 后立即：

1. 从该 Component 获取 exact active projection；
2. 从既有 `CombatRunFixedTimeline` 获取 canonical sample；
3. 调用唯一 SwordRhythm ProductSession 登记 contribution。

补充 contribution handoff 失败只记录精确诊断，不回滚已经提交成功的 SpiritEvasion 产品动作。

### 2.2 Perfect WeaponGuard live contribution

新增 GameMode 私有 helper，只接受同时满足以下条件的最终敌方攻击结果：

- `AttackResult.IsExecuted()`；
- 本次确实检查了 WeaponGuard；
- defense composition 存在 guard layer；
- timing projection 有效且 band 精确为 `Perfect`。

helper 已接入 Basic melee、melee dash、ranged projectile、heavy sector、boss shape 与 boss volley 六条既有产品攻击路径。普通 Guard、失败 delivery、无效 projection 与非执行攻击不会产生 contribution。

### 2.3 immutable evaluator input

`FShanmenSwordRhythmEvaluationInput::TryCapture()` 冻结：

- 当前 accepted BasicSword 的 rhythm receipt；
- 可选的 next-action contribution binding receipt；
- 由两者 canonical identity 派生的稳定 `InputId`。

存在 binding 时，其 target observation 必须与当前 rhythm observation 一致，且 scope 的 Run、Owner、timeline 必须一致。没有 pending contribution 时仍生成合法 evaluator input，并以显式 `NO_CONTRIBUTION_BINDING` 身份参与 deterministic ID。

### 2.4 ProductSession 原子提交

新增完整 BasicSword overload 同时返回 rhythm receipt、optional binding 与 evaluator input。旧两个 overload 保留并委托新路径，现有调用方保持源码兼容。

Session 先在候选 Host/ledger 上完成 observation 与 binding，再构造 evaluator input、presentation state，最后以完整 `IsValid()` 一次性提交。`LastEvaluationInput` 与 `LastReceipt` 必须指向同一 rhythm receipt；teardown/reset 同步清空。

## 3. 完整性

测试扩展覆盖：

1. active ProductHost projection 有效且 exact replay ID 稳定；
2. Component 只暴露其 owned active Host 的同一 Activation；
3. 第一剑 evaluator input 携带两条 earlier defensive contributions；
4. 无 binding 的第二剑仍产生合法 input，贡献数为 0；
5. exact BasicSword replay 返回同一 evaluator `InputId`；
6. 将另一 target 的 binding 与当前 rhythm 组合时 fail closed 并清空输出；
7. 后续剑携带一条 precise-link contribution；
8. teardown 清空 evaluator input 与 contribution ledger；
9. GameMode 六个最终 WeaponGuard call site 静态计数为 6；
10. source-aware regression mapping 的正例与缺证据反例均通过。

## 4. 权威与兼容性边界

- SwordRhythm ProductSession 仍是唯一产品 contribution ledger 所有者；
- SpiritEvasion Component/Host 只增加 read-only projection，不改变动作、窗口、位移或资源生命周期；
- WeaponGuard 只消费最终 execution/defense receipt，不重新判定 timing 或 impact；
- evaluator input 属于证据交接，不是 evaluator policy 或属性修改器；
- 不修改 Impact、Vitality、inventory、schema、GAS、输入、动画或表现资产；
- 不把 contribution 直接应用到伤害，也不引入 multiplier、RNG、timer 或 World dependency；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `ShanmenSwordRhythmEvaluation.h/.cpp`：新增 immutable evaluator input 与 deterministic self-validation；
- `demo_mapShanmenSwordRhythmProductSession.h/.cpp`：新增完整 handoff overload、latest input 与完整性校验；
- `demo_mapShanmenSpiritEvasionProductHost.h/.cpp`：新增 active canonical projection 出口；
- `demo_mapShanmenSpiritEvasionComponent.h`：新增 owned Host 的只读转发；
- `demo_mapGameMode.h/.cpp`：接入 SpiritEvasion、六类 PerfectGuard 与 BasicSword evaluator handoff；
- 三份既有测试文件：扩展 projection、binding、replay、mismatch 与 teardown 覆盖；
- `ShanmenRegressionMap.json` 与 self-test：新增 source-aware evaluator input 规则及 fail-closed fixture；
- Report/Log 生成前 14 个代码/流程文件净变更 `+427 / -8`。

## 6. Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | `26B2C13E4257369997B01BAC4B033AE6625FE4BF6718CBFCA825E49065FF8DE0` |
| `Shanmen.0_0_10.Product.SpiritEvasionProductHost` | 7 | 0 | `838ABC46CD875623EB9849863A36FD00A2FA4882B2F70C09F2DBFB25A540DE3A` |
| `Shanmen.0_0_10.Product.SpiritEvasionComponent` | 7 | 0 | `ADE59EBF1CDD42DB6EC796B900B985CCD3B6B05F6307905D96D63F7ECEE4FEFD` |
| `Shanmen.0_0_10.Product.SpiritEvasionProductRoute` | 7 | 0 | `78E139ABD6CA1AA5AD95A84E34BD438CF126841E84740280601BFFD52AE85E43` |
| `Shanmen.0_0_10.Product.WeaponGuardImpactRoute` | 3 | 0 | `57703AD3534E4D4B40A4D9C8504E68273F6DD543C7A994CB48BF68B309D9F658` |
| `Shanmen.0_0_10` | 562 | 0 | `5977BAD42EEC2E6B4C9B5FEB829A60A1CC7C139E06231B5F63A5838AC3EE3BC4` |
| `demo_map.V3.Attributes` | 4 | 0 | `BE3F81FACD3BF85B9F06EFFA4D1F98CD9A050A8A4DFE6310BEF8264E8A71AA33` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `2F60845174F231BEBA203D142B2046EDEC47BA58085E1FB8DED128EBBC337050` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `71977924BDB8725FF33CC7FA2975A9076DB35BB6E8F09E56C9E11C8A41EE267F` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `B07DFF9219372AB981F8ECCDDD450158C1BDD095334920724657B95873D43F55` |

每份日志均有唯一 `RunTests` group、terminal success、进程原生退出码 `0`、Fail 0；最后一个 `Cmd: Automation RunTests` 之后的选定阶段 Fatal/Unhandled/Ensure/Automation Error 为 0。UE 5.8 启动期固定噪声位于选定阶段之前，未计入结果。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=14 Rules=6 Required=48 Logs=10
SELF_TEST: PASS 182/182
WEAPON_GUARD_LIVE_CALLS: PASS 6/6
BOUNDARY_SCAN: PASS no World/Actor/damage-application/RNG/timer code dependencies
EVALUATOR_POLICY_SCAN: PASS identity and evidence only
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial compile | Failed: test accessor compile error | 50 attempted / 176.07s | 1 |
| Editor corrected compile | Succeeded | 4 / 5.66s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.92s | 0 |
| Game final | Succeeded | 47 / 158.71s | 0 |

首次失败是 `demo_mapShanmenSpiritEvasionComponentTests.cpp` 误把 `FShanmenActionTransitionReceipt` 当成 action snapshot 并调用不存在的 `GetAction()`。测试改为从真实 Component-owned Host 的 `ActionRuntime.GetAction()` 读取后通过。该错误是测试源码编译失败，不是 Windows commit memory、页面文件或其它环境故障。

最终产物：

- `UnrealEditor-demo_map.dll`：13,069,312 bytes，SHA-256 `504094F66F80A0966F07F6B3A9C83465A464C64C2621B5AF99CC82D594AAC62C`；
- `demo_map.exe`：354,692,096 bytes，SHA-256 `1784B4886DE0ECCCB9BDF2DFDF62C70806E05A05D957A7520293145568431E94`。

最终构建没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

- 首次 Editor 编译暴露一处测试 accessor 错误，修正后 Editor、focused、全量、遗留回归与 Game 构建全部通过；
- 所有正式 Automation 首次执行成功，无失败 case 或重跑；
- raw log 的 SHA-256、选定阶段计数与 terminal marker 均二次解析核验；
- 只暂存并提交本阶段明确文件，不使用 `git add .`。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段产品路由、immutable evidence handoff、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.9 可在独立纯函数层定义 versioned evaluator policy 与 immutable evaluation receipt，把三类 contribution kind 映射到可审计的 authored effect；产品应用与 damage commit 继续保持独立，先冻结 policy/receipt 契约再接入数值。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route/Docs/Report/Dev.D.UE.0.0.10.P12.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route/Docs/Log/Dev.D.UE.0.0.10.P12.8.r0_log.md>
