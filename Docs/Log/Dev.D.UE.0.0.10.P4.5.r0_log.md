# Dev.D.UE.0.0.10.P4.5.r0 Development Log

## 目标

把 P3.1 BasicSword 纯纵切、P4.4 共享 Run Registry／M01 近战 vitality 与现有真实 `PrimaryAttack` 输入、球形 sweep 串成一个产品执行路径；M01 禁止同次攻击继续调用 legacy `ApplyDamage`。

## 基线

- 分支基线：`agent/0.0.10-p4-4-m01-enemy-vitality`；
- 基线提交：`827b2906a0777b5e4ecce03f74b047c10ad7cfd5`；
- 新分支：`agent/0.0.10-p4-5-basic-sword-product-path`；
- 基线全量：`Shanmen.0_0_10` 100/100；
- 既有真实输入：统一 registry 的 `PrimaryAttack -> StartBasicAttack -> TryBasicAttack`；
- 既有几何：玩家前方一次 `SweepMultiByObjectType`；
- 既有旧写入：命中循环直接调用 `UGameplayStatics::ApplyDamage`；
- canonical 后半段：P4.4 已能把 resolved BasicSword receipt exactly-once 提交到 7 个 M01 近战 vitality host。

## 审计与决策

### 1. 切换点必须在一次 sweep 之后、legacy 写入之前

保留现有输入、冷却和几何可以避免另起一套产品操作。M01 在 hits 数组生成后立刻切到 canonical executor，并无条件返回；这样产品成功、miss、缺武器或内部拒绝都不可能落到 legacy damage writer。

### 2. 武器实例必须来自当前装备权威

Action 的 `SourceItemInstanceId` 读取 `Udemo_mapItemSubsystem::GetAuthority().GetEquippedInstance(WeaponSlot)`。空槽失败关闭，不用固定 GUID、定义 ID、随机值或玩家 ID冒充物品实例。

### 3. action identity 使用 Run 内单调序列

协调器持有从 1 开始的 `NextPlayerBasicSwordActivationSequence`。只有完整 action／definition／offense 已准备并成功进入 Active 后才消费序列；合法 miss 也属于已执行动作。Run 精确结束与 reset 都复位序列，RunId 保证跨 Run identity 隔离。

### 4. 一次 emission 可命中多个目标，重复 contact 不可重复提交

沿用 P2.1 的语义：同一 sweep 的目标共享 emission ordinal，ImpactId 还包含 TargetEntityId。Registry 与 emission session 分别处理瞬态对象解析和同目标重复接触；不依据 hits 数组回调顺序生成身份。

### 5. 本轮只迁移已存在 canonical vitality 的近战宿主

远程、重甲、Boss 虽已在 Registry 中，但尚没有 P4.1 ledger。executor 对它们不制造第二生命真值，也不 fallback 到旧写入。本限制显式留给 P4.6 处理。

## 实现过程

1. 审计 PlayerController 输入、旧攻击循环、PlayerCombat 属性边界、GameMode Run 激活、Code A 装备权威、BasicSword execution、WorldHitAdapter 与 P4.4 coordinator。
2. 建立 P4.5 分支。
3. 在 `Fdemo_mapCombatRunCoordinator` 增加产品 BasicSword action／definition 捕获、Run 内 activation sequence 和完整 sweep executor。
4. 新增执行错误枚举与结果摘要，覆盖前置、生命周期、候选、交付与 commit 计数。
5. GameMode 从当前 `WeaponSlot` 获取精确实例，并成为 controller 与 coordinator 之间唯一产品入口。
6. PlayerController 在 M01 sweep 后原子切换到新入口；旧 `ApplyDamage` 只留给非 M01 兼容内容。
7. 从 PlayerCombat 提取明确的 `CaptureAttackPower`，旧伤害 API 改为基于同一属性快照计算。
8. 新增产品 sweep 与 fail-closed 两条自动化；重复接触、miss、序号、跨 Run、缺武器、NaN offense、未注册 Actor 和确定性重放均纳入覆盖。
9. 首次 Editor 编译发现测试 NaN API 不兼容，改用标准库后重新编译、测试和双目标构建。
10. 复审生命周期准备顺序，使 definition／execution 在 ActionRuntime 进入 Active 前全部构造完成，再执行最终验证。

