# Dev.D.UE.0.0.10.P4.12.r0 Development Log

## 目标

把玩家 GroundCircle 与 SelfSector 两个既有 overlap shape 技能接入 0.0.10 canonical combat。保留 `SkillComponent` 的世界几何、垂直容差、目标策略、冷却和反馈语义，由 coordinator 统一拥有 Run-local identity、稳定候选顺序、纯函数 impact resolve 与 M01 vitality ledger exactly-once delivery。M01 产品失败不得 fallback；非 M01 compatibility 继续存在。

## 基线

- 基线分支：`agent/0.0.10-p4-11-boss-attack-product`；
- 基线提交：`b876d1c5e825827c8c201b06bcdf389816520b19`；
- 当前分支：`agent/0.0.10-p4-12-player-shape-skills`；
- 基线 coordinator 定向：`12/12`；
- 基线 0.0.10 全量：`108/108`；
- P4.11 结束时 GroundCircle 与 SelfSector 仍由 `Udemo_mapSkillComponent` 直接调用 `ApplyDamage`。

## 剩余入口审计

1. ordinary melee、heavy、Boss、BasicSword 与共享 projectile 中剩余的 `ApplyDamage` 均已位于非 M01 compatibility fallback；
2. `demo_mapV3ProgressionManager` 的伤害调用属于 automation driver，不是新的产品权威入口；
3. 当前真正未迁移的玩家产品 family 是 `Udemo_mapSkillComponent` 的 GroundCircle、SelfSector 与 Straight Projectile；
4. GroundCircle／SelfSector 共享“瞬时 overlap shape”身份语义，可在同一阶段迁移但必须保留独立 action family 与 sequence；
5. Straight Projectile 拥有发射、飞行与命中生命周期，必须作为下一独立阶段处理，不能借用 shape identity。

## 设计决定

### 两个 family，共用一个 canonical 执行器

GroundCircle 与 SelfSector 各自冻结 Action／Detector／Formula／Digest 和 sequence；共用 world-hit adapter、pure defense resolver 与 vitality delivery。这样复用统一结算管线，同时不混淆两个技能的 action identity。

### 世界几何仍归 SkillComponent

SkillComponent 完成 overlap broad phase、圆形／sector 几何、垂直容差和 faction policy，只把授权 contacts 传给 coordinator。Coordinator 不访问 World，而是把 Actor／Component alias 解析为稳定 EntityId，忽略未注册、自目标与重复项，并按 EntityId 排序。

### M01 gate 内 fail-closed

GameMode 声明 M01 shape-skill 产品所有权后，SkillComponent 不先写 legacy damage。Canonical 返回结构化失败时施法返回 false、冷却不提交、legacy `ApplyDamage` 不执行；非 M01 路径完全保留。

### 回归按改动文件推导

- coordinator 与测试：coordinator 定向 + 0.0.10 全量；
- GameMode：0.0.10 全量；
- SkillComponent：EnemySkillFramework + V2RangedCompatibility + 0.0.10 全量；
- 未修改 Profile／CodeB／Items 路径，因此不追加无映射依据的存档／物品组。

## 实现过程

1. 新增 `Edemo_mapPlayerShapeSkillFamily`、impact receipt、execution error/result。
2. 冻结 GroundCircle 与 SelfSector 的 Action／Detector／Formula／Version／Digest 和 tags。
3. 为两个 family 增加独立 Run-local next sequence，并接入 Run end／reset 生命周期。
4. 新增 `ExecutePlayerShapeSkill`：校验输入，构造 deterministic action，启动 orchestrator，创建 shape hit context。
5. 使用 `FShanmenWorldHitAdapter::TryFromOverlap` 解析稳定目标 identity，去除未注册、自目标与重复目标，按 EntityId 排序。
6. 每个候选捕获 M01 vitality，构造 physical damage request，经 `FShanmenDefenseResolver` 解析，并通过 vitality ledger 提交。
7. 新增 player shape receipt 的独立 delivery／replay 入口，校验 active Run、player source、target binding 与 vitality identity。
8. GameMode 增加产品 gate、coordinator bridge 与结构化 family／activation／candidate／commit 日志。
9. SkillComponent 将 GroundCircle 与 SelfSector 分成互斥 canonical／compatibility 路径；canonical 失败返回 `INDEX_NONE`，施法不提交 cooldown。
10. 新增 `PlayerShapeSkillsProduct` 与 `PlayerShapeSkillsFailClosed` 两个无头自动化测试。
11. 执行 Editor 构建、定向测试；修正测试中错误的固定 vitality 基准后增量重建。
12. 执行最终 coordinator、全量及 SkillComponent 映射回归、静态检查和 Game 构建。

## 验证时间线

1. `git diff --check` 初检：退出码 `0`。
2. Editor Development：`24/24`，Succeeded，退出码 `0`。
3. CombatRunCoordinator 首轮：`13/14 Success`、`1 Fail`，queue empty，原生退出码 `1`。
4. 根因定位：测试以固定数字断言两个既有 enemy vitality，未使用 fixture 的 authored 非零初始值；产品提交逻辑与伤害数值正常。
5. 仅修正测试基准为“捕获的真实 baseline − committed damage”。
6. Editor Development 增量：`4/4`，Succeeded，退出码 `0`。
7. CombatRunCoordinator 最终：`14/14 Success`、`0 Fail`，queue empty，退出码 `0`。
8. 0.0.10 全量：`110/110 Success`、`0 Fail`，queue empty，退出码 `0`。
9. EnemySkillFramework：`44/44 Success`、`0 Fail`，queue empty，退出码 `0`。
10. V2RangedCompatibility：`22/22 Success`、`0 Fail`，queue empty，退出码 `0`。
11. 唯一写入点、随机身份、模块边界与 compatibility fallback 扫描通过。
12. Game Development：`23/23`，Succeeded，退出码 `0`。
13. `git diff --check` 终检：退出码 `0`。

