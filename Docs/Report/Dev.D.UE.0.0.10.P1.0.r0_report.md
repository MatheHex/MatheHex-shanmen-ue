# Dev.D.UE.0.0.10.P1.0.r0 开发报告

## 结论

`PASS`。0.0.10 已新增独立 Runtime 模块 `ShanmenItems`，完成第一版单一物品权威与可持久重放的资源事务内核。

本轮建立 Definition、Container、ItemInstance、Reservation、ProcessedRequest 与 AuthoritySnapshot；实现 `Reserve / Commit / Cancel / ReleaseDeployment`，覆盖暗器和阵材数量、飞剑部署与回收、已部署飞剑耐久、护心镜触发充能。相同 RequestId 可在进程重载后精确重放，冲突 payload、跨 Owner/Run、内容版本不一致、陈旧 Revision、超额预留和损坏闭包均失败关闭。

本轮没有连接 Code A、Code B、旧技能、Actor、GAS、UI 或产品运行入口，因此没有形成迁移期双写。

## 模块边界

`ShanmenItems` 只依赖：

- `Core`
- `CoreUObject`
- `GameplayTags`
- `ShanmenCore`

它不依赖 `demo_map`、`ShanmenCombatCore`、Actor、World、GAS 或表现层。CombatCore 的护心镜 Impact receipt 与 Items 的 Charge Commit 将由后续组合模块衔接，两个底层模块保持兄弟依赖关系。

## 权威数据模型

### Definition

定义使用 Gameplay Tags 声明资源能力：

- `ConsumeQuantity`
- `Deploy`
- `Durability`
- `Charges`

P1.0 明确禁止一个 Definition 同时把 Quantity 与其它实例资源混合，避免给堆叠中的每个单位伪造共享耐久或共享充能。需要独立状态的装备、飞剑或灵器必须使用独立 ItemInstance。

### Container 与 ItemInstance

- 容器使用固定逻辑槽位和稳定 ContainerId。
- 每个非耗尽实例必须与一个槽位形成双向精确闭包。
- RunId、OwnerId、DefinitionId、父容器、槽位和 Revision 全部进入不变量。
- 数量完全耗尽后实例进入 `Depleted` 审计墓碑并退出槽位；历史 Receipt 仍可引用原身份。

### AuthoritySnapshot

Snapshot 同时保存：

- 物品定义、容器和实例图；
- Reserved/Committed/Cancelled/Released 状态；
- 每个已处理 RequestId 的 canonical fingerprint 与完整 Receipt；
- ContentStamp 和 AuthorityRevision。

`TryLoadSnapshot` 先在候选状态中验证完整闭包，成功后才原子替换 Repository；失败不会部分污染已有权威。

## 事务语义

### Reserve

- 验证 Content、Run、Owner、Item、Capability 和 ExpectedItemRevision。
- 只减少 Available，不提前减少 Quantity、Durability 或 Charges。
- 多个数量预留可以并存，但总额不能超过真实资源。
- 飞剑 DeploymentLock 在 Reserved 阶段独占实例。

### Commit / Cancel

- Commit 使用已成立 Reservation，不因其它已提交预留推进 ItemRevision 而失效。
- Cancel 只释放预留，不修改物品资源。
- Commit 后 Cancel、Cancel 后 Commit 使用新 RequestId 均明确拒绝。
- 完全消耗会清槽并保留 Depleted 墓碑。

### Deployment

- 飞剑 Reserve 后仍为 Stored；Commit 后同一 ItemInstance 转为 Deployed，不扣数量。
- Deployed 状态拒绝再次部署，但允许命中 Durability/Charges 事务。
- ReleaseDeployment 使用原 Deployment Reservation 恢复 Stored，清除部署身份。

### 幂等与失败

- 相同 `RequestId + payload` 返回字节语义一致的 Receipt，不重复修改资源。
- 相同 RequestId 携带不同 payload 返回 `RequestIdConflict`。
- 成功和失败 Receipt 都持久化；AuthorityRevision 覆盖 ledger 变化。
- 失败不会修改物品、容器、Reservation 或资源数值。

