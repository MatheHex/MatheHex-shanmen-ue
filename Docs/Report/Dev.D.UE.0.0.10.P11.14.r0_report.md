# Dev.D.UE.0.0.10.P11.14.r0 Report

## 1. 结论

P11.14 已完成并通过 P 阶段门禁。

本阶段把 WeaponGuard 的输入松键、有效受伤硬直、武器授权变化、玩家失败、Pawn／Controller 生命周期与 Run teardown 统一到一个 typed Session terminal command route。旧 `TryRelease` / `TryInterruptAndReset` API 已移除，产品代码只有 GameMode route 能直接调用 Session `TryTerminate`。

最终结果：

- 七个终止原因均形成 typed receipt；
- 输入松键保持 `Active -> Recovery -> Completed`；
- 六类非输入原因统一 `Active -> Interrupted`；
- 同一 Host 只有第一次终止返回 HostId，重复调用为无 HostId 幂等 no-op；
- 正伤害在当前 Impact 完成后中断 guard，perfect guard 的零伤害不误触发硬直；
- 致命伤害优先记录 `PlayerDefeated`；
- 武器替换／卸下会在 Tick 或下一次 hostile Impact capture 前中断 stale guard；
- Pawn unpossess、Controller EndPlay 与 Run teardown 不再遗留 active guard；
- 0.0.10 全量 535/535，映射旧回归 146/146，focused 7/7；
- changed-file gate、静态审查、Editor/Game Development 构建全部通过。

## 2. 功能性

### 2.1 唯一终止契约

`Fdemo_mapShanmenWeaponGuardProductSession::TryTerminate` 是唯一 Session terminal API。合法原因：

| Reason | Route | 语义 |
|---|---|---|
| `InputReleased` | Completed | Active -> Recovery -> Completed |
| `EffectiveDamageStagger` | Interrupted | 当前伤害提交后进入受击硬直 |
| `WeaponAuthorizationChanged` | Interrupted | captured weapon 不再是装备真值 |
| `PlayerDefeated` | Interrupted | defeat／致命伤害 |
| `PawnUnpossessed` | Interrupted | 控制 Pawn 生命周期终止 |
| `ControllerEndPlay` | Interrupted | Controller 生命周期终止 |
| `RunTeardown` | Interrupted | Combat Run teardown |

`None` 与未知枚举 fail closed，不修改 active Session。Result 校验同时约束 Reason、Status 与 Host transition proof，不能把 interruption 伪装为 orderly completion。

### 2.2 恰好一次

终止继续使用 copy -> transition -> validate -> clear：

```text
active Host + valid typed reason
  -> copy active route
  -> complete or interrupt copied Host
  -> validate reason/status/HostId proof
  -> clear sole Session ownership
```

首个成功 terminal receipt 携带原 HostId。Session 清空后，同一或另一终止原因只能得到 `NoActiveHost`，且 HostId 无效。因此重复松键、damage + defeat 双广播、OnUnPossess + EndPlay 或 teardown 重入不会产生第二个 terminal receipt。

### 2.3 产品事件接入

- **受击硬直**：`OnPlayerDamaged` 只在 positive applied damage 后广播，因此本次 hostile Impact 已完成 guard composition 与 vitality commit；回调再中断 guard，不会提前取消当前命中的防御。零伤害 perfect guard 不广播，不会误中断。
- **致命伤害**：damage 回调读取已提交 vitality；`<= 0` 时首个 reason 为 `PlayerDefeated`，随后 defeat callback 是幂等 no-op。
- **武器切换**：GameMode 同时在 Tick 和 hostile Impact capture 前核对 Session 的 captured item authorization。后者关闭“同帧换武器后、下一 Tick 前仍用旧武器防御”的窗口。若中断本身拒绝，向 Coordinator 传递 enabled-invalid context，使生命结算前 fail closed。
- **Pawn／Controller 生命周期**：PlayerController 只向 GameMode 发 typed intent，不持有或直接修改 Session。
- **Run teardown**：SpiritEvasion 与 WeaponGuard cleanup 都会被尝试，再分别判定；一个产品的 cleanup 错误不再跳过另一个产品。

## 3. 完整性

测试覆盖：

