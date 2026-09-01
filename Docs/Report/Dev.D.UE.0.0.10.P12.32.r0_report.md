# Dev.D.UE.0.0.10.P12.32.r0 Report

## 1. 结论

P12.32 已完成并通过 P 阶段门禁。

本阶段在 P12.31 immutable ordered checkpoint 外增加 caller-owned、schema-versioned immutable value envelope。Envelope 只负责版本、checkpoint 值和 deterministic identity 的严格绑定；它不是 byte/file/network codec，也不接管 restore、执行或 retry 权威。

最终结果：

- 当前唯一支持 schema version `1`，未知版本 fail closed；
- EnvelopeId 由 schema version、CheckpointId 与 record count 在独立 namespace 下确定性派生；
- 同一 checkpoint 重复 wrap 得到相同 identity，交换有效 record 顺序后 identity 不同；
- 默认/无效 checkpoint、无效 envelope 与未知版本均拒绝，失败时输出重置为空值；
- unwrap 后的 checkpoint 可沿 P12.31 恢复惰性 replay，并允许一条 fresh command 经既有 P12.30/P12.29 链恰好执行一次；
- focused Envelope `5/5`、Checkpoint 父前缀 `10/10`、0.0.10 全量 `674/674`，Fail `0`；
- changed-path gate：`Changed=5 / Rules=1 / Required=33 / Logs=3`；
- regression gate self-test：`226/226`；
- 静态边界扫描、JSON parse、`git diff --check`、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Versioned immutable value envelope

`TryWrap` 只接受当前 schema 与有效 P12.31 checkpoint。成功后以独立 namespace `...CommandJournalCheckpointEnvelope.r1` 和以下 canonical parts 派生 EnvelopeId：

1. schema version；
2. checkpoint GUID（Digits 格式）；
3. record count。

Envelope fields 为 private，只公开 schema、identity、record count 和严格只读行为。`IsValid` 不信任已存 GUID：它会重新校验 checkpoint 并重算 EnvelopeId。`Matches` 同时要求双方有效、schema/identity 相同且 underlying checkpoint exact match。

### 2.2 Fail-closed wrap / unwrap

Wrap 与 unwrap 均先把 caller 输出重置为默认空值。未知 schema、无效 checkpoint 或被破坏的 envelope 不会留下旧输出。成功 unwrap 后再次验证复制出的 checkpoint，并要求它与 envelope 内 checkpoint exact match。

本阶段不公开可写 payload、不接受外部 identity 注入，也没有“只看版本号就通过”的兼容分支。schema `1` 是当前唯一受支持契约。

### 2.3 恢复后的 replay 与继续推进

测试使用真实 product chain 产生 P12.30 journal 与 P12.31 checkpoint，再执行 wrap → unwrap → restore：

- exact pending command replay 直接返回原 receipt，empty Host 与全新 fake executors 的 invocation count 保持 `0`；
- fresh command 使用既有 Host cursor 与 renewal budget，经 P12.30/P12.29 路径恰好执行一次；
- Envelope 本身不保存 Host cursor、executor、retry episode、budget 或 scheduler 状态。

## 3. 完整性

新增五个 focused Automation contract：

