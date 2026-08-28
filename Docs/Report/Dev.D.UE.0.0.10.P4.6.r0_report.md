# Dev.D.UE.0.0.10.P4.6.r0 开发报告

## 结论

`PASS`。P4.6 已把 M01 全部 14 个 authored 敌人统一接入同一 canonical vitality-host 契约。P4.5 真实产品 BasicSword sweep 不再只接受近战宿主：7 个近战、3 个远程、3 个重甲和 1 个 Boss 均由共享 Run Registry 解析稳定 EntityId，经同一 receipt／revision／idempotency 门提交伤害。

各 Actor 继续拥有自己的生命状态、受击表现、死亡、掉落和任务副作用；coordinator 只依赖 `Idemo_mapCombatVitalityHost`，不再按具体敌人类分支。旧 `TakeDamage` 入口暂为非 M01 兼容内容保留，但其写入也通过同一 ledger 的 external mutation 门推进 authority revision，不会形成第二份生命真值。

## 功能性

### 1. 统一产品契约

新增 `demo_mapCombatVitalityHost.h`，定义所有 authored M01 战斗宿主必须实现的最小能力：

- 绑定／精确释放 Run 内 Target EntityId；
- 查询绑定状态、稳定 EntityId 与 authority revision；
- 捕获只读 vitality snapshot；
- exactly-once 提交 canonical impact；
- 查询已提交 Impact 数；
- 开发自动化下查询正伤害表现回调次数。

接口不暴露具体 Actor 类型，也不把死亡、掉落、任务或表现职责搬进 combat core。

### 2. 14/14 M01 宿主迁移

- `Ademo_mapEnemyCharacter`：保留 P4.4 ledger，改为实现统一接口；
- `Ademo_mapRangedEnemyCharacter`：生命改为 float vitality，新增 ledger、canonical commit 与 external mutation；
- `Ademo_mapHeavyEnemyCharacter`：生命改为 float vitality，新增同一契约；
- `Ademo_mapM01BossCharacter`：生命改为 float vitality，新增同一契约，既有 Boss 死亡与任务推进链仍由首次有效伤害提交驱动。

三个新迁移宿主的 legacy 整数 getter 使用向上取整维持旧 UI／行为接口；canonical 路径保留小数伤害，不会因旧整数显示层而截断。只有 `Committed` 且 applied damage 大于零的 receipt 才触发一次受击表现和死亡判断；`AlreadyCommitted` replay 不重复触发副作用。

### 3. Coordinator 去具体类型化

- M01 注册要求每个 authored Actor 都实现并成功绑定统一 vitality-host；否则失败关闭；
- binding 只保存 Actor、SpawnMarkerId 与碰撞根，不保存近战类专用指针；
- Run end／reset／绑定计数均通过统一接口完成；
- `DeliverBasicSwordImpactToM01Enemy` 接受任意 `AActor`，但只向当前 Registry 中身份一致且 vitality 已绑定的宿主提交；
- 产品 sweep 对所有 14 个宿主执行相同 Registry → snapshot → resolve → commit 链；coordinator 中没有 `ApplyDamage`、`TakeDamage` 或 `ApplyIncomingDamage` 回退。

### 4. 一致性与兼容性

- canonical 小数伤害直接落到 float vitality；
- receipt replay 返回 `AlreadyCommitted`，生命、revision、receipt 数和表现计数都不重复变化；
- retained legacy `TakeDamage` 仍保持旧整数伤害语义，但通过同一 ledger 推进 revision；
- Actor 配置在未绑定时初始化 vitality；绑定后任何外部生命变更必须经过 ledger；
- transient 自动化 Actor 没有 World 时不再访问 TimerManager，真实 World 下受击闪烁逻辑保持不变；
- Run 精确结束后，玩家及全部 14 个敌人宿主的绑定均释放。

## 自动化证据

