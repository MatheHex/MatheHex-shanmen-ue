# Dev.D.UE.0.0.10.P12.31.r0 Report

## 1. 结论

P12.31 已完成并通过 P 阶段门禁。

本阶段在 P12.30 caller-owned retry-command journal 外增加 immutable ordered checkpoint 与空 journal restore 边界。checkpoint 只保存已有 journal record 的有序值副本，不保存或信任持久化索引；restore 从 records 重建 CommandId 索引并在 candidate journal 全量自校验后一次提交。

最终结果：

- 同一有序 record 序列导出稳定、可比较的 deterministic checkpoint identity；
- record 顺序参与 identity，交换有效 records 后 checkpoint identity 必然不同；
- 无效 record、重复 CommandId 或 identity/内容不一致均 fail closed；
- restore 只接受有效且为空的目标 journal，拒绝覆盖既有记录；
- restore 后 exact command replay 直接返回原 receipt，Host/executor 调用为 0；
- restore 后 journal 仍可通过 P12.30/P12.29 接受一条 fresh command，并恰好执行一次；
- focused checkpoint `5/5`、Journal 父前缀 `10/10`、0.0.10 全量 `669/669`，Fail `0`；
- changed-path gate：`Changed=6 / Rules=2 / Required=32 / Logs=3`；
- regression gate self-test：`224/224`；
- 静态边界扫描、`git diff --check`、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable ordered checkpoint

`TryCreate` 先验证每条 P12.30 record 及 CommandId 唯一性，再以以下 canonical parts 派生 checkpoint GUID：

1. record count；
2. 每条 record 的 CommandId；
3. 对应 P12.29 ReceiptId；
4. 对应 adapter status。

parts 按 record 插入顺序编码，并使用独立 namespace `...CommandJournalCheckpoint.r1`。因此相同内容与顺序可重复得到相同 identity；顺序、receipt 或 status 变化都会改变 identity。`IsValid` 会重新派生并比对 identity，不能只凭外部 GUID 把任意内容伪装成 checkpoint。

checkpoint fields 为 private，只公开只读 identity/count、按序 record copy 和 exact `Matches`。空 journal 也可形成稳定的有效 checkpoint，用于统一 caller 流程。

### 2.2 Export 与 restore

Export 只接受内部一致的 P12.30 journal，并复制其 private ordered records。P12.30 journal 只增加一个 checkpoint service friend seam；既有 record/index API 与执行路径未改变。

Restore 顺序固定为：

1. 校验 checkpoint identity 与全部 records；
2. 校验目标 journal 自身一致；
3. 要求目标 record count 为 0；
4. 只从 ordered records 新建 candidate array 和 CommandId index；
5. 调用既有 journal `IsValid` 校验 record/index 精确对应；
6. candidate 完整通过后 move-commit。

checkpoint 本身不包含可被信任的 map/index。restore 不执行 command、不访问 Host/executor，也不覆盖非空目标。

### 2.3 恢复后的幂等与继续推进

测试先让真实 product chain 产生 RetryPending command receipt，导出并恢复 journal。随后把同一 command 提交给 empty Host 与全新 fake executors：恢复后的 journal 在外部访问前返回原 ReceiptId，两个 executor invocation count 均保持 0。

调用方再使用原 Host 的新 cursor 与 receipt 中的 `NextRenewalsUsed` 捕获 fresh CommandId；恢复后的 journal 通过既有 P12.30/P12.29 路径执行一次并完成。这证明 checkpoint 只恢复幂等账本，不复制 Host cursor、budget、retryability 或 execution 权威。

## 3. 完整性

新增五个 focused Automation contract：

