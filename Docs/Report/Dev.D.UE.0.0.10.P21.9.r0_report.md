# Dev.D.UE.0.0.10.P21.9.r0 Report

## 1. 结论

P21.9 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.8 的“发射 → 定向飞行 → 首次阻挡 → 一次伤害 → Completed”扩展为可持续产品闭环：同一把实体飞剑在终态后沿唯一 Combat Run 30 Hz 固定时间线返回初始环绕锚点；抵达后使用既有 Active Run Route 原子换代 activation，并恢复为可再次发射的 Orbiting 状态。整个过程复用同一 Actor、同一物品实例和同一库存权威，不生成/销毁 Actor，也不建立第二套飞剑状态机。

```text
Controlled-weapon focused:                 57 Success / 0 Fail
Shanmen.0_0_10 full:                     1242 Success / 0 Fail
Required 0.0.9B compatibility groups:     123 Success / 0 Fail
Regression coverage:                       PASS (Changed=9 / Rules=5 / Required=70 / Logs=6)
Regression gate self-test:                 PASS 435/435
Game + Editor Development:                 PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是带 physics scene 的真实 World Actor 返回/换代闭环、确定性身份、库存不变、自动化与原生构建；不宣称玩家镜头下的视觉品质、操作手感或音画反馈已经验收。

## 2. 返回阶段与唯一时间线

返回没有新增枚举状态或平行 lifecycle。`Completed` 且尚未位于初始锚点，直接构成唯一可返回条件：

- `Fdemo_mapShanmenControlledWeaponProductController` 保存既有 initial orbit location，并提供只读完成/锚点查询；
- `TryAdvanceCompletedReturn` 继续使用既有 `Motion.DirectedSpeed` 与每 tick 最大位移；
- `Fdemo_mapShanmenControlledWeaponRunHost::AdvanceCompletedReturnsFixedTicks` 消费 GameMode 已捕获的同一份 canonical timeline sample；
- 多把飞剑继续按稳定 GUID 顺序推进，每轮最多追赶 300 个固定 tick；
- 没有完整 tick、没有 Host 或没有待返回飞剑时，是结构化成功 no-op；
- timeline、Run、Host 或 receipt 不一致时在换代前失败关闭。

返回移动故意不执行 sweep：P21.8 的出航 sweep 是唯一 contact/Impact 路径，返航不会形成第二次接触、重复伤害或墙体终止。Actor 仍以逐 tick `SetActorLocation` 真实移动，而不是瞬移到锚点。

## 3. 抵达、换代与再次发射

`Fdemo_mapShanmenControlledWeaponWorldLifecycle::TryRedeployReturned` 只接收满足以下条件的返回结果：同一 Run、同一 item、同一已管理 Actor、旧 controller 已 Completed、Actor 精确抵达初始锚点。

换代采用 candidate Host 事务：

1. 复制当前 Host 到候选对象；
2. 从候选对象删除旧终态 activation；
3. 以同一 Actor、同一碰撞体和同一 source，通过既有 `ActiveRunRoute` 启动下一 activation sequence；
4. 验证新 controller 为 Orbiting、Actor/物品身份未变、activation identity 已更新；
5. 仅在全部验证通过后提交候选 Host 与下一序号。

因此失败不会留下“旧 controller 已删、新 controller 未建”的半提交状态。序号 0 与 `MAX_uint64` 被拒绝；每次成功换代产生新的确定性 ActivationId，不复用旧幂等身份。

GameMode 先处理进入本帧前就已终态的返回，再处理出航。刚在本帧撞击完成的飞剑从下一帧开始返回；刚在本帧完成换代的飞剑跳过同帧 orbit advance，避免一份 frame time 被消费两次。

## 4. 产品级自动化覆盖

新增/扩展两条关键证明：

- `FixedTimelineVisibleReturn`：验证零 tick 不移动、非法 timeline 与超预算在移动前拒绝、逐 tick 返回、精确抵达锚点，以及抵达后不再产生位移；
- Controlled Weapon canonical World lifecycle：在带 physics scene 的临时 GamePreview World 中完成 launch → directed sweep → blocking terminal → visible return → redeploy → second launch，验证同一物理 Actor 和 item、不同确定性 ActivationId、场上始终只有一个飞剑 Actor、库存快照不变，且第二次发射确实移动同一 Actor。

全量测试由 P21.8 的 1241 项精确增加到 1242 项。focused 父组共 57/0，覆盖 Controller、Host、World lifecycle、Route、Input 与现有受控武器契约。

## 5. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused | 57 | 0 | 326,272 | `D9E6C46F8EB7D13829DA1984C5543FD779AB2F83DA05C6DB753F0DACB3ECF77D` |
| `Shanmen.0_0_10` full | 1242 | 0 | 1,896,451 | `074EE581F2DC90D374FAFA5F5B4B811275B6066D9B307C85697D6520EAC9896B` |
| `demo_map.V3.Attributes` | 4 | 0 | 264,360 | `04781DE73D716C789BBC564A5F1221217BED831F9BAD8301DEAE60B933F53EA7` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 304,851 | `A6DFDF3F2C5B63C63FD9B17C2D406A36A93661F6CBF4CB95C82BE2C328A09242` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,491 | `F89C3818B73BE51B6B88267A06425A784DDDE10020CCB8FB2242452808074C3B` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 309,572 | `2DD33AF12382B49971366C9F9EF6C9476CD8E1525A29969EFE10363F435D60CB` |
| `demo_map.P4.Hotbar` | 7 | 0 | 267,282 | `3CE6F7190E98053DFB0FA7840B2CE60839A8EF43EEBD2B0FEA1099C2D549366D` |

full 从 `2026-09-08 07:25:40.908` 至 `08:39:33.549` 自然清空 1242 项并收到 native exit 0。五组旧版兼容合计 123/0。全部七份自动化日志的 `Result={Fail}`、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

focused 启动阶段含 UE 自带 Automation 自检产生的通用 `Condition failed` 文本，但发生在本轮 RunTests 命令前，不是选中用例失败；57 个选中用例全部形成 `Result={Success}`，且进程自然输出 `TEST COMPLETE. EXIT CODE: 0`。

## 6. 覆盖门、静态审计与构建

- regression self-test：`PASS 435/435`，42,923 bytes，SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- changed-file gate：`PASS Changed=9 Rules=5 Required=70 Logs=6`，8,473 bytes，SHA-256 `9582CB85A8EFDF7546E9E0B99194801D2D9914B533ED56AFFC3D27E490F8E10F`；
- 9 个实现/测试文件，`+920 / -2`；
- 新增生产行中的 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；
- 生产 `AdvanceCompletedReturnsFixedTicks` 与 `TryRedeployReturned` 调用点各严格为 1，均在 GameMode 的唯一 fixed-timeline 消费处；
- `git diff --check` 通过，仅有工作树 LF→CRLF 提示；验证结束后无 UnrealEditor、UnrealEditor-Cmd 或产品进程残留。

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial compile | Succeeded / native 0 | 62 actions / 43.87s | session evidence | session evidence |
| Game Development final | Succeeded / native 0 | 61 actions / 45.44s | 14,857 | `0FB615FF501DC4DC92F9217A01D662333104961C06FDF54DB2DCB36D0501AD4B` |
| Editor Development final | Succeeded / native 0 | 0 actions / 0.95s | 6,274 | `13288F918663521FAF561920872D1B3810599A62B5B0C52BA7FDFBD4BCA747E2` |

最终产物：

- `demo_map.exe`：359,481,344 bytes，SHA-256 `658E51A3003928A7A540B71F3D038446FA5FAFB2820E5AE60EB18E202B965C20`；
- `UnrealEditor-demo_map.dll`：18,659,328 bytes，SHA-256 `8A3CF333313F2AA9156CDFC66E072D87995FD0C06953B18CE85BF394C1BB0446`。

## 7. P/F 边界

PASS：既有终态触发同一固定时间线上的可见逐 tick 返回；返航不产生 contact/Impact；抵达精确锚点后原子换代；同一 Actor 与 item 保持、activation identity 更新、库存权威不变；第二次发射可继续移动同一 Actor；完整、兼容、覆盖门与双目标构建通过。

未声明：玩家镜头下的返回轨迹观感、环绕与返航的动画混合、音效/特效、手柄或键鼠实测、UI 状态提示、PIE/Standalone/打包体验。返回路径目前采用直线位置步进且不做避障；这是为确保返航不成为第二伤害通道而冻结的 P21.9 边界。

## 8. 后续建议与 GitHub

下一轮 P21.10 建议在不改变 gameplay authority 的前提下建立只读 flight-phase/presentation cue：让表现层可区分 Orbiting、Directed、Returning 与 Redeployed，并为未来一次受控的 PIE 视觉验收准备可观测证据。不得把表现 cue 反向变成第二状态机或第二时钟。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-9-flying-sword-return-redeploy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-9-flying-sword-return-redeploy/Docs/Report/Dev.D.UE.0.0.10.P21.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-9-flying-sword-return-redeploy/Docs/Log/Dev.D.UE.0.0.10.P21.9.r0_log.md>
