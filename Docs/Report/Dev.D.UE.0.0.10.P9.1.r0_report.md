# Dev.D.UE.0.0.10.P9.1.r0 Report

## 1. 结论

P9.1 **PASS**。P9.0 的 SpiritShield projection 不再只是声明 `bRequiresCommitOnTrigger`：本阶段新增单一写入的纯值 capacity authority，将当前 revision 的投影、CombatCore canonical resolution、容量扣减和 Impact 幂等账本闭成一条可验证链路。

最终验证为 Capacity `6/6`、SpiritShield `11/11`、CombatCore `9/9`、CombatRuntime `45/45`、`Shanmen.0_0_10` 全量 `348/348`；五份正式日志合计 `419` 条 success、`0` fail。changed-file gate、静态边界、`git diff --cached --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 单一容量权威

`FShanmenSpiritShieldCapacityAuthority` 由一份有效 activation receipt 初始化，唯一持有：

- 当前 `AvailableCapacity`；
- 单调递增的 `AuthorityRevision`；
- 以 `ImpactId` 为键的已提交 receipt ledger。

调用方不再自行填写 revision/capacity。`TryProjectDefenseLayer()` 只允许匹配且仍 Active 的 `FShanmenSpiritShieldRuntime`，由 authority 把自己的当前状态投影成 P9.0 defense layer。容量归零后不再产生投影。

### 2.2 Canonical commit command

`FShanmenSpiritShieldCapacityCommitCommand::TryCreate()` 同时接收 projection、`FShanmenImpactRequest` 与 `FShanmenImpactResult`，并在生成不可变 command 前完成：

- request/result 合法性与伤害守恒检查；
- 重新执行 `FShanmenDefenseResolver::Resolve()`，逐字段比对 canonical result；
- request 中 shield layer 的 source 唯一性和 projection 完整一致性检查；
- triggered layer 的 Layer/Rule/Source/Operation/Order/Tags/commit 标志检查；
- 受保护 Target 与 shield activation source 的身份绑定；
- 确定性 `ResolutionId` 与 `CommandId` 派生。

因此，手工拼接、foreign layer、重复 source、守恒但非 canonical 的篡改 result 均不能进入写 authority。

### 2.3 原子扣减、revision 与重放

`Commit()` 只接受当前 projection 的 exact revision 与 exact capacity fingerprint。唯一成功路径原子生成 receipt、扣减容量、revision `+1` 并登记 Impact。

- stale projection：拒绝且不变更状态；
- foreign shield：拒绝且不变更状态；
- 相同 `ImpactId` + 相同 `CommandId`：返回原 receipt，状态不重复推进；
- 相同 `ImpactId` + 不同 proof：`ImpactConflict`；
- 精确重放即使发生在后续合法 commit 之后，仍返回第一次 receipt，不双扣容量；
- receipt 强制 `CapacityAfter < CapacityBefore`，并以 float bit fingerprint 验证精确算术。

### 2.4 耗尽不窃取生命周期权威

容量耗尽只使 capacity authority 进入 depleted 状态并停止投影，不会暗中结束 shield runtime。持续时间、输入、能量和显式 deactivation 仍由产品 owner 管理；测试证明耗尽后 runtime 保持 Active，随后仍可通过 P9.0 的显式 deactivation receipt 收口。

## 3. 完整性与兼容性

- P9.0 Definition、activation、projection、deactivation API 未改签名；
- CombatCore resolver 与 defense schema 未修改；
- 未接入 `demo_map`、World/Actor、Timer/Tick、输入、能量或 UI；
- 未建立第二套生命、物品或 shield capacity 权威；
- 新 authority 是普通 C++ 纯值对象，所有跨层输出均为 read-only USTRUCT receipt；
- `ShanmenCombatRuntime.Build.cs` 明确 `bUseUnity = false`，修复 clean unity 聚合会把多个 `.cpp` 的匿名命名空间 helper 合并并产生同名冲突的既有构建缺陷，不改变运行时产品行为。

## 4. 关键不变量

1. authority activation 必须与 shield runtime activation receipt 完全一致；
2. projection revision/capacity 只能来自 authority 当前状态；
3. commit proof 必须来自同一 projection 的 canonical CombatCore resolution；
4. 每个成功的新 Impact 只推进一次 revision；
5. `CapacityBefore = CommittedCapacity + CapacityAfter`，且成功 commit 必须真实降低容量；
6. exact replay 永不二次扣减，conflicting replay 永不覆盖旧 receipt；
7. depleted 只关闭 defense projection，不隐式拥有 lifecycle。

## 5. 测试覆盖

新增 6 个 focused tests：

- `AuthoritativeProjection`：初始化、当前 revision 投影、exact replay、foreign runtime 拒绝；
- `CanonicalCommit`：12 点触发层原子扣减、receipt 与 revision；
- `FailClosedProofs`：stale、foreign、守恒但篡改 result 全部失败关闭；
- `ReplayAfterProgress`：两次合法 commit 后重放第一次 proof，不双扣；同 Impact 异 proof 冲突；
- `DepletionBoundary`：100 入伤被 30 容量封顶，耗尽停止投影但保留显式 lifecycle；
- `DeterministicReplay`：等价输入复现 projection、resolution、command 与 receipt IDs。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldCapacityAuthority.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldCapacityAuthority.cpp`；
- `Source/ShanmenCombatRuntime/ShanmenCombatRuntime.Build.cs`。

