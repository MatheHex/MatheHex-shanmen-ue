# Dev.D.UE.0.0.10.P4.2.r0 Development Log

## 目标

选择真实产品生命宿主，将其改为单一浮点 vitality 真值，并把规范 Impact、旧伤害、治疗、最大生命和物品回滚接入 P4.1 的同一 ledger；用 BasicSword 纵切证明没有旧伤害 API 双写。

## 基线

- 分支：`agent/0.0.10-p4-2-player-vitality-adapter`；
- 基线提交：`756acb0cddab9f5fc8dbbea4ea87b7f3d7a2d8e7`（P4.1）；
- 基线全量：`Shanmen.0_0_10` 92/92；
- 真实产品宿主：`Udemo_mapPlayerHealthComponent`。

## 审计与决策

### 1. 选择玩家生命组件

该组件已经承载旧伤害、治疗、物品 receipt、属性最大生命、装备／物品回滚、死亡和视觉反馈，是当前最适合先迁移的单一产品宿主。把新 authority 并列放在 Actor 外会保留双写，因此直接把组件内部状态改为唯一浮点真值。

### 2. 保留旧接口但不保留第二份状态

旧 UI、物品结果和动态委托大量使用 `int32`。本轮不扩散接口重写，而是让整数只成为读取／信号投影：内部、Combat 快照、receipt 和回滚均使用 float。正分数向上投影，保证活着的目标不会显示为零血。

### 3. EntityId 必须外部注入

项目已有确定性 World EntityId 工厂，但尚无把玩家 Pawn 注册到该系统的产品协调器。本轮只提供严格绑定入口，不生成随机 ID、不使用对象地址。这样测试可以验证适配器，而产品接线不会继承伪身份。

## 实现过程

1. `demo_map` 显式依赖 CombatCore、CombatRuntime 与 WorldGameplay。
2. 玩家组件将 `CurrentHealth`／`MaxHealth` 替换为私有浮点 vitality；旧 getters 改为兼容投影。
3. 新增稳定实体绑定、Combat 快照捕获、规范 Impact 提交和 revision／receipt 只读查询。
4. 新增 `TryCommitVitalityState`，把旧伤害、治疗、最大生命变化、回滚和自动化设置收束到同一外部变更入口。
5. 将伤害广播／视觉／死亡副作用提取为 `PublishAppliedDamage`；只有首次有效提交调用它。
6. 物品事务的三个 rollback 捕获点改为 float；满血策略读取 float 真值。
7. 新增绑定与旧变更、exactly-once、旧变更使快照过期、BasicSword 产品纵切共 4 条测试。

## 问题与修正时间线

1. 首次 Editor 构建完成 205/208 actions 后在 `UnrealEditor-demo_map.dll` 链接失败：`FShanmenWorldHitContext` 构造／析构符号未解析；原生退出码 `1`。
2. 根因是产品测试直接使用 WorldGameplay 导出的上下文类型，但 `demo_map.Build.cs` 只声明了 CombatRuntime。补充 `ShanmenWorldGameplay` 公共依赖。
3. Editor 增量重链 2/2 成功，原生退出码 `0`。
4. 首次定向自动化发现 4 条测试，但 BasicSword 夹具对派生 `MaxHealth` 调用 `SetBaseValue`，触发断言退出；原生退出码 `1`。
5. 修正为写可变主属性 `Primary03 = MaximumVitality - 5`，并用 `bReady` 把夹具初始化失败转为普通测试失败。
6. Editor 增量编译／链接 4/4 成功，原生退出码 `0`。
7. 产品定向 4/4 Success，0 Fail，queue empty，原生退出码 `0`。
8. 全量 0.0.10 96/96 Success，0 Fail，queue empty，原生退出码 `0`。
9. 首次 Game 全量 202/202 actions 成功，原生退出码 `0`。
10. 复审发现整数 UI 投影会把 `9.5/10` 显示为 `10/10`，若物品策略也读投影会误判满血；改为比较 float 真值。
11. 最终 Editor 增量 4/4、全量自动化 96/96、Game 增量 3/3 全部成功，原生退出码均为 `0`。
12. `git diff --check` 与静态写入口扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次：链接失败，原生退出码 `1`；
- 最终：`Result: Succeeded`，原生退出码 `0`。

### 产品 Vitality 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.PlayerVitality" -TestExit="Automation Test Queue Empty"
```

- found／performed：4；
- result：4 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.2.r0_player_vitality_automation_final.log`；
- SHA-256：`637A4613331CE16FF369CAF5F34E799C7DE5149BA0A11C2B97C1467854FA1B66`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：96；
- result：96 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.2.r0_full_automation_final.log`；
- SHA-256：`6B8E6B8B3EF4F903D8A6B68D95BED5EC271720615395D4B06743AA4C21D855F4`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 最终：3/3 incremental actions；
- `Result: Succeeded`；
- 原生退出码：0。

## 最终不变量

1. 玩家组件的 current／maximum float 是唯一生命数值真值。
2. ledger 只持有状态指纹、revision 和 Impact receipts。
3. 绑定后的全部产品写入都通过 `Commit` 或 `TryCommitExternalMutation`。
4. 稳定实体身份不得由 Actor 指针、名称或随机 GUID 代替。
5. 首次规范 Impact 直接修改产品真值；replay 不修改、不广播。
6. 新 Impact 不调用旧伤害入口，不重跑随机闪避／固定减伤。
7. 旧写入推进相同 revision，使旧快照失败关闭。
8. 物品回滚保存精确分数生命，满血判断不使用整数 UI 投影。
9. 死亡判断始终读取浮点真值；正分数生命在旧 UI 中不会显示为零。

## 诊断与边界

- 最终两份 Automation 日志各有 13 条测试发现前的 UE 5.8 既有启动诊断，目标测试全部成功；handled ensure 为 0。
- Win64 SDK 为 `VALID 10.0.22621.0`；非目标 LinuxArm64／VisionOS metadata 警告未描述为源码失败。
- `git diff --check`：原生退出码 0。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-2-player-vitality-adapter/Docs/Report/Dev.D.UE.0.0.10.P4.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-2-player-vitality-adapter/Docs/Log/Dev.D.UE.0.0.10.P4.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-2-player-vitality-adapter>
