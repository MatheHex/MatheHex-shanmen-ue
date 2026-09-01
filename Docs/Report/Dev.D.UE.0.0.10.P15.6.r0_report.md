# Dev.D.UE.0.0.10.P15.6.r0 Report

## 1. 结论

P15.6 完成 legacy `demo_map.AutomationRootBoundary.26` 的有界收敛：同一测试现在按 canonical 路径来源区分“真实项目 Production”与显式 `-UserDir` 下的“任务本地 runtime Production”，不再假定所有启动方式都必然重定向到 `-UserDir`。

标准启动 focused suite 从 `46 Success / 1 Fail / 47 Total` 收敛为 `47 Success / 0 Fail / 47 Total`；显式隔离 `-UserDir` 的同一 suite 在修改前后均为 `47/0`。legacy `demo_map` 父组从 P15.5 的 `1321 Success / 9 Fail / 1330 Total` 变为 `1322 Success / 8 Fail / 1330 Total`，精确消除这一项失败。

本阶段 production C++ 零修改；没有移除真实存档根、真实用户配置根或外部交付目录的保护，也没有放宽 root evaluator 的失败关闭规则。

## 2. 根因

旧 case 26 从进程命令行解析 `UserDir=`，覆盖 fixture 的 `UserConfigRoot`、`UserDirRoot` 与 `GeneratedConfigRoot`，随后无条件断言 runtime `Fdemo_mapProfileStorageContext::Production()` 位于该命令行目录下且不是 protected production fact。

该假设只在显式隔离启动时成立：

- 标准启动没有任务级 `-UserDir`；命令行解析结果落到 UE Engine binaries，而 runtime Production 正确指向项目 `Saved/SaveGames/Shanmen`。旧测试把真实 Production 当成“应被忽略”的任务本地路径，产生 4 条错误断言；
- 显式 `-UserDir=.../Saved/Automation/.../Tests/User` 时，runtime Production 确实位于任务本地目录，旧测试通过。

因此失败是测试对启动前提的错误硬编码，不是 root boundary 产品保护缺失。

## 3. 实现

case 26 现在保留 fixture 由 `SetLeaf(...F0.0.r0)` 建立的显式任务本地根，并对 runtime Production 做 canonical 来源分类：

```text
runtime 位于 task-local UserDir -> 必须不在 protected roots
runtime 位于真实项目 Production -> 必须在 protected roots
其它来源                     -> 失败关闭
```

同一测试同时继续证明：

- 显式任务本地 UserDir 本身永不成为 protected production fact；
- Generated Config 保持在任务本地 UserDir 下；
- 项目 `Saved/SaveGames/Shanmen` 始终受保护；
- `%LOCALAPPDATA%/demo_map` 始终受保护；
- task-local fixture 可由 root evaluator 接受，且无 production overlap。

删除了不再需要的测试侧 `CommandLine` 与 `Parse` include，并将用例名改为 `RuntimeProductionContextClassifiedByCanonicalOrigin`。产品 root 枚举与 evaluator 均未修改。

## 4. 双启动上下文证据

修改后的同一断言在两种真实启动上下文中得到相反但正确的分类：

| 启动方式 | runtime Production | task_local | protected | 结果 |
|---|---|---:|---:|---|
| 标准启动 | 项目 `Saved/SaveGames/Shanmen` | 0 | 1 | 47/47 |
| 隔离 `-UserDir` | `.../Tests/User/Saved/SaveGames/Shanmen` | 1 | 0 | 47/47 |

这证明测试验证的是路径来源契约，而不是某一种机器命令行形态。

## 5. 回归覆盖门禁

现有映射已把 `demo_mapAutomationRootBoundaryTests.cpp` 精确映射到 `demo_map.AutomationRootBoundary`；本轮无需扩大或修改流程配置。

```text
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 256/256
JSON_PARSE: PASS Rules=153
git diff --check: PASS
```

## 6. Automation 证据

| Evidence | Success | Fail | Total | SHA-256 |
|---|---:|---:|---:|---|
| standard focused baseline | 46 | 1 | 47 | `333F614C90AF0A6EDDA7CFF664F4F36B792B2C2BD91E74D2427F3A597790A34A` |
| isolated focused baseline | 47 | 0 | 47 | `DC3F40068E2F29C3AD003FB858AF339EFBAFEDB8143192699457AA5179782C66` |
| standard focused after | 47 | 0 | 47 | `4C7BA25D21EDB83A2FEEA42FA04FF5318391BADCC72D1F6806FD2736BAD2C9F8` |
| isolated focused after | 47 | 0 | 47 | `3015E8C24BD871DDB49070BCABE5D274BA27EA158EF7B009B84AB52EF7D5CF04` |
| legacy `demo_map` after | 1322 | 8 | 1330 | `78722F60BA4FE6CFADF99125B473EA7FA77493C4356A4D845FF42118404C6286` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | `D0FE7DAF0F361BD7DB82256A79024B98E1A4874334E7B44AE379E73496E2516B` |

六份 Automation 日志都有 queue-empty terminal evidence；Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。legacy 父组的 8 项失败保持可见，没有被过滤或改写为成功。

0.0.10 全量首末 Success 时间为 `18:40:26.161 -> 19:15:19.874`，约 `34m53.71s`，随后出现 `712 tests performed` queue-empty terminal evidence。

流程日志 SHA：gate `93D4FE7A11640418133D2B2F5DD1811723AE4D4C69A8C7FB4A317E3186C533E5`；self-test `2766FC234CD0FF722B1E41F1B878F30B564706E140F5279932BA4DFE62FD9BD2`。

## 7. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 4 / 16.27s | 0 | `9EAEC8751BB14442E95C56ED17DF8C7BF28B5618C113F15ACA8E0D66E7DC78E4` |
| Game final | Succeeded | 3 / 36.95s | 0 | `F1D0AF965493B98D6BBE1AE07F8D80A49070FF917926FA0EC7E23BE327E4D94F` |
| Editor final | Succeeded, up to date | 0 / 1.13s | 0 | `F685062FEB96A0A25F5CDDFC4D8655BE5C749CAE354EC524D293F90CE7CB0947` |

最终产物：

- `demo_map.exe`：355,743,744 bytes，SHA-256 `AAF11263B4A626A7EE6E56860B87223AFDDC2D7DE38C83649D910F89BC915DA4`；
- `UnrealEditor-demo_map.dll`：14,329,344 bytes，SHA-256 `FE19CCF907E679F80145FF286E96054B4B7761B26A538D096F57F1A125FDED1E`。

## 8. 修改范围与 P/F 边界

- `demo_mapAutomationRootBoundaryTests.cpp`：修正一个双启动上下文 fixture，删除两个无用 include；
- Report/Log；
- production C++、回归映射、产品配置与资产零修改；
- raw logs 仅本地保存；长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

本 Report 仅包含 P 阶段 legacy test contract 修复、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 剩余失败与下一步

legacy 父组剩余 8 项：EnemyRouteLoot 1、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。

P15.7 可优先审查 `demo_map.EnemyRouteLoot.42.EnhancedCorpseCapacity`，核对当前 Definition-backed corpse capacity 与历史固定容量断言，再沿用“生产契约不降级、fixture 可重放”的原则收敛。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-6-automation-root-boundary-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-6-automation-root-boundary-regression/Docs/Report/Dev.D.UE.0.0.10.P15.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-6-automation-root-boundary-regression/Docs/Log/Dev.D.UE.0.0.10.P15.6.r0_log.md>
