# Dev.D.UE.0.0.10.P1.13.r0 开发报告

## 结论

**PASS。READY_FOR_0_0_10_P1_14。** P1.13 已把正常宗门／Profile preparation UI 的产品开始链路切换到 P1.12 的 ShanmenItems 原子 `StartPreparedRun` 权威入口。产品路径不再以旧 Profile transaction 发布活动 Run，也不再通过旧 `CommitBatch + ClaimPreparedRun` 双阶段链路建立身份；瞬态 Runtime 只物化由 ShanmenItems durable receipt 确认的同一个 `ActiveRunId`。

当 durable authority 已存在活动 Run、但 Runtime 尚未物化或因技术性世界激活失败被清理时，宗门入口与战备投影会显示可恢复的精确 Run 身份。再次开始只重建同一 Runtime，不产生第二个 Run、不重复写 authority，也不会把技术故障记录成玩家 `Abandon`。Extraction、Death、Abandon 等真实终局通过 Shanmen lifecycle 终结。

最终状态通过：

- P1.13 产品链路定向回归：2/2 Success；
- `Shanmen.0_0_10` 完整回归：66/66 Success；
- Editor Development 构建：成功，原生退出码 0；
- Game Development 构建：成功，原生退出码 0；
- `git diff --check`：原生退出码 0；
- Shanmen core、旧双阶段启动与旧 writer 边界扫描：0 命中。

## 功能性

### 1. 正常产品入口完成 existing-first cutover

`Ademo_map0909BFrameworkHost` 在普通产品初始化时先探测既有 ShanmenItems authority：

1. 已存在、同 storage root、同 Owner 且 Ready 的 authority 直接复用；
2. 首次 0.0.10 启动时才打开稳定旧源，并由既有 cutover coordinator 发布 generation one；
3. cutover 成功后，宗门入口才报告 ShanmenItems 产品路径就绪；
4. 若 authority 中已有未终结 Run，入口直接显示其可恢复身份。

Code B loadout selection 仍可被读取用于世界确认后的审计关联，但不再决定资源提交、活动 Run 身份或 Runtime 物化。没有建立第三库存、第二 active marker 或旁路持久化。

### 2. Profile preparation UI 切到单次原子 Run-start

`Fdemo_mapProfilePreparationFlow` 动态识别与当前 root / Owner 精确绑定的 Ready authority。命中时：

- widget Start 与宗门直接 Start 均进入同一 `Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun`；
- `StartPreparedRun` receipt 同时提供 durable prepared identity 与 `ActiveRunId`；
- Runtime 仅在 durable Start 成功后物化；
- UI 使用兼容 presentation snapshot 显示活动状态，但不把 active Run 回写旧 Profile；
- 原始 Profile session 始终保持 `ReadyForPreparation`，其磁盘字节在完整开始／终局周期中不变。

authority 未 Ready、root / Owner 不匹配或旧独立测试未建立 cutover 时，原 Profile 流程保持不变，因此历史自动化和旧文档兼容面未被强制迁移。

### 3. 可见、同身份的 Runtime 恢复

Run lifecycle 新增只读 `TryFindRecoverableActiveRun`：从 authority processed-request ledger 中识别成功且尚未 finalized 的 atomic Start 或 legacy Claim，并拒绝多 active Run 的异常状态。该探针不写 Runtime、不写磁盘、不推进 revision。

若 Runtime materialization 中途失败：

- Runtime 回滚为 inactive；
- durable Start receipt 保留；
- Flow 返回 `RuntimeMaterializationFailed`，但保持 Preparation 可重试；
- preparation projection 开启 Start，并显示“恢复同一活动远征”；
- 下一次 Start 重放同一 authority receipt，重建同一个 `ActiveRunId`；
- authority document 与 save generation 不发生第二次变化。

若既没有 recoverable Run，也没有任何装备或完整物资选择，Start 保持禁用并显示可行动的选择提示，避免空战备误触。

### 4. 技术激活失败不再伪装成玩家终局

世界激活失败使用新增的 `RuntimeRollbackReady` 结果：

- 只清理瞬态 Runtime；
- 不调用 `FinalizePreparedRun`；
- 不生成 Extraction、Death 或 Abandon terminal receipt；
- 不把该状态声明为 durable settlement；
- coordinator 保留而不是“释放”可恢复 RunId；
- 返回宗门后提示再次开始将恢复同一远征。

因此产品技术故障与玩家主动放弃在类型、ledger、诊断和 UI 行为上均已分离。

### 5. 真实终局统一由 Shanmen lifecycle 持久化

authority-backed Run 的 Runtime settlement evidence 仍由现有 Runtime 产生，但 durable terminal 交给 `Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement`。成功后才清理 Runtime、清除 Flow 的 transient run identity 并返回 Preparation。可重试失败保留 settlement summary；不可恢复错误进入 RecoveryRequired。

