# Dev.D.UE.0.0.10.P18.0.r0 Report

## 1. 结论

P18.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮建立了第一条剑气纯运行时契约：一次由精确剑实例发起的 canonical action，在 Active 阶段冻结发射原点、归一化方向、速度与最大距离；后续 Projectile 接触通过既有候选、统一 Impact ID、幂等 Ledger 与防御 Resolver 结算；结束或动作中断后进入不可再次发射的 Dissipated 状态。

这不是世界 Actor、视觉表现或输入接线。本轮刻意不决定剑气最终属于物理、灵力或其它伤害通道，伤害标签由内容定义提供；也没有建立第二套投射物、生命、库存或技能权威。

最终结果：

```text
Sword Qi exact:                         4 Success / 0 Fail
Shanmen.0_0_10.CombatRuntime:         126 Success / 0 Fail
Shanmen.0_0_10 full:                  755 Success / 0 Fail
Regression coverage:                   PASS (Changed=3 / Rules=1 / Required=1 / Logs=1)
Regression gate self-test:              PASS 273/273
Game + Editor Development:              PASS / native status 0
```

## 2. 产品定位与取舍

人工 0.0.10 战斗规划把剑定义为近战主武器，并明确允许向远程剑气延伸，同时要求手感优先、数值后置。P18.0 因而只冻结可复用的语义骨架：

- 剑气是 `Combat.Action.Sword.Qi.Basic01`，不是通用 Projectile、暗器或飞剑代理；
- 每次执行必须绑定有效的 `SourceItemInstanceId`，不能以无来源动作伪造剑气；
- 飞行速度、最大距离、伤害公式、基础伤害、攻击力系数、伤害标签和目标标签均来自 Definition；
- 伤害公式仍是可替换内容值，本轮 fixture 的 `10 + 40 * 0.5 = 30` 只用于证明契约；
- World delivery、碰撞、特效、音频、输入、GAS task 与产品生命写入保留给后续 Adapter。

## 3. 不可变定义与身份

新增 `FShanmenSwordQiDefinition`、`FShanmenSwordQiOffenseSnapshot`、`FShanmenSwordQiLaunchReceipt` 与 `FShanmenSwordQiImpactReceipt`。可编辑 Capture 只负责输入，成功捕获后由只读字段和 getter 暴露冻结值。

Launch ID 使用命名空间 `Shanmen.SwordQi.Launch.r1`，由 Run、Owner、Activation、Source Entity、精确 Sword Item、Action Definition、内容版本/摘要、原点、规范方向、速度与距离共同确定。相同输入稳定重放；改变激活身份、原点或方向会产生不同身份或被拒绝。

浮点身份编码使用规范化 bit 表示，并把正负零归一，避免视觉等价参数生成不同 Launch ID。

## 4. 状态机与发射生命周期

运行时状态固定为：

```text
Ready
  -> Active commit / one canonical launch
InFlight
  -> zero or more explicit Projectile contact samples
  -> close active sample
  -> dissipate before caller advances to Recovery
Dissipated
```

Startup 不能发射；Active 只提交一份 Launch receipt。同一份等价发射调用幂等返回既有 receipt，不会生成第二枚剑气；不同方向或原点不能重定向已在飞行的剑气。开放 contact sample 时拒绝普通 dissipate；动作被中断时，`EndForActionTermination` 会关闭残留 sample 并进入 Dissipated。

契约要求 caller 在剑气消散后再推进 Recovery；本轮测试按该顺序执行，但没有把产品 Action Orchestrator 与世界对象耦合。

## 5. 接触、伤害与防御

每次 contact sample 复用 `FShanmenDetectorEmissionSession` 生成 `Projectile` 候选上下文。目标必须满足 Definition 的 required tags，默认要求 `Target.Living`，并拒绝 Source Entity 自身。

合法候选使用既有 `FShanmenCombatIdFactory::MakeImpactId` 和 `FShanmenImpactLedger`：同一回调不能重复结算；同一 sample 的不同目标得到不同 Impact ID；后续 sample 的 ordinal 递增，同一目标也得到新的 Impact ID。

伤害包使用 `BaseDamage + AttackPower * AttackPowerCoefficient`，随后进入通用防御层 Resolver。Spirit fixture 中 raw 30 被 7 点 Spirit shield 吸收，结果为 23，且 `RawDamage = PreventedDamage + FinalDamage` 守恒。等价执行会重现相同 Launch ID、Impact ID 和最终伤害。

