# Dev.D.UE.0.0.10.P20.53.r0 Development Log

## 基线与目标

- base：79036d5479aa8e0bfa720d2ea347edc024d846e8；
- branch：agent/0.0.10-p20-53-thrown-weapon-arc-preview-recovery-bundle-storage；
- 目标：把 P20.52 canonical recovery bundle 接入 stable-lineage、caller-owned、单槽 atomic local storage；
- 权限边界：loaded evidence only，不自动执行 recovery、不 append journal、不触碰 surface。

## 设计判断

1. JournalId 随 append 改变，不能作为跨 generation 的稳定文件名。
2. 第一条 CheckpointPrepared record 是 lineage 内不可变事实，可用于派生 stable lineage ID。
3. 同一 lineage 只保存一个 current primary，generation advance 通过同槽 atomic replace 完成。
4. P20.51 的 filesystem seam 已经是 byte-oriented，可直接复用，不应再实现第二套文件 IO。
5. Save 必须先读取并验证既有 primary；未知、损坏或 foreign evidence 不能被静默覆盖。
6. 同 generation 的 identical canonical bytes 是幂等 no-op；不同 bytes 是 conflict。
7. MinimumGeneration 必须来自文件外；和 bundle 同文件保存的 watermark 不具备 rollback 信任价值。
8. atomic replace 后的 read-back failure 不能简单重试，必须由显式 Load 解析最终状态。
9. Load 只返回 evidence，P20.48 live authority gate 仍由调用方在执行恢复前完成。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.cpp。

核心契约：

- caller absolute root，最大 1024 characters；
- lineage ID = deterministic(first RecordId, CheckpointId, RunId, ConsumerDefinitionDigest)；
- directory = ShanmenArcPreviewRecoveryBundles；
- primary = <lineage-id>.smarc-bundle；
- temporary = primary + .tmp；
- MinimumGeneration 范围 1–8；
- P20.52 maximum canonical bundle = 68,924 bytes；
- P20.51 filesystem seam 与 full-flush local backend 复用；
- existing primary bounded read/decode/lineage/generation fence；
- temporary write + full flush + exact read-back + codec validation；
- one same-volume atomic replace；
- committed primary exact read-back + codec validation；
- no retry、no scheduler、no automatic recovery。

## 故障与幂等语义

Pre-commit：

- invalid context/bundle/lineage/watermark；
- existing read/size/decode/lineage/newer/conflict；
- directory/stale-temp/open/write/flush/temp-read/temp-validation；
- atomic replace failure。

以上结果均不宣称 primary 已改变；temporary 是否可能残留由 result 明示。

Post-commit：

- committed read-back failure；
- committed validation failure。

以上结果说明 replace 已发生但最终证据需要显式 Load 判断。AlreadyCurrent 只在相同 generation、相同 semantic bundle 且相同 canonical bytes 时成立，并保证不再次 write/replace。

## 自动化测试

新增 8 项：

- StableLineageContext；
- CanonicalSaveLoadAndReplay；
- GenerationAdvanceAndRollbackFence；
- PrecommitFailureAtomicity；
- ExistingPrimaryFence；
- PostcommitOutcomeRequiresLoad；
- LoadBoundsDecodeLineageAndWatermark；
- LocalFileAtomicGenerationReplace。

开发 focused：8/0，native 0。正式结果：

- focused：8/0，native 0，175.23 s，271,232 bytes，SHA-256 66C28D596F4A58314ACEE62F02A33D6A6FD9250C1E5CCC33066742626AB90537；
- legacy ItemUseAndArmor：46/0，native 0，15.86 s，303,002 bytes，SHA-256 A31EEE1C7FDC888E6327E72792D16340736AF3685C4B29AE5E635D03193B2DC9；
- full Shanmen.0_0_10：1151/0，native 0，2802.72 s，1,688,347 bytes，SHA-256 8D3B85292EFA8E583DE496C702A3104C8C77A1F11D919E39DEA6C09D29454FE7；
- formal total：1205/0；
- UE native TEST COMPLETE / EXIT CODE 0 markers：1 / 1 / 1；
- Fatal / Unhandled / Ensure / test-fail：0 / 0 / 0 / 0。

## Changed-file regression 与静态边界

- 新增 RecoveryBundleStorage exact-path mapping rule；
- RecoveryBundle、PayloadStorage、PayloadEnvelope 与 RecoveryJournal mapping 加入 storage focused group；
- mapping JSON：224 rules / parse PASS；
- mapping self-test：399/399；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=38 / Logs=3；
- storage files：2 / 770 physical lines；
- World/Actor/UObject/gameplay damage/RNG：0；
- recovery execute / exact surface mutation：0；
- retry/sleep/scheduler：0；
- global project path selection：0；
- network IO：0；
- focused declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS。

## 构建与产物

- initial Editor integration：5 actions / native 0 / 27.42 s；
- final Editor：0 actions / native 0 / 1.33 s；
- final Game：4 actions / native 0 / 23.76 s；
- Editor DLL：17,858,560 bytes / SHA-256 3BFA22C1CEF5D86B14455E20AFF22D8070E2824CC63A770E8F3530B4448A9BD9；
- Game EXE：358,819,328 bytes / SHA-256 4308411A0998E03EA5920817E5E5F201B8B4971506BF51A052ADE323670EA039。

## P/F 边界

PASS：stable lineage、single-slot canonical bundle storage、existing-primary fail-closed fence、generation advance、rollback watermark、pre/post-commit result classification、real local full-flush/atomic replace、changed-file regression、legacy 与 full suite。

未声明：trusted watermark persistence、simultaneous bundle/watermark rollback resistance、power-loss/restart durability、concurrent writers、cross-process lock、slot discovery、automatic recovery、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.54：建立与 bundle 文件分离的 caller-owned trusted watermark authority，提供单调 compare-and-advance、明确 receipt 与“bundle save before watermark advance”的崩溃顺序；仍不自动执行 recovery。
