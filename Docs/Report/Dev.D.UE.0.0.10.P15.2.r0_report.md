# Dev.D.UE.0.0.10.P15.2.r0 Report

## 1. 结论

P15.2 完成一个有界的 legacy regression convergence 阶段：修复当前 `Dev.D.UE.0.0.9B` 项目根未进入 Automation Root 精确白名单的问题，并把该安全边界纳入“改动文件 → 必跑测试组”门禁。

同命令、同环境的 `demo_map` 父组从基线 `1246 Success / 83 Fail / 1329 Total` 收敛为 `1273 Success / 57 Fail / 1330 Total`。净减少 `26` 个失败；新增的 1 项是本轮 exact-root/near-match contract。`demo_map.AutomationRootBoundary` 从 `27 Fail` 收敛为仅剩 1 个缺少 `-UserDir` 的环境前置失败；按该套件规定的隔离 UserDir 运行时为 `47/47 Success`。

本阶段没有宣称整个旧套件已恢复。其余 `56` 个历史失败保持可见并留给后续独立阶段。0.0.10 全量为 `712/712 Success`，Game 与 Editor Development 构建均通过。

## 2. 基线与根因

修复前运行完整 `demo_map` 父组，日志完成队列且进程原生退出 `0`，但逐项结果包含 `83` 个 Fail：

| Cluster | Fail |
|---|---:|
| `demo_map.AutomationRootBoundary` | 27 |
| `demo_map.RewardJackpot` | 12 |
| `demo_map.RewardAffix` | 10 |
| `demo_map.RewardBossSource` | 10 |
| `demo_map.RareExtremeValue` | 9 |
| 其它 8 个小簇 | 15 |

最大簇的根因位于 `Fdemo_mapAutomationRootBoundary::Evaluate`：项目根采用显式精确叶名白名单，已有 `0.0.5`、`0.0.6`、`0.0.7` 与 `0.0.9-XFix1`，但遗漏当前仓库根 `Dev.D.UE.0.0.9B`。因此任务 leaf、路径包含关系、普通 uproject、生产目录隔离等条件即使全部正确，`bProjectRootLeafMatch` 仍为 false。

这不是应通过放宽路径规则解决的测试问题；当前项目根确实需要成为一个新的精确允许项。

## 3. 实现

### 3.1 精确项目根补录

在既有 allowlist 中仅增加：

```text
Dev.D.UE.0.0.9B
```

判断继续使用 case-insensitive exact equality。没有改成 `Contains`、`StartsWith`、通配符或正则前缀；Automation parent、Saved grandparent、uproject 普通文件、reparse、storage/user/config 子路径、production overlap 与 external delivery overlap 等既有 fail-closed 条件全部保留。

### 3.2 近似名拒绝契约

新增 `CurrentProjectRootExactMatchOnly`：

- 当前 `Dev.D.UE.0.0.9B` 根必须被接受；
- 构造包含普通 `demo_map.uproject` 的 sibling `Dev.D.UE.0.0.9B.suffix`；
- 即使其余路径事实完整，near-match root 仍必须因 `bProjectRootLeafMatch == false` 被拒绝。

这使新增 allowlist 项同时拥有正向与负向证据。

## 4. 环境前置与可比结果

`AutomationRootBoundary.26.RedirectedRuntimeProductionContextIgnoredAsProtectedFact` 本身验证命令行 `-UserDir` 重定向。与基线相同的不带 `-UserDir` 父组命令中，它仍按设计失败；本阶段没有把它 skip、删除或改成恒真断言。

使用其历史任务 identity 对应的隔离目录：

```text
-UserDir=<Project>/Saved/Automation/Dev.D.UE.0.0.7.F0.0.r0/Tests/User
```

聚焦组得到 `47/47 Success`。因此两个结果分别回答不同问题：

- 同命令父组对比证明本代码改动净消除 `26` 个实际失败；
- 正确隔离上下文的聚焦组证明边界契约本身 `47/47` 完整通过。

## 5. 回归覆盖门禁

新增精确映射：

```text
Source/demo_map/demo_mapAutomationRootBoundary(?:Tests)?.(h|cpp)
  -> demo_map.AutomationRootBoundary
```

流程自检增加：

1. production 与 test 文件由精确聚焦组覆盖时通过；
2. 无关的 `Shanmen.0_0_10` 证据不能替代该安全套件。

