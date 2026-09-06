# Dev.D.UE.0.0.10.P20.58.r0 Development Log

## 基线与目标

- base：23b7c4a159e357f50fc486442821cfef148b2a09；
- branch：agent/0.0.10-p20-58-thrown-weapon-arc-preview-recovery-next-generation-rotation；
- 目标：在 P20.57 exact caller adoption 后，以一个真实新 recovery checkpoint 明确开启 G+1 pending generation；
- 权限边界：caller-explicit request/context/terminal journal；completion 与 adoption 只读；只允许既有 pending bundle/storage authority 旋转；不扫描、删除、自动重试或调用 surface。

## 设计判断

1. P20.57 的 NextGeneration 只是容量边界，不代表 checkpoint 或 pending bundle 已经存在。
2. 开启下一代必须绑定一个真实 handoff 失败产生的 checkpoint，不能伪造空记录或复用旧 checkpoint identity。
3. 旋转前必须重新验证 completion authority + exact completion artifact + adoption authority，不能仅相信调用方对象。
4. completion、adoption、pending 三域已经足够；新增第四 authority 会制造双写和恢复歧义。
5. source terminal journal 只能作为值拷贝前缀，实际 append 必须发生在副本中。
6. pending authority 的 source state 必须绑定 completion 已消费的 exact BundleId；target state只允许 exact candidate BundleId 重放。
7. durable commit 继续复用 P20.54 save/read-back/CAS coordinator，避免第二套文件或 CAS 协议。
8. CAS unknown outcome 仍只允许一次 exact reread；没有 Session 内 retry loop。
9. journal 最多 16 条，因此历史身份检查和前缀比较是显式有界循环。

## 实现记录

新增：

- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.h；
- demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.cpp。

核心对象：

- deterministic NextGenerationRotationRequest；
- source terminal + pending bundle 绑定的 NextGenerationRotation evidence；
- typed NextGenerationRotationSessionResult；
- caller-explicit bounded NextGenerationRotationSession。

旋转路径：validate request/contexts/terminal/history → completion authority read → exact completion load → adoption rederive → adoption authority read → journal-copy append → envelope/bundle/rotation create → pending authority preflight → existing save/read-back/CAS coordinator。

## 故障、重放与拒绝语义

- completion/adoption 不可信：在 pending 访问前拒绝；
- completion artifact 损坏：在 adoption/pending 写入前拒绝；
- pending source/ahead/foreign state：在 bundle save 前拒绝；
- exact target bundle + watermark：Replayed，不重复 write/CAS；
- durable bundle + unavailable CAS：保留 bundle，下一显式调用只修 watermark；
- unknown-after-commit：一次 reread收敛；
- unknown-before-commit：一次 reread后 unresolved，后续调用可安全复用 bundle；
- callback re-entry：OperationInProgress；
- completion 文件、adoption authority、source journal、surface：始终不修改。

## 首轮测试修正

首轮 focused 为 7/2；失败只涉及两个测试中的 fake pending authority `AdvanceCount` 基线。Completion fixture 的 pending watermark 已通过 `SetState` 建立，既有 completion/adoption 阶段不会对该 fake pending authority执行 CAS，因此初始计数是 0 而非 1。

保留日志：P20.58_Focused_FirstFailure.log，288,285 bytes，SHA-256 05E80E147B027AAD8761A66A8763929491B4AA5D7C2AA0F388AADA34051EF91A。只修正测试期望：pending fence 预检后仍为 0；unavailable + repair 两次 CAS 后为 2。随后 Editor 重编译成功，生产实现未修改。

## 自动化测试

新增 9 项：RequestContractAndDeterminism、ExactTrustedRotation、TrustedReplay、CompletionTrustFences、AdoptionTrustFences、PendingAuthorityFences、DurableBundleBeforeWatermarkRepair、UnknownOutcomeClosure、InputAndReentryFences。

正式结果：

- focused：9/0，native 0，440.644 s，286,889 bytes，SHA-256 EDD16086510AB41705D1793B26C1328031E4D58AA839552D1E5E7133F5F5E10A；
- legacy ItemUseAndArmor：46/0，native 0，1.653 s，310,341 bytes，SHA-256 D852DED3CBE07213952FE2ADBBF7C79C71A81CD207D9BDAE9F3D1EAA500C6269；
- full Shanmen.0_0_10：1193/0，native 0，3823.827 s，1,831,379 bytes，SHA-256 562D6D71411BF75D383422AC07F4CCA83301F2D1AD1640C1ACDDCB80CC7D4531；
- formal total：1248/0；
- terminal-success markers：正式三份日志均为原生成功；
- automation test failure / Fatal / Unhandled / Ensure：0 / 0 / 0 / 0。

## Changed-file regression 与静态边界

- mapping JSON：229 rules / parse PASS；
- 新增 NextGenerationRotationSession exact rule，并将其作为 TerminalAdoption、Completion、Admission、WatermarkAuthority、BundleStorage、Bundle、PayloadStorage、PayloadEnvelope、Journal 与 Recovery 的反向依赖；
- mapping self-test：409/409；
- focused-only gate：按预期因 42 个依赖组缺失而拒绝；
- final gate：PASS，Changed=7 / Rules=2 / Required=43 / Logs=3；
- production：2 files / 1,052 physical lines；
- direct orchestration：authority Read 3、CompletionStorage.Load 1、Journal.AppendCheckpoint 1、CommitBundleThenAdvance 1、direct CAS 0；
- bounded loops：2，均受 journal MaxRecordCount=16 约束；
- World/Actor/UObject/gameplay damage/RNG/global path selection/sleep/timer/async：0；
- focused declarations：9；
- boundary scan 与 git diff --check：PASS；0 placeholders；生产禁用依赖 0；direct CAS 0；2 个 bounded loops；0 whitespace errors。

## 构建与产物

- initial production Editor integration：4 actions / native 0 / 33.10 s；
- test integration Editor：4 actions / native 0 / 16.39 s；
- expectation correction Editor：4 actions / native 0 / 9.49 s；
- final Editor：Succeeded / native 0 / 0 actions / 2.24 s / up to date；
- final Game：Succeeded / native 0 / 4 actions / 36.57 s；
- Editor DLL：18,203,136 bytes / SHA-256 ACA95DA90C40B7C47FA7994A180FDF88F7B474D00775A9357F37B9D9FC6A1DCC；
- Game EXE：359,103,488 bytes / SHA-256 7168CB9F755A97A68271FDC4D996729066E33F322474D8756D9A5933E97CCF45。

## P/F 边界

PASS：explicit G→G+1 rotation、deterministic request/evidence、completion/adoption trust revalidation、source terminal prefix proof、historical identity rejection、pending authority source/target fence、save/read-back/one-shot CAS、exact replay、durable-before-watermark repair、unknown-only reread、read-only source evidence、re-entry fence、mapping/self-test/gate、legacy 与 full regression、Editor/Game builds。

未声明：真实 protected backend/fsync/process lock/remote consensus、跨系统 caller transaction、长期 generation archive、自动扫描/恢复/清理、真实 renderer/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.59：multi-generation cycle proof。把本阶段的 G+1 pending bundle 送入既有 admission/recovery/completion/adoption 链，再以另一个真实 checkpoint 旋转到 G+2；验证历史 checkpoint 去重、代际单调性、重放和 16-record 容量边界，不增加平行 authority、latest-file 扫描或自动重试器。
