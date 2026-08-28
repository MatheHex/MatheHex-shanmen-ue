# Dev.D.UE.0.0.10.P4.0.r0 Development Log

## 目标

在接入任何产品角色生命前，冻结最终生命应用的版本、身份、幂等和冲突语义，并把 P3.1 基础剑击的规范 Impact 无头贯通到 exact-once vitality receipt。

## 基线

- 分支：`agent/0.0.10-p4-0-vitality-authority`；
- 基线提交：`75149d0a8c558fde3ddd623c7e164b4b69a2bd63`（P3.1）；
- 基线全量：`Shanmen.0_0_10` 84/84；
- 本轮只修改 CombatCore 的 vitality snapshot、CombatRuntime 的 authority 契约／测试和 P4.0 Report／Log。

## 设计过程

### 1. 先审计现有权威，避免直接接出第三条生命路径

产品代码中尚无可直接复用的 Combat ASC 生命权威；旧角色／属性组件仍持有现行数值。若 P4.0 直接让 Ability 同时调用旧扣血和新 receipt，会形成双写甚至第三条真值路径。因此本轮先实现不依赖 Actor／World 的提交协议和参考 authority，不连接产品状态。

### 2. revision 必须与生命一起冻结

仅比较生命数值无法区分“值恰好相同但中间发生过写入”的 ABA 情况。为 `FShanmenTargetVitalitySnapshot` 增加单调 `AuthorityRevision`，commit 必须同时匹配 revision、当前生命和最大生命。拒绝不会消费 Impact，调用方可重新捕获、重新解析后提交。

### 3. ImpactId 与 ResolutionId 分工

- `ImpactId` 表示同一个物理／逻辑命中身份；
- `ResolutionId` 表示该 Impact 在指定内容、生命版本、公式、伤害标签和有序防御层下的完整解析身份。

完全重复的两者返回原 receipt；相同 ImpactId 但不同 ResolutionId 是显式冲突。这样网络重送不会二次扣血，错误的分歧解析也不会静默覆盖首次结果。

### 4. 命令工厂重算规范 Resolver

不能仅依赖 `Raw = Prevented + Final`，因为攻击方可构造守恒但不符合防御规则的结果。命令工厂重新运行纯函数 Resolver，逐字段比较结果后才冻结命令。首次实现使用近似浮点比较；定向测试通过后的源码复审认为确定性快照没有容差需求，随后收紧为位级一致，关闭亚容差伪造窗口。

### 5. receipt 区分请求伤害和实际伤害

若目标只有 20 生命而请求 30，receipt 保留 `RequestedDamage=30`，同时记录 `AppliedDamage=20` 和 `VitalityAfter=0`。这既保持上游结算证据，又让实际生命变化可审计。

### 6. Revision 上限失败关闭

authority 到达 `MAX_int64` 时拒绝新提交并返回 `RevisionExhausted`，不会发生有符号整数溢出。该边界与目标错配、未初始化、无效命令、过期快照和 Impact 冲突均有结构化错误。

## 实现与验证时间线

1. 为 CombatCore vitality snapshot 增加 `AuthorityRevision` 及有效性约束。
2. 新增不可变 CommitCommand、CommitReceipt、CommitResult 和单目标 VitalityAuthority。
3. 加入规范 Resolver 重算、确定性 ResolutionId 和 Impact 处理账本。
4. 新增 5 条 authority 自动化及 P3.1 BasicSword 集成纵切。
5. 首次 Editor 构建含 UHT 与新增源文件，28/28 actions 成功，原生退出码 0。
6. 首次定向 CombatRuntime 自动化 13/13 成功，0 fail。
7. 源码复审后把命令和快照比较收紧为位级一致，并补目标错配和 overkill 断言。
8. 最终 Editor 增量构建 5/5 actions 成功，原生退出码 0。
9. 最终定向 CombatRuntime 自动化 13/13 成功，0 fail，queue empty，原生退出码 0。
10. 最终全量 0.0.10 自动化 89/89 成功，0 fail，queue empty，原生退出码 0。
11. 最终 Game Development 构建 22/22 actions 成功，原生退出码 0。
12. 产品源静态边界扫描与 `git diff --check` 均通过。

整个过程没有产品源码编译失败、目标测试 fail 或 handled ensure。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 最终增量：5/5 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRuntime 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.CombatRuntime" -TestExit="Automation Test Queue Empty"
```

- found／performed：13；
- result：13 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.0.r0_combatruntime_automation.log`；
- SHA-256：`AF3E48D3C395D6952F5ECC98CEB52BE39CC82F8FFDC6B3295ADDDB390119727E`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：89；
- result：89 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.0.r0_full_automation.log`；
- SHA-256：`DB8C9842073448D49F865931AD16E44FACC952D9EF4B6F67A780F31B728A34E7`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 22/22 actions；
- `Result: Succeeded`；
- 原生退出码：0。

## 诊断说明

- 两份 Automation 日志在 test discovery 之前各有 13 条 UE 5.8 既有 unified-error/self-test `Condition failed` 启动诊断；实际目标测试其后全部成功。
- 两份日志 handled ensure 均为 0。
- ValidatePlatforms 报告 Win64 SDK `VALID 10.0.22621.0`；LinuxArm64／VisionOS 的 `MainVersion` metadata 缺失属于非目标平台诊断。
- 未把上述启动／非目标平台诊断描述为源码失败。

## 最终不变量

1. CommitCommand 只能从有效 Request 和规范 Resolver 输出构造。
2. 生命快照的 current／maximum／revision 必须与 authority 精确一致。
3. 一个 authority 只接受自身稳定 TargetEntityId 的命令。
4. 完全重复投递返回首次 receipt，不再次扣血或推进 revision。
5. 同一 ImpactId 的分歧 ResolutionId 必须显式冲突。
6. 拒绝不会改变生命、推进 revision 或占用 ImpactId。
7. 成功提交只推进一次 revision，并满足 `VitalityAfter = VitalityBefore - AppliedDamage`。
8. `AppliedDamage = min(VitalityBefore, RequestedDamage)`，过量伤害不会产生负生命。
9. Revision 上限拒绝新提交，不发生整数溢出。
10. P3.1 基础剑击的最终伤害只经过一个提交入口。

## 静态边界与 P/F 边界

- `git diff --check`：0；
- 排除 Tests 后，`demo_map`、GetWorld／UWorld／AActor、ApplyDamage／TakeDamage、`ShanmenItems`、RNG、LineTrace／SweepMulti／OverlapMulti：全部 0；
- 未接入旧产品生命组件，因此本轮没有产品侧双写；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；
- 未修改或提交工作区中的无关未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-0-vitality-authority/Docs/Report/Dev.D.UE.0.0.10.P4.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-0-vitality-authority/Docs/Log/Dev.D.UE.0.0.10.P4.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-0-vitality-authority>
