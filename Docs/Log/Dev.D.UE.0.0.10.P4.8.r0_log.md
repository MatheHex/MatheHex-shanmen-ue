# Dev.D.UE.0.0.10.P4.8.r0 Development Log

## 目标

把 M01 standard／enhanced melee dash 的首次合法 contact 接入 0.0.10 canonical combat；复用 P4.7 的稳定敌人 Registry identity、玩家 defense snapshot 与 vitality ledger，并保证现有 knockback 只由首次成功的 applied vitality damage 驱动。M01 产品失败必须关闭，不得回落 legacy；非 M01 保持兼容。

## 基线

- 基线分支：`agent/0.0.10-p4-7-enemy-melee-strike`；
- 基线提交：`aed84c8244c366a8ada168937b2e051587eb1211`；
- 当前分支：`agent/0.0.10-p4-8-melee-dash-product`；
- 基线产品定向：8/8；
- 基线 0.0.10 全量：104/104；
- P4.7 已接入 M01 ordinary melee，但 `HandleDashSegment` 仍直接调用 player health legacy 入口并以整数 applied damage 决定击退。

## 审计与设计决定

### 1. 不建立第二套 dash 结算

P4.7 的 enemy basic receipt、action lifecycle、Registry 复核、defense resolve 和 vitality delivery 已具备通用结构。本轮将 basic-specific C++ 类型泛化为 `M01EnemyAttack` family，并以冻结 spec 区分 Basic、StandardDash、EnhancedDash；底层 resolver 与 ledger 不复制。

### 2. 技能 runtime serial 是正确的 action sequence

`Udemo_mapEnemySkillRuntimeComponent` 已在每次 accepted activation 时递增 `ActivationSerial`，并在 `ResetForNewRun` 时归零。它比 Actor 时间、帧号、hit order 或新随机 GUID 更适合作为 dash action identity。每个 dash 仅允许 first legal contact，因此 candidate ordinal 固定为 0。

### 3. exact replay 与 reconstruction 必须区分

同一完整 receipt 重放应返回 `AlreadyCommitted`。但重新调用产品入口会重新捕获目标 authority revision；如果 revision 已变化，即便同 serial 产生同 ImpactId，也不是同一冻结 receipt，必须由 ledger 拒绝。测试显式覆盖这两个不同边界。

### 4. knockback 是 commit 后副作用

旧路径使用 `ApplyIncomingDamage` 返回的整数。canonical damage 是浮点且可能被 defense 部分减免，因此 execution result 只在首次 `Committed` 时暴露 ledger receipt 的真实 applied damage。replay、reject、zero damage 与 defeat 都不得请求 knockback。

### 5. M01 产品路由原子失败关闭

Actor 的几何和目标授权发生在产品调用前。一旦地图是 M01，contact 不允许 fallback；即使产品失败，也消费 first legal contact 并进入 recovery，避免同一 dash 在后续帧改走第二写入权威。

## 实现过程

1. 从 P4.7 基线创建 P4.8 分支，审计 melee dash runtime、Actor contact、player health 与 knockback 路径。
2. 新增 `Edemo_mapM01EnemyAttackFamily`，把 P4.7 basic receipt／execution result 泛化为 enemy attack receipt／result。
3. 建立 Basic、StandardDash、EnhancedDash 冻结 spec，统一 action 构建、candidate、resolver 与 delivery。
4. 在 M01 enemy binding 中冻结 authored `SkillProfileId`；重复注册也要求 Actor、marker、profile 与 vitality host 一致。
5. 新增 `ExecuteM01EnemyMeleeDashContact`，验证 profile、serial、source binding、target binding 与 source vitality。
6. GameMode 暴露统一 M01 enemy attack gate 与 dash 产品入口，输出结构化 action／impact／commit 日志。
7. `HandleDashSegment` 在 M01 中调用 canonical 产品；非 M01 保留 legacy；first-legal-hit 与 recovery 语义保持。
8. `ShouldRequestEnemySkillKnockback` 改为有限正浮点判断；Actor 使用 first committed applied damage 与 commit receipt defeat state。
9. 新增 `M01EnemyMeleeDashProduct` 自动化，覆盖 profile、identity、fractional defense、replay、reconstruction reject、Run isolation、enhanced family 与 lethal no-knockback。
10. 执行 Editor 构建、定向／全量自动化、旧 EnemySkillFramework 回归、静态扫描与 Game 构建。
11. 诊断并最小修正旧 P6 Schema 4 测试字面量漂移；不改产品 Profile。

## 验证时间线

