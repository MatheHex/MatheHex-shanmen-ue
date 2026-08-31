# Dev.D.UE.0.0.10.P11.14.r0 Development Log

## 1. 目标

承接 P11.13 的 hostile-damage 审计，关闭 active WeaponGuard 在输入松键和 Run teardown 之外没有明确终止语义的问题。目标是让受击硬直、武器授权变化、玩家失败、Pawn／Controller 生命周期与 Run teardown 全部进入唯一 Session terminal command route，并保留精确原因与幂等证明。

## 2. 基线审计

基线只有两条 Session 终止入口：

- `TryRelease()`：输入松键，`Active -> Recovery -> Completed`；
- `TryInterruptAndReset()`：Run teardown，`Active -> Interrupted`。

发现的真实缺口：

1. 正伤害广播只更新 M01 extraction risk，不中断 active guard；
2. 当前武器被替换／卸下后，Session 的 captured item authorization 会变 stale，但没有产品 caller 处理；
3. `OnUnPossess` 与 Controller `EndPlay` 只拆输入／移动 hook；
4. 玩家 defeat 没有 guard lifecycle 命令；
5. Run teardown 在 SpiritEvasion cleanup 失败时会提前返回，可能跳过 guard cleanup；
6. 终止原因只存在于自由文本 diagnostic，无法形成 typed receipt。

## 3. 实现

Session 以单一 `TryTerminate(Reason)` 取代两个旧 terminal API。新增七个合法原因：

- `InputReleased`；
- `EffectiveDamageStagger`；
- `WeaponAuthorizationChanged`；
- `PlayerDefeated`；
- `PawnUnpossessed`；
- `ControllerEndPlay`；
- `RunTeardown`。

只有 `InputReleased` 走 Recovery + Completed；其余原因全部走 Interrupted。`None` 与未知枚举拒绝且不改变 active Host。首个成功终止 receipt 携带精确 HostId 和 Reason；清空后重复请求只返回带请求 Reason、无 HostId 的 `NoActiveHost`，因此同一 Host 不会产生第二个 terminal receipt。

GameMode 新增唯一产品入口 `RouteWeaponGuardTerminationIntent`：

- 输入 release wrapper 委托该入口；
- positive applied damage 在伤害完成广播后触发 stagger；致命伤害通过已提交 vitality 判断为 `PlayerDefeated`；
- GameMode Tick 和每次 hostile Impact capture 前都核对 captured weapon authorization；stale 时先中断，避免同帧旧武器 guard 参与下一次 defense；
- `HandlePlayerDefeated` 提供无正伤害 defeat 的兜底；
- PlayerController `OnUnPossess` / `EndPlay` 只向 GameMode 发 typed lifecycle intent，不直接操作 Session；
- Run teardown 会独立尝试 SpiritEvasion 与 WeaponGuard cleanup，再分别判定结果，避免一个产品的错误跳过另一个产品的终止。

## 4. 测试

新增／强化 ProductSession 测试：

- `None` 与未知枚举原子拒绝；
- 七个合法原因的 route 分类；
- InputReleased 的 Recovery -> Completed 顺序；
- 六个 interruption 原因的 Active -> Interrupted；
- 首次 terminal receipt 携带原 HostId；
- 重复终止为无 HostId no-op；
- stale weapon identity 使用 `WeaponAuthorizationChanged` 清理；
- InputAdapter 的手工 no-op receipt 也必须携带 typed reason。

最终 Automation：

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 7 | 0 | 1 | `7414C5D29D9DD5DB2E86D2C438419A42C121BE815C76A7C7DFE136E3C1EA3D33` |
| `Shanmen.0_0_10` | 535 | 0 | 1 | `057DD84E02FEBBF49FD093BBA34E7AC3B3EC6FFA7CADCABE8FE214533C0A3B0A` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `C4515C6D153CBAD0D97592356CDAB3A8A0B846A3638964A1C43C4A45D6FFC16B` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `271351BD027987B2BC2ACD7F8786858F5F75F5CD969A7B21D153F00B9867958A` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `4E7B3040C65AB94D7EDE7801769EDEEC45C6E1042DDFF188144309732FFF1301` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `49BE30CB0B38CE8301EFEBF2409EEB4AC68467D5677F2AD70DCD2992E61739A4` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `715249A702B597C2D4BD453BC70D505AF85772B5FB5D695805FA0660CBC8E3AE` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `05FFB3150CEB1058F067DCCE5A3A628185F3569160D574FD9393398F2A33A18F` |

原始 Success 合计 688、Fail 0；focused 7 项包含于 full 535，按 test identity 去重为 681 项。

## 5. 门禁

```text
REGRESSION_MAP_JSON: PASS Rules=102
SELF_TEST: PASS 164/164
REGRESSION_COVERAGE: PASS Changed=8 Rules=5 Required=44 Logs=8
ADDED_AUTHORITY_SCAN: PASS AddedLines=369 ForbiddenHits=0
OLD_TERMINATION_APIS: PASS 0
DIRECT_WEAPON_GUARD_TERMINAL_WRITER: PASS 1 (GameMode route)
git diff --check: PASS (native exit 0)
```

## 6. 构建

- Editor implementation：76/76，238.58s，原生退出 0；
- Editor final：6/6，13.09s，原生退出 0；
- Game final：75/75，202.02s，原生退出 0；
- 无源码失败、Automation 失败或内存环境重试。

## 7. 边界

本阶段未让 PlayerController、health、item UI 或 GameMode 成为第二套 guard Host／damage authority。它们只产生 typed intent；Session 仍是唯一 active Host owner，CombatRunCoordinator 仍是 Impact/vitality authority，ItemSubsystem 仍是装备真值。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
