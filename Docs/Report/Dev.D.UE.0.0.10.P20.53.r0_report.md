# Dev.D.UE.0.0.10.P20.53.r0 Report

## 1. 结论

P20.53 已为 P20.52 canonical recovery bundle 建立 caller-owned、stable-lineage 的单槽本地存储。每条 recovery lineage 在调用方指定的绝对根目录下只占用一个 primary 文件；后续 generation 原子替换同一文件，不再把 journal 与 payload 分成两个独立落盘事务。

Save 与 Load 都要求文件外的最低可信 generation watermark。加载成功仍只返回 bundle evidence，不执行 recovery、不追加 journal，也不触碰旧/新 surface。P20.48 的 live authority gate 仍是实际恢复前的最终权限边界。

## 2. 基线、分支与改动范围

- 基线：79036d5479aa8e0bfa720d2ea347edc024d846e8（P20.52）；
- 分支：agent/0.0.10-p20-53-thrown-weapon-arc-preview-recovery-bundle-storage；
- 新增 recovery bundle storage context、Save/Load result、adapter 与本地文件系统复用别名；
- 扩展 ProductLifecycle automation，新增 8 项存储契约测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未修改 P20.49 journal、P20.50 payload envelope、P20.51 byte filesystem seam、P20.52 bundle codec 或 P20.48 recovery 执行语义；
- 未新增第二套文件 IO、自动恢复、surface mutation、scheduler、retry 或全局存储目录。

## 3. Stable lineage 与单槽路径

JournalId 会随每次 append 改变，不能作为跨 generation 的稳定文件名。P20.53 从第一条不可变 CheckpointPrepared journal record 派生 lineage identity，输入包括：

- first RecordId；
- first CheckpointId；
- first RunId；
- first ConsumerDefinitionDigest；
- 固定 deterministic namespace。

因此，同一 journal lineage 的 generation 1–8 保持同一个 lineage ID；foreign lineage 得到不同 slot。调用方必须提供有效绝对根目录与预期 lineage ID，根目录长度上限为 1024 字符。规范路径为：

- directory：ShanmenArcPreviewRecoveryBundles；
- primary：<lineage-id>.smarc-bundle；
- temporary：<lineage-id>.smarc-bundle.tmp。

temporary 与 primary 固定处于同一目录和卷，为单次 atomic replace 提供结构前提。模块不读取 ProjectDir、ProjectSavedDir 或其它进程全局路径。

## 4. Save 协议

Save 先验证 context、canonical bundle、lineage slot、1–8 范围内的外部 watermark，以及 candidate generation 不低于 watermark。随后编码 P20.52 canonical bytes，并在任何写入前检查既有 primary：

- unreadable、oversized、decode failure 或 foreign lineage：拒绝且不覆盖；
- existing generation 高于 candidate：拒绝 rollback；
- 同 generation 且 canonical bytes 完全相同：返回 AlreadyCurrent，不写 temporary、不 replace；
- 同 generation 但 evidence 不同：返回 generation conflict；
- existing generation 低于 candidate：允许进入 generation advance。

实际写入严格有界且无重试：

1. 创建 lineage directory；
2. 删除同 slot 的 stale temporary；
3. 写 temporary 并 full flush；
4. bounded read-back，要求长度、bytes 与 P20.52 codec 全部精确匹配；
5. 执行一次同卷 atomic replace；
6. bounded read-back committed primary，并再次要求长度、bytes 与 codec 全部精确匹配。

## 5. Load、watermark 与 rollback 围栏

Load 只读取 caller 明确给出的 lineage slot，最大读取长度沿用 P20.52 codec 上限 68,924 bytes。它区分 missing、read failure、oversized、decode failure、foreign lineage 与 generation below watermark。

MinimumGeneration 必须来自 bundle 文件之外的可信调用方状态。P20.53 刻意不把 watermark 写进同一个 bundle 文件，因为攻击者可将 evidence 与同文件 watermark 一起回滚。成功 Load 只说明磁盘中的 canonical bundle 属于预期 lineage 且未低于当前 caller watermark；调用方仍须用当前 journal 和 P20.48 live Owner/Host/Adapter 事实重新授权。

本阶段不宣称抵御“bundle 与外部 watermark 同时回滚”，也不负责保存或提升 watermark。

## 6. 既有文件与故障语义

所有 pre-commit failure 都在 atomic replace 前结束，既有 primary 保持不变；结果会明确指出 temporary 是否可能残留。无法确认既有 primary 内容时采用 fail closed，而不是把未知文件当作可覆盖旧版本。

atomic replace 一旦成功，后续 committed read-back failure 属于 post-commit outcome ambiguity：Save 不谎报成功，也不自动重试。调用方必须显式执行 Load 判断 primary 的最终状态。这样可避免一次不确定返回触发第二次隐式 replace。

本实现复用 P20.51 的 byte-oriented filesystem interface 与 full-flush local backend，仅由 P20.53 context 和 P20.52 codec 决定 slot 与内容。没有复制第二套平台文件实现。

