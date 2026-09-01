# Dev.D.UE.0.0.10.P12.30.r0 Development Log

## 1. 目标

在 P12.29 immutable retry-step command/receipt adapter 外增加 caller-owned idempotency journal。journal 只记录有有效 receipt 的 command；exact replay 返回原 receipt；相同 CommandId 的不同 immutable command 冲突拒绝；新 command 仍只调用 P12.29 一次，不引入 retry episode、loop、scheduler、timer、queue 或后台 ownership。

## 2. 实现

- 新增 private ordered record array 与 CommandId-to-index map；
- record 自校验 adapter status 与 handled/decision-rejected/execution-rejected receipt 形态；
- journal 校验 record/index 数量、CommandId 唯一性与 exact index identity；
- exact replay 在 Host/executor 访问前返回原 receipt；
- same-id different-command 返回 `CommandReplayConflict`；
- 新 command 只调用一次 P12.29 adapter；
- adapter 无 receipt 时不入账，同一 command 可在正确 Host 后续执行；
- candidate-copy 验证通过后才原子提交新 record；
- journal 不持有 Host/executor 指针，不复制 budget/root/retryability/execution 权威。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournal` | 5 | 0 | 0 | `27D59B884C46466D16976B5EE9FBDAC4BFD9BBCEB42DD4B6ED5F0A7B53BAEE45` |
| `RetryStepCommand` | 10 | 0 | 0 | `36F64A51BD126C519B66A2D7A97071A3B00726FAC7B4566224B93F31782309F0` |
| `Shanmen.0_0_10` | 664 | 0 | 0 | `674F0995854464F36BC5FE946F124371AEC90A5D83AA1CD2E722EBFC28DB7BD7` |

三份最终日志均 Fail `0`、terminal `1`、Fatal/Unhandled/Ensure `0`。focused、父组与 full 分别覆盖新增 journal、P12.29 组合和当前代码全部 31 个 required groups。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=31 Logs=3
SELF_TEST: PASS 222/222
JOURNAL_BOUNDARY_SCAN: PASS P12_29_CALLS=1 RetryWhile=0 InfiniteFor=0 SchedulerTimerThread=0 Queue=0 WorldObject=0 HostExecutorPointer=0
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 24.72s / exit `0`；
- Game final：4 actions / 38.88s / exit `0`；
- Editor final：0 actions / 1.01s / exit `0`；
- Editor DLL：13,916,160 bytes / SHA `37471E5DD47ED4FF2CA68BAA4B2F4BF7DFAC0C7BA93E2832029C59A0AB34722A`；
- Game EXE：355,420,160 bytes / SHA `2646AA3D8CCB63A017EFE9AC89DB53B3D5185B57A51210D1D025AD4A7C90C24B`；
- 所有有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 compile、focused Automation 与最终 full 均成功；本轮无源码、断言或构建失败。

一个按旧惯例启动的 23 组逐层批次在首组 `RetryStepCommand 10/10` 完成后被主动终止，因为后续组会重复执行同一子树，且 full 当前代码证据能覆盖 mapping 的全部 required groups。Ctrl+C 令该冗余 wrapper 返回 `1`；未完成的 RetryStep 日志未用于门禁。最终采用三份最小充分日志并通过 `Required=31 / Logs=3` gate。

UE 的 generate_204 网络超时、EOS 配置更新和 large-delta warning 未造成测试失败；三份最终日志均有原生 terminal marker 和 exit `0`。raw Automation/build 日志仅本地保留。

## 7. 修改、兼容性与边界

Report/Log 前五个代码/流程文件净变更 `+945 / -0`。既有 P12.29 command adapter 与更低层 authority API 未改；新层仅追加 caller-owned receipt ledger。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal/Docs/Report/Dev.D.UE.0.0.10.P12.30.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal/Docs/Log/Dev.D.UE.0.0.10.P12.30.r0_log.md>
