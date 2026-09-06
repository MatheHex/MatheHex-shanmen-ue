# Dev.D.UE.0.0.10.P20.55.r0 Development Log

## 基线与目标

- base：fdf6d07f1fb13c1a38dcec08837854db3dd72cf8；
- branch：agent/0.0.10-p20-55-thrown-weapon-arc-preview-recovery-admission-session；
- 目标：在 P20.54 trusted watermark 与 P20.48 live recovery 之间建立 caller-explicit、exact-evidence recovery admission Session；
- 权限边界：一次 Read、一次 Load、最多一次 Recovery.Execute；不自动发现、保存、推进、追加、重试、调度或修改 surface。

## 设计判断

1. 磁盘存在 pending bundle 不等于调用方请求恢复，因此 request 必须显式绑定 domain、lineage、journal 与 checkpoint。
2. RequestId 由四项 canonical identity 确定性派生，并在 IsValid 中重算，不能接受任意 GUID。
3. current journal 是调用方当前事实；trusted bundle 不能仅因 generation 更新就替换调用方传入的 journal。
4. authority state 必须先于文件读取建立可信最低 generation，并与 exact BundleId 和 lineage 一致。
5. checkpoint 只能从已验证 bundle 解出，且必须同时匹配 request、loaded journal 与 current journal latest record。
6. evidence admission 不能代替 P20.48 live Owner/Host/Adapter/Run/cursor/dual-surface authority 检查。
7. 同一请求 replay 仍重新读取 authority 与 bundle，不引入 Session cache 或隐式长期状态。
8. callback re-entry 必须在 authority callback 前以 Session guard 拒绝，避免递归造成第二次读取或恢复。
9. P20.55 不持久化 completion；把恢复成功写入 durable terminal evidence 留给独立后续协议。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.cpp。

核心流程：

- request/context/current-journal preflight；
- operation guard；
- P20.54 Authority.Read exact domain + lineage；
- P20.53 Load at trusted generation；
- exact generation + BundleId + derived lineage；
- exact loaded/current journal identity + record count；
- exact pending checkpoint unwrap；
- one P20.48 Recovery.Execute；
- Recovered / Replayed / typed rejection result。

生产副作用计数：Read 1、Load 1、Recovery.Execute 1；Save 0、CompareAndAdvance 0、journal append 0、surface mutation 0、retry loop 0、scheduler 0。

## 拒绝、幂等与崩溃语义

Pre-callback：

- invalid request；
- request/context lineage 不一致；
- Session re-entry；
- invalid、foreign 或非 pending current journal。

Pre-recovery：

- missing/rejected/unavailable/malformed/foreign authority state；
- missing/read-failed/oversized/corrupt bundle；
- watermark generation、BundleId 或 lineage 不一致；
- loaded journal 与 caller current journal 不一致；
- requested checkpoint 与 canonical pending evidence 不一致。

Post-admission：

- P20.48 live authority 漂移映射为 RecoveryRejected；
- exact first execution 返回 Recovered；
- exact repeat execution返回 Replayed，并复用同一 P20.48 receipt identity；
- 所有路径由 result 自验证，Session guard 在返回后恢复。

本阶段不追加 recovery receipt，不生成 terminal bundle，不推进下一 generation。内存恢复成功与 durable completion 明确分离。

## 自动化测试

新增 8 项：

- ContractAndDeterminism；
- ExactAdmissionAndRecovery；
- IdempotentReplay；
- RequestAndCurrentJournalFences；
- AuthorityFences；
- BundleAndWatermarkFences；
- JournalAndCheckpointFences；
- LiveAuthorityAndReentryFences。

正式结果：

- focused：8/0，native 0，87.419 s，270,726 bytes，SHA-256 76EDDA804D76DF75B447579193E137F3AF3F9E7DFD566D7A7CF9F1509BA78CB6；
- legacy ItemUseAndArmor：46/0，native 0，1.548 s，303,789 bytes，SHA-256 B0055F8E51DE401CA6DC54391B56960187C9912CCFE2D9CEFD9BEF747C3A61F5；
- full Shanmen.0_0_10：1167/0，native 0，3089.296 s，1,719,655 bytes，SHA-256 043BF14D70CB7C57D5379E1855858B107C751086F2EC540B2521868372E04FE5；
- formal total：1221/0；
- UE native TEST COMPLETE / EXIT CODE 0 markers：1 / 1 / 1；
- post-test automation errors / Fatal / Unhandled / Ensure / test-fail：0 / 0 / 0 / 0 / 0。

## Changed-file regression 与静态边界

- 新增 RecoveryAdmissionSession exact-path mapping rule；
- WatermarkAuthority、BundleStorage、Bundle、PayloadStorage、PayloadEnvelope、RecoveryJournal 与 Recovery mapping 加入新 focused group；
- mapping JSON：226 rules / parse PASS；
- mapping self-test：403/403；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=40 / Logs=3；
- admission files：2 / 564 physical lines；
- authority Read / bundle Load / recovery Execute：1 / 1 / 1；
- Save / CompareAndAdvance / journal append / surface mutation：0 / 0 / 0 / 0；
- World/Actor/UObject/gameplay damage/RNG/global path selection：0；
- focused declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS。

## 构建与产物

- initial production Editor integration：4 actions / native 0 / 16.04 s；
- test integration Editor：5 actions / native 0 / 15.30 s；
- final Editor：0 actions / native 0 / 1.87 s；
- final Game：4 actions / native 0 / 36.87 s；
- Editor DLL：17,991,680 bytes / SHA-256 989329616EA035B2FF621CE52681BDBB4096A8A44B4A8A60A33F8839F8840E18；
- Game EXE：358,924,288 bytes / SHA-256 5A25718D7582BE65DBB5B18581597606C2644A5C04E926E97716E1055086D106。

## P/F 边界

PASS：deterministic explicit request、trusted authority read、watermark-bound canonical bundle load、caller current journal agreement、exact checkpoint admission、single live P20.48 recovery、idempotent replay、re-entry fence、zero renderer mutation、changed-file regression、legacy 与 full suite。

未声明：durable recovery completion、terminal evidence、post-recovery watermark advance、power-loss/process restart closure、concurrent processes、真实 protected authority backend、automatic recovery、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.56：建立显式 durable recovery completion/tombstone 协议，只接受 P20.55 success 与 exact P20.48 receipt；定义 pointer commit 后崩溃的重放闭包，且不得重复 surface recovery 或把 pending bundle 原地伪装为 terminal evidence。
