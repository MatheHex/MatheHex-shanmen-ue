# Dev.D.UE.0.0.10.P1.10.r0 开发日志

## 基线

- 日期：2026-08-27（America/New_York）
- 分支：`agent/0.0.10-p1-10-terminal-import`
- 基线提交：`1abb8a4cfcca1e71689418bf738d8e23e2753696`
- 上一阶段：P1.9 durable prepared Run claim / Extraction lifecycle
- 阶段目标：补齐 Runtime 新战利品进入 ShanmenItems 的 canonical import，以及 Death / Abandon 的 durable terminal policy。

## 开工审查

确认 P1.9 已具备：

- prepared receipt、active claim 与 Runtime materialization 的稳定身份链；
- Extraction 对 prepared originals 的精确来源格位返还；
- processed-request ledger、原子 candidate state、写盘失败回滚与重启重放；
- Profile / Code B retired-writer fence。

缺口集中在两处：Runtime 新身份无法进入 authority，Death / Abandon 被拒绝。实现继续扩展既有 `FinalizePreparedRun`，没有创建平行 settlement store。

## Authority 类型与不变量

新增：

- `EShanmenItemInstanceState::Destroyed`；
- `FShanmenItemRunAcquiredItem`；
- `AcquiredItemMismatch` 与 `ImportPlacementUnavailable`；
- `Death`、`Abandon`、`RecoveredStorage` lifecycle purpose。

`Destroyed` 是持久审计 tombstone：资源值全零、无 parent/child placement、无 deployment reservation。Repository 的 state validation、reserve gate、resource total 和 Preparation projection 均将其视为不可用。

Acquired request 携带完整 ShanmenItems definition、数量以及可选 child container type/capacity。请求校验拒绝无效 identity、非法数量、重复 identity、超过 4096 的 child capacity，以及 destructive outcome 中出现 secured/acquired rows。

## Extraction import 实现

`FinalizePreparedRun` 的 extraction 分支执行以下预检：

1. 验证 active claim、全部 committed reservations 与 prepared originals；
2. 验证 acquisition identity 不与 prepared、authority 或同请求身份碰撞；
3. 验证既有/新引入 definition 一致；
4. 为 child container 派生确定性 ID，并检查碰撞；
5. 选择当前 owner/scope 的唯一 Warehouse；忽略其它 context 的 Warehouse；
6. 在副本格位上预演全部 Quantity original 返还；
7. 按 acquisition GUID 顺序分配首个空格位；容量不足立即拒绝。

全部预检通过后才构造 candidate state：恢复 originals、写入缺失 definitions、创建可选空 child containers、插入新 items、释放 reservations、增加一次 authority revision 并记录一个 terminal receipt。最终 `ValidateState` 失败时不发布 candidate。

finalize fingerprint namespace 升级为 `Shanmen.Items.Command.FinalizePreparedRun.r2`，纳入 terminal reason、definition tags、资源上限、数量与 child topology。

## Death / Abandon 实现

destructive 分支不接受任何 secured/acquired row。它将 claim 中全部 prepared identity 转为 `Destroyed`，清除容器格位、资源与 deployment link，并释放所有 reservation。

对子容器采用保守策略：

- 空容器删除；
- 非空容器脱离已销毁父物品，改名为 `RecoveredStorage`；
- 容器内未参与本 Run deployment 的 Stored item 保留。

Death 与 Abandon 共享损失算法，但 receipt purpose 不同，便于审计和后续统计。

## Runtime adapter 实现

Adapter 现在映射 `Extraction`、`Death`、`Abandon`，并要求 Runtime summary reason 与 immutable snapshot reason 完全一致。

Extraction 中：

- prepared identity 必须匹配 receipt definition；
- 新 identity 必须带当前 `OriginRunId`；
- product registry 被映射为 ShanmenItems definition；
- Material / Consumable 映射 Quantity capability；
- 可装备项映射 Deployment capability；
- Backpack / SpatialRing 映射确定性 item-owned child topology；
- acquisitions 按 GUID 排序后进入一个 finalize command。

当前 Runtime settlement POD 含 reward event、source role、rare policy/tier/bonus 与 affix，但 ShanmenItems schema 尚无对应字段。为避免静默数据损失，adapter 在提交 authority 前逐项检测，metadata 非默认即返回 `AcquiredMetadataUnsupported`。

## 自动化测试

### Repository lifecycle ledger

扩展 `PreparedRunLifecycleLedger` 覆盖：

- Death 全 prepared identity tombstone；
- Abandon 同损失策略、独立 marker；
- 非空 child container 脱离并保留 safe item；
- Extraction 导入新 definition 与带两格 child storage 的新 Backpack identity；
- unrelated owner/scope Warehouse 不干扰当前导入；
- Warehouse capacity failure 不恢复原物、不导入新物；
- terminal exact replay 与 restart graph 一致。

### Runtime lifecycle adapter

扩展 `ClaimRestartFinalize`：

- active Run 生成一个 plain loot identity；
- metadata-bearing copy 被拒绝，authority snapshot 前后相等；
- 真实 plain loot 在 Extraction 后进入 Warehouse；
- terminal marker、imported identity 与 restart persistence 一致。

新增 `DeathAndAbandon`：

- 两种 Runtime terminal reason 分别执行完整 durable adapter；
- snapshot 不包含 secured item；
- prepared weapon 变为 Destroyed；
- marker reason 正确；
- replay 为 NoChange；
- retired Profile bytes 不变；
- restart 后 snapshot 与 terminal state 相等。

### 最终结果

定向命令语义：

`Automation RunTests Shanmen.0_0_10.Items.PreparedRunLifecycleLedger+Shanmen.0_0_10.Items.RunLifecycle`

- 3/3 Success、0 Fail、queue empty、原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.10.r0_terminal_final.log`；
- SHA-256：`AF4D84130CC4E691E355F21A2F350A179DC485C2ABA4D8C88BB4EFA886BE2F3E`。

完整命令语义：

`Automation RunTests Shanmen.0_0_10`

- 61/61 Success、0 Fail、queue empty、原生退出码 0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.10.r0_automation_final.log`；
- SHA-256：`E279C276C36AD501A839EBFD79C0C03DB8AED0C064427C0AAA66B2FF0A5C8E77`。

## 构建

最终源代码状态统一使用：

`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- Editor Development：4/4 actions，成功，10.64 秒，原生退出码 0；
- Game Development：3/3 actions，成功，14.15 秒，原生退出码 0。

本阶段主实现后的首次 Editor 增量构建为 25/25 actions、93.07 秒、退出码 0；首次 Game 增量构建为 22/22 actions、78.55 秒、退出码 0。最终两次增量构建覆盖之后发生变化的 repository/test translation units。

## 静态审查

- `git diff --check`：原生退出码 0；
- P1.10 authority / adapter 范围内 World、Actor、伤害、GameplayStatics 与随机 API：0；
- Profile writer、Code B writer 与旧 `BeginRun`：0；
- 所有 mutations 继续先发生在 repository candidate state，再经 durable authority service 写盘；
- unrelated dirty/untracked files 保留原状，未加入阶段提交。

## 有意保持关闭

- reward provenance / rare metadata / affix persistence；
- metadata-bearing acquisition import；
- 产品唯一 `StartPreparedRun` 入口；
- Editor UI、PIE、Standalone、真实输入、Smoke、Cook 与 Package。

## 下一阶段入口

P1.11 应版本化 authority document 和 item instance metadata，建立 reward provenance / affix 的 immutable canonical schema、迁移与 round-trip 测试。完成后删除 metadata gate，再将全部 reward source 接入同一 Extraction import。
