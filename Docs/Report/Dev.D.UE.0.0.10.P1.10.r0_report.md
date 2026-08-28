# Dev.D.UE.0.0.10.P1.10.r0 开发报告

## 结论

**PASS。** P1.10 已补齐 prepared Run 的三类终局语义：Extraction 可原子返还原物并导入普通新战利品，Death / Abandon 可原子销毁本局携带身份并留下可审计 tombstone。全部路径继续由 ShanmenItems 单一权威、同一 durable command ledger 和一次 authority revision 完成。

最终源代码状态通过：

- P1.10 terminal / import 定向回归：3/3 Success；
- `Shanmen.0_0_10` 完整回归：61/61 Success；
- Editor Development 构建：成功，原生退出码 0；
- Game Development 构建：成功，原生退出码 0；
- `git diff --check`：原生退出码 0。

## 功能性

### 1. Extraction 新战利品导入

`FinalizePreparedRun` 现在可在同一事务中处理 prepared originals 与 Runtime-created acquisitions：

- prepared originals 继续按 P1.9 记录的精确来源容器/格位返还；
- acquisition 身份必须未与 prepared、现有 authority 或同请求中的其它身份冲突；
- definition 不存在时由请求携带完整 ShanmenItems canonical definition 并写入 authority；已存在时必须逐字段一致；
- 新身份按 GUID 稳定排序，并放入同 owner/scope 的唯一 `Warehouse` 首个可用格位；
- 放置前预演全部原 Stack 返还，容量不足时整笔请求失败，原物、definition、新身份和 terminal marker 均不发生部分写入；
- 无关 owner/scope 的 Warehouse 不参与选择，也不会阻断当前 authority context；
- Backpack / SpatialRing 类新身份可同时创建确定性、空的 item-owned child container；其 ID 由 `ActiveRunId + ItemInstanceId` 派生，重放与重启稳定。

### 2. Runtime acquisition adapter

`Fdemo_mapShanmenRunLifecycleAdapter` 将 Runtime settlement 中的身份分为：

- prepared originals：definition 必须与 prepared receipt 一致；
- acquired identities：必须由当前 `ActiveRunId` 创建，且只能在 Extraction 导入。

Adapter 从 immutable product registry 构造 ShanmenItems definition、数量能力、部署能力以及 Backpack / SpatialRing 子容器拓扑，并按 GUID 排序后提交一个 durable finalize command。普通战利品完成身份、definition、数量、格位与可选子容器的一次性导入。

### 3. Reward metadata 失败关闭

当前 ShanmenItems schema 尚无 reward provenance、rare policy/tier/bonus 与 affix 的 canonical 字段。P1.10 不静默丢弃这些信息：

- 任一 metadata-bearing acquisition 在 authority mutation 之前返回 `AcquiredMetadataUnsupported`；
- 拒绝前后 authority snapshot 完全一致；
- plain loot 可正常导入；metadata schema 留给后续显式版本化阶段。

### 4. Death / Abandon 终局策略

新增 `Destroyed` item state 与 `Death`、`Abandon` reason-specific terminal marker：

- 当前 Run 的全部 prepared identity 变为永久审计 tombstone；
- Quantity、Durability、Charges 清零，placement、child link 与 deployment reservation 清除；
- 所有 prepared reservations 在同一次事务中释放；
- 空的 item-owned child container 删除；
- 非空 child container 若仍持有未部署的安全物品，则从已销毁父物品脱离并改为 `RecoveredStorage`，其中物品保持 Stored；
- Death 与 Abandon 使用相同损失策略、不同 durable marker；exact replay 返回同一 receipt，不重复修改。

### 5. 不可用身份全链封锁

`Destroyed` 与既有 `Depleted` 一样：

- 不可占用容器格位；
- 不可建立新 reservation；
- resource total / available resource 为 0；
- 不进入 Preparation projection，也不能再次被装备或选作材料。

## 完整性与兼容性

- 继续使用 ShanmenItems schema-1 document 与 processed-request ledger，没有新增第二库存、旁路文件或第三套事务系统。
- finalize fingerprint 升级为 `FinalizePreparedRun.r2`，覆盖 terminal reason、全部 originals、acquisition definition/tags/数量与 child topology。
- P1.9 Extraction、claim、旧 Purpose envelope、写盘回滚和重启重放语义保持兼容。
- Runtime summary 与 immutable settlement snapshot 的 terminal reason 必须一致。
- retired Profile item fields 与 Code B 均未被生命周期 adapter 写入；产品集成测试验证 retired Profile 文件前后字节一致。
- 无关未跟踪 Prompt、旧 Report、PDF 与自动化文档未纳入本阶段提交。

## 有意保持关闭

- reward provenance、rare reward metadata 与 affix 的持久化导入尚未开放；当前明确 fail closed；
- 产品级唯一 `StartPreparedRun` 入口仍未开放；本阶段只补齐其 terminal prerequisites；
- 不依据 Runtime metadata 临时扩充并行 JSON、Profile 字段或 Code B writer。

## 验证

### P1.10 定向终局回归

命令筛选：

`Shanmen.0_0_10.Items.PreparedRunLifecycleLedger + Shanmen.0_0_10.Items.RunLifecycle`

- 结果：3/3 Success，0 Fail，队列正常清空；
- 覆盖：plain loot import、metadata reject 无 mutation、deterministic child container、容量失败原子性、unrelated warehouse、Death、Abandon、replay、durable restart 与 retired Profile 不变；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.10.r0_terminal_final.log`；
- SHA-256：`AF4D84130CC4E691E355F21A2F350A179DC485C2ABA4D8C88BB4EFA886BE2F3E`。

### 完整 0.0.10 回归

- 结果：61/61 Success，0 Fail，队列正常清空；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.10.r0_automation_final.log`；
- SHA-256：`E279C276C36AD501A839EBFD79C0C03DB8AED0C064427C0AAA66B2FF0A5C8E77`。

### 构建

最终源代码状态使用：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- Editor：4/4 actions，成功，10.64 秒，原生退出码 0；
- Game：3/3 actions，成功，14.15 秒，原生退出码 0。

### 静态边界

- `git diff --check`：0；
- P1.10 ShanmenItems / lifecycle adapter 路径中的 `UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`、`FMath::Rand`、`FMath::FRand`、`FRandomStream`：0；
- `SaveProfile`、`WriteProfile`、`CommitProfile`、旧 `BeginRun` 调用：0；
- 纯数据 authority 不依赖产品 UI、World 或随机执行顺序。

## P/F 边界声明

本报告属于 P 阶段功能实现、静态审查、headless automation 与必要 Editor/Game 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续建议

P1.11 先对 authority schema 做显式版本化，加入 immutable reward provenance 与 affix 数据，再移除 `AcquiredMetadataUnsupported` gate。完成后即可在不丢信息的前提下，把所有 reward source 接入统一 Extraction import，并继续推进唯一产品 `StartPreparedRun` 入口。