测试与流程：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldCapacityAuthorityTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

代码与脚本净变更为 `6` 个文件、`1234` insertions；加入本 Report 与同名 Development Log 后 exact stage 为 `8` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `SpiritShieldCapacity-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity` | 6 | 0 | `3F737B9BAAA49EF15775376A29B775C1A5168D6F226879DD79B06469785A2449` |
| `SpiritShield-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShield` | 11 | 0 | `3D1AEF32CBDBE0D2F07569C90BB2E758AA5FF029E5CD0EFAB5FA6CA19C00443C` |
| `CombatCore-final.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | `C539B2CC2B48F8B458E213EEEBF3D8C369025FBCCF51D0D03E68C02CDCC1F6AB` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 45 | 0 | `77A7F4862F7C3962344997CAC69CF59CC1382F9005E2838A9B36AB5F24506DCB` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 348 | 0 | `F1733B6D80E5F199B6BC18B8587510E7070DFB38C371F59ED79767D5247899B9` |

每份正式日志均有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=71
SELF_TEST: PASS 104/104
REGRESSION_COVERAGE (implementation): PASS Changed=6 Rules=2 Required=4 Logs=5
REGRESSION_COVERAGE (exact staged code/scripts): PASS Changed=6 Rules=2 Required=4 Logs=5
Required: CombatRuntime.SpiritShieldCapacity, CombatRuntime.SpiritShield, CombatRuntime, CombatCore
git diff --cached --check: PASS
```

- mapping SHA-256：`04F907EB3AB21EC2EABF8522D90C202ED8E56D83EF5C56D224E4604D11F044AA`；
- self-test SHA-256：`958599D9E792CAE12189C6E0D14ABA253AA921FEED701D13C6612BCC645AEA35`；
- 新生产文件扫描：`demo_map/UWorld/AActor/ApplyDamage/UGameplayStatics/GetWorld/GetSubsystem/Timer/Tick/while/RNG` 均为 `0`。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Failed: pre-existing unity helper collisions | 6 / 39.54s | 6 | `54104442FF0FEE69FDADBA984E3ACB93B740F0776DB1BA62A18CE9D544433EA8` |
| Editor recovery | Succeeded | 4 / 10.57s | 0 | `2716F8E5B53A94DC4E5F31B3C6620C105F85DC3914D54D4BCFBFF9B3FEC61060` |
| Editor final | Succeeded | 4 / 4.44s | 0 | `C4939472CBECFAD9035E0170E4A3B1B3DCC5639AF87BA5D35086D3AE7D396E2B` |
| Game final | Succeeded | 5 / 30.29s | 0 | `CEDFE7BFDC519FE66FECA578F5E1DA9CA44BEB674D0A94D214D99096545CF5DE` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`818176` bytes，SHA-256 `7817ACD85C1D130FF85B7E6693E64F456A44E30134D152712B39C9F1FF7B7DAA`；
- `demo_map.exe`：`353284608` bytes，SHA-256 `887B1125B8C27583C536CDC7D4D39228EC6B8D4F95F519C541933E4BE71A9B72`。

## 9. 真实异常

首次 Editor build 原生退出码为 `6 / OtherCompilationError`。新 P9.1 的 authority 与 test 两个独立编译单元均已成功编译；失败单元是重新生成的 `Module.ShanmenCombatRuntime.cpp`，它把既有 `.cpp` 中重复的 `GuidDigits`、`ActionsMatch`、`CanonicalZero` 等匿名命名空间 helper 合并进同一翻译单元，产生 C2084/C2264 等冲突。

根因属于模块 clean-unity 配置，而非内存、环境或 P9.1 逻辑。通过在模块规则中固定 `bUseUnity = false` 修复；相同标准 Build 命令随后 recovery、final Editor 和 final Game 均原生退出 `0`。首次失败日志完整保留并记录 SHA，不伪装为环境失败。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值开发、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P9.2 建立唯一产品 adapter：以现有能量/输入事务的正式 commit 激活 shield，把外部 deadline 作为显式 duration 事件输入，并通过本 authority 完成每次 capacity commit；adapter 只能投影 receipt，不得保存第二份可写容量。