1. Editor Development 首次完整构建：34/34，Succeeded，退出码 0。
2. coordinator 定向初跑：8 Success、1 Fail、queue empty，进程退出码 0。失败断言误把重新解析的新 revision snapshot 当作 exact receipt replay。
3. 调整测试边界：exact receipt delivery 断言 `AlreadyCommitted`；same-serial reconstruction 断言 `CommitRejected`。增量 Editor 4/4 成功。
4. coordinator 修正版：9/9 Success、0 Fail。
5. 增加 `DidNewCommitDefeatTarget`，Actor 的 defeat／knockback 决策只读取首次 commit receipt。增量 Editor 23/23 成功。
6. 0.0.10 全量初跑：105/105 Success、0 Fail。
7. EnemySkillFramework 初跑：43 Success、1 Fail；唯一失败为旧测试硬编码 Schema 4，而当前产品基线为 Schema 7。
8. 最小测试维护后增量 Editor 4/4 成功；EnemySkillFramework 最终 44/44。
9. coordinator 最终 9/9；0.0.10 全量最终 105/105。
10. Game Development 最终 33/33，Succeeded，退出码 0。
11. `git diff --check` 与禁用 API／legacy 分支范围扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次完整：34/34 actions；
- 最终增量：4/4 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- performed：9；
- result：9 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_combat_run_automation_final.log`；
- SHA-256：`6D02A5986F310882B543B17CC02C7604EB697AFAE7F47F45A5115563D5CE7625`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- performed：105；
- result：105 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_full_automation_final.log`；
- SHA-256：`A4F40A20D592A30BBAF28FB8C3E1884429565105C4BD9E970A17006440B32AD8`。

### EnemySkillFramework 兼容回归

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests demo_map.EnemySkillFramework" -TestExit="Automation Test Queue Empty"
```

- performed：44；
- result：44 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_enemy_skill_automation_final.log`；
- SHA-256：`91EEF58EEE229ECD402F1ABE208ECCC41078FE94295DACD3FE9892CAB95CB0DE`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 33/33 actions 成功；
- `Result: Succeeded`；
- 原生退出码：0。

### 静态检查

- `git diff --check`：退出码 0；
- coordinator 禁止 legacy damage、随机 GUID／RNG 与地址身份 API：0 匹配；
- `HandleDashSegment` 的 M01 canonical block：0 个 `ApplyIncomingDamage`；
- 整个函数保留 1 个 `ApplyIncomingDamage`，仅用于非 M01 compatibility；
- 最终三份自动化日志：fatal／unhandled exception／handled ensure 共 0。

## 初始日志证据

- 定向初跑：8 Success／1 Fail，SHA-256 `493D3BCE2839B803A9B7B7821AE4C59E210C7819CE54C7CC3DAEA727EC3A29DB`；
- 定向修正版：9/9，SHA-256 `67F4F4C32E7DC05E6EA852E8D87106F4DF423F6456E4CA2A94477262CFBECBDD`；
- 全量初跑：105/105，SHA-256 `6463A5FB4ABE0FA204590615E77CCA4749C7987B9F01F11FD06DCCD9509EB1EF`；
- EnemySkillFramework 初跑：43/44，SHA-256 `C41B9211E118166C725F3D132DF767D63C5BE9C9AF649A7996ED760B2055E8C9`。

## 最终不变量

1. M01 melee dash 只有 canonical vitality 写入；canonical 失败不 fallback。
2. 非 M01 dash 仍使用既有 legacy 路径。
3. Standard 与 Enhanced profile 只能调用各自 authored dash family。
4. Dash identity 只依赖 RunId、稳定 source EntityId、冻结 action definition 与 runtime ActivationSerial。
5. Invalid profile／serial／binding 在创建有效 action identity和 mutation 前拒绝。
6. ImpactId 只依赖稳定 action、detector、target 与 ordinal。
7. Resolver receipt 保持 raw = prevented + final。
8. Exact receipt replay 不重复生命、revision、ledger、表现或 knockback。
9. Same identity／different snapshot 被拒绝，不能伪装为 replay。
10. Knockback 只跟随首次 Committed 的正 applied damage，defeat 时不触发。
11. First legal contact 无论 canonical 成功或失败都只消费一次并进入 recovery。
12. 旧 Run receipt 不能写入新 Run。

## 诊断与边界

- 首次两个 Automation failure 均已保留并解释：一个是测试语义错误，一个是遗留 Schema 测试漂移；没有伪装为环境故障或删除证据。
- 三份最终日志各含 13 条 UE 既有负向自检 `Condition failed`，目标测试全部通过。
- Win64 SDK 有效；非目标 LinuxArm64／VisionOS SDK metadata 警告不影响 Win64 构建。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-8-melee-dash-product/Docs/Report/Dev.D.UE.0.0.10.P4.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-8-melee-dash-product/Docs/Log/Dev.D.UE.0.0.10.P4.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-8-melee-dash-product>
