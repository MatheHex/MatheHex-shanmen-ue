# Dev.D.UE.0.0.10.P18.3.r0 Report

## 1. 结论

P18.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.2 的 `SwordQiRunHost` 接入最小产品命令路由：Run Coordinator 分配确定性动作身份，Product Authority 冻结配置与设备无关命令，Product Session 在一次动作授权后创建惰性载体、跨越 Startup/Active、采用 Host，并把飞行中剑气投影为持久玩家动作占用。相同命令重放返回原始审计结果，不会重复授权、保留序列或创建 Actor。

```text
Sword Qi Product Session exact:        4 Success / 0 Fail
Player Action Arbitration exact:       4 Success / 0 Fail
Shanmen.0_0_10 full:                 765 Success / 0 Fail
Required legacy groups:              141 Success / 0 Fail
Regression coverage:                  PASS (Changed=12 / Rules=3 / Required=21 / Logs=7)
Regression gate self-test:             PASS 279/279
Boundary scan:                         PASS (Files=10 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

本轮没有接真实输入、装备 UI、表现资源、声音或关卡投放，也没有启动 Unreal Editor UI 或产品。因此证明的是产品命令、授权、Run 身份、物理 Host 与持久占用的代码级闭环，不宣称实际游戏手感或表现验收通过。

## 2. 冻结配置与动作身份

新增 P18.3 canonical 产品配置：

- content version：`0.0.10.P18.3`；
- digest：`Shanmen.SwordQi.ProductConfig.r1`；
- detector：`Detector.SwordQi.Basic01.Straight`；
- formula：`Combat.Formula.SwordQi.Basic01.r1`；
- BaseDamage `0.5`、AttackPowerCoefficient `0.01`；
- FlightSpeed `900`、MaximumRange `1400`；
- damage/target tags：`DamageSpirit`、`TargetLiving`。

Player Action 增加显式 `SwordQi` 种类。CombatRunCoordinator 使用独立单调序列分配 Sword Qi reservation；所有外部值在消耗序列前先校验。命令身份复用 Run 发出的 ActivationId，并冻结 exact source item、offense、origin 与 normalized direction。

## 3. 产品命令与启动顺序

产品链固定为：

```text
existing equipment authority supplies exact item id
  -> ProductAuthority validates and freezes command
  -> ProductSession requests one action authorization
  -> spawn collision-inert carrier
  -> Action Startup -> Active
  -> P18.2 SwordQiRunHost launches/adopts carrier
  -> persistent SwordQi occupancy until terminal retirement
