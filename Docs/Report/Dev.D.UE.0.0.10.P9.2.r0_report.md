# Dev.D.UE.0.0.10.P9.2.r0 Report

## 1. 结论

P9.2 **PASS**。本阶段把 P9.0/P9.1 仍由注释约定的 SpiritShield duration 收口为可验证的外部 deadline gate：一份已激活 shield 绑定一条 caller-owned 单调时间线、起始 tick 与截止 tick；只有来自同一时间线且到达截止点的 observation 才能以 `DurationElapsed` 结束该 shield，并生成不可变、可重放 receipt。

最终验证为 Deadline `6/6`、Capacity `6/6`、SpiritShield `17/17`、CombatCore `9/9`、CombatRuntime `51/51`、`Shanmen.0_0_10` 全量 `354/354`；六份正式日志合计 `443` 条 success、`0` fail。changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

源码审查同时确认：当前产品层没有可复用的“灵力/法力”可写权威，也没有 SpiritShield 的真实产品调用点。故本阶段没有伪造能量字段、输入事务或第二套产品 owner；产品激活与能量扣除仍是后续明确契约。

## 2. 功能性

### 2.1 不可变 deadline contract

`FShanmenSpiritShieldDeadlineContract` 只从有效 activation receipt 创建，并冻结：

- shield activation 与 instance identity；
- caller-owned `TimelineId`；
- `StartTick` 与严格更大的 `DeadlineTick`；
- 由上述输入确定性派生的 `ContractId`。

CombatRuntime 不解释 tick 的单位，也不拥有时钟；它只验证身份与顺序。空 timeline、负 tick、非前进 deadline 均失败关闭。

### 2.2 显式 observation 与到期收口

`FShanmenSpiritShieldTimelineObservation` 表示外部时间 owner 提交的一次采样。`FShanmenSpiritShieldDeadlineGate::TryElapse()` 依次验证：

- observation 自身确定性身份；
- gate/contract 完整性；
- timeline 一致性；
- shield activation 一致性；
- `ObservedTick >= DeadlineTick`；
- shield 仍可由 deadline 路径结束。

成功后生成 `FShanmenSpiritShieldDeadlineElapsedReceipt`，同时绑定 contract、observation 与既有 deactivation receipt。早到、foreign timeline、foreign shield、已被其它原因结束的 shield 均不变更状态。

### 2.3 DurationElapsed 的唯一入口

公开 `FShanmenSpiritShieldRuntime::TryDeactivate()` 现在拒绝直接传入 `DurationElapsed`。deadline gate 通过 private friend seam 调用同一内部 deactivation 实现；`Explicit`、`Interrupted` 等其它既有显式原因保持公开行为。

因此调用方不能绕过 deadline proof 自称“持续时间已结束”，也没有复制第二套生命周期状态机。

### 2.4 幂等与冲突

- 同一 terminal observation 的精确重放返回原 receipt，不二次结束 shield；
- deadline 已完成后提交不同 terminal observation 返回 `DeadlineConflict`，不重写历史；
- equivalent frozen input 在独立 runtime 中复现 contract、observation、deactivation 与 elapsed receipt IDs；
- capacity authority 在到期前仍可投影；到期后 projection 关闭，但剩余容量不被 deadline gate 篡改。

## 3. 完整性与兼容性

- P9.0 Definition、activation、projection receipt 格式未改变；
- P9.1 capacity authority、revision 与 Impact ledger 未改变；
- 所有 duration 终止仍复用原 `FShanmenSpiritShieldDeactivationReceipt`；
- 未引入 `demo_map`、World、Actor、Timer、Tick、轮询、输入、能量、UI 或随机数依赖；
- deadline gate 是普通 C++ 纯值对象，所有跨层证据是 read-only USTRUCT；
- 产品层现有源码中没有 SpiritShield runtime 调用点，故本阶段不声称已完成玩家可见接线。

## 4. 关键不变量

1. contract 必须绑定一份有效且确定的 activation receipt；
2. timeline identity 由调用方提供，但一旦 capture 后不可替换；
3. deadline 必须严格晚于 start；
4. observation 必须来自同一 timeline 且达到或越过 deadline；
5. `DurationElapsed` 只能由 deadline gate 写入 lifecycle receipt；
6. foreign/early/conflicting evidence 不产生状态写入；
7. exact terminal replay 返回原 proof，不产生第二次 deactivation；
8. deadline gate 不拥有 capacity、能量、输入、时钟或产品对象。

## 5. 测试覆盖

新增 6 个 focused tests：

