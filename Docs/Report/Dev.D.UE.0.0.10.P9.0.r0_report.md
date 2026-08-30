# Dev.D.UE.0.0.10.P9.0.r0 Report

## 1. 结论

P9.0 已建立 0.0.10 术法体系的首个正式契约：短时主动灵力护盾。实现位于 `ShanmenCombatRuntime`，以不可变 Definition、Action commit 后激活、显式解除、单调容量 revision、防御层投影与确定性 receipt 组成纯值 Runtime；不引入 Actor、World、Timer、Tick、RNG、输入、能量权威或产品 UI。

本阶段结论为 **PASS**：SpiritShield `5/5`、CombatCore `9/9`、CombatRuntime `39/39`、`Shanmen.0_0_10` 全量 `342/342`，四份正式日志合计 `395` 条 success、`0` fail。映射 JSON、self-test `102/102`、changed-file regression gate、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 阶段选择与 P8 审计结论

P8.41 后继续审计 formation 产品调用链，结果是：生产代码中没有 `Fdemo_mapShanmenFormationProductHost::TryStart` 或 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen` 的真实调用者，现有使用仍全部在 Automation fixture。P8.41 已把 consumer Run activate/deactivate 暴露给正确的 LifecycleCommandHost owner；继续强接只会制造第二层 wrapper 或 GameMode glue。

因此本轮没有伪造 P8.42 接线，而是冻结“无安全生产调用点”结论，转入人工确认的 0.0.10 五体系路线中尚未启动的术法体系。首个目标采用规划已明确的“短时主动灵力护盾”。

## 3. 功能性

### 3.1 不可变内容契约

`FShanmenSpiritShieldDefinitionCapture` 是唯一可写 authoring 输入；`FShanmenSpiritShieldDefinition` 捕获后字段私有且 Blueprint read-only。契约固定 canonical Action ID、Rule ID、最大容量及 Damage/Source/Target required/blocked tags，并拒绝零容量、错误 Action ID 与逻辑不可满足的 tag 过滤器。

### 3.2 commit 后显式激活

`FShanmenSpiritShieldRuntime` 只允许匹配的 `FShanmenActionOrchestrator` 在 `Startup -> Active` commit 后激活。Startup、foreign content 与 foreign action 均失败关闭；相同激活重放返回原 receipt，不创建第二个 shield instance。

### 3.3 revisioned 容量投影

产品容量权威以 `AuthorityRevision + AvailableCapacity` 显式投影到 `FShanmenDefenseLayer`：

- 相同 revision + 相同容量返回原 projection；
- 相同 revision + 不同容量拒绝；
- revision 回退拒绝；
- 容量超过 authored maximum 拒绝；
- 同一 activation 中容量回升拒绝；
- 后续更高 revision 可投影更低剩余容量。

投影固定使用 `AbsorbPoints`、`FShanmenDefenseOrder::Shield`、`Shanmen.Defense.Shield` 与 exact shield source identity。`bRequiresCommitOnTrigger=true`，因此 CombatCore 的 mitigation receipt 不能被误写成已经完成容量扣除。

### 3.4 显式解除

Runtime 不读取墙钟，也不持有 Timer。外部 owner 以 `Explicit / DurationElapsed / Interrupted / OwnerEnded` 原因显式解除；foreign shield ID、终止原因改写与解除后再次投影均拒绝。相同解除重放返回原 receipt，历史 activation replay 不会复活护盾。

## 4. 确定性与 CombatCore 兼容

shield instance、activation receipt、stable layer、revisioned projection 与 deactivation receipt 均由 canonical parts 派生。身份输入包含 Run/Owner/Activation/Source、ContentStamp、Rule、最大容量和排序后的 tag filters；浮点值使用 canonical bit representation，避免文本 locale 或 `-0` 差异。

专项 resolver fixture 将 `30` 点护盾层应用到 `100` 点物理伤害：`PreventedDamage=30`、`FinalDamage=70`、守恒成立，triggered layer 保留 exact `SourceInstanceId` 且明确要求 commit。等价冻结输入复现全部 identity；不同 content 产生不同 shield instance。

## 5. 完整性与边界

- 复用既有 `FShanmenActionOrchestrator`、`FShanmenDefenseLayer`、`FShanmenDefenseResolver` 与 `FShanmenDeterministicId`；
- 没有新增第二套伤害、Action、资源、Timer 或产品 authority；
- Runtime 只保留最近 projection receipt，不使用 history map、全局 registry 或后台任务；
- 不声称已完成真实容量扣除：产品 adapter 必须依据 triggered layer 的 source identity 与 authority revision 提交容量；
- 不声称已完成“短时”调度：duration owner、输入/GAS 编排、能量消耗、产品接线和表现均未进入 P9.0；
- P8 formation 的产品接线未被本阶段修改。

## 6. 修改范围

生产与验证代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShield.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShield.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

文档：本 Report 与同名 Development Log。代码与脚本净变更为 `5` 个文件、`1182` insertions；exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `SpiritShield-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShield` | 5 | 0 | `A99751B563C8306B12D936965B7CAF57F1FDD4396C1574A1B9CD41E140D66764` |
| `CombatCore-final.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | `7FB1F177EF1791829DAB69A49EC2FF5D1AE741136E55A20F4D341FECCE6742D3` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 39 | 0 | `8C1B3C4B398955C7FFB1A1E7709A0BA28764B9AC7F817F1C6CE9E16A290CCCE2` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 342 | 0 | `98DE39ED48BA937D63DA9728834A40A1312D84DDDA0AD6E9F4F7DA4A9CBD98CC` |

