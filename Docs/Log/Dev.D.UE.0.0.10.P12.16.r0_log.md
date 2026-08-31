# Dev.D.UE.0.0.10.P12.16.r0 Development Log

## 1. 目标

在 P12.15 single-consumer execution driver 之上增加 Run-local Visual/Audio execution host。host 固定拥有两个 canonical coordinator，调用方注入两个 executor 与 attempt identity；同一调用按 Visual→Audio 处理，两个 channel 的 acknowledgement 与 retry 独立。

## 2. 实现

- 新增 typed aggregate status：`Completed`、`RetryPending`、`ConsumerRejected` 与 preflight/lifecycle failures；
- `TryCreate` 建立同 Run、不同 consumer identity 的 canonical Visual/Audio scopes；
- `Process` 先统一验证 host/event/双 attempt，再固定执行 Visual driver、Audio driver；
- host 不直接 prepare 或调用 executor adapter，既有 driver 是唯一 mutation path；
- partial retry/reject 不回滚成功 sibling；恢复轮只 reinvoke 未完成 executor；
- exact acknowledged sibling 返回 replay evidence，不 reinvoke；
- zero-command 双 channel 始终 executor-free；
- cross-Run、invalid input、错误 end identity 与 post-end process fail closed；
- ProductSession teardown 后 immutable event 仍可执行；
- host 不拥有 executor、资产、World、timer、线程或 ProductSession。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `3E627071BE6A795052E5D2DA36CCCAC9B603B7015CF71FA1D62768C6FFAF3E92` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `331A338A3D53A8BCA845D1C530846FB1BCAD90C3F849455876BF6A2FB0816074` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `A9D57ACA3E5128B951CB14D47FBF0AB2FFA115B439928C4C388200F8C5E43FCA` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `23111F1AAA9816B9AF8967446618577496F4844FC431DED639C61F54396FD595` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `DAE5E1A1A6455A9554A0D8C558AE28CA85F26C1FFCE8D28B5BD2AB9D3D65B56A` |
| `SwordRhythmEffectCue` | 27 | 0 | 0 | `A3D3CC6BC068962BCB1A601F0D47674BB24E10A1C91606EDF88D09984BD1E2C1` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `6C29EB2AD175F66A1DB591701F8BD871BDD2803BC892EF7D112F11F242B7EAFD` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `0B69D80648E879AF32834F268AFCDAF5307244BFDD678BCDC1138F56CCD10050` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `B327C24066269E0BE079B29613F30E47E0EE84EE7FEAAE453586CFCAD2188061` |
| `Shanmen.0_0_10` | 593 | 0 | 0 | `AC8811CE467BE7B6571D6797637EB3908476316293D54ADA3B0FEAA265A84EE9` |

最终选定阶段均为 native exit `0`、Fail `0`、terminal marker 有效；全量唯一用例 `593/593`。

## 4. 首次失败与修正

无源码、Automation、自检或构建失败。只读证据汇总命令曾出现两次 PowerShell table parser error 和一次数组参数绑定错误；修正命令表达式后通过，未触碰产品状态、未重复构建。

选定 `RunTests` 之前的既有 13 条启动 condition diagnostics 未计入本阶段结果；选定阶段错误数为 0。

## 5. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=17 Logs=10
SELF_TEST: PASS 194/194
TRANSACTION_SCAN: PASS
TRANSACTION_BYPASS_SCAN: PASS
CHANNEL_ORDER_SCAN: PASS Visual before Audio
PRESENTATION_BOUNDARY_SCAN: PASS
VALUE_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 6. 构建

- Editor initial：5 actions / 28.41s / exit `0`；
- Game final：4 actions / 23.24s / exit `0`；
- Editor final：0 actions / 0.89s / exit `0`；
- Editor DLL：13,387,776 bytes / SHA `27681DC28CDAF3DB72B8CC187394C9B675461F1FA3F02F001378B92F518C9E94`；
- Game EXE：354,994,176 bytes / SHA `2F0F2AD8FE8F666CE05CC6F49BD275DF93CAE61A7962E02C5804CE39637BE509`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+854 / -0`。host 是 caller-driven Run-local composition；两个 executor 仅是调用参数，两个 coordinator 的状态独立。production 文件不依赖 reflection、UObject、World、Actor、资产、timer、async/thread、RNG、damage、attribute 或 GAS application。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host/Docs/Report/Dev.D.UE.0.0.10.P12.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host/Docs/Log/Dev.D.UE.0.0.10.P12.16.r0_log.md>