最终结果：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 250/250
JSON_PARSE: PASS Rules=145
ROOT_ALLOWLIST_SCAN: PASS exact equality retained; near-match sentinel present
git diff --check: PASS
```

## 6. 修改范围

- `demo_mapAutomationRootBoundary.cpp`：精确补录 `Dev.D.UE.0.0.9B`；
- `demo_mapAutomationRootBoundaryTests.cpp`：新增 current-root 与 near-match contract；
- `ShanmenRegressionMap.json`：新增 AutomationRootBoundary 精确路径映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增一正一反流程自检；
- Report/Log 之前 4 个源码、测试与流程文件净变更 `+53 / -1`。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 7. Automation 证据

| Evidence | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| legacy `demo_map` baseline | 1246 | 83 | 0 | `1506C1E0006D476870032A802CAB80BE2F6BA1EDE7FE9083D199AC5C9DF1DA48` |
| `demo_map.AutomationRootBoundary` isolated | 47 | 0 | 0 | `723B4872F16C178F0FC93B89FE39495F1B0AB8B624D5476FB995537DBC3FA129` |
| legacy `demo_map` after, same baseline command | 1273 | 57 | 0 | `292A0200BA471C4EA51307CB01147F2D7430BF2E618F9C3716DA8F0CD534808B` |
| `Shanmen.0_0_10` | 712 | 0 | 0 | `5B028AA752EAE01D8FD69BF02905BEC8C8F1CC2C887616DDD1D70483FBB25DB6` |

四份日志都有 terminal completion evidence。两个 legacy 父组日志再次证明原生 exit `0` 不等于测试全绿；结论以逐项 Success/Fail 和 queue-empty 同时判定。

0.0.10 全量首末 Success 时间为 `15:33:05.875 -> 16:05:26.849`，约 `32m20.97s`；其中 Fatal error、Unhandled Exception、Ensure condition failed 与 Result Fail 均为 `0`。慢段仍来自 UE 后台 connectivity probe warning，没有转化为产品失败。

修复后 legacy 父组剩余失败分布为 Jackpot 12、Affix 10、BossSource 10、RareExtremeValue 9、P6 4、InputRestore 3、PreparedRunRuntime 2、RewardSourceProjection 2，以及 5 个单项失败；AutomationRootBoundary 仅剩上述无 `-UserDir` 的 1 项。

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Game final | Succeeded | 4 / 27.82s | 0 | `B3BE953FBD1CDACBB6EDA74B146D6A04D906DFB6271A76E83545AB9E63DF1475` |
| Editor final | Succeeded, up to date | 0 / 1.11s | 0 | `AAF76AD28C7A2D1F6D63D1D3C2F699B6D0EFF253616C811B06630D231EF7C3F0` |

最终产物：

- `demo_map.exe`：355,738,624 bytes，SHA-256 `B4F60074A6F5F7FD1E1256536860AF42FC05474C6E57550CC1199775B1B95BB4`；
- `UnrealEditor-demo_map.dll`：14,323,712 bytes，SHA-256 `0D40AC7C08A379F959048E06E152401541B1B61E361FD2E079F640CC57D3DA23`。

## 9. 异常与修复记录

- 首次不带 `-UserDir` 的聚焦运行为 `46 Success / 1 Fail`，失败准确指向 case 26 的重定向环境前置；使用匹配 `Dev.D.UE.0.0.7.F0.0.r0` 的隔离 UserDir 后为 `47/47`。没有通过删除断言修绿。
- 一次仅为捕获自检日志而误用 Windows PowerShell 5，因脚本采用 PowerShell 7 的行首 pipeline 语法发生 parser error；改回项目既有 PowerShell 7 runtime 后 `250/250`、exit `0`。它不是源码、Automation 或产品失败，也没有触发代码修改。
- baseline 与 after 中保留的 Fail 日志是本阶段审计证据，不作为通过证据；raw logs 仅本地保存。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 automation-only 安全边界修复、测试契约、NullRHI 无头 Automation、静态审查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P15.2 只收敛最大、单根因的 RootBoundary 失败簇。下一阶段可优先审查剩余最大的 `RewardJackpot` 12 项，但应先确认它们是否共享一个当前契约漂移根因；若不是，不应把多个 reward 子系统塞进同一提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-2-legacy-regression-convergence>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-2-legacy-regression-convergence/Docs/Report/Dev.D.UE.0.0.10.P15.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-2-legacy-regression-convergence/Docs/Log/Dev.D.UE.0.0.10.P15.2.r0_log.md>
