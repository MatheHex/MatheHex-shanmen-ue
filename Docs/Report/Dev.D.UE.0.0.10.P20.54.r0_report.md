# Dev.D.UE.0.0.10.P20.54.r0 Report

## 1. 结论

P20.54 已为 P20.53 的 canonical recovery bundle 建立独立、caller-owned 的最低可信 generation watermark authority 契约，并固定“先让 exact bundle 可读，再单调推进可信 watermark”的崩溃顺序。

本阶段提供确定性 compare-and-advance request、审计 receipt、当前 authority state，以及一个有界 commit coordinator。coordinator 不执行 recovery、不自动重试、不触碰旧/新 surface；P20.48 live authority gate 仍是实际恢复前的最终权限边界。

本阶段刻意不提供普通文件实现。与 bundle 位于同一磁盘信任域的 watermark 文件会随 bundle 一起被回滚，不能被诚实地称为可信权威。真实后端应由调用方注入，例如平台受保护存储、远端单调权威或等价的独立信任域。

## 2. 基线、分支与改动范围

- 基线：9e78fe097668175af924e157b1c1d2c8020a2c8a（P20.53）；
- 分支：agent/0.0.10-p20-54-thrown-weapon-arc-preview-recovery-watermark-authority；
- 新增 watermark advance request、receipt、state、Read/CompareAndAdvance result 与 authority interface；
- 新增 bundle-before-watermark commit coordinator；
- 扩展 ProductLifecycle automation，新增 8 项水位权威契约测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Development Log；
- 未修改 P20.49 journal、P20.50 payload envelope、P20.51 filesystem、P20.52 bundle codec、P20.53 bundle storage 或 P20.48 recovery 执行语义；
- 未新增普通文件 watermark backend、自动启动恢复、surface mutation、scheduler、后台 retry 或网络客户端。

## 3. 权威身份、单调代际与审计证据

每次推进由五项不可变输入组成：

- AuthorityDomainId；
- stable LineageId；
- ExpectedGeneration；
- TargetGeneration；
- exact BundleId。

Generation 0 表示该 lineage 在权威中尚无已提交水位；target 范围为 1–8，且必须严格大于 expected。RequestId 使用固定 r1 namespace 和以上五项 canonical parts 确定性派生。ReceiptId 再绑定 RequestId、domain、lineage、previous/committed generation 与 BundleId；receipt 和 state 在读取时重算并验证这些身份，不能仅靠一个任意 GUID 冒充有效证据。

Current state 保存完整 last receipt，而不只保存一个整数。这样同 generation 的不同 BundleId 会成为明确冲突，审计方也能恢复本次单调推进的前值、目标值和请求身份。

## 4. 有界提交顺序

CommitBundleThenAdvance 严格执行以下顺序：

1. 验证 authority domain、P20.53 storage context、canonical bundle 与 stable lineage 一致；
2. 对 authority 执行一次 Read；
3. 拒绝 malformed/rejected/unavailable state、authority 高于 candidate，或同 generation 绑定不同 bundle；
4. 用当前可信 generation 作为 P20.53 external minimum watermark，执行一次 Save；
5. 无论 Save 是新提交、AlreadyCurrent，还是 replace 后结果不确定，都显式执行一次 Load；
6. 要求 Load 返回与 candidate 相同 generation 和 exact semantic bundle；
7. exact bundle 可读后，才构造并执行一次 CompareAndAdvance；
8. 只有 CompareAndAdvance 返回 OutcomeUnknown 时，额外执行一次 authority Read；除此之外没有重试或循环。

生产实现中 CompareAndAdvance 调用点严格为 1，authority Read 调用点严格为 2（初读和仅限未知结果的复查）。因此，可信 watermark 不会先于 exact bundle evidence 被推进。

## 5. 崩溃窗口、幂等与恢复语义

- bundle commit 前崩溃：可信 watermark 保持旧值，旧 bundle 仍受 P20.53 原子写边界保护；
- bundle 已提交、authority advance 前崩溃：磁盘存在较新 bundle，但 watermark 保持保守旧值，下一次显式调用可通过 AlreadyCurrent Save + exact Load 后继续推进；
- authority 返回 OutcomeUnknown：coordinator 只复查一次；若 state 精确匹配 request，返回 CommittedAfterAuthorityRecheck，否则返回 WatermarkOutcomeUnresolved；
- authority 在 advance 时 unavailable：返回 BundleCommittedWatermarkPending，已验证 bundle 保持可重试，但不在当前调用内自动重试；
- authority 已绑定同一 generation 和同一 BundleId：仍先验证 exact bundle 可读，再返回 AlreadyCommitted，不重复 CompareAndAdvance；
- authority 高于 candidate 或同代绑定其它 bundle：在存储回调前 fail closed；
- malformed success receipt/state：不得被当作提交成功。

所有重试都必须由调用方发起新的显式调用。加载或提交 evidence 仍不等于执行 recovery，也不授权 surface mutation。

## 6. 结果分类与信任边界

Read 区分 Missing、Current、Rejected 与 Unavailable；CompareAndAdvance 区分 Advanced、AlreadyCurrent、Conflict、Rejected、Unavailable 与 OutcomeUnknown。commit result 进一步区分输入、authority read/state、authority ahead/conflict、bundle save/verification、watermark advance、pending 与 unresolved outcome。