## 6. Automation 覆盖与证据

新增四条测试：

- `DefinitionAndSwordBoundary`：canonical action、开放伤害通道、正速度/距离、精确剑来源与失败关闭；
- `LaunchLifecycle`：Startup/Active/Recovery 时序、规范方向、幂等发射、冲突拒绝、sample 与消散；
- `ImpactPolicyAndOrdinals`：Living/self policy、候选幂等、多目标与跨 sample ordinal；
- `DefenseReplayAndTermination`：标签防御、守恒、确定性重放、不同 Activation 与中断清理。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi exact | 4 | 0 | `BB0C5E1D62B8A637214BAB1A8E184DBD69C2E0CB703BC510A6E62034EA437511` |
| `Shanmen.0_0_10.CombatRuntime` | 126 | 0 | `485393C83B3A419F5BFBC572A8843E3115970652B4A8C7AEAC1B7B1CCBF40506` |
| `Shanmen.0_0_10` full | 755 | 0 | `E3EF30196ED3A17284D39E8D95CB3D1D9B89075613C0A0F5407BD251B285F0F3` |

全部最终日志具有 native test exit 0，Fail 与 Fatal 计数为 0。完整套件首末 Success 时间为 `2026.09.02 19:30:11.760 -> 19:58:45.185 UTC`，约 28m33.425s。

## 7. 执行观察

实现后的首次 Editor 编译通过：UHT 生成 3 个文件，8 actions，总耗时 49.91s。剑气 exact Automation 首次执行即为 4/0；没有为测试结果修补生产源码，也没有保存伪成功日志。

完整套件包含既有长时重试路径，因此持续约 28 分钟，但最终 755 条全部成功。P17.2 的 751 条加上本轮四条，计数变化与新增测试严格一致。

## 8. 门禁、边界与构建

三个新增源码路径均位于 `Source/ShanmenCombatRuntime`，回归映射要求并实际运行 `Shanmen.0_0_10.CombatRuntime`：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 273/273
BOUNDARY_SCAN: PASS / 0 matches
GIT_DIFF_CHECK: PASS
```

边界扫描对 `GetWorld|SpawnActor|ApplyDamage|TakeDamage|Commit|Consume|Tick|Timer|FMath::Rand|FRandomStream` 为 0。regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`；coverage 结果 SHA 为 `C03AD5DB4C3BDDB2B724CF90BC5EA7292570E2908EC98A3ED817D0F6AE463860`。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 5 / 43.60s total | `E938B3CFE67489C7D775C683D5B6E6CDC7A9F5ECEB0E8D0AD79680E59559B40B` |
| Editor Development | Succeeded / up to date | 0 / 1.09s total | `993278A7CDDF9BCDCD01BFBCC769B9F7F2F65FEC6BCE27B9748B640AFC71091A` |

最终产物：

- `demo_map.exe`：356,204,544 bytes，SHA-256 `41B8F6054A8236116D9D1977DBF297120DA2D85D2B090F6E8B99C61F12D53B5A`；
- `UnrealEditor-demo_map.dll`：14,848,000 bytes，SHA-256 `0EBC93380817B435F92B8AA54466F12EEB6CFCADAAE478299A82B4499878E921`。

## 9. 修改范围与 P/F 边界

新增 3 个源码文件与本 Report/Development Log，共计划提交 5 个文件。未修改既有生产文件、Content、地图、资源、配置、Windows、UE Engine 或用户设置。

本轮只执行 unattended、NullRHI 的 P 阶段 Automation、静态门禁和 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，也没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P18.0`。

## 10. 下一阶段

P18.0 已提供可测试、可重放且不依赖世界对象的剑气内核。下一阶段若继续剑气，应做最小产品 Adapter：在正式剑动作的 Active commit 点创建 world delivery，把接触转为该契约的 Projectile candidate，并在销毁/超距/动作终止时 dissipate；不得复制伤害、生命、库存或幂等权威。

在得到实际表现与内容选择前，不冻结 Actor class、碰撞形状、速度曲线、穿透/爆炸、最终伤害通道、输入映射或数值平衡。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-0-sword-qi-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-0-sword-qi-contract/Docs/Report/Dev.D.UE.0.0.10.P18.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-0-sword-qi-contract/Docs/Log/Dev.D.UE.0.0.10.P18.0.r0_log.md>
