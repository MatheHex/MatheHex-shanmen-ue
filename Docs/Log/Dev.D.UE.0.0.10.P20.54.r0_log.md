# Dev.D.UE.0.0.10.P20.54.r0 Development Log

## 基线与目标

- base：9e78fe097668175af924e157b1c1d2c8020a2c8a；
- branch：agent/0.0.10-p20-54-thrown-weapon-arc-preview-recovery-watermark-authority；
- 目标：为 P20.53 canonical recovery bundle 增加 caller-injected trusted minimum-generation authority，并固定 bundle-before-watermark 崩溃顺序；
- 权限边界：evidence commit only，不执行 recovery、不触碰 surface、不自动重试。

## 设计判断

1. 与 bundle 同盘的普通 watermark 文件可被一起回滚，不具备独立信任价值，因此本阶段只定义 authority interface，不伪造 file backend。
2. authority state 必须绑定 domain、stable lineage、generation、exact BundleId 和完整 last receipt，而不能只有一个整数。
3. request/receipt identity 必须由 canonical parts 确定性派生，并在 IsValid/Matches 中重算。
4. generation 0 表示 missing；target 为 1–8 且严格大于 expected。
5. coordinator 必须先读 authority，再用可信 generation 调用 P20.53 Save，并显式 Load exact same bundle。
6. CompareAndAdvance 在 exact bundle 可读之前绝不能发生，且单次 coordinator 调用最多一次。
7. OutcomeUnknown 只允许一次 Read 复查；其它失败不得隐式重试。
8. 同 generation 同 bundle 的 replay 仍要先验证磁盘 exact evidence，再返回 AlreadyCommitted。
9. 加载或提交 evidence 不等于执行 recovery；P20.48 live authority gate 保持不变。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.cpp。

核心契约：

- deterministic advance request：AuthorityDomainId + LineageId + ExpectedGeneration + TargetGeneration + BundleId；
- deterministic audit receipt：完整绑定 request 与 previous/committed generation；
- current state：保存 exact last receipt；
- Read：Missing / Current / Rejected / Unavailable；
- CompareAndAdvance：Advanced / AlreadyCurrent / Conflict / Rejected / Unavailable / OutcomeUnknown；
- commit：authority read → P20.53 Save → exact P20.53 Load → one CAS → optional one re-read；
- ordinary-file watermark backend：0；
- automatic retry / scheduler / recovery execution：0 / 0 / 0。

## 故障、幂等与崩溃语义

Pre-storage：

- invalid domain/context/bundle；
- foreign lineage；
- malformed/rejected/unavailable authority read；
- authority ahead of candidate；
- same generation bound to another BundleId。

这些结果不调用 storage Save/Load 或 authority advance。

Post-bundle verification：

- authority Advanced/AlreadyCurrent 必须返回精确匹配 request 的 receipt/state；
- Conflict 与 Rejected 明确失败；
- Unavailable 返回 BundleCommittedWatermarkPending；
- OutcomeUnknown 只进行一次 exact authority re-read；
- re-read 匹配则 CommittedAfterAuthorityRecheck，否则 WatermarkOutcomeUnresolved。

崩溃发生在 bundle commit 与 watermark advance 之间时，watermark 留在保守旧代；调用方后续显式重试会通过 P20.53 AlreadyCurrent + exact Load 后继续 CAS。coordinator 内部没有 retry loop。

## 自动化测试

新增 8 项：

- ContractAndDeterminism；
- BundleBeforeWatermarkAndReplay；
- GenerationAdvanceAndRollbackFence；
- PrecommitFailureNeverAdvances；
- PostcommitVerificationAndRetry；
- AuthorityFailureLeavesRetryableBundle；
- UnknownOutcomeBoundedRecheck；
- InputAuthorityAndConflictFences。

正式结果：

- focused：8/0，native 0，141.74 s，270,425 bytes，SHA-256 5994F938FFA422B5AF3BC611EAFF414A2599B3B180B8EFD61E7A443FDB1D9C05；
- legacy ItemUseAndArmor：46/0，native 0，1.50 s，303,487 bytes，SHA-256 AE4D80F5C079B4BA28C549993837554FA7FDD5C8BB57A6AA439071461AFC9FB3；
- full Shanmen.0_0_10：1159/0，native 0，3000.77 s，1,704,882 bytes，SHA-256 2572CDE7277EBAFB875CB6E6E88E0CE0040B69CD5CE60F12F5EC1F729EADB0F8；
- formal total：1213/0；
- UE native TEST COMPLETE / EXIT CODE 0 markers：1 / 1 / 1；
- post-test automation errors / Fatal / Unhandled / Ensure / test-fail：0 / 0 / 0 / 0 / 0。

## Changed-file regression 与静态边界

- 新增 RecoveryBundleWatermarkAuthority exact-path mapping rule；
- RecoveryBundleStorage、RecoveryBundle、PayloadStorage、PayloadEnvelope 与 RecoveryJournal mapping 加入新 focused group；
- mapping JSON：225 rules / parse PASS；
- mapping self-test：401/401；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=39 / Logs=3；
- authority files：2 / 1,015 physical lines；
- CompareAndAdvance / Read production call sites：1 / 2；
- World/Actor/UObject/gameplay damage/RNG：0；
- recovery execute / exact surface mutation：0；
- project-global path selection：0；
- ordinary-file backend / network client：0 / 0；
- focused declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS。

## 构建与产物

- initial production Editor integration：4 actions / native 0 / 20.17 s；
- test integration Editor：4 actions / native 0 / 13.94 s；
- final Editor：0 actions / native 0 / 1.80 s；
- final Game：4 actions / native 0 / 25.03 s；
- Editor DLL：17,940,480 bytes / SHA-256 A3C129848F7FA75717C968255827DBDA62BEE6A15B8C8C0B1F5DE47D0A75D9BE；
- Game EXE：358,879,744 bytes / SHA-256 A0B494BD411E123F80151E5098029407617695C4B4814C6962BE7B9A289CDC2F。

## P/F 边界

PASS：deterministic request/receipt/state、caller-owned authority interface、generation 0–8 monotonic CAS、exact bundle binding、bundle-before-watermark ordering、bounded unknown-outcome recheck、pending/retry semantics、changed-file regression、legacy 与 full suite。

未声明：真实 protected authority backend、simultaneous bundle/watermark rollback resistance、power-loss/restart durability、concurrent writers、cross-process lock、remote consensus、automatic recovery、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.55：建立显式 recovery admission/session，以 caller-injected trusted watermark 选择 exact bundle，再以当前 journal 与 P20.48 live authority 进行最终授权；只有调用方明确请求时才把 checkpoint evidence 交给 recovery coordinator。真实受保护 authority backend 保持为独立平台 adapter，不得退化为普通同盘文件。
