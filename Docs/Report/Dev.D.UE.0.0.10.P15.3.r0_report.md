# Dev.D.UE.0.0.10.P15.3.r0 Report

## 1. 结论

P15.3 完成 legacy reward projection binding regression 的有界收敛：修复 5 个旧 reward 自动化套件仍把 registry policy prototype 直接交给 P1.9 concrete projection planner 的 fixture 漂移，同时保持生产侧 fail-closed 绑定契约不变。

同命令 legacy `demo_map` 父组从 P15.2 的 `1273 Success / 57 Fail / 1330 Total` 收敛为 `1316 Success / 14 Fail / 1330 Total`，精确消除 `43` 个 Fail。5 个聚焦套件从合计 `275 Success / 43 Fail` 变为 `318 Success / 0 Fail`。0.0.10 全量仍为 `712/712 Success`，Game 与 Editor Development 构建均通过。

本阶段没有放宽 `Fdemo_mapRewardSourceProjection::IsValid()`，没有修改 reward 生产代码，也没有把 registry prototype 伪装成可执行实例。修复只发生在测试 fixture、回归映射及其自检。

## 2. 基线与根因

P1.9 将 `Fdemo_mapRewardSourceProjection` 的运行时契约收紧为 concrete binding：除 projection policy 外，还必须具备非空 `DistributionProfileId` 与 `SlotId`。canonical registry 中的条目则有意保持为未绑定的 policy prototype，其注册表验证使用 `IsPolicyPrototypeValid()`。

5 个旧测试文件仍把这些 prototype 直接传给 `Fdemo_mapRewardSourceProjectionPlanner::Plan`。Planner 按新契约正确 fail closed，因此出现以下完全一致的 fixture regression：

| Suite | Baseline Success | Baseline Fail |
|---|---:|---:|
| `demo_map.RewardJackpot` | 40 | 12 |
| `demo_map.RewardAffix` | 80 | 10 |
| `demo_map.RewardBossSource` | 54 | 10 |
| `demo_map.RareExtremeValue` | 63 | 9 |
| `demo_map.RewardSourceProjection` | 38 | 2 |
| 合计 | 275 | 43 |

这 43 项共享同一根因；不是五种 reward 生产逻辑同时损坏。

## 3. 实现

### 3.1 测试专用 concrete binding

新增 header-only `demo_mapRewardProjectionTestSupport.h`，仅在 `WITH_DEV_AUTOMATION_TESTS` 下提供：

- `BindPrototype`：复制 registry prototype，并按 projection id 派生稳定的测试专用 distribution/slot identity；
- `FindBound`：从 canonical registry 查找 prototype 后立即绑定；
- identity 命名空间固定为 `Automation.Distribution.*` 与 `Automation.Slot.*`，不进入生产配置。

### 3.2 旧 fixture 对齐

更新 Jackpot、Affix、BossSource、RareExtremeValue 与 RewardSourceProjection 五个测试文件：

- registry/prototype 结构断言继续读取原始 prototype；
- 只有进入 Planner 的路径使用 bound copy；
- BossSource 明确区分 `BossProjection()` 与 `BoundBossProjection()`，避免测试把 prototype contract 与 execution contract 混为一谈；
- 没有删除失败断言，也没有为缺失 binding 增加旁路。

## 4. 回归覆盖门禁

`ShanmenRegressionMap.json` 新增 6 条精确规则：

- 共享 test support header 的改动要求五个 reward suite 全部有证据；
- 五个测试文件各自要求对应 suite。

自检增加五份 synthetic evidence，并验证：

1. support header 与五个测试文件由完整五组日志覆盖时通过；
2. 故意缺少 `RewardSourceProjection` evidence 时必须 fail closed。

最终流程证据：

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=6 Required=5 Logs=5
SELF_TEST: PASS 252/252
JSON_PARSE: PASS Rules=151
git diff --check: PASS
```

## 5. 聚焦 Automation 证据

| Suite | After Success | After Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RewardJackpot` | 52 | 0 | 0 | `A94C6F6D0E6C0A3751460614BE3CB31FE0898A34A4125915A1B6E2719B62F7EA` |
| `RewardAffix` | 90 | 0 | 0 | `F30189744992167E7C73289AFD33B8849DC74DC0AFC61348427BCB333B5B5D76` |
| `RewardBossSource` | 64 | 0 | 0 | `1D6F72B31459EBC7F610F8B029641551DBF60C67792F2967C36FF18A434976D5` |
| `RareExtremeValue` | 72 | 0 | 0 | `FC7535A4F6D17562AAE62A0D84F92E391364E2D779B0DEEC0693B65DE656A369` |
| `RewardSourceProjection` | 40 | 0 | 0 | `DDBA5E49251863CE284F603CF541EF1764882867CD4424B934848F346ABEA453` |
| 合计 | 318 | 0 | — | — |

