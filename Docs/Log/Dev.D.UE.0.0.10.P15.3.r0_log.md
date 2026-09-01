# Dev.D.UE.0.0.10.P15.3.r0 Development Log

## 1. 目标

从 P15.2 剩余 legacy 失败中选择一个共享根因簇，修复旧 reward projection test fixture 与 P1.9 concrete binding contract 的漂移；不放宽生产验证，并把改动文件映射到精确必跑测试组。

## 2. 基线与根因

- 五个聚焦组：`275 Success / 43 Fail`；
- P15.2 legacy 父组：`1273 Success / 57 Fail / 1330 Total`；
- 根因：registry 返回未绑定 policy prototype，旧 fixture 直接交给要求 `DistributionProfileId + SlotId` 的 Planner；
- 选择该簇是因为 43 项具有同一结构性原因；其余 14 项未混入本阶段。

## 3. 实现

- 新增仅测试编译的 `demo_mapRewardProjectionTestSupport.h`；
- 以稳定 `Automation.Distribution.*` / `Automation.Slot.*` identity 把 prototype 复制为 concrete test binding；
- Jackpot、Affix、BossSource、RareExtremeValue、RewardSourceProjection 五个测试只在 Planner 边界使用 bound copy；
- registry prototype 断言继续针对未绑定原值；
- production validation、Planner、registry 与 reward 产品代码零修改。

## 4. 回归门禁

- regression map 新增 6 条 exact rule；
- support header 要求五个 reward suite 全覆盖；
- 五个测试文件分别要求对应 suite；
- synthetic positive 覆盖完整五组，negative 故意缺一组并验证 fail closed。

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=6 Required=5 Logs=5
SELF_TEST: PASS 252/252
JSON_PARSE: PASS Rules=151
git diff --check: PASS
```

流程日志 SHA：gate `0F00AED81E38B56EA941ECD4936FE0D85FBDB6A18C07F48E899D1FDF3586449E`；self-test `EC80874015132960C951EFFE976993131E02407376B026DFB573E56EC3DB27A9`。

## 5. Automation

| Suite | Baseline S/F | After S/F | After SHA-256 |
|---|---:|---:|---|
| RewardJackpot | 40/12 | 52/0 | `A94C6F6D0E6C0A3751460614BE3CB31FE0898A34A4125915A1B6E2719B62F7EA` |
| RewardAffix | 80/10 | 90/0 | `F30189744992167E7C73289AFD33B8849DC74DC0AFC61348427BCB333B5B5D76` |
| RewardBossSource | 54/10 | 64/0 | `1D6F72B31459EBC7F610F8B029641551DBF60C67792F2967C36FF18A434976D5` |
| RareExtremeValue | 63/9 | 72/0 | `FC7535A4F6D17562AAE62A0D84F92E391364E2D779B0DEEC0693B65DE656A369` |
| RewardSourceProjection | 38/2 | 40/0 | `DDBA5E49251863CE284F603CF541EF1764882867CD4424B934848F346ABEA453` |

- focused 合计：`275/43 -> 318/0`；
- legacy after：`1316 Success / 14 Fail / 1330 Total`，status `0` 且 queue-empty terminal evidence 1 条，SHA `7B508E1A1D8DBF8348EB725FBD541B491EDFB836C8AE9ACBF831487C9F7D8DE1`；
- full 0.0.10：`712/712 Success`，status `0` 且 queue-empty terminal evidence 1 条，SHA `2550630BD55410B018D1DEEDF6AC884C457F6E96A3F57AA73897938740EED54B`；
- 相对 P15.2 精确净消除 43 Fail，没有新增或隐藏失败。

## 6. 构建

- 首次 Editor 编译：8 actions / 39.90s / succeeded；
- final Game：7 actions / 38.16s / exit 0 / log SHA `7166FD7B4D9982E3C6D38572AA97F199D8DD1DC25AC6B656039DE090BE6B39D3`；
- final Editor：up to date / 0 actions / 1.07s / exit 0 / log SHA `05C13F7778E27E5099B953A477B12FBE8297E29351BE928976DC26F8A3E9FA6E`；
- Game EXE：355,741,184 bytes / SHA `7B0E99F052A9691B8C9BF5013A67F5245753335244EA66AF764450FD320BA04E`；
- Editor DLL：14,327,296 bytes / SHA `6C67306EF2C1E165B89583EEEC5F56E3C385B41C93D1456DE3F119A8E9EFE798`。

## 7. 剩余失败与范围

legacy 父组还保留 14 项：RootBoundary 1、EnemyRouteLoot 1、InputRestore 3、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、PreparedRunRuntime 2、V3 Lifecycle 1。父组 status `0` 未被误写为全绿；全绿判断仍以逐项结果与 queue-empty 共同成立为准。

本轮仅 P 阶段 fixture、流程门禁、NullRHI Automation 与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-3-reward-projection-binding-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-3-reward-projection-binding-regression/Docs/Report/Dev.D.UE.0.0.10.P15.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-3-reward-projection-binding-regression/Docs/Log/Dev.D.UE.0.0.10.P15.3.r0_log.md>
