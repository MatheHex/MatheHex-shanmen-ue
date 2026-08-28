# Dev.D.UE.0.0.10.P1.11.r0 开发日志

## 基线

- 日期：2026-08-27（America/New_York）
- 分支：`agent/0.0.10-p1-11-reward-metadata`
- 基线提交：`c47dcd441da96623daad30de17ba00f6fcfe5764`
- 上一阶段：P1.10 terminal settlement / plain loot import
- 阶段目标：版本化 ShanmenItems authority document，建立 reward provenance、rare metadata 与 affix 的 immutable canonical schema，并移除 metadata-bearing acquisition 的临时失败关闭 gate。

## 开工审查

P1.10 已保证 identity、definition、placement、child topology 与 terminal marker 在一个 durable finalize transaction 中提交，但对任何非默认 Runtime reward metadata 返回 `AcquiredMetadataUnsupported`。产品侧已有两种事实来源：

- `Fdemo_mapPersistentItemRecord`：Code A 旧持久物品；
- `Fdemo_mapRuntimeSettlementItem`：当前 Run 新生成战利品。

二者字段语义相同但结构不同。P1.11 选择“一个 canonical core schema + 一个共享产品适配器”，而不是分别维护迁移 metadata 和 Runtime metadata 两套逻辑。

## Canonical 类型与不变量

在 `ShanmenItemTypes` 中新增：

- `EShanmenItemRewardEventKind`；
- `EShanmenItemRewardAffixTier`；
- `EShanmenItemRewardAffixAcquisition`；
- `FShanmenItemResolvedRewardAffix`；
- `FShanmenItemRewardMetadata`。

Metadata 被加入 `FShanmenItemInstance` 与 `FShanmenItemRunAcquiredItem`。结构校验要求：

- event kind 与 event ID / multiplier / source role 成组出现；
- rare event、policy、tier、bonus 成组且数值有效；
- affix event、policy、acquisition 与非空 affix set 成组；
- affix 数量不超过 16，ID 唯一，tier/acquisition 必须是明确支持值；
- magnitude 非零，resolved value 为正；
- 空 metadata 必须全部为默认值，不接受半初始化状态。

Unknown enum 不被隐式当作 None，所有不认识的序列化值均失败关闭。Equality、snapshot validation、acquisition validation 与 repository state validation 都纳入 metadata。

## 产品边界映射

新增：

- `Source/demo_map/demo_mapShanmenItemMetadataAdapter.h`
- `Source/demo_map/demo_mapShanmenItemMetadataAdapter.cpp`

共享适配器为 persistent record 与 Runtime settlement item 提供两个入口，内部汇合到同一映射逻辑。流程是：

1. 读取 reward event、source role、rare policy/tier/bonus 和 affix set；
2. 使用 `Fdemo_mapRewardEventRules::IsValid` 验证 reward/rare 组合；
3. 使用 `Fdemo_mapRewardAffixPolicyRegistry::ValidateSet` 验证 affix policy 与解析结果；
4. 显式映射 product enum 到 core enum；
5. 调用 `FShanmenItemRewardMetadata::IsValid` 做 canonical 结构复核。

这使产品规则继续归 `demo_map` 所有，ShanmenItems 只持有稳定事实与通用结构不变量。

## Repository 与 lifecycle

`FinalizePreparedRun` fingerprint namespace 从 r2 升级为 `Shanmen.Items.Command.FinalizePreparedRun.r3`，逐字段包含 metadata 和每个 affix。导入时 `FShanmenItemRunAcquiredItem::RewardMetadata` 复制到 authority identity；exact replay 仍返回旧 receipt，不同 metadata 则触发 request conflict。

Runtime lifecycle adapter：

- 删除 `AcquiredMetadataUnsupported` gate 与结果码；
- 使用共享 adapter 构造 acquisition metadata；
- lifecycle request ID namespace 升级为 `demo_map.ShanmenRun.Finalize.r3`；
- request ID 同样覆盖 reward、rare 与 affix 字段；
- 成功诊断改为 canonical loot with metadata。

Destroyed tombstone 保留 metadata，防止终局后丢失 provenance；资源、placement、reservation 等不可用状态约束保持不变。

## Schema 1→2 持久化

`FShanmenItemAuthorityDocument` 现在声明：

- `LegacySchemaVersion = 1`
- `CurrentSchemaVersion = 2`

新增 `ComputeLegacySchema1SnapshotDigest`，通过复制 document、将 schema 设回 1、移除所有 item metadata 后计算旧格式 digest。严格 reader 的处理为：

1. schema 2：按当前结构与 current digest 验证；
2. schema 1：拒绝任何已携带 metadata 的伪 legacy 文档，以 legacy digest 验证原始证据，再注入默认 metadata 并规范化为 schema 2；
3. schema > 2 或其它未知版本：返回 unsupported future schema，不静默 reset。