- `Contract`：有效 contract、非法 deadline、匿名 timeline、observation identity；
- `EarlyExactAndReplay`：早到失败、精确截止成功、精确重放与冲突；
- `ForeignEvidence`：foreign timeline 与 foreign shield 均失败关闭；
- `LifecycleInterlock`：先被 Interrupted 结束后，deadline 不覆盖原原因；
- `CapacityComposition`：到期前可投影、到期后关闭投影且不复制容量；
- `DeterministicReplay`：等价冻结输入复现全部 deadline IDs。

既有 SpiritShield 测试同步增加公开 `DurationElapsed` 绕过拒绝，并继续覆盖其它显式结束与 receipt 重放。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldDeadlineGate.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldDeadlineGate.cpp`；
- `Source/ShanmenCombatRuntime/Public/ShanmenSpiritShield.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSpiritShield.cpp`。

测试与流程：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldDeadlineGateTests.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

代码与脚本净变更为 `8` 个文件、`904` insertions、`4` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `10` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `SpiritShieldDeadline-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline` | 6 | 0 | `02D72B6276727C833B779F9A66857180161FAF2F13C03A747AE9AA4F5EAD0B4A` |
| `SpiritShieldCapacity-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity` | 6 | 0 | `96C0276ADEA42885E121F67804BD87D165C3265557182EB6EE2E388FFC1774E7` |
| `SpiritShield-final.log` | `Shanmen.0_0_10.CombatRuntime.SpiritShield` | 17 | 0 | `94CCDC6999C40DB2705DB4BDC2C01FA0F98486617954105B2045352337F5B504` |
| `CombatCore-final.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | `4738AD553FF87A8DC8852970087A57B04620897D19149948667D213AAE724F7E` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 51 | 0 | `F539D1DB8461CE3F2787FAB08C5313A2866E65CD5F0346465650786023299493` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 354 | 0 | `CB758E122EBFE3DFAF50FE50CF001961218D2215CAB6F5E3E7BCB4E29BACDEA1` |

每份正式日志均有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，命令进程退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=72
SELF_TEST: PASS 106/106
REGRESSION_COVERAGE (implementation): PASS Changed=8 Rules=3 Required=5 Logs=6
REGRESSION_COVERAGE (exact stage): PASS Changed=10 Rules=3 Required=5 Logs=6
Required: CombatRuntime.SpiritShieldDeadline, SpiritShieldCapacity,
          SpiritShield, CombatRuntime, CombatCore
git diff --check: PASS
```

- mapping SHA-256：`328E90668CBE6AB33FFBE8AE242DD0F9AC7A166ADB284F50A659F4052BBA1F13`；
- self-test SHA-256：`62171B0EA1B8D01EA4CDF55B48B5637229F5A04009119E7B2DB280C4AC523969`；
- 本阶段 C++ 边界扫描：`demo_map/UWorld/AActor/ApplyDamage/GetWorld/Timer/Tick/while/RNG` 均为 `0`。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Failed: test 缺少 resolver declaration include | 6 / 34.00s | UBT 6; runner 1 | `97BFB2BA07820DA047BEB4B60B7CB6FAD028037B3714FBFC8F8981C09B067138` |
| Editor recovery | Succeeded | 4 / 4.55s | 0 | `1BC973544DBE9018EBE550F3039A246198394D763F3211D86D98FC60421A5AE7` |
| Editor final | Succeeded, target up to date | 0 / 0.91s | 0 | `BB7C3599B353A4A99B914956235823022A682152EB2BDA740DC402E0ADA927F4` |
| Game final | Succeeded | 9 / 35.78s | 0 | `625204CED916B11DD36BAE063A7198436FFC6E7981B37631B8165CB5AE6888D9` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`876032` bytes，SHA-256 `A4E0EBF886B31CE4968885467C7B4769753DEEEA0955FE2AC1E384E43C167E36`；
- `demo_map.exe`：`353324544` bytes，SHA-256 `5BF1BF0A325717B7ADA47224CA9D1C9DCB15D7B052B30866F5EF65777D55237D`。

## 9. 真实异常

首次 Editor candidate 的 `ShanmenSpiritShieldDeadlineGate.cpp` 与生成代码已成功编译；失败只发生在新增 test 编译单元：使用 `FShanmenCombatIdFactory` 时未包含声明它的 `ShanmenCombatResolver.h`，产生 C2653/C3861。加入直接头文件后，相同标准构建 recovery 原生退出 `0`；后续最终 Editor、Game 与六组 Automation 全部通过。

该错误是源码 include 完整性错误，不是内存、页面文件、SDK 或环境故障。首次失败日志和失败语义完整保留，没有将其描述为环境问题。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值开发、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P9.3 先建立唯一、typed 的灵力资源 authority/cost transaction，或明确复用哪个现有产品属性写 seam；随后由一个产品 host 组合输入、能量 commit、SpiritShield activation、外部 deadline observation 与 P9.1 capacity commit。没有该权威前，不应把临时 float 字段伪装成产品接线。
