# Dev.D.UE.0.0.10.P15.6.r0 Development Log

## 1. 目标

收敛 legacy `AutomationRootBoundary.26` 对显式 `-UserDir` 的错误硬依赖；保持真实生产根保护与任务本地自动化根隔离均不降级。

## 2. 基线与根因

- standard focused baseline：`46 Success / 1 Fail / 47 Total`，SHA `333F614C90AF0A6EDDA7CFF664F4F36B792B2C2BD91E74D2427F3A597790A34A`；
- isolated `-UserDir` baseline：`47/0`，SHA `DC3F40068E2F29C3AD003FB858AF339EFBAFEDB8143192699457AA5179782C66`；
- standard 启动中的 runtime Production 是真实项目 `Saved/SaveGames/Shanmen`，旧 fixture 却无条件要求它位于命令行 `UserDir` 并且不受保护；
- isolated 启动中的 runtime Production 才是任务本地路径，因此同一旧测试只在该启动方式下通过。

## 3. 实现

- 保留 fixture 的显式 F0.0.r0 task-local UserDir，不再从进程命令行覆盖测试路径；
- 用 canonical same-or-child 关系分类 runtime Production；
- task-local runtime 必须 `protected=0`，真实项目 runtime 必须 `protected=1`，其它来源失败关闭；
- 继续断言真实 Save、真实 `%LOCALAPPDATA%/demo_map` 受保护，task-local UserDir 与 Generated Config 不进入 protected roots；
- 删除测试侧未使用的 `CommandLine` / `Parse` include；production C++ 零修改。

## 4. 验证

```text
standard focused: 46/1 -> 47/0
isolated focused: 47/0 -> 47/0
legacy demo_map: 1321/9 -> 1322/8, Total=1330
Shanmen.0_0_10: 712/0
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 256/256
JSON_PARSE: PASS Rules=153
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| focused standard after | 47 Success / 0 Fail | `4C7BA25D21EDB83A2FEEA42FA04FF5318391BADCC72D1F6806FD2736BAD2C9F8` |
| focused isolated after | 47 Success / 0 Fail | `3015E8C24BD871DDB49070BCABE5D274BA27EA158EF7B009B84AB52EF7D5CF04` |
| legacy after | 1322 Success / 8 Fail | `78722F60BA4FE6CFADF99125B473EA7FA77493C4356A4D845FF42118404C6286` |
| full 0.0.10 | 712 Success / 0 Fail | `D0FE7DAF0F361BD7DB82256A79024B98E1A4874334E7B44AE379E73496E2516B` |

标准上下文 marker：`task_local=0 protected=1`；isolated 上下文 marker：`task_local=1 protected=0`。两种上下文均由同一测试直接执行。

流程日志 SHA：gate `93D4FE7A11640418133D2B2F5DD1811723AE4D4C69A8C7FB4A317E3186C533E5`；self-test `2766FC234CD0FF722B1E41F1B878F30B564706E140F5279932BA4DFE62FD9BD2`。

## 5. 构建

- initial Editor：4 actions / 16.27s / status 0 / SHA `9EAEC8751BB14442E95C56ED17DF8C7BF28B5618C113F15ACA8E0D66E7DC78E4`；
- final Game：3 actions / 36.95s / status 0 / SHA `F1D0AF965493B98D6BBE1AE07F8D80A49070FF917926FA0EC7E23BE327E4D94F`；
- final Editor：up to date / 0 actions / 1.13s / status 0 / SHA `F685062FEB96A0A25F5CDDFC4D8655BE5C749CAE354EC524D293F90CE7CB0947`；
- Game EXE：355,743,744 bytes / SHA `AAF11263B4A626A7EE6E56860B87223AFDDC2D7DE38C83649D910F89BC915DA4`；
- Editor DLL：14,329,344 bytes / SHA `FE19CCF907E679F80145FF286E96054B4B7761B26A538D096F57F1A125FDED1E`。

## 6. 剩余失败与范围

legacy 父组剩余 8 项：EnemyRouteLoot 1、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。它们未被跳过或归入本阶段。

本轮仅 P 阶段 fixture、NullRHI Automation、静态检查与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-6-automation-root-boundary-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-6-automation-root-boundary-regression/Docs/Report/Dev.D.UE.0.0.10.P15.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-6-automation-root-boundary-regression/Docs/Log/Dev.D.UE.0.0.10.P15.6.r0_log.md>