authority interface 只定义语义，不猜测存储位置、账号、平台能力或网络重试策略。普通本地文件 backend 被有意排除，因为它无法抵抗 bundle 与 watermark 同时回滚。本阶段也不宣称跨进程互斥、多写者共识、远端一致性或硬件防回滚。

## 7. 权限与副作用边界

新生产 API 只接受 P20.53 storage context、canonical bundle、authority domain、filesystem seam 与 caller-injected authority。它不接受 recovery coordinator、CompositionOwner、Host、Adapter、World、Actor 或 surface 参数。

两份生产文件中没有 UWorld、AActor、UObject、ApplyDamage、RNG、ProjectDir、ProjectSavedDir、GameSavedDir、Recovery.Execute、sleep 或自动调度调用。文件副作用仍完全委托给 P20.53 storage；authority 副作用只可发生在 exact bundle 显式 Load 验证之后。

## 8. 自动化验证

新增 8 项 focused automation：

1. ContractAndDeterminism：request/receipt/state 确定性、范围与 tamper fence；
2. BundleBeforeWatermarkAndReplay：exact bundle 先于 CAS 可读，重复提交无重复推进；
3. GenerationAdvanceAndRollbackFence：generation 单调推进、旧代与 authority-ahead 拒绝；
4. PrecommitFailureNeverAdvances：所有 bundle pre-commit failure 均不调用 authority advance；
5. PostcommitVerificationAndRetry：replace 后不确定结果必须经显式 Load，失败时不推进，后续显式重试可恢复；
6. AuthorityFailureLeavesRetryableBundle：advance unavailable 后 bundle 保持可验证与可显式重试；
7. UnknownOutcomeBoundedRecheck：unknown-before/after-commit 均只做一次 authority 复查；
8. InputAuthorityAndConflictFences：invalid/cross-lineage、malformed state/success、domain 与同代 bundle 冲突 fail closed。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.54_Focused.log | Product...RecoveryBundleWatermarkAuthority | 8/0 | 0 | 141.74 | 270,425 | 5994F938FFA422B5AF3BC611EAFF414A2599B3B180B8EFD61E7A443FDB1D9C05 |
| P20.54_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 1.50 | 303,487 | AE4D80F5C079B4BA28C549993837554FA7FDD5C8BB57A6AA439071461AFC9FB3 |
| P20.54_Full.log | Shanmen.0_0_10 | 1159/0 | 0 | 3000.77 | 1,704,882 | 2572CDE7277EBAFB875CB6E6E88E0CE0040B69CD5CE60F12F5EC1F729EADB0F8 |

三份正式日志累计 1213/0，每份均有唯一 UE 原生 TEST COMPLETE / EXIT CODE 0。完整套件从 P20.53 的 1151 增至 1159；P20.54 的 8 项测试在 focused 与 full 中各通过一次。各日志从首项项目测试开始后的 automation test errors、Fatal、Unhandled、Ensure 与 test-fail 均为 0。

## 9. Changed-file、构建与产物

- mapping JSON：225 rules / parse PASS；
- mapping self-test：401/401；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=39 / Logs=3；
- watermark authority 边界：2 files / 1,015 physical lines；
- CompareAndAdvance / authority Read 生产调用点：1 / 2；
- World/Actor/UObject/gameplay damage/RNG：0；
- recovery execute / exact surface mutation：0；
- global project path selection：0；
- ordinary-file watermark backend / network client：0 / 0；
- focused test declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS，0 whitespace errors；
- initial production Editor integration：4 actions / native 0 / 20.17 s；
- test integration Editor：4 actions / native 0 / 13.94 s；
- final Editor：0 actions / native 0 / 1.80 s；
- final Game：4 actions / native 0 / 25.03 s；
- Editor DLL：17,940,480 bytes / SHA-256 A3C129848F7FA75717C968255827DBDA62BEE6A15B8C8C0B1F5DE47D0A75D9BE；
- Game EXE：358,879,744 bytes / SHA-256 A0B494BD411E123F80151E5098029407617695C4B4814C6962BE7B9A289CDC2F。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：deterministic request/receipt/state、caller-owned authority contract、generation 0–8 monotonic CAS、same-generation bundle binding、bundle-save-before-watermark ordering、exact post-save Load、AlreadyCommitted replay、authority unavailable/pending、unknown-outcome single recheck、input/state/success/conflict fences、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：受保护 watermark backend 的真实持久化、bundle/watermark 同时回滚抵抗、power-loss durability、process/machine restart、concurrent writers、cross-process lock、remote consensus、credential/platform integration、自动启动/恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.55 建立显式 recovery admission/session：读取 caller-injected trusted authority state，只加载该 watermark 所绑定的 exact bundle，再以当前 journal 与 P20.48 live Owner/Host/Adapter 事实进行最终授权；仅在调用方明确请求时把 checkpoint evidence 交给 recovery coordinator，仍不得自动启动恢复。受保护 authority backend 应作为独立平台 adapter 实现，不能退化成普通同盘文件。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-54-thrown-weapon-arc-preview-recovery-watermark-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-54-thrown-weapon-arc-preview-recovery-watermark-authority/Docs/Report/Dev.D.UE.0.0.10.P20.54.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-54-thrown-weapon-arc-preview-recovery-watermark-authority/Docs/Log/Dev.D.UE.0.0.10.P20.54.r0_log.md>