旧 Profile settlement 仅服务没有 Ready Shanmen authority 的兼容流，不参与新的产品 Run。

## 完整性与兼容性

- 产品开始只有一个 durable `StartPreparedRun` receipt，无中间 `CommitBatch` 或 `ClaimPreparedRun`。
- technical rollback 后 authority 文档逐字段不变，且 `FinalizePreparedRun` 数量仍为 0。
- exact-identity recovery 不增加 save generation、不创建第二活动 Run。
- 真实 Extraction 后只有一个 Shanmen `FinalizePreparedRun`，Runtime 回到 inactive。
- 完整产品测试验证 Profile primary document 在开始、恢复、技术回滚与最终 Extraction 后字节不变。
- presentation snapshot 仅用于 UI / coordinator 兼容，不成为新的持久真值。
- Code B 仅保留 read-only audit correlation；产品 start / terminal 不调用旧 Profile、Code B writer 或 `StartRunWithoutPreparation`。
- legacy Profile 流程、旧 atomic/claim ledger 读取和既有测试继续工作。
- 无关未跟踪 Prompt、旧 Report、PDF 与自动化文档未纳入本阶段提交。

## 修改范围

- 产品 host existing-first cutover、宗门恢复提示与审计选择读取；
- GameMode / V3 progression 的 presentation snapshot、恢复身份与 technical rollback 接线；
- Profile preparation flow 的 authority-backed start、恢复、terminal 与兼容投影；
- Profile preparation UI 的 Start 能力与可恢复诊断；
- Shanmen Run lifecycle 的只读 active Run 恢复探针；
- 产品链路与 compatibility 自动化；
- 本 Report 与同名开发 Log。

未修改 ShanmenItems repository/schema、Profile schema、Code A / Code B 持久格式、战斗数值、地图、输入、奖励、Cook 或 Package 配置。

## 验证

### P1.13 定向回归

筛选：`Shanmen.0_0_10.Items.ProductFlow`

- 结果：2/2 Success，0 Fail，队列正常清空；
- 覆盖：normal product atomic Start、旧 Profile 零写入、single receipt、真实 terminal、Runtime mutation failure、same-ID recovery、technical activation rollback 与后续可终结性；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_targeted_final.log`；
- SHA-256：`EDE016A15AD32439A94F447172B02E0F12FF3C2592DACCFDF20A61FDF3698932`。

### 完整 0.0.10 回归

- 首轮：66 个测试中 65 Success、1 Fail；失败是旧 preparation projection 仍断言产品 Start 必须禁用，与本阶段明确启用原子 Start 的新契约冲突。更新该兼容断言并增加“空选择仍禁用”覆盖；没有产品实现失败。
- 首轮日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_automation_initial.log`；
- 首轮 SHA-256：`72ADBBA17F0952A22DC201CABDED6673FFB22EC1A8457EC4417239767C5B8983`。
- 最终：66/66 Success，0 Fail，队列正常清空；
- 最终原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_automation_final.log`；
- 最终 SHA-256：`BCD5067846C35913F81C3A90E1FB4D07A941F4F87A75605F5912462EB5396230`。

### 构建

统一使用：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- 首次 Editor 尝试：产品代码已编译，新增测试辅助代码使用了当前容器不支持的 `CountByPredicate`，并引用了未声明的 `ReadBytes`；原生退出码 1。改为显式循环和本地 `ReadFlowBytes` 后继续，没有把该失败描述为环境或产品源码故障。
- 最终 Editor：23/23 actions，成功，104.53 秒，原生退出码 0；
- Editor 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_editor_build_final.log`；
- Editor SHA-256：`11BCD9FB95F56A708A430160E8D5EB638945893259508BBF34BFB49B3C09D4A7`。
- 最终 Game：22/22 actions，成功，99.18 秒，原生退出码 0；
- Game 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.13.r0_game_build_final.log`；
- Game SHA-256：`8890C89B7E74C40690D169753701D9A24C5F73FBA02362D8B593511D9974E46C`。

### 静态边界

- `git diff --check`：原生退出码 0；
- `Source/ShanmenCore` 与 `Source/ShanmenCombatCore` 中 `UWorld`、`AActor`、`UGameplayStatics`、`ApplyDamage`、随机 API 和 `demo_map`：0；
- product preparation / lifecycle 中 `CommitPreparedLoadout`、`ClaimPreparedRunDurable`、legacy claim request：0；
- Run lifecycle adapter 中旧 Profile writer、Code B writer、`StartRunWithoutPreparation`：0。

## P/F 边界声明

本报告属于 P 阶段功能实现、静态审查、headless Unreal automation 与必要 Editor/Game 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续建议

P1.14 建议把仍用于世界确认审计的 Code B loadout selection 收敛为 Shanmen authority 原生、不可变的 selection correlation，同时继续保留旧 Code B 文档的只读兼容；这样产品 host 不再需要在 active Run 恢复前后重新打开旧仓库来获取审计摘要。
