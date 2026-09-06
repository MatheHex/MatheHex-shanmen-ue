# Dev.D.UE.0.0.10.P20.52.r0 Report

## 1. 结论

P20.52 已将 P20.49 canonical recovery journal 与 P20.50 checkpoint payload envelope 合并为一个 generation-bound、digest-sealed 的 canonical recovery bundle。每份 bundle 同时携带精确 journal bytes 与 payload bytes，并从二者的完整字节、JournalId、EnvelopeId、generation 和 schema 派生 BundleDigest 与 BundleId。

bundle 只允许封装 journal 当前最新的 pending checkpoint。decode 后得到的仍是证据，不是恢复权限：调用方必须提供当前 journal 和可信最低 generation watermark，随后仍需由 P20.48 重新读取 live old/new surface 并核验 Owner、Host 与 Adapter 权限。已 committed、foreign 或低于 watermark 的历史 bundle 均不能导出可提交 checkpoint。

## 2. 基线、分支与改动范围

- 基线：`8e49374f6763f4d4496a0c74dc71fa290738da7e`（P20.51）；
- 分支：`agent/0.0.10-p20-52-thrown-weapon-arc-preview-recovery-bundle`；
- 新增 recovery bundle value object、decode result 与 canonical codec；
- 扩展 ProductLifecycle automation，新增 6 项 bundle 契约测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未修改 P20.49 journal、P20.50 payload、P20.51 storage 或 P20.48 recovery 的既有 schema/执行逻辑；
- 未新增文件 IO、网络 IO、surface mutation、scheduler、retry 或自动恢复入口。

## 3. Generation-bound 配对

bundle 只接受 valid、current、`CheckpointPending` journal 与 exact matching envelope。generation 不是任意调用方输入，而由 append-only journal 的形态确定：

- pending journal 的 record count 必须为奇数；
- latest record 必须为 `CheckpointPrepared`；
- latest sequence 必须等于 `record count - 1`；
- generation 固定为 `(record count + 1) / 2`；
- P20.49 的 16-record 上限因此对应 1–8 代。

同一 journal/envelope 重复创建会得到相同 generation、BundleDigest、BundleId 与完全相同的 canonical bytes。完成一代 recovery 并追加下一 checkpoint 后，generation、journal identity、bundle digest、bundle identity 与 bytes 全部改变。

## 4. Digest 与 identity

`BundleDigest` 覆盖：

- schema version 与 generation；
- JournalId 与 EnvelopeId；
- journal section 的精确长度和全部 bytes；
- payload section 的精确长度和全部 bytes。

`BundleId` 再由 schema、generation、JournalId、EnvelopeId 与 BundleDigest 派生。decoder 先验证 outer digest/identity，再分别运行 P20.49 与 P20.50 decoder，最后从解码语义重新创建 bundle 并要求 re-encode 与输入逐字节相同。

这使 header、任一 section、长度、顺序或 identity 的单 bit 改动都 fail closed。即使两个 section 各自合法，只要不是同一 pending checkpoint generation，pairing 仍被拒绝。

## 5. Canonical binary schema

current schema version 为 1，使用显式 big-endian 编码：

- magic：`SMARCBND`；
- header：92 bytes；
- header 保存 schema、declared total size、generation、两个 section size、BundleId、BundleDigest、JournalId 与 EnvelopeId；
- journal section 最大 3296 bytes；
- payload section 最大 65536 bytes；
- bundle 总上限 68924 bytes；
- journal section 必须是 P20.49 current schema，legacy schema 迁移结果必须先 canonical re-encode；
- payload section 必须是 P20.50 current schema。

decoder 区分 empty、magic、schema、total/section size、generation、digest、identity、nested journal、nested payload、pairing 与 non-canonical representation failure。

## 6. 当前 journal 与 rollback 围栏

`TryCopyPendingCheckpointEvidenceForJournal` 同时要求：

1. bundle 自身完整有效；
2. caller 提供 1–8 范围内的最低可信 generation；
3. bundle generation 不低于该 watermark；
4. caller 当前 journal 的 JournalId 与 record count 精确匹配 bundle 内 journal；
5. P20.50 envelope 仍能从该当前 journal unwrap。

一旦 caller journal 追加 recovery receipt，旧 bundle 仍是可解码的历史证据，但不能再导出 checkpoint。foreign journal 同样被拒绝。

最低 generation 必须来自 bundle 文件之外的可信调用方状态；若攻击者能同时回滚 bundle 与 watermark，本阶段不宣称 rollback protection。该限制在 API 和 P/F 边界中显式保留。

## 7. 权限与副作用边界

