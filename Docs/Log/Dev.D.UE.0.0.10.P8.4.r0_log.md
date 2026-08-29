# Dev.D.UE.0.0.10.P8.4.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.4.r0`；
- 基线：`18f6890514521f57bc2670b007ac006b2d262fb3`（P8.3）；
- 分支：`agent/0.0.10-p8-4-formation-product-host`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.4 把 P8.2 product session 与 P8.3 World adapter 组合成唯一 Formation product host，冻结 prepare → durable commit → placement 顺序、post-commit forward retry、Cancel/End gate 与 terminal teardown replay。

明确后置：区域规则 provider、战斗效果、投料交互、输入/UI、正式阵图商品、配方与数值。

## 契约设计

### 三段式命令

Host 不隐藏不可逆边界。Prepare 只建立 durable reservation；Commit 发布 material evidence、deployment receipt 与 placement intent；Place 只进入 World adapter。每一层原始 result/receipt 保留在 Host result 中。

### Sole pending placement

commit 后只允许 exact PlacementId 前向处理。另一个 anchor 不能 overtaking。首次合法 World/class 请求冻结 retry binding；失败后换 World/class 会被拒绝。exact commit replay 直接返回原 audit，不进入 authority。

### Terminal

pre-commit cancel 可释放 reservation；post-commit Deploying cancel 不退款并执行 teardown；Active formation 必须先完成 final placement 才能 End。teardown 失败后不重做 Session transition，只重试 P8.3 cleanup。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationProductHost.h`；
- `Source/demo_map/demo_mapShanmenFormationProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

Host 没有 legacy inventory、damage、RNG、Tick/timer、输入、UI、效果或 content 依赖。

## 诊断与修正记录

1. 首次 Editor source build 一次通过。
2. 首次 focused 为 `1 Success / 3 Fail`。一个断言在恢复完成后读取 pending；另两个按 DefinitionId 汇总迁移后的 active-Run item snapshot，基准恒为 `0`。
3. pending 断言改为故障当刻取证后，focused 提升为 `2 Success / 2 Fail`。
4. diagnostic run 确认 commit/replay snapshots 完全相等，而旧 quantity locator 仍为 `0`。最终采用 authority finalize receipt、plan 中 `ExpectedQuantityBefore` 与完整 snapshot equality，四项全部通过。
5. 提交前语义审查补齐 exact pending-commit replay 的嵌套 material/deployment audit，避免 Host 成功但调用方看不到原 authority evidence；最终 focused/full 基于该修正后源码重跑。

三份失败日志均保留。Automation 失败时进程仍返回 `0`，判定使用 Result 与 queue-empty。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationProductHost.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 4 | 0 | 0 | `FABCB136CE3E3DDF7D724BAF2A6299AD70CCDD50187E376FB837FE731AB02534` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 227 | 0 | 0 | `35302F2CBE9B5C1A4721152F31BC173885CE885821E236DE4D689670A6FA3697` |

### 保留的失败日志

| 日志 | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `FormationProductHost-first.log` | 1 | 3 | 0 | `0A19388C280365313425C4ADF114AD16F0347974D76E025ECC936555735DB790` |
| `FormationProductHost-second.log` | 2 | 2 | 0 | `935B3358D8CD0F4E84803D39CAE8D1A2493033534AEBD9C0FCC3FEB625C6E8B1` |
| `FormationProductHost-diagnostic.log` | 2 | 2 | 0 | `A8FDDFEE757BD61B8E102F90D801ECC84930C909CD24F6B50287E0B64ABD6510` |

## 测试内容

1. commit intent、authority receipt、snapshot-stable replay 与 anchor serialization；
2. duplicate tag、frozen class binding、survivor adoption 与 replay；
3. final placement End gate、两 Actor teardown 与 receipt replay；
4. prepared cancel no-consume 与 committed cancel no-refund。

## Changed-file regression gate

新增 FormationProductHost rule，要求 Host、WorldDelivery、Session、MaterialAdapter、Items、WorldGameplay 与 FormationDeployment 七组证据。child-only fixture 必须失败，broad full 可覆盖所有 parent groups。

```text
REGRESSION_MAP_JSON: PASS Rules=40
SELF_TEST: PASS 41/41
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=7 Logs=2
```

- coverage SHA-256：`CB483252DE5BE63EE51DE97AF24555B8CEDB83142A4FAABCEFD8BA5930A40062`；
- self-test SHA-256：`813E982FD667B1E9A5725D4E67DBE3720256DD1A6F88F97496A74B63FA90563D`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor final | Succeeded | 0 | 8.45s | `19FC21B60C3717047F1BFB8311FBE7B01BC6F71D06A665FCC5FAA2896577B07F` |
| Game final | Succeeded | 0 | 27.77s | `D2DBA93AC4239E93A9ECCDA12824CF1AB81D6C5AB7BD396123A85CEABCD124FA` |

首次 Editor source build也成功，exit `0`、总耗时 `33.35s`。无源码、UHT、link、commit-memory 或环境失败。

## 静态、兼容性与 P/F 边界

- map JSON、boundary scan、mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- staged `git diff --cached --check` 在提交前执行；
- 未修改 Build.cs、tags、schema、Content、P8.0—P8.3、GameMode、输入、UI 或旧产品链；
- 长期未跟踪用户文件未修改、未 stage。

本轮仅执行源码、静态门禁、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.5 建议增加纯区域规则 provider，消费 committed placement receipts 并输出 deterministic membership/coverage；不直接施加伤害、Buff 或正式内容数值。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-4-formation-product-host/Docs/Report/Dev.D.UE.0.0.10.P8.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-4-formation-product-host/Docs/Log/Dev.D.UE.0.0.10.P8.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-4-formation-product-host>