## 7. 权限与副作用边界

storage API 不接受 recovery coordinator、CompositionOwner、Host、Adapter、World、Actor 或 surface 参数。生产文件中没有 Recovery.Execute、Show/Replace/Hide/ClearToEmpty/RetireForHandoff、gameplay damage、RNG、sleep、retry 或 scheduler 调用。

本地文件副作用仅限 caller root 下的固定 lineage directory、一个 primary 与一个同目录 temporary。无目录扫描、slot discovery、网络 IO、产品启动或自动恢复入口。

## 8. 自动化验证

新增 8 项 focused automation：

1. StableLineageContext：跨 generation 稳定 lineage、规范路径与 invalid context；
2. CanonicalSaveLoadAndReplay：canonical save/load、同代幂等 AlreadyCurrent 与零重复 replace；
3. GenerationAdvanceAndRollbackFence：generation 1→2、旧代 save 拒绝与 caller watermark；
4. PrecommitFailureAtomicity：directory、temporary、write、flush、read-back、validation 与 replace failure；
5. ExistingPrimaryFence：oversized、unreadable、corrupt、foreign 与同代冲突 primary；
6. PostcommitOutcomeRequiresLoad：replace 后 read-back/validation ambiguity 必须由 Load 解析；
7. LoadBoundsDecodeLineageAndWatermark：missing、size、read、decode、lineage 与 watermark 分类；
8. LocalFileAtomicGenerationReplace：真实 P20.51 local backend 的 generation 1→2 单槽替换。

| Log | Group | Success/Fail | Native exit | Seconds | Bytes | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| P20.53_Focused.log | Product...RecoveryBundleStorage | 8/0 | 0 | 175.23 | 271,232 | 66C28D596F4A58314ACEE62F02A33D6A6FD9250C1E5CCC33066742626AB90537 |
| P20.53_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | 15.86 | 303,002 | A31EEE1C7FDC888E6327E72792D16340736AF3685C4B29AE5E635D03193B2DC9 |
| P20.53_Full.log | Shanmen.0_0_10 | 1151/0 | 0 | 2802.72 | 1,688,347 | 8D3B85292EFA8E583DE496C702A3104C8C77A1F11D919E39DEA6C09D29454FE7 |

三份正式日志累计 1205/0，每份均有唯一 UE 原生 TEST COMPLETE / EXIT CODE 0。完整套件从 P20.52 的 1143 增至 1151；P20.53 的 8 项测试在 focused 与 full 中各通过一次。Fatal、Unhandled、Ensure 与 test-fail 均为 0。

## 9. Changed-file、构建与产物

- mapping JSON：224 rules / parse PASS；
- mapping self-test：399/399；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=38 / Logs=3；
- bundle storage 边界：2 files / 770 physical lines；
- World/Actor/UObject/gameplay damage/RNG：0；
- recovery execute / exact surface mutation：0；
- retry/sleep/scheduler：0；
- ProjectDir/ProjectSavedDir/GameSavedDir：0；
- network IO：0；
- focused test declarations：8；
- placeholder scan：PASS；
- git diff --check：PASS，0 whitespace errors；
- initial Editor integration：5 actions / native 0 / 27.42 s；
- final Editor：0 actions / native 0 / 1.33 s；
- final Game：4 actions / native 0 / 23.76 s；
- Editor DLL：17,858,560 bytes / SHA-256 3BFA22C1CEF5D86B14455E20AFF22D8070E2824CC63A770E8F3530B4448A9BD9；
- Game EXE：358,819,328 bytes / SHA-256 4308411A0998E03EA5920817E5E5F201B8B4971506BF51A052ADE323670EA039。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：stable lineage derivation、caller absolute root、single primary slot、canonical Save/Load、bounded reads、full-flush temporary、exact temporary/primary read-back、single atomic generation replace、AlreadyCurrent no-op、foreign/corrupt/oversized existing-primary fence、generation rollback fence、post-commit ambiguity、external minimum-generation fence、真实 local backend、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：可信 watermark 的持久化与单调提升、bundle/watermark 同时回滚抵抗、power-loss durability、process/machine restart、concurrent writers、跨进程锁、恶意 reparse/symlink root、slot discovery、自动启动/恢复、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.54 建立独立信任域中的 caller-owned watermark authority：提供单调 compare-and-advance 与可审计 receipt，并固定“先保存 generation N bundle，再将可信 watermark 提升到 N”的崩溃顺序；不得把 watermark 降级为同 bundle 文件中的字段，也不得因此自动执行 recovery。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-53-thrown-weapon-arc-preview-recovery-bundle-storage>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-53-thrown-weapon-arc-preview-recovery-bundle-storage/Docs/Report/Dev.D.UE.0.0.10.P20.53.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-53-thrown-weapon-arc-preview-recovery-bundle-storage/Docs/Log/Dev.D.UE.0.0.10.P20.53.r0_log.md>