## 验证时间线

1. 初始 `git diff --check`：退出码 0。
2. Editor 首次完整构建：25 actions；新增测试在 `TNumericLimits<float>::QuietNaN()` 处编译失败，`OtherCompilationError`，退出码 1。
3. 改为 `std::numeric_limits<float>::quiet_NaN()`。
4. Editor 修正后增量：4/4，`Result: Succeeded`，退出码 0。
5. coordinator 定向初跑：6/6 Success，0 Fail，queue empty，退出码 0。
6. 0.0.10 全量初跑：102/102 Success，0 Fail，queue empty，退出码 0。
7. Game 首次完整：24/24，`Result: Succeeded`，退出码 0。
8. 生命周期准备顺序复审与整理。
9. Editor 最终增量：4/4，`Result: Succeeded`，退出码 0。
10. coordinator 定向最终：6/6 Success，0 Fail，queue empty，退出码 0。
11. 0.0.10 全量最终：102/102 Success，0 Fail，queue empty，退出码 0。
12. Game 最终增量：3/3，`Result: Succeeded`，退出码 0。
13. 最终 `git diff --check` 与禁用 API 扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：测试代码编译失败，退出码 1；
- 修正后：4/4 成功，退出码 0；
- 最终：4/4 成功，退出码 0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- found／performed：6；
- result：6 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.5.r0_combat_run_automation_final.log`；
- SHA-256：`656F2775855FBBEE5AA319C8E5A1D1370992D83FF451DFD278825D51B756BD9D`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：102；
- result：102 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.5.r0_full_automation_final.log`；
- SHA-256：`FACE8501C379A34E2376AB247805A57F5014A7B0FCAA7C8B3C01AF100733730A`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：24/24 成功，退出码 0；
- 最终：3/3 成功，退出码 0。

## 最终不变量

1. M01 PrimaryAttack 一次只选择 canonical 或 legacy 中的一条；当前固定选择 canonical。
2. M01 产品拒绝不会回退到 `ApplyDamage`。
3. Source item 只能是当前 WeaponSlot 的真实实例 GUID。
4. AttackPower 在 action activation 前冻结，后续装备变化不能回写该 action。
5. ActivationId 只由 Run、稳定 source、action definition 与单调序列派生。
6. World contact 只能经当前 Run Registry 解析 target。
7. 同一 emission 的重复 target 只形成一次 Impact／commit。
8. 生命只由 canonical final-damage receipt 提交；executor 不调用旧伤害入口。
9. 合法 miss 正常闭合生命周期；无效前置不消费 activation sequence。
10. Run 精确结束释放 identities、receipt ledger 并复位 action sequence。

## 诊断与边界

- 首次失败是新增测试使用了当前 UE 版本不存在的 API，不是产品源码失败、内存错误或环境故障；已修正并由后续构建／测试覆盖。
- 最终两份 Automation 日志各包含 13 条测试发现前的既有自检诊断；目标测试全部成功，无 fatal 或 handled ensure。
- Win64 SDK 为 `VALID 10.0.22621.0`；LinuxArm64／VisionOS `MainVersion` metadata 警告属于非目标平台。
- `git diff --check`：退出码 0。
- 身份／RNG 禁用 API 与 coordinator 旧伤害 API 扫描均为 0 匹配。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-5-basic-sword-product-path/Docs/Report/Dev.D.UE.0.0.10.P4.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-5-basic-sword-product-path/Docs/Log/Dev.D.UE.0.0.10.P4.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-5-basic-sword-product-path>