首次失败日志被保留，没有把测试基线问题描述成源码或环境故障。测试修正后生产源码未再变化，因此不重复运行已经覆盖最终源码的其它映射组。

## 命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次：`24/24` actions，`Result: Succeeded`，原生退出码 `0`，`107.22s`；
- 测试修正后增量：`4/4` actions，`Result: Succeeded`，原生退出码 `0`，`5.54s`。

### CombatRunCoordinator 首轮

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- `13/14 Success`、`1 Fail`、queue empty、原生退出码 `1`；
- 失败测试：`PlayerShapeSkillsProduct`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_combat_run_automation_initial.log`；
- SHA-256：`25E4AE7F69D5F7D753E3C5DED9AF3B902E429C65D5FBDFA6A3BBBA650F074612`。

### CombatRunCoordinator 最终

- `14/14 Success`、`0 Fail`、queue empty、退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_combat_run_automation_final.log`；
- SHA-256：`6256F4585205150C3E6207E08C29DE84503949AC00A7F62F6D0E22C182B687E6`。

### 0.0.10 全量

- 命令组：`Automation RunTests Shanmen.0_0_10`；
- `110/110 Success`、`0 Fail`、queue empty、退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_full_automation_initial.log`；
- SHA-256：`3099805E820149F57E3E4291BC371303BFA5F747BECCE5A4C9AA4B8DBBB60D00`。

### EnemySkillFramework

- 命令组：`Automation RunTests demo_map.EnemySkillFramework`；
- `44/44 Success`、`0 Fail`、queue empty、退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_enemy_skill_automation_initial.log`；
- SHA-256：`8A762DB65CC274270419BBB9C9BDAF6E088CC620FD0460B6C6B9DEC481F90A9D`。

### V2RangedCompatibility

- 命令组：`Automation RunTests demo_map.V2RangedCompatibility`；
- `22/22 Success`、`0 Fail`、queue empty、退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.12.r0_v2_ranged_automation_initial.log`；
- SHA-256：`5992D6A4E5F6CE15C191DC9C446DB193A384537FB7F1170B1142F73A7A9A2A4B`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `23/23` actions；
- `Result: Succeeded`；
- 原生退出码：`0`；
- 总执行时间：`96.76s`。

### 静态检查

- `git diff --check`：退出码 `0`；
- coordinator 中 `ApplyDamage`／`NewGuid`／RNG／地址身份 API：`0` 匹配；
- `ShanmenCombatCore`、`ShanmenCombatRuntime`、`ShanmenItems` 中 `demo_map` include／`UWorld`／`AActor`／`ApplyDamage`／随机 API：`0` 匹配；
- `demo_mapSkillComponent.cpp`：`ApplyDamage` 共 `2` 处，均在 `!bUseM01ProductPath` 分支；
- 五份日志中 fatal／unhandled exception／handled ensure：均为 `0`。

## 最终不变量

1. M01 GroundCircle 与 SelfSector 的合法 overlap 只有 canonical vitality 写入；产品失败不 fallback。
2. 非 M01 compatibility 保留原 legacy path。
3. 世界几何、垂直容差、faction、cooldown 与 feedback 留在 SkillComponent；resolver 不查询 World。
4. 两个 family 使用不同冻结 Action、Detector、Formula、Digest 与独立 sequence。
5. identity 只依赖 RunId、稳定 source／target EntityId、冻结 action、sequence、detector 与 ordinal。
6. invalid family／damage／contact／sequence 在 mutation 前拒绝且不消费 sequence。
7. 未注册、自目标与重复 overlap 不生成 impact；候选按稳定 EntityId 排序。
8. 每个合法目标 ordinal 固定为 `0`，一次 action 可产生多个不同 target ImpactId。
9. exact receipt replay 不重复生命、revision、ledger 或 broadcast。
10. old-Run receipt 不能写入新 Run；新 Run 将两个 player shape sequence 各自重置为 `1`。
11. canonical failure 不提交本次技能 cooldown，且不会落回 legacy damage。
12. Straight Projectile 未借用 shape identity，保留为下一独立迁移阶段。
13. 回归组由实际改动路径推导，所有映射组均有本轮日志证据。

## 诊断与边界

- 首轮 coordinator 的两条断言失败来自同一测试基准错误；修正后 `14/14` 通过，生产代码未因该失败改动。
- 五份 Automation 日志各有 UE 测试发现阶段既有的 `13` 条负向自检 `Condition failed`；最终目标测试全部通过。
- Win64 SDK 有效；未发生 commit-memory、页面文件或工具链环境错误。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-12-player-shape-skills/Docs/Report/Dev.D.UE.0.0.10.P4.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-12-player-shape-skills/Docs/Log/Dev.D.UE.0.0.10.P4.12.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-12-player-shape-skills>