## 自动化覆盖

P1 测试命名空间：`Shanmen.0_0_10.Items`

| 测试 | 结果 |
|---|---|
| `SnapshotInvariants` | PASS；闭包、重复放置、原子失败加载 |
| `QuantityReserveCommit` | PASS；Reserve 不扣、Commit 精确扣除、重试不双扣、终态冲突 |
| `CancelAndRequestConflict` | PASS；取消回退 availability、payload 冲突拒绝 |
| `ConcurrentReservations` | PASS；6+4 并发预留、超额拒绝、完全耗尽墓碑与重载 |
| `DeploymentLifecycle` | PASS；飞剑锁定、部署、部署中耐久提交、回收与再次可用 |
| `TriggeredChargeCommit` | PASS；护心镜触发前保留 charge，Commit Point 扣除一次 |
| `PersistenceReplay` | PASS；Reserve/Commit 跨两次 Snapshot 重载精确重放 |
| `FailureIsolation` | PASS；不足、跨 Owner、错误 Content、陈旧 Revision、错误资源通道 |

最终执行整个 `Shanmen.0_0_10` 命名空间：P0 CombatCore 9 项 + P1 Items 8 项，共 `17/17 Success`、`0 Fail`，队列正常清空，原生退出码 `0`。

自动化日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.0.r0_automation.log`；SHA-256：`2D8487589CFE660E1FEA6241AD8DEEF5578963CECC99F4E9BB3C21D3B206A424`。

UE 5.8 在目标测试开始前仍输出与 P0 基线相同的 13 条内置 `Condition failed` 启动诊断；它们不属于目标命名空间，随后 17 项目标测试均成功。

## 构建与静态检查

- `git diff --check`：退出码 `0`。
- `demo_mapEditor Win64 Development -MaxParallelActions=1 -NoUBA`：最终退出码 `0`，4 actions，`Result: Succeeded`。
- `demo_map Win64 Development -MaxParallelActions=1 -NoUBA`：最终退出码 `0`，3 actions，`Result: Succeeded`。
- ShanmenItems 边界扫描：`demo_map / UWorld / AActor / ApplyDamage / UGameplayStatics / GameplayAbility / AbilitySystem / RNG` 匹配数 `0`。

## 修改范围

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenItems/**`
- `Docs/Architecture/Dev.D.UE.0.0.10_ItemTransactions_ADR.md`
- 本 Report 与同名 Log

工作区原有未跟踪 Prompt、Report、自动化文档和用户文件均未纳入本轮提交。

## 未包含与下一步

本轮冻结的是可序列化 Snapshot 和事务内核，不是完整产品接入：

- 尚未实现磁盘／云存档 I/O 与原子文件替换。
- 尚未实现 Code A／Code B 到新 Snapshot 的一次性迁移器。
- 尚未实现装备移动、Loadout、词缀 Fragment、定义 Manifest 或 UI。
- 尚未实现网络复制、异步并发调度或产品运行入口。
- Schema 6 -> 7 的旧 Profile 迁移缺陷仍是首次真实迁移前置任务。

下一轮建议为 P1.1：先修复并参数化验证旧 Profile 的 N-1 迁移，再建立 Code A／Code B 只读候选到 ShanmenItems Snapshot 的失败关闭迁移入口。

## P/F 边界

本轮仅执行代码开发、静态扫描、命令行 Automation 和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-item-transactions/Docs/Report/Dev.D.UE.0.0.10.P1.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-item-transactions/Docs/Log/Dev.D.UE.0.0.10.P1.0.r0_log.md>
- 事务 ADR：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-item-transactions/Docs/Architecture/Dev.D.UE.0.0.10_ItemTransactions_ADR.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p1-item-transactions>

`READY_FOR_0_0_10_P1_1_PERSISTENCE_MIGRATION`
