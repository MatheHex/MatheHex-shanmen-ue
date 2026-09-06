# Dev.D.UE.0.0.10.P20.55.r0 Report

## 1. 结论

P20.55 已为 P20.54 trusted watermark 与 P20.48 live recovery 之间建立显式 recovery admission/session。调用方必须指定 exact authority domain、stable lineage、current journal 与 pending checkpoint；Session 只在可信水位、canonical bundle、调用方当前 journal 与 checkpoint evidence 四者完全一致后，才把 checkpoint 交给一次 P20.48 recovery。

单次调用最多执行一次 authority Read、一次 P20.53 bundle Load 与一次 P20.48 Recovery.Execute。它不保存 bundle、不推进 watermark、不追加 journal、不修改 renderer surface、不重试、不调度，也不会因启动或发现磁盘文件而自动恢复。

## 2. 基线、分支与改动范围

- 基线：fdf6d07f1fb13c1a38dcec08837854db3dd72cf8（P20.54）；
- 分支：agent/0.0.10-p20-55-thrown-weapon-arc-preview-recovery-admission-session；
- 新增 deterministic explicit admission request、typed result 与 bounded Session；
- 扩展 ProductLifecycle automation，新增 8 项准入、证据、活体权限与重入测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Development Log；
- 未修改 P20.48 recovery、P20.49 journal、P20.50 envelope、P20.51 filesystem、P20.52 bundle、P20.53 storage 或 P20.54 authority 的既有生产语义；
- 未新增自动发现、自动启动、Save、CompareAndAdvance、journal append、surface mutation、scheduler、后台 retry 或平台 authority backend。

## 3. 显式请求与确定性身份

Admission request 绑定四项调用方选择：

- AuthorityDomainId；
- stable LineageId；
- ExpectedJournalId；
- ExpectedCheckpointId。

RequestId 使用固定 r1 namespace 和以上 canonical GUID parts 确定性派生，并在 IsValid 中重算。空 identity、任意 GUID、跨 lineage storage context 或请求内容与 RequestId 不一致均在任何 authority、filesystem 或 surface callback 前拒绝。

Session 没有无参数 Execute、目录扫描、latest-file 选择或自动启动入口；磁盘上存在 recovery evidence 本身不构成执行授权。

## 4. 有界准入顺序

ExecuteExplicit 固定执行以下步骤：

1. 验证 request、P20.53 storage context 与 requested lineage；
2. 拒绝同一 Session 的 callback re-entry；
3. 验证调用方 current journal 为 exact requested journal，且 latest record 为 pending checkpoint；
4. 对 caller-injected P20.54 authority 执行一次 Read；
5. 要求 current authority state 精确匹配 domain 与 lineage；
6. 以 authority generation 为 external minimum watermark 执行一次 P20.53 Load；
7. 要求 loaded generation、BundleId 与 derived lineage 精确匹配 trusted state；
8. 要求 loaded journal 与 caller current journal 的 identity、record count 完全一致；
9. 从 canonical bundle 解出 exact requested pending checkpoint；
10. 仅此时执行一次 P20.48 Recovery.Execute，由其重新读取 live Owner、Host、Adapter、Run、cursor 与旧/新 surface authority。

任何前置栅栏失败都不会调用后续层。TGuardValue 在 authority callback 前建立，并在所有返回路径自动释放；authority callback 对同一 Session 的递归调用得到 OperationInProgress，且不发生第二次 authority/storage/recovery callback。

## 5. 结果分类、幂等与活体权限

结果明确区分 RequestRejected、OperationInProgress、CurrentJournalRejected、AuthorityMissing、AuthorityReadRejected、AuthorityUnavailable、AuthorityStateRejected、BundleLoadRejected、BundleWatermarkMismatch、BundleJournalMismatch、CheckpointEvidenceRejected、RecoveryRejected、Recovered 与 Replayed。

Result 携带 request、authority read/state、bundle load status、loaded generation/BundleId/JournalId/CheckpointId、admission/invocation 标志、rehydrated checkpoint 与完整 P20.48 result。每个状态都按其允许出现的证据层级自验证，不能把半完成结果解释成成功。

同一显式请求重复调用时仍重新读取 authority 与 exact bundle，不依赖 Session 内缓存；若 Owner 已由同一 P20.48 receipt 恢复，则返回 Replayed。不同 live old/new surface、Owner/Host/Adapter/Run/cursor 漂移会在证据准入后由 P20.48 拒绝，并映射为 RecoveryRejected；准入成功不覆盖 live authority。

## 6. 崩溃与持久化边界

P20.55 只消费已提交 evidence，不产生新的持久化事实。它不会在恢复成功后把 P20.48 receipt 追加到 journal，也不会生成 terminal bundle 或推进下一代 watermark。

因此，本阶段保证“旧 pending evidence 只能经显式、可信、活体复核后执行”，但不声明恢复成功后的 durable completion closure。若进程在 Owner pointer commit 后、上层持久化 completion 前崩溃，下一次调用仍必须依据调用方重建的 live authority 和既有 pending evidence 作新一轮显式判断；不得把内存成功冒充持久完成。