`FShanmenItemLoadResult` 新增 `bSchemaUpgraded`。Authority open 接受 legacy initial snapshot evidence，并用当前 Code A migration candidate 对 immutable initial items 做受控 metadata enrichment；数量、状态、identity、placement 与 topology 必须保持不变。

为兼容已发布 schema-1 migration identity：

- MigrationId 继续从 metadata-free source candidate 派生；
- schema-2 CandidateDigest 覆盖完整 metadata；
- 仅在 legacy initial evidence 且同一 migration source 时允许旧 CandidateDigest 被当前完整 digest 替换。

升级成功后使用既有原子 writer 写入 schema 2，并保留 schema-1 原始字节 backup。若调用 save 时 repository 无业务变化但 durable primary 仍为 schema 1，也强制执行一次正常 commit，避免升级只活在内存。

## 迁移路径

`Fdemo_mapShanmenItemMigration::BuildCandidate` 对每个 Code A persistent record 调用共享 metadata adapter。CandidateDigest 在 metadata 非空时纳入全部 provenance 与 affix，因此旧来源被人工改写或策略失配会被检测，而不是静默接受。

实现后静态复查发现：若 MigrationId 也直接使用 metadata-aware candidate，会让既有 schema-1 authority 的 source identity 漂移。已修正为 legacy identity / current evidence 分离，并在修正后重新完成 Editor、定向测试、完整测试与 Game 构建。

## 自动化测试

### PersistenceDocument.Schema1MetadataMigration

新增测试从当前 fixture 构造真实 schema-1 JSON：移除 `RewardMetadata`、写入 legacy digest，然后验证：

- read-only 读取可在内存升级但不改磁盘；
- authority open 使用 migration candidate 补齐 metadata；
- 写出的主文档为 schema 2；
- backup 与原始 schema-1 bytes 完全相等；
- restart 后 graph/metadata 一致且不再次写盘；
- future schema 继续失败关闭。

### Product migration

旧持久 fixture 增加 WindTalisman Haste affix，并为 child material 增加 jackpot/rare metadata。断言迁移后 canonical authority 精确保留 event、policy、tier、bonus、source role、affix ID/tier/magnitude/value。

### Runtime lifecycle

将旧的 metadata reject 用例改为 metadata commit 用例。TrainingBlade 带 jackpot、rare 与 Power Tier2 affix：

- 注入 pre-commit disk failure 后，snapshot 完全不变；
- 成功重试后 identity 导入 Warehouse；
- metadata 与 Runtime source 精确相等；
- restart 后 metadata、terminal marker 和 authority graph 保持一致；
- retired Profile bytes 不变。

测试总数由 61 增至 62。

## 最终验证

定向命令语义：

`Automation RunTests Shanmen.0_0_10.Items.PersistenceDocument+Shanmen.0_0_10.Items.Migration+Shanmen.0_0_10.Items.RunLifecycle`

- 15/15 Success、0 Fail、queue empty、原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.11.r0_targeted_final.log`；
- SHA-256：`99A48CAF6FF51DC1EC88715DBDF15AB7DECA6AC999F7A7B68AFED63BE31E7115`。

完整命令语义：

`Automation RunTests Shanmen.0_0_10`

- 62/62 Success、0 Fail、queue empty、原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.11.r0_automation_final.log`；
- SHA-256：`C1E31C488A75BFD289B26FC95FC890F34667129B8C4D3C0CC48688A67CB4A15A`。

## 构建

统一使用：

`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- 主实现首次 Editor Development：26/26 actions，101.45 秒，退出码 0；
- 主实现首次 Game Development：23/23 actions，87.30 秒，退出码 0；
- 最终修正后 Editor Development：7/7 actions，17.71 秒，退出码 0；
- 最终修正后 Game Development：4/4 actions，24.64 秒，退出码 0。

## 静态审查

- `git diff --check`：原生退出码 0；
- ShanmenItems core 的 World、Actor、GameplayStatics、damage 与 random API：0；
- 新 metadata adapter、migration 与 lifecycle adapter 的旧 Profile / Code B writer 和旧 `BeginRun`：0；
- core 对 `demo_map` 的唯一搜索命中是注释中的边界说明；
- unrelated dirty/untracked files 保留原状，阶段提交使用显式路径暂存。

## P/F 边界

本阶段只执行代码实现、静态审查、headless Unreal automation 与必要 Editor/Game 编译。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段入口

P1.12 建议实现唯一产品 `StartPreparedRun` 入口，把 selection、reserve/commit、Runtime materialization 与 claim 绑定为单一受控链路；schema-2 metadata 可直接作为后续奖励显示、审计与回放来源，不再读取 Code A/Runtime 并行真值。