`Shanmen.0_0_10.Product.CombatRunCoordinator` 从 6 条增至 7 条。新增 `AllM01VitalityHosts` 覆盖：

- 14 个真实 authored 定义创建对应产品 Actor 并完成统一绑定；
- 每个宿主提交 `0.5` canonical damage，验证小数生命；
- 同一 receipt 重放，验证 exactly-once；
- retained `TakeDamage(1.0)` 与 canonical ledger revision 同步；
- 一次真实产品 sweep 同时命中 14 个宿主，验证 14 resolved／delivered／committed；
- 每个宿主的 vitality、revision、receipt 数和正伤害表现计数一致；
- 精确 Run release 清空全部绑定。

最终结果：

- 产品定向：`7/7 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`103/103 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.6.r0_combat_run_automation_final.log`：`0264EA9AA60557CB0C23684901A0C0142B24F95565622B89B93C07A1F5FE3955`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.6.r0_full_automation_final.log`：`9D438EEF7F9C9C9DA3C3F6C6A3385AE2AB61733BD877BD4666E06DF07A4F23F6`。

两份最终日志各保留 UE 5.8 在测试发现前输出的既有 13 条 `Condition failed` 自检诊断；目标测试全部成功，无 fatal 或 handled ensure。

## 构建、修正与静态检查

- Editor 首次构建：接口显式析构与生成代码重复、测试缺少完整 `FDamageEvent` 定义，`OtherCompilationError`，原生退出码 `1`；
- 移除重复析构并包含 `Engine/DamageEvents.h` 后，Editor 完整 `26/26` 成功，退出码 `0`；
- 定向自动化初跑：远程 transient Actor 的受击反馈无 World 仍访问 TimerManager，触发 access violation，原生退出码 `1`；
- 为远程、重甲和 Boss 的受击 Timer 增加 World 门后，Editor `6/6`、定向 `7/7`、全量 `103/103`、Game `25/25` 均成功；
- 产品多宿主 sweep 复审初跑：`6/7 Success`，测试手工 action 与产品 action 同用 activation sequence 1，确定性 ImpactId 正确判为 replay；UE 命令进程原生退出码仍为 `0`；
- 将测试专用 action sequence 隔离为 99 后，定向 `7/7` 成功；
- 增加 14 宿主 Run release 断言后，最终 Editor `23/23`、定向 `7/7`、全量 `103/103`、Game `22/22` 均成功，原生退出码均为 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 `ApplyDamage`、`TakeDamage`、`ApplyIncomingDamage`、随机／地址身份 API 的扫描：0 匹配；
- 四类敌人的 `CurrentVitality` 赋值仅存在于各自受控的 `TryCommitVitalityState` 边界。

本轮没有 Windows commit memory／页面文件错误。首次编译、首次 transient crash 与测试身份碰撞均如实保留，且分别由后续构建与自动化覆盖。

## 修改范围

- `Source/demo_map/demo_mapCombatVitalityHost.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapEnemyCharacter.h`
- `Source/demo_map/demo_mapRangedEnemyCharacter.h/.cpp`
- `Source/demo_map/demo_mapHeavyEnemyCharacter.h/.cpp`
- `Source/demo_map/demo_mapM01BossCharacter.h/.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## P/F 边界

本 Report 只包含 P 阶段源码开发、静态审查、NullRHI headless Automation、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。真实交互与视觉验收留在 F 阶段。

## 下一步

P4.7 建议开始迁移“敌人 → 玩家”的真实攻击写入：先选一个 M01 攻击族，把其命中从 legacy `ApplyDamage` 接到稳定 source／target identity、玩家 defense snapshot、canonical impact receipt 与 P4.3 player vitality commit；验证无双写后再扩展到投射物、重击与 Boss 技能。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-6-m01-vitality-hosts/Docs/Report/Dev.D.UE.0.0.10.P4.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-6-m01-vitality-hosts/Docs/Log/Dev.D.UE.0.0.10.P4.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-6-m01-vitality-hosts>