1. `EmptyJournalRoundTrips`：空 journal identity 稳定，可恢复为空且有效的 journal；
2. `RestoredReplayAndFreshCommand`：旧 command 零依赖重放，fresh command 经既有链恰好推进一次；
3. `RejectedReceiptsRoundTrip`：decision rejection 与 durable execution rejection 均保持 status、ReceiptId 与惰性重放；
4. `MalformedAndDuplicateRecordsFailClosed`：默认 record、重复 CommandId、无效 checkpoint 均被拒绝，目标不变；
5. `OrderIdentityAndNonEmptyFence`：AB 与 BA identity 不同，restore 保持索引可查，非空目标拒绝覆盖。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host + P12.29 adapter + P12.30 journal` 链。fake executor 只产生既有 opaque receipt、retryable failure 或显式 rejection。

## 4. 权威与兼容性边界

- P12.30 journal 继续拥有 record/index 与 exact replay 权威；
- P12.29 adapter 继续是 immutable command 到 P12.28 的唯一执行入口；
- P12.28 retry step、P12.27 decision、P12.26 prepared retry 与 P12.21 Host/Router/Session 权威均未修改；
- checkpoint 只拥有 ordered value snapshot/identity；不拥有 Host/executor、retry episode、budget、timer、scheduler、queue、thread 或 background task；
- production checkpoint 对 P12.29 adapter 和任何 `TryExecute` 的调用数均为 0；
- 无文件 IO、序列化实现、World/Actor/UObject、资产、RNG、damage、attribute 或 gameplay-effect application；
- restore 中的有限 record 遍历只重建索引，不是 retry loop；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `...CommandJournal.h`：仅增加 checkpoint service friend seam；
- `...CommandJournalCheckpoint.h/.cpp`：新增 immutable checkpoint、状态/result 与 export/restore service；
- `...CommandJournalCheckpointTests.cpp`：新增五个身份、恢复、重放、拒绝与覆盖栅栏测试；
- `ShanmenRegressionMap.json`：新增 checkpoint 路径规则，与 journal header 规则合并要求 32 个 groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前六个代码/流程文件净变更 `+939 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournalCheckpoint` | 5 | 0 | 0 | `A3C5992217EA6D7E597E09AB9DDC29B7B2569EC4BC9AEE1E3313E308C1F44501` |
| `RetryStepCommandJournal` | 10 | 0 | 0 | `585E09590030C6F49162CBFD0217B3EBA028C8A6C7936648A776917C18341621` |
| `Shanmen.0_0_10` | 669 | 0 | 0 | `139AAE383B9085B91AFD89A4CD44BD963BEF479158B701C367FDDAC4372B290C` |

三份 selected-phase 日志均有唯一 `RunTests` group、唯一 terminal marker、Fail `0`、Fatal/Unhandled/Ensure `0`。Journal 父前缀由 `5` 增加至 `10`，全量由 `664` 增加至 `669`。

本轮继续采用“focused + 直接父组 + 当前代码 full”三份最小充分证据；full log 直接覆盖映射要求的全部 32 个 groups，不重复启动 20 余个嵌套父前缀进程。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=32 Logs=3
SELF_TEST: PASS 224/224
CHECKPOINT_BOUNDARY_SCAN: PASS ExecuteCalls=0 P12_29AdapterCalls=0 RetryWhile=0 InfiniteFor=0 FileIO=0 WorldObject=0 AsyncScheduler=0 HostExecutorMembers=0
AUTHORITY_SCAN: PASS P12.29/P12.28/P12.27/P12.26/ProductSession/GameMode unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 7 / 21.32s | 0 |
| Game final | Succeeded | 6 / 33.28s | 0 |
| Editor final | Succeeded, up to date | 0 / 1.09s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,957,632 bytes，SHA-256 `4E68A9F3FF19464D70393FE9F6253E40070E8ABAD76322895B2DE3AE817E679E`；
- `demo_map.exe`：355,453,952 bytes，SHA-256 `E69011D4D99CC4428D72229735B189A0B15282F0B762742F3B592DB1B32200D7`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 错误。

## 9. 流程与异常

首次 Editor compile、首次 focused Automation、Journal 父组、full Automation、Game 与 final Editor 均成功；本轮无源码、断言或构建失败。

UE selected phase 前仍存在 13 条既有 automation condition diagnostics。真实链测试中 `google.com/generate_204` 3 秒探测超时会触发约 15–103 秒 `large delta` warning；controller 明确记录“不惩罚无响应测试”，全部测试随后写入 Success 与 terminal marker 并原生退出 `0`。LinuxArm64/VisionOS SDK 提示不影响 Win64。

汇总证据时一条本地 PowerShell one-liner 因空 pipe 语法返回 parser error；修正命令后门禁原生通过。该命令未运行产品、未修改文件，也未作为验证证据。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned checkpoint value、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.32 增加纯值、带 schema version 的 canonical checkpoint payload codec：只负责 immutable checkpoint 与 caller-owned bytes/value envelope 的严格 round-trip、截断/篡改/未知版本拒绝；仍不负责文件或网络 IO、自动加载、Host/executor ownership、command execution 或 retry scheduling。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-31-sword-rhythm-cue-retry-checkpoint>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-31-sword-rhythm-cue-retry-checkpoint/Docs/Report/Dev.D.UE.0.0.10.P12.31.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-31-sword-rhythm-cue-retry-checkpoint/Docs/Log/Dev.D.UE.0.0.10.P12.31.r0_log.md>
