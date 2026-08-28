# Dev.D.UE.0.0.10.P4.6.r0 Development Log

## 目标

把 P4.4／P4.5 只覆盖 M01 近战 Actor 的 canonical vitality 产品路径扩展为统一宿主契约，使全部 14 个 authored M01 敌人都能消费真实 BasicSword receipt；保留各宿主既有死亡、掉落、任务和表现链，并阻止 canonical 与 legacy 形成两份生命真值。

## 基线

- 分支基线：`agent/0.0.10-p4-5-basic-sword-product-path`；
- 基线提交：`bf7f1c6eeac0c1ada8c11b44a9a9af1c8f1505c1`；
- 新分支：`agent/0.0.10-p4-6-m01-vitality-hosts`；
- 基线全量：`Shanmen.0_0_10` 102/102；
- P4.5 产品限制：14 个 M01 Actor 均在 Registry，但只有 7 个近战宿主拥有 canonical vitality ledger；
- 待迁移：3 个远程、3 个重甲和 1 个 Boss authored 实例。

## 审计与决策

### 1. Coordinator 只依赖能力，不依赖敌人类

建立 `Idemo_mapCombatVitalityHost`。Coordinator 在注册边界强制所有 M01 Actor 提供该能力，后续 snapshot、commit、Run release 和计数全部通过接口完成。这样新增敌人宿主无需修改 BasicSword resolver 或再加具体类型分支。

### 2. Actor 仍是产品副作用所有者

Ledger 只决定生命值、revision 与 receipt 幂等性。首次正伤害提交后，各 Actor 调用自己的 `PublishAppliedDamage`，继续执行原受击表现、死亡、掉落和任务逻辑。Replay 不发布第二次表现。

### 3. 兼容写入必须进入同一 revision 序列

暂时保留的 `TakeDamage` 不直接改生命字段，而是调用 `TryCommitExternalMutation`。它不伪造成 canonical receipt，但与 canonical commit 共享 target identity、生命和 authority revision，避免双权威。

### 4. 生命精度与旧接口分离

宿主内部生命改为 float，canonical receipt 可提交小数伤害。旧整数 getter 仅作为兼容显示面向上取整，不参与 canonical 数学。

### 5. 自动化必须走真实多宿主产品入口

除逐宿主 contract 测试外，构造 14 个真实 M01 定义对应 Actor 和 `FHitResult`，由一次 `ExecutePlayerBasicSwordSweep` 完成 Registry 解析、resolve 与 commit，避免只证明接口单元而未证明产品编排。

## 实现过程

1. 审计四类 M01 Actor 的生命字段、`TakeDamage`、受击反馈、死亡／掉落／任务副作用和 P4.5 coordinator 的具体类依赖。
2. 从 P4.5 基线创建 P4.6 分支。
3. 新增统一 vitality-host UInterface，并让既有近战宿主实现该接口。
4. 为远程、重甲与 Boss 增加 float vitality、ledger binding、snapshot、canonical commit、external mutation 和自动化表现计数。
5. 将 coordinator 注册、绑定、Run release、目标交付和产品 sweep 改为能力驱动。
6. 将 GameMode 的 M01 交付桥从 melee-specific 改为 enemy-generic。
7. 扩展 authored identity 测试，要求 14/14 宿主全部绑定和释放。
8. 新增 `AllM01VitalityHosts`：逐类验证小数伤害、replay、legacy revision，再执行一次 14 目标产品 sweep。
9. 修正接口生成代码冲突与测试类型包含，完成首次成功编译。
10. 根据 transient crash 为三个新宿主的受击 Timer 增加 World 门。
11. 根据多宿主 sweep 复审结果隔离测试 action sequence，避免它与产品首个 activation 使用相同确定性身份。
12. 增加全部宿主精确 Run release 断言，执行最终双目标构建、定向与全量自动化、静态审查。

## 验证时间线

