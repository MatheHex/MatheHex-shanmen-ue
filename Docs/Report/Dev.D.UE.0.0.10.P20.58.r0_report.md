# Dev.D.UE.0.0.10.P20.58.r0 Report

## 1. 结论

P20.58 已在 P20.57 caller adoption 之后建立 explicit next-generation rotation。只有调用方同时提供 exact trusted terminal adoption、与该 adoption 对应的 terminal journal，以及一个真实的新 handoff recovery checkpoint，Session 才会把终态 journal 的副本追加为 G+1 pending journal，并复用 P20.54 的 save/read-back/CAS 协议提交新的 pending bundle。

本阶段没有引入第四套权威。P20.56 completion artifact、completion authority、P20.57 adoption authority 与调用方 terminal journal 均保持只读；唯一持久化变化是现有 pending bundle 稳定槽与 pending watermark 从 G 旋转到 G+1。相同请求可确定性重放，durable-bundle/CAS 分裂和 CAS unknown outcome 均按既有有界协议收敛。

## 2. 基线、分支与改动范围

- 基线：23b7c4a159e357f50fc486442821cfef148b2a09（P20.57）；
- 分支：agent/0.0.10-p20-58-thrown-weapon-arc-preview-recovery-next-generation-rotation；
- 新增 deterministic NextGenerationRotationRequest、rotation evidence、typed result 与 bounded Session；
- 扩展 ProductLifecycle automation，新增 9 项身份、可信链、重放、权限、持久化分裂、未知结果与重入测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Development Log；
- 未修改 P20.48–P20.57 的既有生产实现；
- 未扫描目录、选择 latest 文件、删除历史证据、创建后台任务或引入新的 authority domain。

## 3. 显式请求与确定性身份

NextGenerationRotationRequest 显式绑定：

- exact P20.57 TerminalAdoption；
- 一个由真实失败 handoff 产生的新 P20.48 RecoveryCheckpoint；
- adoption、completion、terminal journal、G/G+1 边界、checkpoint、transition ticket 与 Run identity 共同派生的 RequestId。

RequestId 在 IsValid 中重算。无效 adoption、无剩余 journal 容量、无效 checkpoint，或直接复用 source-generation checkpoint identity，均在任何 authority/storage callback 之前失败关闭。Session 还会在最多 16 条的 canonical journal 内检查全部历史 checkpoint identity，防止后续代际复用更早的 checkpoint。

Rotation evidence 同时保留 source terminal journal 与 G+1 pending bundle，并确定性绑定 RequestId、AdoptionId、source/target generation、terminal/pending journal、checkpoint、envelope、bundle digest 与 BundleId。其有效性要求 pending journal 必须是 source terminal journal 的逐记录精确前缀扩展，且只多一个 pending checkpoint。

## 4. 有界旋转顺序

ExecuteExplicit 固定执行：

1. 验证 request、completion/pending storage context 与 exact caller terminal journal；
2. 拒绝任何历史 checkpoint identity 重用并建立同一 Session re-entry guard；
3. 读取 completion authority，要求 exact source generation 与 CompletionId；
4. 从 exact generation 加载 completion artifact，并与 request、authority、terminal journal 全量匹配；
5. 从可信 completion 重新派生 P20.57 adoption，并要求 exact AdoptionId；
6. 只读检查 adoption authority 已经处于 exact source generation/AdoptionId；
7. 复制 terminal journal，在副本上追加一个新 checkpoint；
8. 创建 payload envelope、G+1 pending bundle 与 deterministic rotation evidence；
9. 预检 pending authority 只能是 exact source BundleId 或 exact target BundleId；
10. 委托既有 coordinator 执行 save → read-back → one-shot CAS；只有 CAS outcome unknown 时允许一次 exact reread。

Session 内没有 retry loop、sleep、timer、scheduler、后台任务或不受界扫描。两个显式 `for` 只比较/检查最多 16 条的 journal 记录。

## 5. 权威、只读事实与唯一持久化变化

- completion authority：只读，必须绑定 exact CompletionId；
- completion artifact：按 source generation 只读加载，不重写；
- adoption authority：只读，必须已经绑定 exact AdoptionId，不由本 Session 推进；
- caller terminal journal：值拷贝输入，不在调用方对象上追加；
- pending authority：唯一允许推进的权威，从 exact G source BundleId CAS 到 exact G+1 BundleId；
- pending storage：唯一允许写入的存储，继续使用 P20.53 canonical stable slot 与临时文件原子替换协议；
- renderer surface、owner、World、input 与 gameplay damage：本阶段均不调用。

因此 P20.58 不会把“恢复完成”“调用方采纳”“下一代 pending”混成同一权威，也不会用普通文件存在性替代 authoritative current state。

## 6. 重放、故障与冲突闭包

- completion/adoption authority missing、unavailable、behind、ahead 或同代 foreign identity：失败关闭；
- completion artifact 缺失、损坏、跨请求或不匹配 terminal journal：在 adoption/pending 写路径前停止；
- pending authority behind、ahead 或 source/target generation foreign BundleId：不写 bundle、不执行 CAS；
- exact G+1 bundle 与 watermark 已当前：Replayed，不重复写入或 CAS；
- bundle 已 durable、CAS unavailable：返回 BundleCommittedWatermarkPending；下一次显式调用复用已验证 bundle，仅修复 watermark；
- CAS 已提交但响应 unknown：一次 exact reread匹配即收敛为成功；
- CAS 未提交且响应 unknown：一次 reread后返回 PendingWatermarkOutcomeUnresolved，后续显式调用可复用 durable bundle；
- callback re-entry：同一 Session 返回 OperationInProgress，外层继续完成且 guard 清理。

