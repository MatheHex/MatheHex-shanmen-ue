# Dev.D.UE.0.0.10.P4.7.r0 Development Log

## 目标

把第一个真实“敌人 → 玩家”攻击族接入 0.0.10 canonical combat：选择 M01 普通近战接触，保持既有 AI／距离／冷却与表现，只迁移伤害的稳定身份、防御结算、receipt 和 player vitality 写入；在 M01 中禁止 canonical 失败后回落 legacy，非 M01 保持兼容。

## 基线

- 基线分支：`agent/0.0.10-p4-6-m01-vitality-hosts`；
- 基线提交：`3fb6da0b0429e84ee44a169002259112ac82b808`；
- 新分支：`agent/0.0.10-p4-7-enemy-melee-strike`；
- 基线全量：`Shanmen.0_0_10` 103/103；
- P4.6 已完成 14/14 authored M01 敌人稳定 Registry identity 与统一 vitality-host；
- P4.3 已建立玩家 canonical vitality ledger，但真实敌人攻击仍走旧伤害入口。

## 审计

### 1. 选择最小真实攻击族

`Ademo_mapEnemyCharacter::AttackPlayer` 是普通近战接触的集中入口：目标合法性、冷却和表现已由 Actor 决定，伤害最终只有一处 `UGameplayStatics::ApplyDamage`。它适合先证明 enemy source → player target 的完整闭环，同时不把 dash、projectile、heavy、boss 四类语义混入一次改动。

### 2. 玩家既有防御必须只结算一次

旧 player health 路径在 `ApplyIncomingDamage` 内执行 dodge 与 flat reduction。canonical path 若先调用旧入口再提交 resolver receipt，会重复防御或双写生命。因此本轮把这两个属性捕获为纯数据防御层，canonical commit 只消费 resolver 的 final damage。

### 3. 随机闪避必须可回放

`FRand` 不能重放，也无法让同一 ImpactId 复核相同 receipt。采用 ImpactId + defense rule 的确定性派生作为采样源；这保留概率阈值语义，同时让测试、日志与重放稳定。

### 4. M01 路由必须原子失败关闭

若 canonical coordinator 尚未 ready、source 未注册或 receipt 被拒绝，M01 Actor 仍回落 `ApplyDamage` 会形成双权威。GameMode 的路由判定只依赖当前地图是否 M01；一旦进入该分支，无论执行成功与否都立即返回。旧入口只供非 M01 使用。

## 实现过程

1. 从 P4.6 基线创建 P4.7 分支，审计普通近战、dash、远程、重甲与 Boss 的 enemy→player 写入。
2. 为普通近战定义 action、detector、formula 与 frozen receipt／structured execution result。
3. 在 coordinator 中加入每敌人 Run-local activation sequence、Action orchestrator、Shape candidate、玩家 snapshot、纯 resolver 与 delivery。
4. delivery 重新解析 source Actor 的当前 Run Registry identity，并验证 authored binding、vitality binding、request source 与 player target。
5. 在 player health component 中加入确定性 dodge／flat reduction defense snapshot。
6. GameMode 暴露 M01 产品路由与结构化执行日志。
7. `AttackPlayer` 在 M01 中调用产品入口并原子返回；保留非 M01 legacy `ApplyDamage`。
8. 新增 `M01EnemyBasicMeleeProduct` 自动化，覆盖 source 拒绝、减伤数学、replay、deterministic dodge、full evade、旧 Run 拒绝与新 Run sequence。
9. 完成 Editor 构建、定向与全量初跑、Game 构建、契约复审、最终定向与全量自动化。
10. 执行精确范围的旧伤害／RNG／地址身份扫描及 `git diff --check`。

## 验证时间线

1. Editor Development 完整构建：33/33 actions，`Result: Succeeded`，退出码 0。
2. coordinator 定向初跑：8/8 Success、0 Fail、queue empty，退出码 0。
3. `Shanmen.0_0_10` 全量初跑：104/104 Success、0 Fail、queue empty，退出码 0。
4. Game Development 完整构建：32/32 actions，`Result: Succeeded`，退出码 0。
5. 最终契约复审确认：`IsReady` 已用 Registry 同时解析 player Pawn 与 HealthComponent；delivery 再解析 source 并核对 authored／vitality binding；无需增加重复产品系统。
6. coordinator 定向最终：8/8 Success、0 Fail、queue empty，退出码 0。
7. `Shanmen.0_0_10` 全量最终：104/104 Success、0 Fail、queue empty，退出码 0。
8. `git diff --check` 与范围静态扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 33/33 actions 成功；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- found／performed：8；
- result：8 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.7.r0_combat_run_automation_final.log`；
- SHA-256：`2C4DA2881BD08FD2B280B47E97A5A6A9CF1D31ED55E160ABC5B5698D6CD82D7A`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：104；
- result：104 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.7.r0_full_automation_final.log`；
- SHA-256：`FFE4208F44C1DA1C6C88DD3E9D826F9AFA361E226BF667A2CE5F45898445A959`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 32/32 actions 成功；
- `Result: Succeeded`；
- 原生退出码：0。

### 静态检查

- `git diff --check`：退出码 0；
- coordinator 禁用旧伤害、RNG 与地址身份 API：0 匹配；
- M01 ordinary-melee 分支旧伤害 API：0 匹配；
- player defense capture 禁用 RNG／随机 GUID／地址身份 API：0 匹配；
- `AttackPlayer` 保留 1 个非 M01 legacy `ApplyDamage`，位于 M01 原子返回之后。

## 最终不变量

1. M01 普通近战只有 canonical vitality 写入；canonical 失败不会 fallback。
2. 非 M01 行为不因本轮迁移而改变。
3. Enemy source 与 player target 都必须属于当前 Run Registry。
4. Source 必须是 authored、vitality-bound、当前存活的 M01 host。
5. 每个 enemy action identity 只依赖 RunId、稳定 EntityId、定义与 monotonic sequence。
6. ImpactId 只依赖稳定 action、detector、target 与 ordinal。
7. 玩家 defense 是 action 结算前捕获的纯数据，不在 commit 时重新运行 legacy 防御。
8. 同一 ImpactId 的 dodge 采样稳定；不使用进程随机状态。
9. Resolver 的 raw damage 必须由 prevented + final 守恒。
10. 同一 receipt replay 不重复生命、revision、ledger 或表现副作用。
11. 完整闪避保留零伤害审计 receipt，但不发布正伤害表现。
12. Run release 清除 activation sequence，旧 Run receipt 不能写入新 Run。

## 诊断与边界

- 本轮没有源码编译失败、Automation 失败或环境内存失败。
- 最终两份日志各有 UE 5.8 测试发现阶段既有的 13 条 `Condition failed` 自检诊断；没有 fatal、unhandled exception 或 handled ensure。
- Win64 SDK 为 `VALID 10.0.22621.0`；LinuxArm64／VisionOS `MainVersion` metadata 警告属于非目标平台。
- 尚未迁移 melee dash、projectile、heavy 与 boss 攻击；它们不是 P4.7 的完成声明。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-7-enemy-melee-strike/Docs/Report/Dev.D.UE.0.0.10.P4.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-7-enemy-melee-strike/Docs/Log/Dev.D.UE.0.0.10.P4.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-7-enemy-melee-strike>
