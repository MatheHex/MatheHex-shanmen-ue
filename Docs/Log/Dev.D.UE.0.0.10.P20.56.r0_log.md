# Dev.D.UE.0.0.10.P20.56.r0 Development Log

## 基线与目标

- base：73b9db8aedaeb2aa14b898d498bfbecf0e8b0f8a；
- branch：agent/0.0.10-p20-56-thrown-weapon-arc-preview-recovery-completion；
- 目标：把 P20.55 的 live recovery success 封闭为独立、可验证、可重放的 durable terminal evidence；
- 权限边界：caller-explicit request/root/domain；不扫描目录、不覆盖 pending bundle、不自动启动、不循环重试、不调度。

## 设计判断

1. P20.53 pending bundle 是“允许尝试恢复”的证据，不能原地改名为“恢复已完成”。
2. Completion 必须有独立 authority domain、storage slot、codec、digest 与 identity。
3. Terminal journal 只能由 source pending journal 加 exact P20.48 receipt 得到，不能由调用方直接提供任意已完成 journal。
4. Completion bytes 必须先 durable save 并 read-back 验证，再推进可信水位。
5. Trusted completion replay 必须在 P20.55 与 renderer surface callbacks 前返回，才能保证重启后不重复 live recovery。
6. Save 成功而 authority 不可用是可修复的中间状态，不是失败回滚，也不是完整成功。
7. CompareAndAdvance unknown outcome 只允许一次 exact re-read；Session 内不得出现重试循环。
8. completion generation 与 pending generation 对齐，但两个 authority domain 永不混用。
9. pending evidence 与 completion evidence 都保留，下一代采纳/轮换留给显式后续协议。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.cpp。

核心对象：

- deterministic CompletionRequest；
- source/terminal journal、source BundleId、ReceiptId、generation、digest 与 CompletionId 绑定的 Completion；
- bounded big-endian CompletionCodec；
- ShanmenArcPreviewRecoveryCompletions/<lineage>.smarc-completion 独立 storage context/adapter；
- typed CompletionSessionResult；
- caller-explicit CompletionSession。

完成路径：completion authority read → trusted replay fast path；否则 P20.55 admission → receipt append to journal copy → completion create/save/read-back → completion CAS advance → unknown-only exact re-read。

## 崩溃、重放与拒绝语义

- save 前失败：不推进 authority；
- save 后 authority unavailable：保留 durable completion，返回 authority pending；
- retry：identical completion accepted，P20.55/P20.48 receipt replay，不重复 surface commit；
- advance 响应未知：一次 exact authority re-read 收敛或返回 unresolved；
- trusted completion current：load/verify 后直接 Replayed；
- ahead/rollback/foreign/tampered/cross-lineage/conflict/re-entry：失败关闭；
- 所有 success 与中间状态由 Result.Validate 根据已出现证据层级自检。

## 自动化测试

新增 9 项：RequestContractAndDeterminism、ExactDurableCompletion、CanonicalCodecAndTamperFence、TrustedReplaySkipsRecovery、DurableEvidenceBeforeAuthorityRepair、UnknownAuthorityOutcomeClosure、TrustedRollbackAndConflictFences、InputAuthorityAndReentryFences、PersistenceAndAdvanceFailureFences。

正式结果：

- focused：9/0，native 0，121.256 s，282,720 bytes，SHA-256 472E8AE9EF3701F640B7C86D27854C2AB2CAF543E509F3972D77657EC2489C1E；
- legacy ItemUseAndArmor：46/0，native 0，1.700 s，310,309 bytes，SHA-256 BB01BAD828D4B6C247D20FBF996EDCA7CA31B6B3B234AFD4B5EEE1D354D135A8；
- full Shanmen.0_0_10：1176/0，native 0，3179.072 s，1,778,920 bytes，SHA-256 B6FA6F0EAB51634A55409EFDF9FFCF3EDAC80CDA99324248B766EB1A586D8C16；
- formal total：1231/0；
- unique queue-empty/TestExit markers：1 / 1 / 1；
- automation errors / Fatal / Unhandled / Ensure / test-fail：0 / 0 / 0 / 0 / 0。

首次 legacy attempt 在 45 success 后、TerminalCleanup 已开始但尚未完成时提前原生退出 0；因无 completion/queue-empty marker，明确判为 incomplete 并保留原始日志：308,470 bytes，SHA-256 8CAFA920FE03235DF53670C4DBFE1D3981DA0787DC1D8E8224C806B3E2AFABD6。TerminalCleanup 单项 probe 1/0，随后正式整组重跑 46/0。

## Changed-file regression 与静态边界

- mapping JSON：227 rules / parse PASS；
- 新增 CompletionSession exact rule，并把其 focused group 回接 Admission、WatermarkAuthority、BundleStorage、Bundle、PayloadStorage、PayloadEnvelope、Journal 与 Recovery；
- mapping self-test：405/405；
- focused-only gate：按预期因 40 个依赖组缺失而拒绝；
- final gate：PASS，Changed=7 / Rules=2 / Required=41 / Logs=3；
- production：2 files / 2,333 physical lines；
- orchestration：completion Read 2、P20.55 ExecuteExplicit 1、journal append 1、Save 1、互斥 Load 2、CAS 1、loop 0；
- World/Actor/UObject/gameplay damage/RNG/global path selection/sleep/timer：0；
- focused declarations：9；
- placeholder scan 与 git diff --check：PASS。

## 构建与产物

- initial production Editor integration：4 actions / native 0 / 27.20 s；
- test integration Editor：4 actions / native 0 / 13.07 s；
- final Editor：0 actions / native 0 / 3.11 s；
- final Game：4 actions / native 0 / 39.78 s；
- Editor DLL：18,085,888 bytes / SHA-256 13C85373032606049214C1997C81D680CC234CB0C5EC01BBA5C0CE43AAFA0B63；
- Game EXE：359,004,160 bytes / SHA-256 DFA7AC67C9F3FCD42A1AC7FBEC30A113A3594813CB10DC40C267CF67901BEA88。

## P/F 边界

PASS：separate completion authority/artifact、canonical codec、append-only terminal journal、save/read-back-before-CAS、trusted replay fast path、three crash windows、rollback/conflict/tamper/re-entry fences、mapping/self-test/gate、legacy 与 full regression、Editor/Game builds。

未声明：真实断电 fsync、跨进程锁、protected authority backend、remote consensus、completion garbage collection、next-generation terminal adoption/rotation、自动恢复、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.57：caller-explicit terminal adoption / next-generation rotation Session。只采纳可信 P20.56 terminal evidence，在新的显式 handoff checkpoint 出现时准备下一 generation；不自动扫描、删除或重用旧 evidence。