所有成功、pending 与 unresolved 结果都携带 deterministic rotation evidence 和底层 commit result，并由 Result.Validate 按已经取得的信任层级自检。

## 7. 架构边界

两份新增生产文件合计 1,052 physical lines。P20.58 自身显式生产调用点为 completion/adoption/pending authority Read 各 1、CompletionStorage.Load 1、Journal.AppendCheckpoint 1 与 CommitBundleThenAdvance 1；CAS、bundle save/read-back 与 unknown-only reread继续封装在既有 P20.54 coordinator 中。

生产文件中 UWorld、AActor、UObject、ApplyDamage、RNG、ProjectDir/ProjectSavedDir/GameSavedDir、sleep、ticker、timer、async 与 task graph 均为 0。没有第四 authority、全局路径选择、目录枚举、latest-file 推断或产品代码对测试实现的依赖。

## 8. 自动化验证

新增 9 项 focused automation：

1. RequestContractAndDeterminism；
2. ExactTrustedRotation；
3. TrustedReplay；
4. CompletionTrustFences；
5. AdoptionTrustFences；
6. PendingAuthorityFences；
7. DurableBundleBeforeWatermarkRepair；
8. UnknownOutcomeClosure；
9. InputAndReentryFences。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.58_Focused.log | Product...RecoveryNextGenerationRotationSession | 9/0 | 0 | 440.644 | 286,889 | EDD16086510AB41705D1793B26C1328031E4D58AA839552D1E5E7133F5F5E10A |
| P20.58_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 1.653 | 310,341 | D852DED3CBE07213952FE2ADBBF7C79C71A81CD207D9BDAE9F3D1EAA500C6269 |
| P20.58_Full.log | Shanmen.0_0_10 | 1193/0 | 0 | 3823.827 | 1,831,379 | 562D6D71411BF75D383422AC07F4CCA83301F2D1AD1640C1ACDDCB80CC7D4531 |

首轮 focused 原样保留为 `P20.58_Focused_FirstFailure.log`：7/2，288,285 bytes，SHA-256 05E80E147B027AAD8761A66A8763929491B4AA5D7C2AA0F388AADA34051EF91A。两项失败均为测试把 fake pending authority 在 fixture 构造后的初始 AdvanceCount 多算 1；生产状态、可信链与持久化断言均通过。修正测试期望后正式 focused 为 9/0，生产代码未因该失败修改。

三份正式日志累计 1248/0；每份具有原生成功终止标记。automation test failure、Fatal、Unhandled 与 Ensure 均为 0。

## 9. Changed-file、构建与产物

- mapping JSON：229 rules / parse PASS；
- mapping self-test：409/409；
- 新增 NextGenerationRotationSession exact rule，并把 focused group 回接 TerminalAdoption、Completion、Admission、WatermarkAuthority、BundleStorage、Bundle、PayloadStorage、PayloadEnvelope、Journal 与 Recovery；
- focused-only gate：按预期拒绝，缺少 42 个依赖组；
- final changed-file gate：PASS，Changed=7 / Rules=2 / Required=43 / Logs=3；
- focused test declarations：9；
- placeholder/boundary scan：PASS，0 placeholders；生产禁用依赖 0；direct CAS 0；2 个 bounded loops；
- git diff --check：PASS，0 whitespace errors；
- initial production Editor integration：4 actions / native 0 / 33.10 s；
- test integration Editor：4 actions / native 0 / 16.39 s；
- expectation correction Editor：4 actions / native 0 / 9.49 s；
- final Editor：Succeeded / native 0 / 0 actions / 2.24 s / up to date；
- final Game：Succeeded / native 0 / 4 actions / 36.57 s；
- Editor DLL：18,203,136 bytes / SHA-256 ACA95DA90C40B7C47FA7994A180FDF88F7B474D00775A9357F37B9D9FC6A1DCC；
- Game EXE：359,103,488 bytes / SHA-256 7168CB9F755A97A68271FDC4D996729066E33F322474D8756D9A5933E97CCF45。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：deterministic G→G+1 request/rotation evidence、trusted completion + adoption chain、exact terminal prefix extension、historical checkpoint identity fence、pending source/target authority fence、durable save/read-back/one-shot CAS、exact replay、unknown-outcome single reread、crash-boundary repair、read-only completion/adoption/source journal、re-entry fence、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：真实 protected authority backend、真实断电 fsync、跨进程锁、remote consensus、调用方跨系统事务、长期 generation archive/retention policy、自动发现或自动恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.59 建立 multi-generation cycle proof：使用本阶段产生的 G+1 pending bundle 重新进入既有 admission → recovery → completion → adoption 链，再证明 G+2 rotation、历史 checkpoint 去重与 16-record 容量边界，不新增平行权威或自动重试器。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-58-thrown-weapon-arc-preview-recovery-next-generation-rotation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-58-thrown-weapon-arc-preview-recovery-next-generation-rotation/Docs/Report/Dev.D.UE.0.0.10.P20.58.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-58-thrown-weapon-arc-preview-recovery-next-generation-rotation/Docs/Log/Dev.D.UE.0.0.10.P20.58.r0_log.md>
