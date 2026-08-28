# Dev.D.UE.0.0.10.P4.1.r0 Development Log

## 目标

在改动产品 Actor 之前，提供一个不会复制生命真值的版本化 Impact ledger，使现有伤害、治疗、装备和回滚写入可以共享同一 revision，并让旁路写入可检测、可失败关闭。

## 基线

- 分支：`agent/0.0.10-p4-1-external-vitality-ledger`；
- 基线提交：`7740de9c757cefdbec7cbec26b9d2802f2ce2afa`（P4.0）；
- 基线全量：`Shanmen.0_0_10` 89/89；
- 本轮只修改 `ShanmenCombatRuntime` 的 vitality authority／tests 和 P4.1 Report／Log。

## 审计与决策

### 1. 不能直接把 P4.0 authority 并列放进旧组件

产品玩家生命不只有伤害写入：治疗、Code A／Code B receipt、装备最大生命变化、事务回滚和自动化设置都会改变状态。敌方近战／远程／重型类与训练目标还各自持有独立整数 Health。若只在其中一个新路径旁放 `FShanmenVitalityAuthority`，旧字段和新 authority 会同时可写。

### 2. 整数旧生命与浮点 Impact 尚未定义量化

旧 `TakeDamage`／玩家伤害以 `FloorToInt` 结算，P4.0 receipt 则精确保留浮点 Requested／Applied Damage。直接把 receipt 投影回 int 会让“实际扣血”与 receipt 不一致。因此本轮不把量化选择隐藏在 Adapter 中。

### 3. ledger 只保留状态指纹

将 P4.0 的 revision 和 processed Impact map 提取为 `FShanmenVitalityCommitLedger`。它保存 current／maximum 的位指纹用于同步校验，但数值本身仍由产品持有。指纹不能用于独立游戏运算，因此不会成为第二份生命权威。

## 实现过程

### 1. 外部状态快照与提交

ledger 初始化时绑定 TargetEntityId、初始状态指纹和 revision。捕获要求外部状态与指纹位级一致。Impact 提交直接接收 current vitality 引用，成功时修改该引用、更新指纹、推进 revision、保存 receipt；失败和 replay 不修改外部状态。

### 2. 外部变更协议

最初接口只让调用方报告 before／after，ledger 推进后仍需调用方自行赋值。源码复审认定这会产生“ledger 已更新但产品赋值遗漏”的窗口，随后改为 `TryCommitExternalMutation`：它接收唯一状态的引用并在同一调用内验证、赋值、更新指纹与推进 revision。

### 3. 旁路检测

若产品绕过两个提交入口直接改值，状态引用与指纹不一致：

- 新快照捕获失败；
- 新 Impact 返回 `StateDesynchronized`；
- processed Impact 和 revision 均不变化；
- 外部变更入口也不能伪造一个不匹配的 before-state。

如旁路写入后又恢复完全相同的位值，单靠值指纹无法观察 ABA；因此产品迁移仍必须封装字段并禁止直接写入。这一限制在 P4.2 通过类型边界解决。

### 4. 单实现复用

P4.0 `FShanmenVitalityAuthority` 保持原 API，但把 revision／指纹／processed receipts 全部委托给同一 ledger。现有 authority 与新外部状态模式共享一套提交实现，不会在后续修复中分叉。

## 实现与验证时间线

1. 审计玩家生命组件、四类敌方／训练目标生命和所有显式写入调用点。
2. 判定直接 Actor 接线会产生 int/float 量化缺口与双写风险，调整 P4.1 范围。
3. 提取 external-state ledger，新增同步指纹和 `StateDesynchronized`。
4. 将 P4.0 authority 改为组合 ledger，保持公开 API。
5. 新增外部提交、外部变更失效和旁路检测 3 条测试。
6. 首次 Editor 构建 6/6 actions 成功，原生退出码 0。
7. 首次定向 CombatRuntime 16/16、全量 92/92 成功。
8. 复审发现“报告变更后自行赋值”窗口，改为引用原子更新接口。
9. 最终 Editor 构建 6/6 actions 成功，原生退出码 0。
10. 最终定向 CombatRuntime 16/16 成功，0 fail，queue empty，原生退出码 0。
11. 最终全量 0.0.10 自动化 92/92 成功，0 fail，queue empty，原生退出码 0。
12. 最终 Game Development 构建 5/5 actions 成功，原生退出码 0。
13. 产品源静态边界扫描与 `git diff --check` 均通过。

整个过程没有产品源码编译失败、目标测试 fail 或 handled ensure。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 最终：6/6 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRuntime 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.CombatRuntime" -TestExit="Automation Test Queue Empty"
```

- found／performed：16；
- result：16 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.1.r0_combatruntime_automation.log`；
- SHA-256：`706E9444F66F56420638D52FF092203FE303DCCC10AD6177FC81A4B868F2A659`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：92；
- result：92 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.1.r0_full_automation.log`；
- SHA-256：`4299D3777874DAE8E39D4429BCD2AF9489B2A94EFE41C0F20E298F33E876B5EE`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 5/5 actions；
- `Result: Succeeded`；
- 原生退出码：0。

## 诊断说明

- 两份 Automation 日志在 test discovery 之前各有 13 条 UE 5.8 既有 unified-error/self-test `Condition failed` 启动诊断；实际目标测试其后全部成功。
- 两份日志 handled ensure 均为 0。
- ValidatePlatforms 报告 Win64 SDK `VALID 10.0.22621.0`；LinuxArm64／VisionOS 的 `MainVersion` metadata 缺失属于非目标平台诊断。
- 未把上述启动／非目标平台诊断描述为源码失败。

## 最终不变量

1. external-state ledger 不持有第二份可写生命值，只持有同步指纹。
2. 快照捕获和 Impact 提交必须看到与指纹精确一致的外部状态。
3. Commit 直接修改调用方的唯一 current vitality 引用。
4. 非 Impact 状态变更在一个调用内原子修改状态、指纹与 revision。
5. no-op 外部变更不推进 revision。
6. 声明的外部变更会使未提交的旧命令过期。
7. 已提交 Impact 在后续治疗／上限变化后仍返回原 receipt，不二次扣血。
8. 未声明写入返回 `StateDesynchronized`，不消费 Impact。
9. revision 上限阻断 Impact 与外部状态变更。
10. P4.0 authority 与 external-state 模式共享同一 ledger 实现。

## 静态边界与 P/F 边界

- `git diff --check`：0；
- 排除 Tests 后，`demo_map`、GetWorld／UWorld／AActor、ApplyDamage／TakeDamage、`ShanmenItems`、RNG、LineTrace／SweepMulti／OverlapMulti：全部 0；
- 未修改旧产品 Actor 或建立临时双写路径；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；
- 未修改或提交工作区中的无关未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-1-external-vitality-ledger/Docs/Report/Dev.D.UE.0.0.10.P4.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-1-external-vitality-ledger/Docs/Log/Dev.D.UE.0.0.10.P4.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-1-external-vitality-ledger>