1. `None` 和未知枚举拒绝且 active Host/HostId 不变；
2. InputReleased 的 Recovery + Completed 顺序与 typed reason；
3. 六种 interruption reason 全部返回 Interrupted；
4. 每种 reason 的第一次 terminal receipt 保留 exact HostId；
5. 重复请求为无 HostId no-op；
6. stale item authorization 以 `WeaponAuthorizationChanged` 中断；
7. InputAdapter no-op 也必须携带 InputReleased reason；
8. 既有 hostile Impact、物理输入、CombatCore、item/equipment 与 legacy enemy/ranged 回归全部通过。

## 4. 兼容性与权威边界

- Product Session 继续是唯一 active WeaponGuard Host owner；
- GameMode 只拥有终止 intent route 与装备授权协调，不拥有 Host transition 算法；
- PlayerController 只产生 input／lifecycle intent；
- PlayerHealth 继续拥有 vitality commit 与 applied-damage broadcast；
- CombatRunCoordinator 继续拥有 hostile Impact、resolver 与幂等生命交付；
- ItemSubsystem/ItemAuthority 继续拥有当前装备真值；
- fixed timeline、arc、perfect window 与 defense layer 语义未修改；
- 新增行未调用 `ApplyDamage`、`TakeDamage`、RNG、随机 GUID、frame counter 或 wall clock；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与其它用户资料未修改、未暂存、未提交。

## 5. 修改范围

实现与测试共 8 个文件，361 行新增、94 行删除（不含本 Report/Log 与原始证据日志）：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSessionTests.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardInputAdapterTests.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardImpactRouteTests.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapPlayerController.cpp`

既有 102-rule regression map 已完整覆盖上述路径，本轮无需新增重复 rule。

## 6. 测试覆盖

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

八份日志合计 688 Success、0 Fail；focused 7 项包含于 full suite，按 identity 去重为 681 项。所有日志都有原生 queue-empty marker，且无 Fatal、Unhandled Exception 或 Ensure。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=102
SELF_TEST: PASS 164/164
REGRESSION_COVERAGE: PASS Changed=8 Rules=5 Required=44 Logs=8
ADDED_AUTHORITY_SCAN: PASS AddedLines=369 ForbiddenHits=0
DIRECT_WEAPON_GUARD_TERMINAL_WRITER: PASS 1 (GameMode route)
PRODUCT_TERMINATION_ROUTE_CALLERS: 7
OLD_TERMINATION_APIS: PASS 0
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor implementation | Succeeded | 76 / 238.58s | 0 | `5A2DFA29885CE9AC70BC2C71933F14D62C34B07591336DCB7CC2BBECD31EAF86` |
| Editor final | Succeeded | 6 / 13.09s | 0 | `121A909913D4B58E3622D9C2C9E12F984B6C503DFF3C7D07650EE35021B79FD2` |
| Game final | Succeeded | 75 / 202.02s | 0 | `948D8675B9E7412410FEA8AC0F9D836E3D039F358BDC1AC18631EFF7908D1502` |

最终产物：

- `UnrealEditor-demo_map.dll`：12,885,504 bytes，SHA-256 `0F8B2128EC8BDE28317342703EF562B0D7697C30291FD54473EE32B4D483A24E`；
- `demo_map.exe`：354,394,112 bytes，SHA-256 `11AC858328A02779E8A7A278DF9D59AC052D70AE238C4261E0178A50DBCFC6FF`。

## 9. 真实异常

- 本阶段没有源码编译失败、Automation 失败、Fatal、Unhandled Exception、Ensure 或内存环境错误；
- UE SDK 检查继续报告未安装的非 Win64 平台 metadata；同一输出明确 Win64 SDK `10.0.22621.0` VALID，Editor/Game 两个目标原生退出均为 0；
- focused 日志第一次成功运行后因最终实现重跑，UE 自动保留了一份 backup；交接只采用最终日志，未把重复 backup 纳入 Git；
- regression map 与 self-test 未修改，现有规则已覆盖全部改动路径。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段代码审查、实现、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

下一阶段建议处理 WeaponGuard 与其它玩家武器动作的互斥／抢占：攻击、投掷、主动闪避或新的 guard start 不应在多个产品 Host 中形成并发武器所有权；继续使用 CombatRunCoordinator identity 与 typed command receipt，不在 PlayerController 新建状态机。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-14-weapon-guard-termination>
