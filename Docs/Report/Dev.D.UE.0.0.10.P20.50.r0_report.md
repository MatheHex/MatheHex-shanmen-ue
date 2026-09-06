# Dev.D.UE.0.0.10.P20.50.r0 Report

## 1. 结论

P20.50 已为 P20.48 的 Arc preview Owner surface handoff recovery checkpoint 建立 journal-bound、canonical、可自校验的完整 payload envelope。合法 pending journal 与 checkpoint 可编码为稳定字节；解码端从不可变语义根重新构造 action、Arc request/plan/preview、input choice、presentation state、transition ticket 与 recovery checkpoint，并逐层核对原始 identity。

该 envelope 只恢复候选证据，不恢复执行权限。解码成功后仍须匹配当前 journal 的最新 pending checkpoint，再交给 P20.48 recovery coordinator；后者继续重新读取 old/new live surface，并重新核验 CompositionOwner、Host 与 Adapter 权限后才可提交绑定。磁盘字节本身不能跳过实时复核、不能触碰 surface，也不能把已 committed 或 foreign journal 重新打开。

## 2. 基线、分支与改动范围

- 基线：`fb85117ba4a7121ce89e8007eced858316c8fb06`（P20.49）；
- 分支：`agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope`；
- 新增 checkpoint payload envelope 与 canonical codec；
- 为 input choice、visible presentation state、transition ticket 与 checkpoint 增加 expected-id 自校验重建工厂；
- 扩展 ProductLifecycle automation，新增 6 项 payload/recovery 契约测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未新增文件 IO、网络 IO、surface mutation、scheduler、retry 或自动 recovery；
- 未连接真实 renderer、MainHUD、widget、component、World 或产品运行路径。

## 3. Journal-bound payload envelope

envelope 保存并自证四个顶层事实：

- schema version；
- deterministic EnvelopeId；
- 精确 pending JournalId；
- canonical payload digest 与完整 checkpoint。

`TryWrap` 只接受 valid checkpoint、valid journal、`CheckpointPending` 最新状态以及 journal latest record 的 exact checkpoint match。`TryUnwrapForJournal` 会再次检查 journal identity、pending disposition 与 checkpoint match；empty、foreign、stale 或已 committed journal 均 fail closed。

同一 checkpoint/journal 重复 wrap 与 encode 产生相同 envelope identity 和完全相同的字节。journal 一旦追加 matching recovery receipt，原 envelope 仍可作为历史证据解码，但不能再从该 journal unwrap。

## 4. 完整语义重建

payload 不复制派生几何数组，而保存足以确定性重建的语义根：

- checkpoint：identity、source failure、retirement response、old/new surface identity；
- transition ticket：request/policy/permit/lifecycle identity、Run、consumer、action；
- presentation：state identity、player、source item、product request、preview activation；
- input choice：state/command identity、revision、trajectory、target intent、apex adjustment；
- Arc preview：preview/plan/request identity 与 segment count；
- action：Run、Owner、activation、source entity/item、definition、content version/digest、排序后的 source GameplayTags；
- ballistics：technique tier、origin、target、gravity、apex clearance、speed/time ceilings。

decoder 按既有 public contract 依次执行 `TryCapture`、Arc planner、preview sampler 与四个新增 `TryRehydrate*` 工厂。每个工厂都重算 deterministic identity，并拒绝不匹配的 expected id。最终再从 reconstructed checkpoint 重新生成 canonical payload，要求与输入逐字节相等，避免接受非 canonical 或语义等价但表示不同的输入。

## 5. Canonical schema 与边界

current schema 为 version 1，使用显式 big-endian 编码：

- magic：`SMARCPAY`；
- header：64 bytes；
- header 保存 schema、declared total size、EnvelopeId、JournalId 与 PayloadDigest；
- 总输入上限：65536 bytes；
- 单字符串 UTF-8 上限：2048 bytes；
- source GameplayTags 上限：64，按字典序唯一编码；
- double 必须 finite，`-0.0` 不是 canonical 表示；
- segment count 还必须通过既有 Arc preview sampler 范围约束。

decoder 在语义重建前拒绝 empty、短 header、超限输入、magic mismatch、unknown schema、declared-size mismatch、trailing bytes 与 payload digest mismatch；随后拒绝非法 UTF-8、嵌入 NUL、非排序/重复/未注册 tag、非法枚举、非 canonical 浮点与任何派生 identity 不一致。

这是 payload envelope 的首个 schema，当前没有可诚实迁移的 N-1 payload。P20.49 metadata journal 的 schema 1→2 迁移能力保持不变。

## 6. 恢复权限边界

恢复链保持三道独立围栏：

