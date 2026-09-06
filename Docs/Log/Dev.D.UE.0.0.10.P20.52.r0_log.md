# Dev.D.UE.0.0.10.P20.52.r0 Development Log

## 基线与目标

- base：`8e49374f6763f4d4496a0c74dc71fa290738da7e`；
- branch：`agent/0.0.10-p20-52-thrown-weapon-arc-preview-recovery-bundle`；
- 目标：把 P20.49 current journal bytes 与 P20.50 payload bytes 绑定为一个 canonical、generation-aware recovery bundle；
- 权限边界：evidence only，不自动执行恢复、不 append journal、不触碰 surface。

## 设计判断

1. P20.51 只原子保存 payload，process restart 仍缺 journal evidence。
2. journal 与 payload 若分成两个独立文件，会产生跨文件 torn-pair 状态。
3. P20.52 先建立单文件可承载的纯 bundle codec，不在本阶段连接文件系统。
4. generation 从 append-only journal 形态派生，禁止 caller 任意声明。
5. outer digest 覆盖 header pairing facts 与两个 section 的完整 bytes。
6. outer BundleId 再绑定 digest，避免 identity 与内容脱节。
7. decoder 必须再次运行两个 nested codec，并要求整体 canonical re-encode 相等。
8. loaded bundle 必须匹配 caller 当前 journal 和外部最低 generation watermark。
9. watermark 若与 bundle 一起回滚则不可信，本阶段不宣称解决该威胁。

## 实现记录

新增：

- `demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.h`；
- `demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.cpp`。

核心契约：

- only `CheckpointPending` journal；
- record count 必须为奇数，latest kind/sequence 必须匹配；
- generation = `(record count + 1) / 2`，范围 1–8；
- deterministic BundleDigest 与 BundleId；
- 92-byte big-endian header；
- journal max 3296 bytes；
- payload max 65536 bytes；
- bundle max 68924 bytes；
- current journal schema only；
- exact JournalId/EnvelopeId header binding；
- nested decode + semantic pairing + re-encode equality；
- current journal、record count 与 minimum-generation extraction fence。

Decode status 分类：

- input/magic/schema/size；
- generation/section bounds；
- bundle digest/identity；
- journal/payload nested decode；
- journal/envelope header identity；
- pair rejection；
- non-canonical representation。

## 自动化测试

新增 6 项：

- `EvidenceContractAndGeneration`；
- `CanonicalRoundTripAndCurrentJournalFence`；
- `CorruptionTruncationAndTrailing`；
- `HeaderAndSectionBounds`；
- `ForeignPairAndCommittedFence`；
- `GenerationAdvanceAndRollbackFence`。

开发 focused：6/0，native 0。正式结果：

- focused：6/0，native 0，82.43 s，266,770 bytes，SHA-256 `0365B6DB03F4B461F5B05DE4DEF35EDA35DBE5DF9E985439D9E870CA28928A81`；
- legacy ItemUseAndArmor：46/0，native 0，18.16 s，303,920 bytes，SHA-256 `B0036EA7ED9AD6F87659038D7B85CE9E196852E5C2A52D2C5C755C9EC4455D86`；
- full Shanmen.0_0_10：1143/0，native 0，2674.61 s，1,671,937 bytes，SHA-256 `90B88910AE11FF62E693BD1EE7437939C2E73CF1BF38838D47C940920934413E`；
- formal total：1195/0；
- terminal success markers：1 / 1 / 1；
- focused/legacy/full non-penalty notices：6 / 1 / 126；
- Fatal/Unhandled/Ensure：0 / 0 / 0。

完整 suite 的长耗时集中在既有 SwordRhythm retry/replay 用例，日志持续前进并最终原生退出 0；P20.52 的 6 项测试在 full 中再次全部通过。

## Changed-file regression

- 新增 RecoveryBundle exact-path mapping rule；
- PayloadEnvelope 与 RecoveryJournal rule 增加 bundle focused group；
- mapping JSON：223 rules / parse PASS；
- mapping self-test：397/397；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=36 / Logs=3。

## 静态边界

- bundle files：2 / 766 physical lines；
- World/Actor/UObject/gameplay damage/RNG：0；
- file/network IO：0；
- recovery execute/surface mutation：0；
- retry/sleep/scheduler：0；
- focused test declarations：6；
- `git diff --check`：PASS。

## 构建与产物

- initial Editor integration：5 actions / native 0 / 25.31 s；
- final Editor：0 actions / native 0 / 1.50 s；
- final Game：4 actions / native 0 / 34.32 s；
- Editor DLL：17,773,056 bytes / SHA-256 `4FDAA1E4372F48ACCAAE4C3AE1343FA2E28BDD0818715B366BD00B79C1068CE7`；
- Game EXE：358,756,352 bytes / SHA-256 `9D16500EA1B8B3BB7BCA2BCCF4A1B606268867362FEED5234702656D29A9F035`。

## P/F 边界

PASS：journal/payload one-generation pairing、deterministic digest/id、bounded canonical schema、nested codec revalidation、exact round-trip、corruption/truncation/trailing rejection、foreign pair rejection、generation advance、external minimum watermark、committed current-journal fence、P20.48 live recovery gate、changed-file regression 与完整 suite。

未声明：trusted watermark storage、simultaneous bundle/watermark rollback resistance、磁盘原子写、restart durability、concurrency、自动 recovery、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.53：建立 stable-lineage、caller-owned 的 atomic bundle storage。单一 bundle 文件复用 P20.51 的 temp/full-flush/read-back/single-replace 纪律；Load 必须携带文件外最低 generation watermark，storage 仍不得执行 recovery。
