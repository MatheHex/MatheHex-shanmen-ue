# Dev.D.UE.0.0.10.P12.33.r0 Development Log

## 1. 目标

为 P12.32 immutable checkpoint envelope 增加 fixed-width canonical manifest codec。manifest 必须稳定编码 schema、EnvelopeId、CheckpointId 与 record count，严格拒绝 malformed/unsupported 数据，并只在 caller 提供的完整 envelope exact match 时返回 verified value。本阶段不承诺 standalone payload reconstruction、文件/网络 IO、执行或自动 retry。

## 2. 实现

- 固定 magic `SMCEVM01`，schema `1`，encoded size `48` bytes；
- uint32 与 GUID words 统一 big-endian；
- `TryEncode`、`TryDecode`、`TryDecodeVerified` 均先重置输出；
- exact size/magic/current schema/valid GUID/bounded count/full consumption；
- manifest 可独立解析 metadata，但不携带 checkpoint records；
- verified decode 只复制 caller-supplied valid envelope，并 exact compare 四项 metadata；
- record 顺序由 lower checkpoint identity 绑定，AB/BA manifest 不同；
- P12.32 envelope 仅追加只读 CheckpointId getter；
- production 无 IO、network、World/UObject、Host/executor、execution、scheduler 或 retry loop。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournalCheckpointEnvelopeManifestCodec` | 5 | 0 | 0 | `164ACEB48AD365321767908C4A240753FFD3EA16E033B41D3D752251E0683BF9` |
| `RetryStepCommandJournalCheckpointEnvelope` | 10 | 0 | 0 | `849459DA469BB6D4D064D4D4B393B7EE43490C35DE7EF98B13C38185F7EEA7E1` |
| `Shanmen.0_0_10` | 679 | 0 | 0 | `5F0EEB6E43CD544DAD35ADA3B091A72A0D2BCCCFBFD44260481C3228C86D95B2` |

最终三份日志均 Fail `0`、Fatal/Unhandled/Ensure `0`、native exit `0`。focused、直接父组与 full 覆盖新增 codec、P12.32 envelope 组合和当前代码全部 34 个 required groups。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=34 Logs=3
SELF_TEST: PASS 228/228
MANIFEST_CODEC_BOUNDARY_SCAN: PASS FileIO=0 Network=0 WorldObject=0 AsyncScheduler=0 HostExecutor=0 Execute=0 RetryLoop=0
AUTHORITY_SCAN: PASS
JSON_PARSE: PASS Rules=134
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 34.70s / exit `0`；
- Editor after validation-cost fix：7 actions / 19.53s / exit `0`；
- Game final：6 actions / 26.27s / exit `0`；
- Editor final：0 actions / 0.95s / exit `0`；
- Editor DLL：14,016,000 bytes / SHA `3813F744E0AA53E8E1AAED7B4E268DA8D7A4F6366A0A3775A9F87BFA8F0EA0BF`；
- Game EXE：355,500,032 bytes / SHA `3920C5AB4E1C04B6F5CA0241840C18BE17ADC17ED3035EDA6B5EAE23EC262B94`。

## 6. 异常记录

首次 focused 运行前四项 Success、Fail `0`，第五项长时间 CPU-bound；人工中断，native exit `1`。根因是新 verified path 重复 unwrap、re-encode 与 deep `Matches`，放大完整非空 checkpoint 的递归 validation 成本。改为一次 expected-envelope validation 与 manifest metadata exact compare 后，重新编译和最终 focused 均成功；身份、schema、count 与 fail-closed 边界未放宽。

全量中的 `generate_204` timeout/large-delta 为既有 UE 联网探测噪声。一次自检调用使用了不存在的 conventional PS7 路径，随后发现并使用 bundled `pwsh`；最终结果 `228/228`。raw logs 只在本地保留。

## 7. 修改、兼容性与边界

Report/Log 前六个代码/流程文件净变更 `+878 / -0`。P12.31 checkpoint、P12.30 journal、P12.29 adapter 与更低 authority 未改。manifest 不是 standalone payload，不能伪称 persistence/load；它只为 caller-owned完整 envelope 提供 canonical metadata 与 candidate-bound verification。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。下一步停止 wrapper chain，转入调用方集成或 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec/Docs/Report/Dev.D.UE.0.0.10.P12.33.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-33-sword-rhythm-cue-retry-envelope-manifest-codec/Docs/Log/Dev.D.UE.0.0.10.P12.33.r0_log.md>