对应 baseline 日志 SHA：

- Jackpot：`1204787AEEA343FCFA0178E86BF8A640266DD41DFDE14904AE26F490C070A226`；
- Affix：`699C2C35160F4F47832D20692C945CE1DA127E6B4637D13456A2A4DF44CA33EB`；
- BossSource：`C1CF889D565F2254D8E61BA779BCB320FB7CB0913DEE84B53A8A03CFE8819766`；
- RareExtremeValue：`8FB79F8B405EF5E836C13C61972535EC2B088828F73886D275A409046519F35C`；
- RewardSourceProjection：`71BC930B3A2459E9EEFE97418C638717EEC9DB8AFADD21D47493500434AF8982`。

## 6. 父组与 0.0.10 全量

| Evidence | Success | Fail | Total | Exit | SHA-256 |
|---|---:|---:|---:|---:|---|
| legacy `demo_map` after | 1316 | 14 | 1330 | 0 | `7B508E1A1D8DBF8348EB725FBD541B491EDFB836C8AE9ACBF831487C9F7D8DE1` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | 0 | `2550630BD55410B018D1DEEDF6AC884C457F6E96A3F57AA73897938740EED54B` |

0.0.10 全量首末 Success 时间为 `16:27:27.878 -> 16:59:44.887`，约 `32m17.01s`；随后出现 queue-empty terminal evidence。Fatal error、Unhandled Exception、Ensure condition failed 与 Result Fail 均为 `0`。

legacy 父组剩余 14 项保持可见：AutomationRootBoundary 的缺 `-UserDir` 前置 1 项、EnemyRouteLoot 1 项、InputRestore 3 项、P1 SectNavigation 1 项、P5RuntimeInterface 1 项、P6 4 项、PreparedRunRuntime 2 项、V3 Lifecycle 1 项。它们未被归入本阶段，也未被跳过。

## 7. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Game final | Succeeded | 7 / 38.16s | 0 | `7166FD7B4D9982E3C6D38572AA97F199D8DD1DC25AC6B656039DE090BE6B39D3` |
| Editor final | Succeeded, up to date | 0 / 1.07s | 0 | `05C13F7778E27E5099B953A477B12FBE8297E29351BE928976DC26F8A3E9FA6E` |

最终产物：

- `demo_map.exe`：355,741,184 bytes，SHA-256 `7B0E99F052A9691B8C9BF5013A67F5245753335244EA66AF764450FD320BA04E`；
- `UnrealEditor-demo_map.dll`：14,327,296 bytes，SHA-256 `6C67306EF2C1E165B89583EEEC5F56E3C385B41C93D1456DE3F119A8E9EFE798`。

## 8. 修改范围

- 新增 1 个测试专用 support header；
- 修改 5 个 legacy reward test fixture；
- 修改 regression map 与 self-test；
- 生产 reward planner、registry、projection validation 与产品模块均未修改；
- raw logs 仅保存在 `Saved/Codex/P15.3`，不进入 git；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 9. P/F 边界

本 Report 只包含 P 阶段 test contract 修复、NullRHI 无头 Automation、静态审查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

父组原生 exit `0` 不被当作全绿；结论同时依据逐项结果、queue-empty 与 terminal evidence。剩余 14 项将在后续按单一根因簇继续审查，不能通过扩大本提交或降低断言处理。

## 10. 下一步

P15.4 应从剩余 14 项中选择一个共享根因簇，优先审查 `InputRestore` 或 `PreparedRunRuntime`，先建立同命令 baseline 与契约归属，再决定修 production、fixture 或环境前置。本阶段不预判其修复方式。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-3-reward-projection-binding-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-3-reward-projection-binding-regression/Docs/Report/Dev.D.UE.0.0.10.P15.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-3-reward-projection-binding-regression/Docs/Log/Dev.D.UE.0.0.10.P15.3.r0_log.md>
