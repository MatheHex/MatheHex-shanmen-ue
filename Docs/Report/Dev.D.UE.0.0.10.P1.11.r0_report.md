# Dev.D.UE.0.0.10.P1.11.r0 开发报告

## 结论

**PASS。READY_FOR_0_0_10_P1_12。** P1.11 已将 P1.10 有意失败关闭的奖励来源、稀有奖励与词缀信息纳入 ShanmenItems 单一权威：统一 canonical metadata 由物品实例持有，经旧 Code A 持久记录迁移和 Runtime Extraction 两条入口使用同一映射器进入 authority，并随 schema 1→2 原子升级、事务指纹、重放、备份与重启完整保留。

最终源代码状态通过：

- P1.11 persistence / migration / lifecycle 定向回归：15/15 Success；
- `Shanmen.0_0_10` 完整回归：62/62 Success；
- Editor Development 构建：成功，原生退出码 0；
- Game Development 构建：成功，原生退出码 0；
- `git diff --check`：原生退出码 0。

## 功能性

### 1. Canonical reward metadata

`FShanmenItemInstance` 与 `FShanmenItemRunAcquiredItem` 现在使用同一 `FShanmenItemRewardMetadata`，覆盖：

- reward event kind、稳定 event ID、倍率与 source role；
- rare event ID、policy ID、tier 与 bonus；
- affix event ID、policy ID、acquisition kind，以及已解析 affix 的 ID、tier、magnitude 和 resolved value。

Metadata 是 authority item graph 的组成部分，不是旁路 JSON、Profile 补丁或第二库存。Stored、Prepared、Depleted 与 Destroyed tombstone 都保留来源事实，因此生命周期终局不会抹掉审计证据。

核心模块只验证结构不变量：字段成组完整、正值、上限、唯一 affix ID、受支持 enum 和 acquisition 组合。产品语义仍由 `demo_map` 的 reward rules 与 affix policy registry 决定，避免 ShanmenItems 反向依赖产品定义。

### 2. 单一路径产品映射

新增共享 `Fdemo_mapShanmenItemMetadataAdapter`，同时服务：

- Code A 的 `Fdemo_mapPersistentItemRecord` 旧持久物品；
- Runtime settlement 的 `Fdemo_mapRuntimeSettlementItem` 新战利品。

适配器先调用产品 `Fdemo_mapRewardEventRules::IsValid` 与 `Fdemo_mapRewardAffixPolicyRegistry::ValidateSet`，再做显式 enum/字段映射并调用 core canonical validation。非法产品策略、未知 tier/acquisition、残缺 provenance、重复 affix 或非法数值均在 authority mutation 前失败关闭。

P1.10 的 `AcquiredMetadataUnsupported` gate 已删除；合法 metadata-bearing loot 现在走既有 Extraction 原子事务，不会静默降级为 plain loot。

### 3. Authority schema 1→2

`FShanmenItemAuthorityDocument` 已显式版本化：legacy schema 1，current schema 2。

读取 schema 1 时执行严格转换：

1. 拒绝声称 schema 1 却已携带新 metadata 的混合文档；
2. 使用移除 `RewardMetadata` 后的 schema-1 legacy snapshot digest 验证旧证据；
3. 仅为结构转换注入默认 metadata，并在内存中规范化为 schema 2；
4. 打开 authority 时，以当前 Code A migration candidate 为 immutable initial item 补齐 metadata；
5. 原子写入 schema 2，并将原始 schema-1 字节完整保留为 backup；
6. 重启后直接读取 schema 2，不重复升级或写盘。

旧 migration identity 继续由 metadata-free schema-1 source 派生，避免既有身份漂移；schema-2 CandidateDigest 则覆盖完整 metadata，确保同一来源内容变化可被检测。未来 schema 仍严格拒绝，不会静默 reset。

### 4. Durable command 与幂等证据

