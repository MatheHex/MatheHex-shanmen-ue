# Dev.D.UE.0.0.10.P12.5.r0 Report

## 1. 结论

P12.5 已完成并通过 P 阶段门禁。

本阶段把策划基线中“准确连接动作、精准格挡、精准闪避可强化下一次行为”的前三类已具备真实 receipt 的事实，统一成窄、不可变、可重放的 SwordRhythm contribution evidence。该层只回答“发生了哪一种合格事实、由哪份 receipt 证明、属于哪个动作、发生在哪条单调时间线”；它不回答强化多少、如何叠加、何时消费，也不修改现有连段和伤害。

最终结果：

- 新增 3 类贡献来源：`PreciseSwordLink`、`PerfectWeaponGuard`、`SpiritEvasion`；
- 新增 private-field、`BlueprintReadOnly` 的 `FShanmenSwordRhythmContribution`；
- 非精准连剑、普通格挡、无效灵闪投影和非法时间样本全部 fail closed；
- focused：`3/3`；CombatRuntime：`114/114`；0.0.10 全量：`558/558`；
- 7 份 Automation 日志原始合计 `702 Success / 0 Fail`，全量唯一用例为 `558`；
- changed-file gate：`Changed=3 / Rules=2 / Required=6 / Logs=7`；
- regression gate self-test：`178/178`；
- `git diff --check`、JSON、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 统一贡献事实

`FShanmenSwordRhythmContribution` 冻结：

- deterministic `ContributionId`；
- contribution kind；
- 原始 combat action snapshot；
- source receipt identity；
- caller-owned monotonic timeline identity 与 observed tick。

所有反射字段均为 private `VisibleAnywhere, BlueprintReadOnly`。`IsValid()` 会重新派生 `ContributionId`，并验证 kind 与 canonical action definition 一致。ID 纳入 Run、Owner、Activation、SourceEntity、可选 SourceItem、ActionDefinition、content version/digest、排序后的 source tags、source receipt、timeline 与 tick，避免内容漂移后旧 identity 继续有效。

### 2.2 精准连剑来源

`TryCapturePreciseSwordLink()` 只接受有效且 band 精确为 `PreciseLinked` 的既有 `FShanmenSwordRhythmReceipt`。`Started`、`RestartedEarly` 和 `RestartedLate` 不会被误标为贡献。时间证据直接复用 current observation 的 timeline/tick，不创建第二时钟。

### 2.3 精准武器格挡来源

`TryCapturePerfectWeaponGuard()` 只接受有效且 timing band 精确为 `Perfect` 的 `FShanmenWeaponGuardTimingProjectionReceipt`。普通格挡即使拥有合法 receipt 也被拒绝。动作、时间线与 tick 全部来自 P11.1 已冻结证据。

### 2.4 成功灵闪来源

`TryCaptureSpiritEvasion()` 要求真实有效的 `FShanmenSpiritEvasionProjectionReceipt`，因此仅“开始移动”或“动作完成”不足以形成贡献；必须已有 active evasion window 成功投影出防御层。由于 P10 receipt 不自带固定 Run timeline，调用方还必须显式提供有效 timeline identity 与非负 tick。该层仍不拥有时钟。

## 3. 完整性

新增 3 条 focused tests：

1. `EligibleSources`：从真实 precise-link、perfect-guard 和 spirit-evasion projection receipt 生成三种贡献，核对 provenance、tick 与互异 identity；
2. `FailClosedEligibility`：拒绝 chain start、普通格挡、空 timeline 和负 tick，并验证复用输出被清空；
3. `DeterministicReplay`：相同冻结证据重放得到相同 ID，不同 observed tick 得到不同事实。

测试使用既有 action orchestrator、guard window/timing evaluator、evasion window/projection 和 sword rhythm chain 生成真实 receipt，没有构造绕过来源契约的伪 receipt。

## 4. 权威与兼容性边界

- 贡献层是证明适配器，不是 accumulator、buff、属性修改器或 damage resolver；
- 不改变 P12.0–P12.4 chain、Host、Session、presentation state/event；
- 不改变 P10 SpiritEvasion 或 P11 WeaponPerfectGuard 的窗口、时序和防御权威；
- 没有 World、Actor、component、Tick、Timer、wall clock、RNG、delegate 或异步任务；
- 没有 RawDamage、FinalDamage、BasePower、damage multiplier、ApplyDamage 或 TakeDamage；
- 生产 header/implementation 共 `274` 行，结构/API 边界扫描命中 `0`，damage 扫描命中 `0`；
- 没有 inventory、schema、GAS、输入、动画资产或存档修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

