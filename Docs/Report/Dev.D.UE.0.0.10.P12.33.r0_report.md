# Dev.D.UE.0.0.10.P12.33.r0 Report

## 1. 结论

P12.33 已完成并通过 P 阶段门禁。

本阶段为 P12.32 checkpoint envelope 增加 fixed-width canonical manifest codec。该 codec 只编码可独立验证的结构元数据，并以 caller-supplied envelope 做 candidate-bound verification；它不是 checkpoint payload serializer，不能脱离原 envelope 重建私有 lower-layer 状态，也不声明磁盘持久化或网络传输能力。

最终契约：

- schema `1` 的 canonical manifest 固定为 `48` bytes；
- layout 为 8-byte magic、big-endian schema、EnvelopeId、CheckpointId、big-endian record count；
- structural decode 可独立读取 manifest，未知 schema、错误长度、magic、无效 GUID 与越界 count fail closed；
- verified decode 只在 manifest 与 caller 提供的有效 envelope 四项元数据完全一致时返回该 envelope；
- record 顺序变化会经 CheckpointId 反映到 canonical bytes；
- codec 不拥有 Host/executor，不执行 command/retry，不做文件、网络、World 或 async 工作；
- focused `5/5`、直接父组 `10/10`，全量与最终构建数据见下文最终证据。

## 2. 功能性

### 2.1 Fixed-width canonical manifest

`TryEncode` 先清空输出并完整验证 P12.32 envelope，再按固定顺序写入：

1. ASCII magic `SMCEVM01`；
2. uint32 big-endian schema version；
3. EnvelopeId 的 A/B/C/D 四个 uint32 big-endian words；
4. CheckpointId 的 A/B/C/D 四个 uint32 big-endian words；
5. uint32 big-endian record count。

输出必须严格等于 `48` bytes，否则再次清空并失败。相同 envelope 重复编码得到逐字节相同结果。

### 2.2 Structural decode 与 candidate-bound verification

`TryDecode` 只解析 manifest 元数据，不尝试伪造 checkpoint payload。它要求 exact size、exact magic、当前 schema、有效双 GUID、非负且不超过 `MAX_int32` 的 record count，并要求消费全部 bytes；失败时输出重置为默认值。

`TryVerify` 将 manifest 的 schema、EnvelopeId、CheckpointId 与 record count 对 caller-supplied envelope 做 exact compare。`TryDecodeVerified` 先重置输出，再执行 parse + verify；成功时复制 caller 已有的有效 envelope。这样 metadata 不能冒充私有 checkpoint 状态，篡改或错误候选均 fail closed。

### 2.3 Validation-cost 修正

首次实现曾在 verified decode 中重复执行 unwrap、re-encode 与深层 `Matches`，在非空 checkpoint 上叠加既有递归 `IsValid` 成本。focused 首轮前四项成功、第五项持续 CPU-bound 且无断言失败，人工中断后把契约收敛为一次 expected-envelope validation 加四项元数据比较，并向 P12.32 envelope 增加只读 `GetCheckpointId()`。修正后不放宽身份校验，focused、父组和全量均使用最终实现重跑。

## 3. 完整性

新增五个 focused Automation contract：