1. `EmptyCheckpointStableRoundTrip`：空 checkpoint 可稳定 wrap/unwrap，重复 wrap identity 相同；
2. `UnsupportedVersionAndInvalidCheckpointFailClosed`：未知版本与无效 checkpoint 被拒绝，输出保持默认；
3. `RecordOrderChangesEnvelopeIdentity`：AB/BA 两个有效 checkpoint 的 envelope identity 不同；
4. `UnwrappedCheckpointRestoresLazyReplay`：unwrap 后 restore 保持零 Host/executor 的 exact replay；
5. `RestoredJournalAcceptsOneFreshCommand`：恢复后的 journal 仍可接收并仅执行一次 fresh command。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host + P12.29 adapter + P12.30 journal + P12.31 checkpoint` 链。fake executors 只提供既有 opaque execution receipt，不建立第二套执行系统。

## 4. 权威与兼容性边界

- P12.31 checkpoint 继续拥有 ordered record snapshot/identity 与 restore service；
- P12.30 journal 继续拥有 record/index 与 exact replay 权威；
- P12.29 adapter 继续是 immutable command 到 P12.28 的唯一执行入口；
- P12.28 及更低 retry、Host、Router、Session 与 product authority 文件均未修改；
- Envelope 只拥有 schema + checkpoint value + deterministic identity，不拥有或调用 Host/executor；
- production Envelope 的 `TryExecute`、文件 IO、byte serialization、网络、World/Actor/UObject、async/scheduler 与 retry loop 命中均为 `0`；
- 本阶段没有 bytes、磁盘格式、网络格式、自动保存/加载、后台任务或迁移执行器；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `...CommandJournalCheckpointEnvelope.h/.cpp`：新增 schema `1` 的 immutable value envelope、deterministic identity 与严格 wrap/unwrap；
- `...CommandJournalCheckpointEnvelopeTests.cpp`：新增五个版本、身份、顺序、恢复 replay 与 fresh-command 测试；
- `ShanmenRegressionMap.json`：新增 Envelope 路径规则，要求当前 full 与 Envelope-to-runtime 33 个 groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前五个代码/流程文件净变更 `+687 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournalCheckpointEnvelope` | 5 | 0 | 0 | `CEE3B4AE8CF5271F6C996B8B717DC9B5DAFCB3D5CCF85FE9FAA76DC882CCA46E` |
| `RetryStepCommandJournalCheckpoint` | 10 | 0 | 0 | `C4EE8B69A1D92169E480593800F0B203914FDF76C54C3C94E0A765D85D8D6DBA` |
| `Shanmen.0_0_10` | 674 | 0 | 0 | `6ED94ABEFE2BC1F2FCDD3BF650E7ECA11FB1354406C0FB8539DF97182F4AA4CA` |

三份 selected-phase 日志均 Fail `0`、Fatal/Unhandled/Ensure `0` 并原生退出 `0`。Checkpoint 父前缀由 `5` 增加至 `10`，全量由 `669` 增加至 `674`。

本轮继续采用“focused + 直接父组 + 当前代码 full”三份最小充分证据；full log 直接覆盖映射要求的全部 33 个 groups，不重复运行大量嵌套父前缀。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=33 Logs=3
SELF_TEST: PASS 226/226
ENVELOPE_BOUNDARY_SCAN: PASS Execute=0 FileIO=0 ByteSerialization=0 Network=0 WorldObject=0 AsyncScheduler=0 HostExecutor=0 RetryLoop=0
AUTHORITY_SCAN: PASS P12.31/P12.30/P12.29/P12.28 and lower authority files unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 27.08s | 0 |
| Game final | Succeeded | 4 / 38.34s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.96s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,985,280 bytes，SHA-256 `903790D46EAC56C11ABBB9D9B99EC0BAE7EF323C3DD2D1508833FE76C881F9D4`；
- `demo_map.exe`：355,474,944 bytes，SHA-256 `1A00F69EFC77723D627EC1ABEA1751C0FE380CCE4BAECECA6259AB92EBEC596A`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 错误。

## 9. 流程与异常

首次 Editor compile、focused Automation、Checkpoint 父组、full Automation、Game 与 final Editor 均成功；本轮无源码、断言或构建失败。

UE selected phase 前仍存在 13 条既有 automation condition diagnostics。真实链测试中 `google.com/generate_204` 3 秒探测超时会触发约 15–103 秒 `large delta` warning；controller 明确记录不惩罚无响应测试，全部 selected tests 随后 Success 并原生退出 `0`。LinuxArm64/VisionOS SDK 提示不影响 Win64。

一次证据汇总误用 Windows PowerShell 5.1 运行采用现代 leading-pipe 语法的回归脚本，得到 `An empty pipe element is not allowed` parser error；改用项目既有 PowerShell 7 `pwsh` 后 self-test `226/226`。该错误未执行产品、未修改文件，也未作为验证证据。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned value envelope、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.33 在独立纯值层增加 canonical byte codec：只编码/解码 schema `1` envelope，并对截断、篡改、非 canonical 数据与未知版本 fail closed；继续禁止文件/网络 IO、自动加载、Host/executor ownership、command execution 与 retry scheduling。完成该最后一个传输边界后，应停止继续叠加 wrapper，转入调用方集成或 F 阶段产品验证。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-32-sword-rhythm-cue-retry-checkpoint-envelope>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-32-sword-rhythm-cue-retry-checkpoint-envelope/Docs/Report/Dev.D.UE.0.0.10.P12.32.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-32-sword-rhythm-cue-retry-checkpoint-envelope/Docs/Log/Dev.D.UE.0.0.10.P12.32.r0_log.md>
