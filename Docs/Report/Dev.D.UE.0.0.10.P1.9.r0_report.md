# Dev.D.UE.0.0.10.P1.9.r0 开发报告

## 结论

**PASS。** P1.9 已建立从 P1.8 prepared loadout 到 Runtime、再由 Extraction Settlement 返回 ShanmenItems 的耐久闭环，并保留崩溃恢复、幂等重放和失败回滚语义。

最终源代码状态通过：

- P1.9 生命周期与旧收据兼容定向回归：3/3 Success；
- `Shanmen.0_0_10` 完整回归：60/60 Success；
- Editor Development 构建：成功，原生退出码 0；
- Game Development 构建：成功，原生退出码 0；
- `git diff --check`：原生退出码 0。

## 功能性

### 1. Durable prepared-run claim

ShanmenItems 增加 `ClaimPreparedRun`：

- 只接受已有成功 `CommitBatch` 的 prepared batch；
- 一次只允许一个 active claim；
- `ActiveRunId` 由稳定输入确定性生成并持久化在现有 receipt ledger；
- 同请求、重启后重放均返回同一 durable receipt，不重复变更 authority；
- 无对应 prepared batch、重复 active claim 或身份不一致时 fail closed。

### 2. Durable extraction finalize

ShanmenItems 增加 `FinalizePreparedRun`：

- 当前只允许 `Extraction` terminal outcome；
- 校验 Runtime 回传的 prepared originals 与 active claim 精确对应；
- DeploymentLock 装备必须完整 secured；
- Quantity remaining 必须位于 `0..CommittedAmount`；
- 剩余完整/部分 Stack 返回准备前记录的精确容器和格位；
- 一次 authority revision 内释放全部 prepared reservations 并写入 terminal marker；
- 写盘失败回滚完整内存状态，重试仅产生一次最终提交；
- terminal 请求重启后幂等重放，不重复返还或释放物品。

### 3. Runtime / Settlement 单向适配

新增 `Fdemo_mapShanmenRunLifecycleAdapter`：

- 从 P1.8 receipt 构造现有 Runtime materialization plan；
- claim 成功后以相同 `ActiveRunId` 物化，失败时 Runtime 临时变更回滚，但 durable claim 保留供自动恢复；
- 进程重启后可从 ShanmenItems ledger 恢复 prepared receipt 与 claim，并重新物化同一 Run；
- Settlement 从 Runtime snapshot 读取 prepared originals，只把明确的 Extraction 结果提交给 ShanmenItems；
- 不写 retired Profile item fields，不写 Code B，不建立第三库存权威。

### 4. 精确来源格位与旧收据兼容

P1.9 在 Quantity reservation Purpose 中封装准备时的 `SourceContainerId` 和 `SourceSlotIndex`，用于 Extraction 精确返还。解析仍兼容 P1.7/P1.8 的 plain logical Purpose：

- 旧收据可读取、提交和跨重启重建；
- 不伪造旧收据中不存在的来源格位；
- 旧收据进入需要精确返还的 P1.9 Runtime 前保持 fail closed，应重新选择 Stack 生成带 placement 的新 intent。

### 5. Manifest 兼容修复

Reward source projection 现在明确区分：

- 不绑定具体 slot 的 policy prototype；
- 已绑定 `DistributionProfileId + SlotId` 的 concrete projection。

Definition/registry 全局验证使用 prototype contract，产品实例仍使用完整 `IsValid()`。同步修正旧四槽测试为当前五槽 manifest，避免把合法的 P73.4 policy prototype 误判为非法产品实例。

## 完整性与兼容性

- 沿用 ShanmenItems schema-1 文档及 processed-request ledger，没有新增并行存储文件或第二套事务系统。
- 保持 P1.8 `CommitBatch` receipt 为 prepared loadout 唯一来源。
- authority service、repository、GameInstance owner 与 Runtime adapter 的 API 链完整。
- retired Profile 文件在生命周期集成测试前后字节一致。
- 旧 plain Purpose parser 回归测试覆盖 reserve、atomic commit、重启重建和无额外写入。
- `bCanStartRun` 产品门仍保持关闭；本阶段没有绕开现有产品启动策略。

## 失败关闭边界

- `Death` 与 `Abandon` 的装备销毁/保留政策尚未冻结，当前拒绝 terminal finalize。
- Runtime 新拾取物尚无 ShanmenItems canonical import/placement 契约，Extraction 检测到新物或重复身份时拒绝结算。
- 旧 plain Purpose 没有精确来源格位，只保证读取兼容，不猜测 Settlement 位置。
- 产品级 `StartPreparedRun` 尚未开放；本阶段只完成可验证的内部闭环。

## 验证

### 定向生命周期回归

命令筛选：

`PlainPurposeReceiptCompatibility + PreparedRunLifecycleLedger + RunLifecycle.ClaimRestartFinalize`

- 结果：3/3 Success，0 Fail，队列正常清空；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.9.r0_lifecycle_final.log`；
- SHA-256：`7735CF0F34AAEBE39655F74611718EBC62E0B67B4698A6A89ACCFBBE28607752`。

### 完整 0.0.10 回归

- 结果：60/60 Success，0 Fail，队列正常清空；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.9.r0_automation_final.log`；
- SHA-256：`97338D0714FEE18A822A1798A530E71BACDE98A18439E0DF8D406B7EE3D5D79E`。

### Manifest 定向修复验证

- 首次：0/2，确认是 prototype/concrete validation 混用与旧四槽测试假设；
- 修复后：2/2 Success；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.9.r0_manifest_final.log`；
- SHA-256：`4CA0513EDF36D93516BB062280E2FF80A91A69BD6787E45B1A08018933C4A3B4`。

### 构建

最终源代码状态使用：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- Editor：5/5 actions，成功，16.29 秒，原生退出码 0；
- Game：4/4 actions，成功，23.44 秒，原生退出码 0。

本阶段此前还完成过一次完整依赖构建：Editor 137/137、Game 144/144，均成功、原生退出码 0。

### 静态边界

- `git diff --check`：0；
- P1.9 ShanmenItems/adapter 路径中的 `UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`、随机数、`SaveProfile`、`BeginRun`：0；
- adapter 中 Profile/Code B 持久化 writer：0；
- 无关未跟踪 Prompt、Report、PDF 与自动化文档未纳入本阶段提交。

## P/F 边界声明

本报告属于 P 阶段实现、静态审查、headless automation 与必要构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续建议

P1.10 优先冻结并实现两项缺口：

1. Runtime 新拾取物进入 ShanmenItems 的 canonical import + deterministic placement；
2. `Death` / `Abandon` 的装备与剩余 Stack terminal policy。

完成后再开放唯一产品 `StartPreparedRun` 入口，避免产品层在不完整 Settlement 语义上提前运行。