## 7. 权限与副作用边界

两份生产文件合计 564 physical lines。生产调用点为 authority Read 1、bundle Load 1、Recovery.Execute 1；Save、CompareAndAdvance、AppendCheckpoint、AppendRecoveryReceipt 与 surface mutation 调用均为 0。

生产边界中没有 UWorld、AActor、UObject、ApplyDamage、RNG、ProjectDir、ProjectSavedDir、GameSavedDir、自动目录选择、sleep 或 retry loop。P20.48 只提交 Owner/Adapter 的已验证绑定并明确定义 DidMutateSurface=false；P20.55 不绕过或复制其 live authority 逻辑。

## 8. 自动化验证

新增 8 项 focused automation：

1. ContractAndDeterminism：四项 request identity 与 deterministic RequestId；
2. ExactAdmissionAndRecovery：exact authority/bundle/journal/checkpoint 后执行一次 P20.48；
3. IdempotentReplay：重复显式调用返回同 receipt replay，仍保持有界回读；
4. RequestAndCurrentJournalFences：invalid/cross-lineage request 与非当前 journal 在 callback 前拒绝；
5. AuthorityFences：missing/rejected/unavailable/malformed/foreign trusted state 分层关闭；
6. BundleAndWatermarkFences：missing/corrupt/oversized bytes 与同代 foreign BundleId 拒绝；
7. JournalAndCheckpointFences：trusted newer journal 与错误 checkpoint identity 不可替换 caller intent；
8. LiveAuthorityAndReentryFences：P20.48 live drift 拒绝，authority callback 递归被 Session guard 截断。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.55_Focused.log | Product...RecoveryAdmissionSession | 8/0 | 0 | 87.419 | 270,726 | 76EDDA804D76DF75B447579193E137F3AF3F9E7DFD566D7A7CF9F1509BA78CB6 |
| P20.55_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 1.548 | 303,789 | B0055F8E51DE401CA6DC54391B56960187C9912CCFE2D9CEFD9BEF747C3A61F5 |
| P20.55_Full.log | Shanmen.0_0_10 | 1167/0 | 0 | 3089.296 | 1,719,655 | 043BF14D70CB7C57D5379E1855858B107C751086F2EC540B2521868372E04FE5 |

三份正式日志累计 1221/0，每份均有唯一 UE 原生 TEST COMPLETE / EXIT CODE 0。完整套件从 P20.54 的 1159 增至 1167；P20.55 的 8 项测试在 focused 与 full 中各通过一次。各日志从首项项目测试开始后的 automation test errors、Fatal、Unhandled、Ensure 与 test-fail 均为 0。

## 9. Changed-file、构建与产物

- mapping JSON：226 rules / parse PASS；
- mapping self-test：403/403；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=40 / Logs=3；
- admission Session 边界：2 files / 564 physical lines；
- authority Read / bundle Load / recovery Execute 生产调用点：1 / 1 / 1；
- Save / CompareAndAdvance / journal append / surface mutation：0 / 0 / 0 / 0；
- World/Actor/UObject/gameplay damage/RNG/global path selection：0；
- focused test declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS，0 whitespace errors；
- initial production Editor integration：4 actions / native 0 / 16.04 s；
- test integration Editor：5 actions / native 0 / 15.30 s；
- final Editor：0 actions / native 0 / 1.87 s；
- final Game：4 actions / native 0 / 36.87 s；
- Editor DLL：17,991,680 bytes / SHA-256 989329616EA035B2FF621CE52681BDBB4096A8A44B4A8A60A33F8839F8840E18；
- Game EXE：358,924,288 bytes / SHA-256 5A25718D7582BE65DBB5B18581597606C2644A5C04E926E97716E1055086D106。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：deterministic explicit request、current pending journal preflight、caller-injected trusted authority read、exact watermark-bound bundle load、lineage/journal/checkpoint agreement、single P20.48 invocation、live authority recheck、idempotent replay、callback re-entry fence、zero surface mutation、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：恢复完成 receipt 的 durable append、terminal completion bundle/tombstone、post-recovery watermark advance、power-loss/process restart closure、concurrent callers/processes、真实 protected authority backend、remote consensus、自动启动/恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.56 建立显式 durable recovery completion 协议：只接受 P20.55 成功 result 与 exact P20.48 receipt，在不重复 surface recovery 的前提下生成可审计 terminal evidence，并固定 Owner pointer commit 与 completion persistence 之间的 crash/replay 语义；不得把 pending P20.53 bundle 原地伪装成已完成。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-55-thrown-weapon-arc-preview-recovery-admission-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-55-thrown-weapon-arc-preview-recovery-admission-session/Docs/Report/Dev.D.UE.0.0.10.P20.55.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-55-thrown-weapon-arc-preview-recovery-admission-session/Docs/Log/Dev.D.UE.0.0.10.P20.55.r0_log.md>