bundle 不接受 recovery coordinator、CompositionOwner、Host、Adapter 或 surface 参数。生产文件中没有 `Recovery.Execute`、`Show/Replace/Hide/ClearToEmpty/RetireForHandoff`、文件系统、World/Actor、gameplay damage、RNG、retry、sleep 或 scheduler 调用。

成功 decode 仅证明“同一代 journal 与 payload bytes 可被完整、规范地重建”。实际恢复仍必须由调用方显式复制 checkpoint evidence，并通过 P20.48 live authority gate；bundle codec 不创建 receipt、不 append journal、不自动恢复、不调度下一步。

## 8. 自动化验证

新增 6 项 focused automation：

1. `EvidenceContractAndGeneration`：pending 配对、generation 1、deterministic replay 与零副作用；
2. `CanonicalRoundTripAndCurrentJournalFence`：双 section round-trip、显式 P20.48 live gate 与 committed fence；
3. `CorruptionTruncationAndTrailing`：整份 bundle 每字节单 bit 翻转、全部 strict prefix 与 trailing byte 拒绝；
4. `HeaderAndSectionBounds`：empty、header、magic、schema、generation、section/total/max size 分类；
5. `ForeignPairAndCommittedFence`：合法但 foreign 的 journal/envelope 交叉配对拒绝；
6. `GenerationAdvanceAndRollbackFence`：generation 1→2、最低 watermark、invalid/future generation fail closed。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.52_Focused.log | Product...RecoveryBundle | 6/0 | 0 | 82.43 | 266,770 | `0365B6DB03F4B461F5B05DE4DEF35EDA35DBE5DF9E985439D9E870CA28928A81` |
| P20.52_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 18.16 | 303,920 | `B0036EA7ED9AD6F87659038D7B85CE9E196852E5C2A52D2C5C755C9EC4455D86` |
| P20.52_Full.log | Shanmen.0_0_10 | 1143/0 | 0 | 2674.61 | 1,671,937 | `90B88910AE11FF62E693BD1EE7437939C2E73CF1BF38838D47C940920934413E` |

三份正式日志累计 1195/0，每份均有唯一 UE 原生 `TEST COMPLETE. EXIT CODE: 0`。完整套件从 P20.51 的 1137 增至 1143。Focused/Legacy/Full 的非惩罚性长 Tick 提示分别为 6 / 1 / 126；Fatal/Unhandled/Ensure 分别为 0 / 0 / 0。

## 9. Changed-file、构建与产物

- mapping JSON：223 rules / parse PASS；
- mapping self-test：397/397；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=36 / Logs=3；
- bundle 边界：2 files / 766 physical lines；
- World/Actor/UObject/gameplay damage/RNG：0；
- file/network IO：0；
- recovery execute/surface mutation：0；
- retry/sleep/scheduler：0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors；
- initial Editor integration：5 actions / native 0 / 25.31 s；
- final Editor：0 actions / native 0 / 1.50 s；
- final Game：4 actions / native 0 / 34.32 s；
- Editor DLL：17,773,056 bytes / SHA-256 `4FDAA1E4372F48ACCAAE4C3AE1343FA2E28BDD0818715B366BD00B79C1068CE7`；
- Game EXE：358,756,352 bytes / SHA-256 `9D16500EA1B8B3BB7BCA2BCCF4A1B606268867362FEED5234702656D29A9F035`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：pending journal/payload exact pairing、generation 1–8、outer digest/identity、canonical current nested codecs、decode/re-encode byte equality、每字节 corruption、全部 strict-prefix truncation、trailing bytes、header/section bounds、foreign cross-pair rejection、current-journal committed fence、caller minimum-generation fence、P20.48 live authority gate、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：可信 watermark 的持久化、攻击者同时回滚 bundle 与 watermark、磁盘写入、atomic replace、fsync、process/machine restart、power-loss durability、concurrent writers、slot discovery、自动启动/恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.53 建立 caller-owned stable-lineage bundle storage：复用 P20.51 的 bounded temp-write/full-flush/single-replace 纪律，但以单一 bundle 文件替代分离 journal/payload 文件；Load 必须接收文件外的最低 generation watermark，且不得把同文件内字段冒充可信 anti-rollback anchor。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-52-thrown-weapon-arc-preview-recovery-bundle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-52-thrown-weapon-arc-preview-recovery-bundle/Docs/Report/Dev.D.UE.0.0.10.P20.52.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-52-thrown-weapon-arc-preview-recovery-bundle/Docs/Log/Dev.D.UE.0.0.10.P20.52.r0_log.md>