```

Session 不查库存也不改库存。未来装备适配器只负责提供现有权威中的 exact item instance id，不能在本路由建立第二套装备真值。

## 4. 生命周期与占用

飞行中 Session 以 non-preemptible `SwordQi` claim 投影到 `Fdemo_mapShanmenPlayerActionOccupancySnapshot`。其它玩家动作通过既有 arbitration 观察这一占用并失败关闭；Session 不另建输入锁、动作队列或并行仲裁器。

中断、超距或 Host 终态都保留不可变 terminal receipt。调用方复制 terminal proof 后才可 retire，并为下一发清空 Host；切换 Run 时会重置 Sword Qi activation sequence 与 Session 归属，旧 Run 命令不能跨 Run 重放。

## 5. 重放、冲突与失败关闭

- exact command replay 返回首次结果，`bReplay=true`，不再次授权或创建 Actor；
- 同一 ActivationId 搭配不同 payload 返回 `CommandIdConflict`；
- action gate 拒绝与 carrier spawn 失败均成为可重放的持久终态；
- 无效 item/offense/origin/direction 在 reservation 前失败，不消耗 sequence；
- Run/source/session 不匹配、Host busy、Action/launch transition 失败均明确关闭；
- 所有已创建但未发布的载体在失败路径销毁。

## 6. Automation 证据

新增四条 Product Session 测试：`CanonicalAuthority`、`RouteReplayAndOccupancy`、`ConflictAndPayloadReplay`、`SpawnFailureAndRunReset`。Player Action Arbitration 同步覆盖 Sword Qi 持久占用。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Product Session exact | 4 | 0 | `0BCD48357AA480B6AC0F4B0692694208373490F23A07B51FB125C8B88A0B9A49` |
| Player Action Arbitration exact | 4 | 0 | `99890E05FEFF2B47DBC40561AB0501DAA269010DC6B4EB12CFDFCE2B4A815828` |
| `Shanmen.0_0_10` full | 765 | 0 | `87446800837E0ABE1A2C6A49FC5BC53BD0C5F7F3A6C361AFB235F82698AC7D14` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `6478BFBF997808098B1F573D5CF6D400D493060614AA162BCAD27BB5C5E7AF0A` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `2F149B1DF4286E133471E83BC4C447E6E508570B38A101759BAD37E2B0AF3CCF` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `7439CE010A43F7768AC2034D6C3C3B8D1AE0E8060AEB1849D90C626AE58C0760` |
| `demo_map.V3` | 29 | 0 | `536AD127E89096B05F0F67E366DD966BB09DA0FE5B1A6E279DD98BF0AE700249` |

所有采用日志都有 native test exit 0，Fail、Fatal、Unhandled 与 Ensure 为 0。完整套件首末 Success 为 `2026.09.02 22:30:41.072 -> 23:00:59.956 UTC`，约 30m18.884s；相对 P18.2 的 761 条精确增加 4。

## 7. 改动驱动回归与静态门禁

新增 `SwordQiProductSession` 路径映射，要求 Product Session、Run Host、World Delivery、Player Action Arbitration、CombatRunCoordinator、WorldGameplay、CombatRuntime 与 CombatCore 等 21 个去重测试前缀的健康证据。

```text
REGRESSION_COVERAGE: PASS Changed=12 Rules=3 Required=21 Logs=7
SELF_TEST: PASS 279/279
BOUNDARY_SCAN: PASS Files=10 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`00260411621944ADB81A8A74A4F0CCAB322E6FFA89B0C9439CBD503E762F1ACF`；
- self-test SHA：`357A522A520897382DC78E92BB6243D788B3A0B2E12F623337EDD8F650DB9786`；
- regression map SHA：`A1715B5596909DC3C1509FC440AF4D2C6F5FCA5FAC70200CB027A54E8FFDD12E`；
- boundary SHA：`DB0CA745A4B37C1A3F879D15D284FC23E20A0BDBBFF815AC7D83FE62F6CF1134`；
- git diff check SHA：`6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

首次 coverage 检查因四组 legacy evidence 缺失而按设计失败关闭；补齐后，单独 `demo_map.V3.Attributes` 两次快速退出日志都因缺少 terminal marker 被拒绝，最终改用包含该组且具完整终止证据的父级 `demo_map.V3` 日志。门禁没有把不完整日志当成成功。

## 8. 构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 116 / 356.10s | `F58C1431BC5EA5B8366C0F18A2632C51BFA6B6581F88CB4D620253B3C028C9B1` |
| Game Development final | Succeeded | 115 / 298.69s | `FF6D01B2877EB1E6485B7775EC619240DFE9DE83DA0811703423366EEF22603C` |
| Editor Development final | Succeeded / up to date | 0 / 1.01s | `F287061EF32B583C9036B4E55BBD940E8BC09ED493B42328812AF9EE761D784D` |

最终产物：

- `demo_map.exe`：356,320,256 bytes，SHA-256 `F2A54784AF565925DFB4E7ADA321856FFCB0F91B1CB30ED995C0752FCB02D3E1`；
- `UnrealEditor-demo_map.dll`：14,980,096 bytes，SHA-256 `76D707DDC5DBB305788C34DA1DA7CA5E18B3A014AE5B5366AC24F79D94BF0129`。

## 9. 修改范围与 P/F 边界

本轮提交 5 个新增源码文件、5 个既有源码文件、2 个回归门禁文件及本 Report/Development Log，共 14 个文件。没有修改 Content、地图、资源、配置、Windows、UE Engine、既有库存或生命权威。

只执行 unattended、NullRHI Automation、静态门禁和 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交；raw logs 只保存在本地 `Saved/Codex/P18.3`。

## 10. 下一阶段

P18.4 建议建立 Run-scoped Sword Qi 产品控制器：由现有装备权威适配器提供 exact equipped sword instance，调用 P18.3 Authority/Session，并统一驱动 terminal retirement。继续不接真实输入；先把 Run owner、装备来源与 Session 生命周期组合为唯一产品入口，再在后续阶段连接输入与表现。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-3-sword-qi-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-3-sword-qi-product-route/Docs/Report/Dev.D.UE.0.0.10.P18.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-3-sword-qi-product-route/Docs/Log/Dev.D.UE.0.0.10.P18.3.r0_log.md>
