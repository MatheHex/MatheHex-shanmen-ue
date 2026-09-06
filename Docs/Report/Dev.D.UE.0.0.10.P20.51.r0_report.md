# Dev.D.UE.0.0.10.P20.51.r0 Report

## 1. 结论

P20.51 已在 P20.50 canonical checkpoint payload envelope 之上建立 caller-owned、bounded、one-shot 的本地文件存储适配器。调用方必须显式提供绝对根目录和 expected JournalId；适配器只在该根下派生单一 journal slot 的 primary/temp 路径，不选择全局产品目录，也不扫描其它槽位。

保存流程在替换 primary 前完成临时文件 full flush、bounded read-back、逐字节比较和 P20.50 codec 重验；随后只进行一次同目录替换，再读取 committed primary 做同样校验。加载流程只返回合法 envelope evidence。存储层没有 journal 对象、recovery coordinator、CompositionOwner 或 surface 参数，因此不能把磁盘字节升级成恢复权限。

## 2. 基线、分支与改动范围

- 基线：`979464df35192d302501193a281e9aefe3effe61`（P20.50）；
- 分支：`agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage`；
- 新增 payload storage context、filesystem seam、local filesystem backend、save/load result 与 stateless adapter；
- 扩展 ProductLifecycle automation，新增 6 项存储协议测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未修改 P20.50 envelope schema、journal codec、P20.48 recovery 或任何 surface contract；
- 未连接启动流程、自动恢复、真实 renderer、MainHUD、widget、component 或 World。

## 3. Caller-owned journal slot

`StorageContext::TryCreate` 只接受 bounded absolute root 与 valid expected JournalId。primary 文件名由 JournalId 的 32 位十六进制 canonical 表示派生，temporary 文件固定为 primary 同目录的 `.tmp`；因此源和目标在结构上位于同一目录，不会由适配器跨卷移动。

context 不接受相对路径、空路径、嵌入 NUL、超长根目录或空 JournalId。Save 在任何 filesystem callback 前要求 context valid、envelope valid，并要求 `Envelope.JournalId == Context.ExpectedJournalId`。这阻止 foreign envelope 覆盖另一 journal slot。

## 4. 单次保存协议

保存顺序固定：

1. canonical encode P20.50 envelope，并再次实施 65536-byte 上限；
2. 确认/创建 caller-owned storage directory；
3. 若存在 stale temp，则只尝试删除一次；
4. 创建并写入 temp，执行 `Flush(true)` 后关闭；
5. bounded read-back temp，要求长度、字节和 decoded envelope 全部精确匹配；
6. 只调用一次同目录 `AtomicReplace(primary, temp)`；
7. bounded read-back committed primary，再次要求长度、字节和 decoded envelope 精确匹配。

适配器不在失败后循环重试，不删除 primary，不从 backup 恢复，也不自动消费残留 temp。失败结果明确报告 temp 是否可能残留，以及 primary 是否已经被替换。

## 5. 失败原子性与不确定结果

目录创建、stale-temp 清理、open/write/flush、temp read-back/validation 或 replace 失败均发生在 committed primary 被确认替换之前；filesystem seam 约定 replace 返回 false 时 destination 不变。测试对上述 8 个阶段逐一注入失败，并验证原 primary 字节保持不变、replace 最多调用一次。

replace 成功后若 committed read-back 或 codec validation 失败，结果分别为 `CommittedReadBackFailed` 或 `CommittedValidationFailed`，并显式标记 primary 已替换。适配器不猜测、不回滚、不重试；调用方必须执行一次显式 Load 来解析可观察磁盘状态。

## 6. Bounded Load 与错误分类

local backend 先打开文件句柄并读取 size，超出 P20.50 最大 encoded size 时不分配 payload buffer。Load 区分 missing、read failure、size rejection、decode rejection 与 journal-slot mismatch；仅 `Loaded` 返回 envelope。

即使 Loaded，也只证明当前 primary 是 canonical P20.50 evidence 且属于 expected journal slot。调用方仍须显式执行 `Envelope.TryUnwrapForJournal(latestJournal)`，然后由 P20.48 recovery 重新读取 old/new live surface 及 Host/Adapter authority。已 committed journal 会拒绝同一磁盘 envelope，storage 不会重新打开它。

## 7. 文件系统边界

