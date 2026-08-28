# Dev.D.UE.0.0.10.P4.2.r0 开发报告

## 结论

`PASS`。P4.2 已将 P4.1 的 external-state Vitality Ledger 接入真实产品宿主 `Udemo_mapPlayerHealthComponent`，并把玩家当前／最大生命统一为该组件内部的唯一浮点真值。Combat Ledger 只保存状态指纹、单调 revision 与已提交 Impact receipt，不持有第二份生命数值。

规范 Impact 现在可以直接提交到玩家组件：首次成功只修改一次真实产品生命、只发出一次兼容伤害信号；完全重复投递返回 `AlreadyCommitted`，不会再次扣血、再次广播或推进 revision。该路径不调用旧 `ApplyIncomingDamage`、`UGameplayStatics::ApplyDamage` 或 Actor `TakeDamage`，不会重跑旧随机闪避／固定减伤，也没有双写。

## 实现内容

### 1. 玩家生命改为唯一浮点真值

- 原 `int32 CurrentHealth`／`MaxHealth` 改为私有 `float CurrentVitality`／`MaximumVitality`；
- 新增精确浮点读取接口，供 0.0.10 快照、提交与事务回滚使用；
- 旧 UI、物品结果和动态委托仍使用整数兼容视图；正的分数生命以向上投影显示，避免 `0.5` 生命被旧界面误报为死亡；
- 最大生命继续由旧属性系统计算并按既有规则取整，因此没有偷偷改变 0.0.9B 的属性平衡。

### 2. 稳定实体身份绑定

新增 `TryBindCombatEntity(TargetEntityId)`：

- 无效 GUID 失败关闭；
- 同一个稳定 ID 重复绑定幂等成功；
- 活跃组件不能静默换绑到另一实体；
- 不从 Actor 指针、对象名或随机 GUID 伪造身份；
- 绑定后才能捕获 `FShanmenTargetVitalitySnapshot` 或接受规范 Impact。

本轮没有制造临时玩家 ID。实际运行时 ID 必须由 World Entity Registry／玩家生成协调器注入；该接线留给 P4.3，避免把测试身份误当产品身份。

### 3. 所有玩家生命写入共享同一 revision

组件内部新增唯一变更门 `TryCommitVitalityState`。绑定 Combat Entity 后，下列旧入口全部通过 P4.1 ledger 的 `TryCommitExternalMutation` 原子修改产品字段、同步指纹并推进同一个 revision：

- 旧 `ApplyIncomingDamage`；
- `ApplyHealing` 与恢复生命 receipt；
- 属性驱动的最大生命变化与当前生命钳制；
- 物品使用／装备事务回滚；
- 非 Shipping 自动化设置。

旧写入会使此前捕获但尚未提交的 Impact 返回 `StaleSnapshot`。这消除了“新剑击用一套版本、旧治疗或装备改血绕过版本”的窗口。

### 4. 物品事务保留精确生命

`Udemo_mapItemSubsystem` 的三个回滚点改为捕获并恢复浮点 vitality，不再通过整数兼容 getter 丢失分数部分。满血判断也改为比较精确浮点真值；例如 `9.5/10` 不会因旧 UI 显示 `10/10` 而错误拒绝治疗。

### 5. 产品 Impact 提交

新增 `CommitCombatImpact(Command)`：

1. 由同一 ledger 验证命令、目标、状态指纹与 revision；
2. 首次提交直接修改 `CurrentVitality`；
3. 仅 `Committed` 且实际伤害大于零时广播旧 `OnPlayerDamaged`、触发视觉反馈／死亡；
4. `AlreadyCommitted`、`StaleSnapshot`、目标不符或状态失同步均不产生副作用。

## 自动化覆盖

新增 4 条 `Shanmen.0_0_10.Product.PlayerVitality` 测试：

| 测试 | 结果 |
|---|---|
| `BindingAndLegacyMutation` | PASS；稳定 ID 不可换绑，旧伤害／治疗／最大生命／回滚共享 revision，`8.5` 浮点真值投影为旧 UI 的 `9` |
| `ImpactExactlyOnce` | PASS；`1.5` 伤害精确保存，重复投递不二次写入、不二次广播 |
| `StaleAfterLegacyMutation` | PASS；旧伤害使旧 Impact 过期，重新基于新快照解析后可提交 |
| `BasicSwordVerticalSlice` | PASS；BasicSword `50` 原始伤害经 20% Guard 得 `40`，真实玩家宿主从 `100` 变为 `60` 且 replay 不再扣血 |

最终结果：

- 产品定向：`4/4 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`96/96 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.2.r0_player_vitality_automation_final.log`：`637A4613331CE16FF369CAF5F34E799C7DE5149BA0A11C2B97C1467854FA1B66`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.2.r0_full_automation_final.log`：`6B8E6B8B3EF4F903D8A6B68D95BED5EC271720615395D4B06743AA4C21D855F4`。

两份最终日志各保留 UE 5.8 在测试发现前打印的既有 13 条 unified-error/self-test `Condition failed` 启动诊断；目标测试全部成功、无 handled ensure。Win64 SDK 为 VALID；LinuxArm64／VisionOS 的 metadata 警告属于非目标平台诊断。

## 构建与问题修正

- 首次 Editor 全量编译 208 actions 后链接失败，原生退出码 `1`：产品测试直接实例化 `FShanmenWorldHitContext`，而 `demo_map` 未显式链接 `ShanmenWorldGameplay`；补充公共模块依赖后链接成功；
- 首次定向测试原生退出码 `1`：夹具错误地尝试写派生属性 `MaxHealth`，触发测试断言；修正为通过可写 `Primary03` 生成目标最大生命，并让夹具前置失败以普通测试失败返回；
- 最终 Editor Development：`Result: Succeeded`，原生退出码 `0`；
- 最终 Game Development：`Result: Succeeded`，原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`；
- `git diff --check`：原生退出码 `0`。

## 修改范围

- `Source/demo_map/demo_map.Build.cs`
- `Source/demo_map/demo_mapPlayerHealthComponent.h`
- `Source/demo_map/demo_mapPlayerHealthComponent.cpp`
- `Source/demo_map/demo_mapItemSubsystem.cpp`
- `Source/demo_map/demo_mapPlayerVitalityAdapterTests.cpp`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P4.3：把玩家生成／运行开始流程接到 World Entity Registry，向 `Udemo_mapPlayerHealthComponent` 注入真实稳定 EntityId；随后建立 BasicSword resolved Impact 到产品组件的运行时 delivery coordinator，并以产品测试证明一次候选只形成一次提交。不得用随机 GUID 或 Actor 地址代替注册身份。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-2-player-vitality-adapter/Docs/Report/Dev.D.UE.0.0.10.P4.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-2-player-vitality-adapter/Docs/Log/Dev.D.UE.0.0.10.P4.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-2-player-vitality-adapter>

`READY_FOR_0_0_10_P4_3`