Repository 的 `FinalizePreparedRun` command fingerprint 已升级到 `Shanmen.Items.Command.FinalizePreparedRun.r3`，覆盖全部 reward、rare 与 affix 字段。Runtime lifecycle request ID 同步升级到 `demo_map.ShanmenRun.Finalize.r3`。

因此：

- 完全相同请求可在进程内或重启后安全重放；
- 相同 request ID 携带不同 metadata 会冲突失败；
- 写盘失败不会发布 metadata、物品、terminal marker 或 revision 的任何部分；
- 成功 Extraction 后 identity、placement、definition 与 metadata 在一次 authority revision 中共同生效。

### 5. 旧持久物品与新战利品一致性

迁移测试为旧持久 child material 注入 jackpot/rare metadata，并验证 schema 2 authority 中精确保留。Runtime lifecycle 测试为 TrainingBlade 注入 jackpot、rare 与 Power Tier2 affix，验证：

- 故障注入的 disk commit 会完整回滚；
- 成功 Extraction 后 canonical metadata 精确一致；
- authority 重启后 metadata、identity 与 terminal graph 保持一致；
- retired Profile bytes 不变，没有恢复 Code B 或旧 Profile writer。

## 完整性与兼容性

- 继续使用 ShanmenItems repository、authority service、processed-request ledger 与原子 persistence；没有平行库存或额外产品真值。
- schema-1 文档可严格升级，原始字节有可审计 backup；schema-2 restart 为 write-free。
- P1.10 plain loot、prepared originals、Death / Abandon tombstone、child-container 恢复与 replay 语义保持兼容。
- schema-1 migration ID 保持稳定；新的 schema-2 candidate evidence 与 finalize fingerprint 纳入 metadata。
- `Destroyed` item 仍不可被消费或重新准备，但 metadata 被保留用于审计。
- 核心 `ShanmenItems` 不依赖 World、Actor、GameplayStatics、伤害执行或随机 API。
- 无关未跟踪 Prompt、旧 Report、PDF 与自动化文档未纳入本阶段提交。

## 验证

### P1.11 定向回归

命令筛选：

`Shanmen.0_0_10.Items.PersistenceDocument + Shanmen.0_0_10.Items.Migration + Shanmen.0_0_10.Items.RunLifecycle`

- 结果：15/15 Success，0 Fail，队列正常清空；
- 覆盖：schema-1 authenticity、schema-2 原子升级与 exact backup、future-schema 拒绝、migration metadata、Runtime metadata import、commit failure rollback、restart round-trip 与 retired writer fence；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.11.r0_targeted_final.log`；
- SHA-256：`99A48CAF6FF51DC1EC88715DBDF15AB7DECA6AC999F7A7B68AFED63BE31E7115`。

### 完整 0.0.10 回归

- 结果：62/62 Success，0 Fail，队列正常清空；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.11.r0_automation_final.log`；
- SHA-256：`C1E31C488A75BFD289B26FC95FC890F34667129B8C4D3C0CC48688A67CB4A15A`。

### 构建

最终源代码状态使用：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- Editor：7/7 actions，成功，17.71 秒，原生退出码 0；
- Game：4/4 actions，成功，24.64 秒，原生退出码 0。

### 静态边界

- `git diff --check`：原生退出码 0；
- ShanmenItems 中 `UWorld`、`AActor`、`UGameplayStatics`、`ApplyDamage` 与随机执行 API：0；
- lifecycle / migration metadata adapter 中 `SaveProfile`、`CommitOutOfRaid`、Code B save 与旧 `BeginRun` 调用：0；
- 唯一文本命中 `demo_map` 位于 core 注释中的“产品策略留在边界”说明，不是代码依赖。

## P/F 边界声明

本报告属于 P 阶段功能实现、静态审查、headless automation 与必要 Editor/Game 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续建议

P1.12 可在 schema-2 单一权威已经承载完整奖励事实的基础上，开放唯一产品 `StartPreparedRun` 入口：把选择、reserve/commit、Runtime materialization 与 claim 绑定为一条受控命令链，并继续保持 Code A→ShanmenItems 的单向迁移与 retired-writer fence。