真实 backend 仅实现 directory exists/create、file exists/delete-temp、write/full-flush、bounded read 与 same-directory replace。接口由调用方持有并传入 adapter，便于产品层选择受控 root，也便于测试用确定性 fake 注入每个故障阶段。

本阶段不声明 power-loss durability、directory fsync、跨进程锁、并发 writer 仲裁、backup generation、全历史 archive 或跨机器可移植性。Windows/平台文件系统的 replace 语义仍是外部依赖；P20.51 只保证单次调用顺序、预提交 primary 保持和可诊断的提交后不确定状态。

## 8. 自动化验证

新增 6 项 focused automation：

1. `ContextAndJournalFence`：absolute-root canonicalization、invalid request 与 foreign slot 在 filesystem callback 前拒绝；
2. `AtomicRoundTripAndEvidenceOnly`：in-memory backend 的 exact bytes、single replace、无 surface/recovery side effect；
3. `PrecommitFailureAtomicity`：8 个预提交故障点、旧 primary 保持与 no retry；
4. `PostcommitOutcomeRequiresLoad`：replace 后 read/validation failure 显式不确定，并仅由 Load 解析；
5. `LoadBoundsDecodeAndJournalFence`：missing、oversize、read、codec corruption 与 foreign JournalId 分类；
6. `LocalFileAtomicReplaceAndCommittedFence`：真实 Saved 临时根的 flush/replace/read-back/replacement，以及 committed journal fence。

开发 focused 首轮即为 6/0；加强真实文件测试对“cleanup 前 temp 已消失”的断言后，正式 focused 仍为 6/0。

| Log | Group | Success/Fail | Native exit | Bytes | SHA-256 |
|---|---|---:|---:|---:|---|
| P20.51_Focused.log | Product...CheckpointPayloadStorage | 6/0 | 0 | 265,901 | `20B0ACC9B95E8680DA88C77280C27E13A9C3C5A9E793366C7532221D02E10BC9` |
| P20.51_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 303,003 | `A676EDD9404A103AFB5C4230403336936717C77B55C06FC5A177094CC1D20F20` |
| P20.51_Full.log | Shanmen.0_0_10 | 1137/0 | 0 | 1,660,714 | `7EF1B89EAC227CB13D6D0DECB141F0FDAE833E81AF155A14F0D8558233ECE0B1` |

三份正式日志累计 1189/0。Focused/Legacy/Full 的非惩罚性超时提示分别为 6 / 0 / 119；Fatal/Unhandled/Ensure 分别为 0 / 0 / 0。

## 9. Changed-file、构建与产物

- mapping JSON：222 rules / parse PASS；
- mapping self-test：395/395；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=36 / Logs=3；
- storage 边界：2 files / 703 physical lines；
- World/Actor/UObject/gameplay damage/RNG：0；
- journal unwrap/append、recovery execute、surface mutation：0；
- scheduler/retry/sleep：0；
- primary delete：0；
- production `AtomicReplace` call site：1；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors；
- final Editor：0 actions / native 0 / 1.79 s；
- final Game：0 actions / native 0 / 1.15 s；
- Editor DLL：17,700,352 bytes / SHA-256 `688CE434ADF2CB5C0181B45B0F11F1C2B8748F60300427708F39EB802FE1B717`；
- Game EXE：358,699,008 bytes / SHA-256 `10FFEC03408BC39010AD10E95EB3FF8E3376331242CBFDB590D036116662DB8E`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：caller-owned absolute root、JournalId-derived slot、bounded allocation、temp write/full flush/read-back/codec verification、single same-directory replace、committed read-back、8-stage precommit failure atomicity、2-stage postcommit uncertainty、missing/oversize/read/decode/foreign classification、真实本地文件 round-trip/replacement、P20.50 journal fence、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：power-loss durability、directory fsync、OS crash during rename、network filesystem、concurrent writers、file locks、backup/rollback、journal+payload 两文件事务、自动启动/恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.52 解决 restart 所需的另一半证据：将 P20.49 journal bytes 与 P20.50 payload bytes 组合为一个 canonical recovery bundle，并以 generation/digest 证明二者匹配；bundle 仍只作为证据，加载后继续走 current journal disposition 与 P20.48 live authority gate。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage/Docs/Report/Dev.D.UE.0.0.10.P20.51.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage/Docs/Log/Dev.D.UE.0.0.10.P20.51.r0_log.md>
