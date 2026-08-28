# Dev.D.UE.0.0.10.P4.3.r0 Development Log

## 目标

把玩家生成／Run 开始接到 P2.1 World Entity Registry，向 P4.2 产品生命宿主注入真实稳定 EntityId；建立 BasicSword resolved Impact 到产品组件的唯一运行时交付门，并证明一个 world candidate 最多形成一个产品提交。

## 基线

- 分支：`agent/0.0.10-p4-3-player-combat-coordinator`；
- 基线提交：`f6b4cd6a80bf10e39c6aa395d050cd572edf9c02`（P4.2）；
- 基线全量：`Shanmen.0_0_10` 96/96；
- 现有稳定身份：`FShanmenWorldEntityIdFactory` 与 `FShanmenWorldEntityRegistry`；
- 产品生命宿主：`Udemo_mapPlayerHealthComponent`。

## 审计与决策

### 1. 绑定时机

准备阶段已有可回滚 Run，但在 RuntimeReady 前仍可能因世界激活、输入恢复或相关性校验失败。若在 `Prepare0909BRun` 提前绑定，持久 Pawn 会携带一个被回滚 Run 的身份。

最终选择 `ActivateV3MissionContentForRun` 的世界投影成功点：此时 Code A 已有有效 ActiveRunId，Pawn／health／碰撞根均存在；后续任何激活失败都会进入既有 `DeactivateV3MissionContentForPreparation` 清理链。正式 0.0.9B coordinator、普通 Profile start 和既有可见验收共用该入口，不另建第二条产品路径。

### 2. Run 结束必须允许持久 Pawn 换绑

当前 M01 在同一世界内可完成多次 Run，Pawn 不一定销毁。P4.2 只允许同 ID 幂等绑定，因此 P4.3 增加精确身份释放：只有当前 ID 匹配才能清除 ledger。生命数值仍留在唯一产品字段中，下一 Run 以新的确定性 ID 和 revision 0 重新建立提交账本。

### 3. 交付协调器不重新结算

BasicSword execution 已负责候选去重、ImpactId 和纯防御结算；产品 coordinator 只验证 receipt、Run、Target 并构造 canonical command。它不再调用 resolver，也不触发旧 damage API，避免同一命中发生“新结算 + 旧伤害”双写。

## 实现过程

1. 新建 `Fdemo_mapPlayerCombatCoordinator` 与结构化 delivery result/error。
2. 使用 `RunId + Spawn.Player.Primary + 0` 派生玩家实体 ID。
3. 在临时 Registry 中绑定 Pawn、health 与可选碰撞根，全部成功后再提交到产品 coordinator。
4. health 增加 exact-ID Run release；coordinator 的 `TryEndRun` 同时清理 Registry 与 vitality ledger。
5. GameMode 在真实 Run world activation 成功后绑定；Preparation／EndPlay 退出时解除。
6. GameMode 暴露 `DeliverResolvedPlayerImpact`，内部只转交 coordinator。
7. 新增生命周期测试与 BasicSword world-hit-to-product delivery 纵切。
8. 复审时补充“进入下一 Run 后拒绝旧 Run 延迟 receipt”断言，锁死跨 Run 消息边界。

## 验证时间线

1. `git diff --check` 初始通过。
2. Editor 首次完整构建：33/33 actions，`Result: Succeeded`，原生退出码 `0`。
3. 产品 coordinator 定向初跑：2/2 Success，0 Fail，queue empty，原生退出码 `0`。
4. 全量初跑：98/98 Success，0 Fail，queue empty，原生退出码 `0`。
5. Game 首次完整构建：32/32 actions，`Result: Succeeded`，原生退出码 `0`。
6. 提交前复审增加跨 Run 延迟 receipt 拒绝测试。
7. Editor 最终增量构建：4/4 actions，`Result: Succeeded`，原生退出码 `0`。
8. 产品 coordinator 最终定向：2/2 Success，0 Fail，queue empty，原生退出码 `0`。
9. 全量最终：98/98 Success，0 Fail，queue empty，原生退出码 `0`。
10. Game 最终增量构建：3/3 actions，`Result: Succeeded`，原生退出码 `0`。
11. 最终 `git diff --check` 与新路径禁用 API 静态扫描通过。

本轮没有编译失败、测试失败、Windows commit memory／页面文件错误或其它需要重试的环境错误。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：33/33，退出码 0；
- 最终：4/4，退出码 0。

### 产品 PlayerCombatCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.PlayerCombatCoordinator" -TestExit="Automation Test Queue Empty"
```

- found／performed：2；
- result：2 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.3.r0_player_combat_automation_final.log`；
- SHA-256：`275620244551C7F7F70C07ED44CC0E81629754B3391F8BB06DC54B2C9D18F88D`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：98；
- result：98 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.3.r0_full_automation_final.log`；
- SHA-256：`0AAAAFC52A23151BA4F460D796B50ECD3605647E75CDAF01D8A6294FD76DCFDB`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：32/32，退出码 0；
- 最终：3/3，退出码 0。

## 最终不变量

1. 玩家 EntityId 只由 authority Run 与固定 spawn tuple 派生。
2. Pawn、碰撞根、health 和 vitality ledger 始终共享同一 ID。
3. 活跃 coordinator 不能静默换 Run、Pawn 或 health。
4. Run 结束必须精确匹配当前 ID，之后同一 Pawn 才能绑定下一 Run。
5. World callback 先经 Registry；重复 candidate 不能形成第二 Impact。
6. 产品交付只接受当前 Run、当前玩家目标和规范 BasicSword receipt。
7. 首次 receipt 提交一次；replay 不修改、不广播；旧 Run 延迟 receipt 在新 Run 中失败关闭。
8. coordinator 不调用旧 damage API、不重跑防御、不制造随机身份。

## 诊断与边界

- 两份最终 Automation 日志各有 13 条测试发现前的 UE 5.8 既有启动诊断；目标测试全部成功，handled ensure 为 0。
- Win64 SDK 为 `VALID 10.0.22621.0`；非目标 LinuxArm64／VisionOS metadata 警告未描述为源码失败。
- `git diff --check`：原生退出码 0。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-3-player-combat-coordinator/Docs/Report/Dev.D.UE.0.0.10.P4.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-3-player-combat-coordinator/Docs/Log/Dev.D.UE.0.0.10.P4.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-3-player-combat-coordinator>