四份日志均有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=70
SELF_TEST: PASS 102/102
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=3 Logs=4
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=2 Required=3 Logs=4
Required: CombatRuntime.SpiritShield, CombatRuntime, CombatCore
EXACT_STAGE: PASS Files=7
```

- mapping SHA-256：`115C9B554100A72F1EFDF26EE8917E8957F7F5344E80EBD1F21B32F979E4AC5C`；
- self-test SHA-256：`E2F10B6D09AB83646674A989E84C2BC75F503A9B065FA0420008181F726116DE`；
- 新文件扫描：`demo_map/UWorld/AActor/ApplyDamage/UGameplayStatics/FindComponent/GetSubsystem/Tick/FTimer/while/TMap/RNG` 均为 `0`；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 8 / 34.68s | 0 | `8A340943425BDBEA2591A774FF1840B0FC9422C2F934D44A0A51F3C56FF92DD6` |
| Editor final | Succeeded | 6 / 11.15s | 0 | `72E1595E42E3598A2DFD39792D407CA16832C42D6FDB93EE94920A67F4242881` |
| Game final | Succeeded | 5 / 29.44s | 0 | `C50077546B883DC5262665F59969E67A81FA7C8A640E60336C30C50860F379C5` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`741888` bytes，SHA-256 `DAD0382AC84D00FD077EC2ECBBAFCCC26CC4E196FE07AE6D80E28C1A1C6DCE54`；
- `demo_map.exe`：`353230336` bytes，SHA-256 `E23502176BAF6C2E8DE0A6B0ACD0E73238DAC4E46F2E98F94BCC7ADCB03DCCD5`。

## 9. 真实异常

没有源码、UHT、Automation、gate 或构建失败。首次专项 invocation 的 `5/5` 用例和进程退出码均为成功，但显式排队的 `Quit` 抢在 `TestExit` 写出 queue-empty 前退出；该日志（SHA-256 `449B5ACBA58597D3928FF5287462383BC06AFDD99955F976CDA5311B2598E5AB`）被主动判为不合格证据。删除 `Quit`、仅由 `TestExit` 收口后生成上表正式日志，产品代码无需修复。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P9.1 建立 exact shield-capacity commit authority：消费 CombatCore triggered layer receipt，以 shield instance + expected revision 原子扣除容量并返回可重放 commit receipt；仍不在 Runtime 内引入 Timer 或 World 对象。P9.2 再由产品 owner 组合能量 Reserve/Commit、duration 调度与显式解除。