1. Editor 首次构建：26 actions；接口显式析构与 generated destructor 重复，且测试中的 `FDamageEvent` 类型不完整；`OtherCompilationError`，退出码 1。
2. 移除显式析构并包含 `Engine/DamageEvents.h`。
3. Editor 修正完整构建：26/26，`Result: Succeeded`，退出码 0。
4. coordinator 定向初跑：远程 transient Actor 在 `PublishAppliedDamage` 中无 World 访问 TimerManager；fatal access violation，退出码 1。
5. 为远程、重甲、Boss 的 damage-feedback timer 增加 `GetWorld()` 门。
6. Editor 修正构建：6/6 成功，退出码 0。
7. coordinator 定向初跑 2：7/7 Success，0 Fail，queue empty，退出码 0。
8. 0.0.10 全量初跑：103/103 Success，0 Fail，queue empty，退出码 0。
9. Game 首次完整构建：25/25 成功，退出码 0。
10. 扩展测试，使一次产品 sweep 命中全部 14 个宿主。
11. Editor 复审构建：4/4 成功，退出码 0。
12. coordinator 复审：6/7 Success、1 Fail；测试的手工 action 与产品 action 同用 sequence 1，后者被正确识别为 replay；UE 进程退出码 0。
13. 测试手工 action 改用 sequence 99；Editor 修正构建 4/4、coordinator 7/7，退出码均为 0。
14. 增加 all-host Run release 断言。
15. Editor 最终完整构建：23/23 成功，退出码 0。
16. coordinator 定向最终：7/7 Success，0 Fail，queue empty，退出码 0。
17. 0.0.10 全量最终：103/103 Success，0 Fail，queue empty，退出码 0。
18. Game 最终完整构建：22/22 成功，退出码 0。
19. 最终 `git diff --check` 与 coordinator 禁用 API 扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：新增接口／测试编译失败，退出码 1；
- 修正后：26/26 成功，退出码 0；
- 最终：23/23 成功，退出码 0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- found／performed：7；
- result：7 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.6.r0_combat_run_automation_final.log`；
- SHA-256：`0264EA9AA60557CB0C23684901A0C0142B24F95565622B89B93C07A1F5FE3955`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：103；
- result：103 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.6.r0_full_automation_final.log`；
- SHA-256：`9D438EEF7F9C9C9DA3C3F6C6A3385AE2AB61733BD877BD4666E06DF07A4F23F6`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：25/25 成功，退出码 0；
- 最终：22/22 成功，退出码 0。

## 最终不变量

1. 每个 authored M01 Actor 必须在注册时实现并绑定统一 vitality-host，否则注册失败关闭。
2. Coordinator 不按 melee／ranged／heavy／boss 分支生命提交。
3. Registry 的稳定 EntityId 必须与宿主当前绑定完全一致。
4. Canonical receipt 只能在 expected revision 匹配时首次提交。
5. 相同 ImpactId replay 不重复扣血、推进 revision 或发布表现。
6. Float vitality 是四类宿主的唯一生命真值；整数 getter 只是兼容视图。
7. Retained legacy `TakeDamage` 必须通过同一 ledger external mutation 门。
8. 产品 BasicSword sweep 不调用旧伤害 API，也不因具体敌人类回退。
9. 受击、死亡、掉落和任务副作用仍由具体 Actor 所有，且只在首次正伤害提交后发布。
10. Run 精确结束释放玩家与全部 M01 vitality identities。

## 诊断与边界

- 首次编译错误属于新增接口／测试代码；首次 crash 属于 transient 测试暴露出的 World 生命周期保护缺口；两者均已修复并由最终双构建和自动化覆盖。
- 复审中的 1 条失败证明幂等 identity 门按设计工作；修正的是测试 activation identity，不是放宽产品 ledger。
- 最终两份 Automation 日志各包含 13 条测试发现前的既有自检诊断；目标测试全部成功，无 fatal 或 handled ensure。
- Win64 SDK 为 `VALID 10.0.22621.0`；LinuxArm64／VisionOS `MainVersion` metadata 警告属于非目标平台。
- `git diff --check`：退出码 0；coordinator 旧伤害、随机和地址身份 API：0 匹配。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-6-m01-vitality-hosts/Docs/Report/Dev.D.UE.0.0.10.P4.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-6-m01-vitality-hosts/Docs/Log/Dev.D.UE.0.0.10.P4.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-6-m01-vitality-hosts>
