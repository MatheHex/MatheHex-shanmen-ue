# Dev.D.UE.0.0.10.P2.0.r0 Development Log

## 目标

按 P0 已冻结路线进入 P2，建立 `ShanmenWorldGameplay` 世界接触适配层，将 Sweep、Overlap、Projectile 三类 UE 几何证据统一输出为 `FShanmenHitCandidate`，同时保持 CombatCore 无 World／Actor 依赖和旧 0.0.9B 产品行为不变。

## 开工复核

P1.14 Report 的“下一步建议”仍列出 Impact 守恒、damage tag、ImpactId 自校验和不可变 Action snapshot；逐项核验后确认这些内容已由更早的 P0.1 完成并以 9 项 CombatCore 自动化覆盖。因此本轮没有重复改动 CombatCore，而是回到 P0.0 明确的正式 P2 路线：WorldGameplay hit candidate adapters。

开工前不存在本地或远端 P2 分支。本轮从 P1.14 已推送提交 `9ce23ea41014e801ed1dfa067371614d06d17a76` 创建 `agent/0.0.10-p2-0-world-hit-adapters`。

## 设计决策

### 不从 UE 对象派生战斗身份

Actor／Component 指针、Object Name 和 callback order 都不具备跨重启或回放稳定性。适配层因此使用注入式 `IShanmenWorldEntityResolver`，把瞬态世界引用变成稳定 `FGuid`；无法解析即失败关闭。

### 不从回调顺序推断 HitOrdinal

UE 多命中数组和事件回调顺序不是战斗身份契约。`FShanmenWorldHitContext` 要求 detector runtime 在调用适配器前明确提供 HitOrdinal。相同 Run／Activation／Detector／Target／Ordinal 才能重放相同 ImpactId。

### 接触通道与 detector 类型集中匹配

- Sweep：WeaponTrajectory、Shape、ControlledObject；
- Overlap：Shape、ControlledObject、PersistentZone；
- Projectile：Projectile；
- TargetedRule：不经过世界接触适配器。

不兼容组合不会静默降级或修改 DetectorKind。

### 几何层不做产品策略

适配器只保留世界证据。自目标、阵营、无敌、目标数量、伤害公式、防御快照、生命提交和物品消耗均不属于本模块。

## 实现过程

1. 将新 Runtime 模块加入 uproject、Game Target 与 Editor Target。
2. 新增模块入口和日志类别。
3. 新增私有只读 `FShanmenWorldHitContext` 字段及 `TryCreate` 有效构造路径。
4. 新增 entity resolver 接口，显式传递 contact source、Actor、Component 与 BodyIndex。
5. 新增 Sweep／Overlap／Projectile 三个 adapter 和统一 private candidate builder。
6. 所有失败路径先清空 OutCandidate，防止 stale output 被误用。
7. 新增 6 项自动化，覆盖三类接触、通道矩阵、失败隔离、身份重放和几何／策略边界。

## 自动化过程

### WorldGameplay 定向

命令：

```powershell
UnrealEditor-Cmd.exe demo_map.uproject -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile "-ExecCmds=Automation RunTests Shanmen.0_0_10.WorldGameplay" "-TestExit=Automation Test Queue Empty" "-AbsLog=...P2.0.r0_worldgameplay_automation.log"
```

结果：找到 6 项，`6/6 Success`、`0 Fail`、queue empty，原生退出码 `0`。

### 0.0.10 全量回归

命令同上，将 filter 改为 `Shanmen.0_0_10`。

结果：找到 72 项，`72/72 Success`、`0 Fail`、queue empty，原生退出码 `0`。其中既有 CombatCore、ShanmenItems、Migration／ProductFlow 与新增 WorldGameplay 均通过。

### 日志完整性

- 定向日志 SHA-256：`381A79B38105C02F4F8D3F5C3FBED936E532F10928F38E8094BE6823F9A7E1FC`；
- 全量日志 SHA-256：`32D57045D0AE219939AFF48E2A9DE468A9FD33AC936EF172E0BE64BFFE95E4A9`。

两次 UnrealEditor-Cmd 启动均先执行 all-platform SDK validation：Win64 VALID，LinuxArm64／VisionOS 缺 `MainVersion` metadata。两份日志也各自保留了 UE 5.8 测试发现前既有的 13 条 `Condition failed` 启动诊断；它们未归属目标测试，目标测试随后逐项成功。

## 构建过程

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次 UHT／模块编译与链接：11/11 actions；
- `Result: Succeeded`；
- 原生退出码 `0`。

Game：

```powershell
Build.bat demo_map Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 6/6 actions；
- `Result: Succeeded`；
- 原生退出码 `0`。

## 最终不变量

1. `ShanmenCombatCore` 继续不依赖 Engine World／Actor。
2. WorldGameplay 可以读取 UE hit evidence，但不拥有 World query，也不应用伤害。
3. 三种接触源只输出同一个 CombatCore candidate contract。
4. 目标 EntityId 只能来自注入 resolver；指针、对象名与回调顺序不进入战斗身份。
5. HitOrdinal 由 detector runtime 明确指定；adapter 不维护第二份序列真值。
6. 不兼容通道、无效上下文、身份解析失败和非有限几何全部失败关闭。
7. 自目标仍是合法几何候选，后续 target policy 才能裁决。
8. 本轮没有修改旧 `demo_map` 战斗、玩家、敌人、技能、Profile、Run、物品或 UI 逻辑。

## 静态边界

`git diff --check` 退出码 `0`。新模块内以下 active 引用计数均为 `0`：

- `demo_map` include/reference；
- `GetWorld(`、`UWorld`；
- `ApplyDamage`、`TakeDamage`、`UGameplayStatics`；
- `FMath::Rand`、`FRandomStream`；
- `LineTrace`、`SweepMulti`、`OverlapMulti`。

## P/F 边界

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；未触碰 unrelated 未跟踪文件。
