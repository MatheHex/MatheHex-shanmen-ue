# Dev.D.UE.0.0.10.P20.57.r0 Development Log

## 基线与目标

- base：c32e5f24c26581fd6b31f22cac2644d9514ad128；
- branch：agent/0.0.10-p20-57-thrown-weapon-arc-preview-recovery-terminal-adoption；
- 目标：把 P20.56 的可信 terminal completion 封闭为 caller-explicit、可验证、可重放的 adoption authority 决策；
- 权限边界：caller-explicit request/context/current journal；不扫描目录、不写文件、不追加 journal、不调用 surface、不创建 checkpoint、不循环重试。

## 设计判断

1. P20.56 completion 证明 terminal journal 已完成并持久化，但不等于调用方已经采纳它。
2. Adoption 必须使用与 pending/completion 均不同的第三 authority domain，避免把“允许恢复”“恢复完成”“调用方已采纳”混成一个水位。
3. 普通 completion 文件不构成信任；必须先由 completion authority 当前状态绑定 exact CompletionId。
4. Adoption evidence 必须绑定 completion digest、source/terminal journal、generation 与完整 request，并确定性派生 AdoptionId。
5. Session 只授权调用方替换 journal 并返回 exact terminal journal；实际调用方内存更新不应伪装成 Session 内副作用。
6. 同代 exact adoption 是可信 replay，应在 CAS 前短路；同代 foreign evidence 与 ahead state 必须失败关闭。
7. CAS unknown outcome 只允许一次 exact reread；Session 内不得出现 retry loop。
8. NextGeneration 只是 G+1 容量/顺序边界，不代表已经存在下一代 checkpoint。
9. 下一阶段必须以一个真实、显式的新 handoff checkpoint 开启 rotation，并保留旧完成证据。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.cpp。

核心对象：

- deterministic TerminalAdoptionRequest；
- generation、NextGeneration、completion identity、journal identity 与 AdoptionId 绑定的 TerminalAdoption；
- typed TerminalAdoptionSessionResult；
- caller-explicit bounded TerminalAdoptionSession。

采纳路径：validate input/current journal → completion authority read → exact completion load/verify → adoption create → adoption authority read → exact replay fast path 或 one-shot CAS → unknown-only exact reread。

## 故障、重放与拒绝语义

- completion authority missing/unavailable/not-current/ahead：拒绝把普通存储当作可信完成；
- completion load/canonical/request/current-journal mismatch：在 adoption authority 前失败；
- adoption authority exact current：Replayed，不重复 CAS；
- adoption authority ahead/foreign：拒绝 rollback/conflict；
- CAS conflict/rejected/unavailable：不返回成功；
- CAS 响应未知：一次 authority reread匹配则 AdoptedAfterAuthorityRecheck，否则 unresolved；
- 同一 Session callback re-entry：OperationInProgress，后续动作计数不增加；
- success 只返回可信 completion、adoption 与 exact terminal journal，不修改 caller journal 本身。

## 自动化测试

新增 8 项：RequestContractAndDeterminism、ExactTrustedAdoption、TrustedReplay、CompletionTrustFences、AdoptionAuthorityFences、UnknownOutcomeClosure、AuthorityFailureFences、InputAndReentryFences。

正式结果：

- focused：8/0，native 0，130.931 s，281,377 bytes，SHA-256 2B7AA091F51331654A0892E05B242187A4BC3DE64A2651C93203989D3F8DB4CC；
- legacy ItemUseAndArmor：46/0，native 0，1.502 s，309,999 bytes，SHA-256 1EBE93F47CC41FDA6C7FACE5FF754A8FBD6AAB86A57DCB024EA849929167015C；
- full Shanmen.0_0_10：1184/0，native 0，3268.196 s，1,800,170 bytes，SHA-256 9B2E9654B6F5A320D3481F88E5568237BEFC17FF0C16D2B03D47669F19371632；
- formal total：1238/0；
- unique queue-empty/TestExit markers：1 / 1 / 1；
- automation errors / Fatal / Unhandled / Ensure / test-fail：0 / 0 / 0 / 0 / 0。

## Changed-file regression 与静态边界

- mapping JSON：228 rules / parse PASS；
- 新增 TerminalAdoptionSession exact rule，并把 focused group 回接 CompletionSession、AdmissionSession、WatermarkAuthority、BundleStorage、Bundle、PayloadStorage、PayloadEnvelope、Journal 与 Recovery；
- mapping self-test：407/407；
- focused-only gate：按预期因 41 个依赖组缺失而拒绝；
- final gate：PASS，Changed=7 / Rules=2 / Required=42 / Logs=3；
- production：2 files / 842 physical lines；
- orchestration：completion Read 1、Storage.Load 1、adoption Read 2、CAS 1、Save 0、append 0、loop 0；
- World/Actor/UObject/gameplay damage/RNG/global path selection/sleep/timer：0；
- focused declarations：8；
- boundary scan 与 git diff --check：PASS。

## 构建与产物

- initial production Editor integration：4 actions / native 0 / 30.74 s；
- test integration Editor：4 actions / native 0 / 16.44 s；
- final Editor：0 actions / native 0 / 1.82 s；
- final Game：4 actions / native 0 / 23.894 s；
- Editor DLL：18,138,624 bytes / SHA-256 7C948579D19CD99624702C5A4AA50B443CBBB6B720407AA54BB3C6DFE7B601B0；
- Game EXE：359,047,680 bytes / SHA-256 8FFF3C0AD1F1534321C23D5615DC30B7D99E7FA5CD6A149588346B98078A7889。

## P/F 边界

PASS：three-domain authority separation、trusted completion verification、deterministic adoption evidence、caller journal fence、exact replay、one-shot CAS、unknown-only reread、rollback/conflict/tamper/re-entry fences、next-generation capacity boundary、mapping/self-test/gate、legacy 与 full regression、Editor/Game builds。

未声明：调用方 journal 替换的跨系统原子性、真实 fsync/process lock/protected backend/remote consensus、真实 G+1 checkpoint rotation、按代 archive/cleanup、自动恢复、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.58：explicit new-checkpoint rotation。要求 caller 已采纳 exact terminal journal，并提供一个真实新 handoff checkpoint 后才创建 G+1 pending evidence；旧 completion/adoption 按代保留，不覆盖、不复用、不自动扫描或删除。