不含本 Report/Log：

- 新增 contribution header：82 行；
- 新增 contribution implementation：192 行；
- 新增 contribution tests：282 行；
- 回归映射新增 1 条 source-aware rule；
- self-test 新增 1 个正例和 1 个 fail-closed 负例。

## 6. Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution` | 3 | 0 | `62989B2F939A95E339886EF50E637316CB803806E66048841B949CF1CBA62C1E` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 9 | 0 | `D5302C9783FFE7056B6CD24FA0F9D8C226AF72D0DD6127B88B8C0EB1F99DF5CB` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | `7551691FE754FD041E6612E64E338AB1B813910C3D86AD0D3A45715B2A804498` |
| `Shanmen.0_0_10.CombatRuntime.SpiritEvasion` | 11 | 0 | `DD14589429B65B0E3563D4421384E13C509FDF7D768F4F955AAF79F38434678F` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `79A5AFCB85778E4744F863C858315E9B0889B7A3E6229132F5BD870B5FD7E302` |
| `Shanmen.0_0_10.CombatRuntime` | 114 | 0 | `6CD6487EA671866053573D37DD23374731A32206418A09B3D27FA2F76C3B9A8D` |
| `Shanmen.0_0_10` | 558 | 0 | `091BA49DF257892DF0AC0E71FABD1C6AB07FC78DB162A5FF63CA7C414004DDEE` |

每份最终 Automation 日志均有唯一 `RunTests` group、terminal queue-empty、原生退出码 `0`、Fail 0；选定测试阶段 Error/Fatal/Unhandled/Assertion/Ensure 为 0。UE 5.8 启动阶段固定的 13 行 `Condition failed` 均在本轮 `Cmd: Automation RunTests` 之前，未删除或伪装。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=6 Logs=7
SELF_TEST: PASS 178/178
REGRESSION_MAP_JSON: PASS
BOUNDARY_SCAN: PASS ForbiddenHits=0
DAMAGE_SCAN: PASS ProductionDamageHits=0
git diff --check: PASS (native exit 0)
```

Coverage 日志 SHA-256：`1648916B1D7000FA05576A4DBA1E44FFBCF02B8A8773E4AD25AA26C6C79BF4A6`。Self-test 日志 SHA-256：`235BE53E94C12BE5B68D92ECD71754F9EF1EF63C4DECC5BB77DEBC5C3EC53A55`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor final | Succeeded | 4 / 5.01s | 0 | `790CCE7627D9C0D8ABB721F598558E22179B6F2D8DC557556F48859434A94840` |
| Game final | Succeeded | 5 / 43.45s | 0 | `ADDCA9AB0A5710E31BB10073CA6CF72148B40DF89B3CB8D1219D1F41E242467A` |

最终产物：

- `UnrealEditor-ShanmenCombatRuntime.dll`：1,589,248 bytes，SHA-256 `93EED19056C78FD91CF31BF85130F0B3DBC55446CD5C4EBD4606DFBCA34912BD`；
- `demo_map.exe`：354,603,008 bytes，SHA-256 `D367AC670DC65D05A7043F442536B59C838B13F2BCFDAAF47DA3533F8B997C2E`。

构建无源码失败，也没有 C3859、C1076、系统代码 1455 或其它内存／页面文件环境错误。

## 9. 流程与异常

- 首轮代码与 focused tests 通过后，静态复审发现 ContributionId 尚未纳入 action content/item/tags；补齐后覆盖重建并覆盖重跑全部 7 份 Automation，最终证据不沿用旧二进制；
- changed-file gate 的一次调用误把 `P12.5-Editor-final.log` 纳入 Automation 日志集合，门禁按设计 fail closed；改为显式列出 7 份测试日志后通过。这是证据选择错误，不是产品或测试失败；
- 没有 Automation case 失败、产品源码失败或构建失败。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段纯证据契约、静态审查、无头 Automation、回归映射与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P12.6 可在不引入倍率的前提下建立 Run/Owner-scoped、幂等、一次性绑定“下一次 BasicSword activation”的 pending contribution ledger；消费与结算仍应分离。具体强化数值、上限、衰减、UI/动画表现和手感调优继续等待内容冻结及 F 阶段验证。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-5-sword-rhythm-contributions>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-5-sword-rhythm-contributions/Docs/Report/Dev.D.UE.0.0.10.P12.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-5-sword-rhythm-contributions/Docs/Log/Dev.D.UE.0.0.10.P12.5.r0_log.md>