1. bytes 必须通过 payload digest、canonical 与逐层 semantic identity 校验；
2. decoded envelope 必须匹配当前 journal 最新 pending checkpoint；
3. P20.48 recovery 必须重新观察精确 old/new surface identity/cursor，并确认当前 Owner、Host、Adapter 同步且权限未漂移。

envelope codec 不持有 surface 指针，不调用 `Show/Replace/Hide/ClearToEmpty/RetireForHandoff`，不创建 recovery receipt，不提交 journal，不替调用方重试，也不调度恢复。成功 decode 仅表示“证据可重建”，不表示“允许执行”。

## 7. 自动化验证

新增 6 项 focused automation：

1. `EvidenceAndJournalBinding`：pending journal binding、deterministic replay 与无 surface side effect；
2. `CanonicalRoundTripAndRecovery`：decode/re-encode 字节一致、完整 checkpoint 重建、P20.48 live recovery 与 committed fence；
3. `CorruptionTruncationAndTrailing`：整份 canonical envelope 的逐字节单 bit 翻转、全部 strict-prefix truncation 与 trailing byte 拒绝；
4. `SemanticIdentityAndTags`：action/choice/request/plan/preview/presentation identity 与 GameplayTags 完整保持；
5. `BoundsAndUnknownSchema`：empty、magic、schema、declared/min/max size 边界；
6. `ForeignJournalAndCommittedFence`：foreign pending 与 committed journal 拒绝，stale bytes 仅保留证据属性。

首轮 focused 为 4/2：两项失败来自测试在 fixture 已完成正常展示后错误要求绝对零历史 surface call。修正为封包操作前后计数快照比较后，不修改生产逻辑；正式 focused 为 6/0。

| Log | Group | Success/Fail | Native exit | SHA-256 |
|---|---|---:|---:|---|
| P20.50_Focused.log | Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope | 6/0 | 0 | `26312494154031548CA7A428867F38B10FF396EC6A9EF69FD82B8968906D9123` |
| P20.50_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | `A5A0270D8449F2E8763162AD8DE0757A512A1479EE04DC1A7647757D92BA8977` |
| P20.50_Full.log | Shanmen.0_0_10 | 1131/0 | 0 | `D118F0B5FF66A954F899099FCF625FB4ABD8CB474FCA64439DBD085C38EB4B7E` |

三份正式日志累计 1183/0，每份均保留 UE 原生 terminal marker。完整套件从 P20.49 的 1125/0 增至 1131/0。Focused/Legacy/Full 的非惩罚性超时提示分别为 6 / 0 / 112；Fatal/Unhandled/Ensure 分别为 0 / 0 / 0。

## 8. Changed-file 与架构证据

- mapping JSON：221 rules / parse PASS；
- mapping self-test：393/393；
- changed-file gate：PASS，Changed=15 / Rules=6 / Required=36 / Logs=3；
- payload envelope 边界扫描：2 files / 1097 physical lines；
- `UWorld/AActor/UObject/ApplyDamage/FMath::Rand/FRandomStream`：0；
- direct `Show/Replace/Hide/ClearToEmpty/RetireForHandoff`：0；
- `FFileHelper/IFileManager/CreateFile/DeleteFile/MoveFile`：0；
- scheduler/retry/sleep：0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 9. 构建与产物

- final Editor：0 actions / native 0 / 2.81 s；
- final Game：0 actions / native 0 / 1.19 s；
- Editor DLL：17,634,816 bytes / SHA-256 `72330953C1F828C2A7C1BFFD911165E6E3D37521510D34CC97D63712631D6FB2`；
- Game EXE：358,653,440 bytes / SHA-256 `9858D9E88C898F47DB3A19EBD4DE5D990E08D8B674AEB2CD9BDDCC85C7DEEF8C`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：pending journal-bound checkpoint payload、deterministic envelope identity、canonical current-schema bytes、完整 action/Arc/choice/presentation/ticket/checkpoint 重建、GameplayTag preservation、decode/re-encode 稳定、逐字节 corruption、全部 strict-prefix truncation、trailing bytes、magic/schema/size bounds、foreign/committed journal fence、P20.48 live recovery 复核、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：N-1 payload migration、磁盘写入、atomic replace、fsync、process/machine restart、crash durability、自动 recovery orchestration、全历史 payload archive、跨线程同步、拥有型 renderer handle、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.51 在此 envelope 之上建立 caller-owned atomic file storage adapter：只负责 bounded temp-write/flush/replace/read 与明确错误分类，读取后仍必须走 P20.50 journal gate 和 P20.48 live authority gate；不得让 storage adapter 自动触发表面恢复。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope/Docs/Report/Dev.D.UE.0.0.10.P20.50.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope/Docs/Log/Dev.D.UE.0.0.10.P20.50.r0_log.md>
