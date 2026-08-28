# Dev.D.UE.0.0.10.P3.1.r0 Development Log

## 目标

实现第一式基础剑击的确定性无头纵切，把 P3.0 动作阶段、P2.1 Detector Emission 与 CombatCore Impact／Ledger／Defense 串联起来，同时把 Actor、世界查询、动画、ASC 属性读取和生命写入保留给下一层产品 Adapter。

## 基线

- 分支：`agent/0.0.10-p3-1-basic-sword-vertical-slice`；
- 基线提交：`ab05fa6b233c545ae910ec648377f82fffabb06a`（P3.0）；
- 基线全量：`Shanmen.0_0_10` 80/80；
- 本轮只修改 `ShanmenCombatRuntime` 与 P3.1 Report／Log。

## 设计过程

### 1. 先冻结输入，不让 Ability 临时取数

把内容平衡值分成 mutable capture 与 immutable definition；把攻击力分成显式捕获的 offense snapshot。零攻击力是合法快照，默认构造则无效。这样 P3.2 从 ASC 或装备系统取得数值后，本次 activation 不会受后续装备变化影响。

### 2. 目标策略位于几何之后、Impact 之前

P2.1 只报告规范 Candidate，不决定自目标或阵营／生命标签是否合法。P3.1 在接受 emission candidate 之前检查：

- Activation／Source／Detector／DetectorKind 身份一致；
- `TargetEntityId != SourceEntityId`；
- TargetTags 满足 `Target.Living`。

策略失败不消费 emission 去重集合或 Impact Ledger，因此同一物理 Candidate 在补齐正确目标快照后仍可合法处理。

### 3. 公式只构造 DamagePacket

公式 `BaseDamage + AttackPower × Coefficient` 使用冻结输入并在 double 中检查溢出，结果携带 `Damage.Physical.Slash`。公式层不做防御、不改生命；有序防御仍由 CombatCore Resolver 独占。

### 4. 双重幂等门

EmissionSession 负责同一 emission 内同一目标只接受一次；ImpactLedger 负责同一 activation 中规范 ImpactId 只提交一次。ImpactId 由 Run／Activation／Detector／Target／Ordinal 派生，完整 Request 在进入两个门之前先通过自身一致性校验。

### 5. 终止路径复审

首次实现与定向测试通过后，源码复审发现：Active emission 若遇 Ability interrupt，阶段门虽然能阻止后续结算，但 emission 状态会继续显示 active。补充 `EndEmissionForTermination`，具体剑击 Ability 在通用 EndAbility 记录 Cancel／Interrupt 前先关闭窗口，并增加 terminal cleanup 回归断言。

## 实现与验证时间线

1. 新增 Sword Definition／Offense Snapshot／Impact Receipt／Execution 与抽象 GAS Ability。
2. Runtime 增加 `ShanmenWorldGameplay` 公共依赖和 `Shanmen.Ability.Combat.Action.Sword.Basic01` 原生 tag。
3. 首次 Editor 编译含 UHT 与新源文件，13/13 actions 成功，原生退出码 0。
4. 首次定向 CombatRuntime 自动化 8/8 成功，0 fail。
5. 完成终止清理复审修正。
6. 最终 Editor 增量编译 7/7 actions 成功，原生退出码 0。
7. 最终定向 CombatRuntime 自动化 8/8 成功，0 fail，queue empty，原生退出码 0。
8. 最终全量 0.0.10 自动化 84/84 成功，0 fail，queue empty，原生退出码 0。
9. 最终 Game Development 构建 11/11 actions 成功，原生退出码 0。
10. 产品源静态边界扫描与 `git diff --check` 均通过。

整个过程没有产品源码编译失败、目标测试 fail 或 handled ensure。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 最终增量：7/7 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRuntime 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.CombatRuntime" -TestExit="Automation Test Queue Empty"
```

- found／performed：8；
- result：8 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P3.1.r0_combatruntime_automation.log`；
- SHA-256：`C6218DB5355734C1A490FE75CDD448D1460C2D11812173553287E878E8B168BC`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：84；
- result：84 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P3.1.r0_full_automation.log`；
- SHA-256：`24142DF9AAB82B17DF2C3B75E73448577D2E1BEBB39F64540100941AEB1F79E3`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 11/11 actions；
- `Result: Succeeded`；
- 原生退出码：0。

## 诊断说明

- 两份 Automation 日志在 test discovery 之前各有 13 条 UE 5.8 既有 unified-error/self-test `Condition failed` 启动诊断；实际目标测试其后全部成功。
- 两份日志 handled ensure 均为 0。
- ValidatePlatforms 报告 Win64 SDK `VALID 10.0.22621.0`；LinuxArm64／VisionOS 的 `MainVersion` metadata 缺失属于非目标平台诊断。
- 未把上述启动／非目标平台诊断描述为源码失败。

## 最终不变量

1. 一式基础剑击只能绑定同一份 frozen Action、Definition 与 Offense Snapshot。
2. Startup／Recovery／terminal 阶段不能产生或处理候选。
3. 策略拒绝不消费候选；合法候选不能因之前的错误快照永久丢失。
4. 同一 emission 的重复目标回调只结算一次。
5. 同一目标在后续 emission 使用新 ordinal 和新 ImpactId。
6. 规范 ImpactId 与 Request 字段不一致时失败关闭。
7. 每个成功 receipt 满足 `RawDamage = PreventedDamage + FinalDamage`。
8. 相同冻结输入与事件序列可重放相同 ImpactId 和防御结果。
9. Ability 终止不会遗留 active detector emission。
10. Runtime 不直接查询 World、不读取 Actor／ASC、不修改生命或库存。

## 静态边界与 P/F 边界

- `git diff --check`：0；
- 排除 Tests 后，`demo_map`、GetWorld／UWorld／AActor、ApplyDamage／TakeDamage、`ShanmenItems`、RNG、LineTrace／SweepMulti／OverlapMulti：全部 0；
- `ShanmenWorldGameplay` 依赖只承接 P2.1 的纯 emission／candidate 契约；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；
- 未修改或提交工作区中的无关未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-1-basic-sword-vertical-slice/Docs/Report/Dev.D.UE.0.0.10.P3.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p3-1-basic-sword-vertical-slice/Docs/Log/Dev.D.UE.0.0.10.P3.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p3-1-basic-sword-vertical-slice>