1. `StableEmptyRoundTrip`：空 envelope manifest 稳定编码、解析、验证和 unwrap；
2. `MalformedAndUnsupportedFailClosed`：截断、扩展、错误 magic、未知 schema、零 EnvelopeId 均拒绝；
3. `CandidateMismatchAndTamperingFailClosed`：EnvelopeId、CheckpointId、count 篡改及错误候选不能通过 verified decode；
4. `RecordOrderChangesManifest`：AB/BA 有效 journal checkpoint 产生不同 manifest，交叉验证失败；
5. `VerifiedManifestPreservesLazyReplay`：verified envelope 经 unwrap/restore 后 exact replay 不调用 Host 或 executor。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythm product/effect-cue + prepared dispatch + transaction/Host + command + journal + checkpoint + envelope` 链。fake executor 仅生成既有 opaque receipt，不建立第二套执行权威。

## 4. 权威与兼容性边界

- P12.32 envelope 继续拥有 schema、checkpoint 值与 deterministic identity；本阶段只增加只读 CheckpointId getter；
- P12.31 checkpoint 继续拥有 ordered snapshot/identity 与 restore service；
- P12.30 journal、P12.29 command adapter 与更低 retry/Host/product authority 未修改；
- manifest 只拥有 schema、两个 identity 与 record count，不包含 checkpoint records 或可执行状态；
- structural decode 成功不等于 payload 恢复，必须与 caller 持有的完整 envelope verified match；
- production codec/envelope 的文件 IO、网络、World/UObject、async/scheduler、Host/executor、command execution 与 retry loop 命中均为 `0`；
- 本阶段没有文件格式、自动保存/加载、网络协议、后台任务、迁移执行器或 standalone reconstruction；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `...CheckpointEnvelopeManifestCodec.h/.cpp`：新增 48-byte canonical manifest、strict decode、candidate verification；
- `...CheckpointEnvelopeManifestCodecTests.cpp`：新增五个稳定性、破坏、顺序和 lazy replay 测试；
- `...CheckpointEnvelope.h`：增加只读 `GetCheckpointId()`，避免 codec 为读取 identity 执行 unwrap；
- `ShanmenRegressionMap.json`：新增 ManifestCodec 路径到 codec-to-runtime 34 groups 的映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 前六个代码/流程文件净变更 `+878 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournalCheckpointEnvelopeManifestCodec` | 5 | 0 | 0 | `164ACEB48AD365321767908C4A240753FFD3EA16E033B41D3D752251E0683BF9` |
| `RetryStepCommandJournalCheckpointEnvelope` | 10 | 0 | 0 | `849459DA469BB6D4D064D4D4B393B7EE43490C35DE7EF98B13C38185F7EEA7E1` |
| `Shanmen.0_0_10` | 679 | 0 | 0 | `5F0EEB6E43CD544DAD35ADA3B091A72A0D2BCCCFBFD44260481C3228C86D95B2` |

三份最终 selected-phase 日志均 Fail `0`、Fatal/Unhandled/Ensure `0` 并原生退出 `0`。父组由 P12.32 的 `5` 增加至 `10`，全量由 `674` 增加至 `679`。

本轮采用 focused + 直接父组 + 当前 full 三份最小充分证据；full 覆盖映射要求的所有 34 groups，不重复执行多层嵌套前缀。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=34 Logs=3
SELF_TEST: PASS 228/228
MANIFEST_CODEC_BOUNDARY_SCAN: PASS FileIO=0 Network=0 WorldObject=0 AsyncScheduler=0 HostExecutor=0 Execute=0 RetryLoop=0
AUTHORITY_SCAN: PASS P12.31/P12.30/P12.29/P12.28 and lower authority files unchanged
JSON_PARSE: PASS Rules=134
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 34.70s | 0 |
| Editor after validation-cost fix | Succeeded | 7 / 19.53s | 0 |
| Game final | Succeeded | 6 / 26.27s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.95s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：14,016,000 bytes，SHA-256 `3813F744E0AA53E8E1AAED7B4E268DA8D7A4F6366A0A3775A9F87BFA8F0EA0BF`；
- `demo_map.exe`：355,500,032 bytes，SHA-256 `3920C5AB4E1C04B6F5CA0241840C18BE17ADC17ED3035EDA6B5EAE23EC262B94`。

## 9. 流程与异常

首次 focused 运行的前四项为 Success、Fail `0`；第五项进入长时间 CPU-bound。该轮由开发端中断，原生退出 `1`，没有将其作为成功证据。代码审查定位为新 codec 对完整非空 envelope 重复执行多层深校验，而非产品断言或环境内存错误。移除重复 unwrap/re-encode/deep-match 后重新编译，最终 focused `5/5`。

全量回归中仍可见 UE/OnlineSubsystem 的 `google.com/generate_204` 3 秒探测超时与 controller `large delta` warning；selected tests 继续完成，controller 明确不惩罚对应无响应间隔。一次自检调用假定了不存在的系统级 PowerShell 7 路径，随后通过 `Get-Command pwsh` 使用 Codex bundled PS7，最终 self-test `228/228`；该调用未修改产品或证据。

raw Automation/build 日志仅本地保留，不纳入 Git。首次中断日志亦保留以供审计。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段纯值 manifest codec、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

P12.33 完成传输边界的结构清单与候选验证后，应停止继续叠加 wrapper。下一阶段应转入真实调用方的保存/读取集成设计或 F 阶段产品验证；若需要 standalone persistence，必须另行定义完整 checkpoint-record payload schema、迁移策略与安全上限，不能把本 manifest 误当 payload。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec/Docs/Report/Dev.D.UE.0.0.10.P12.33.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec/Docs/Log/Dev.D.UE.0.0.10.P12.33.r0_log.md>
