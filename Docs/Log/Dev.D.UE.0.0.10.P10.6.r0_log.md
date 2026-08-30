# Dev.D.UE.0.0.10.P10.6.r0 Development Log

## 目标

在 P10.5 唯一 Spirit Evasion ProductHost 上增加薄 `UActorComponent` 生命周期桥：从真实 owner/World 获取调用期能力，活动期驱动 host，Recovery/终态停 Tick，EndPlay 路由 owner end；不复制状态、不绑定输入、不接临时 SpiritEnergy。

## 审计结论

1. P10.5 已独占 action runtime 与 P10.4 coordinator；
2. 产品层缺少 UActorComponent、World time、Tick 和 EndPlay 接线；
3. 现有 SkillComponent 已采用默认停 Tick、活动期启用、EndPlay 收口的成熟模式；
4. 旧 KnockbackComponent 自有方向、距离、duration 与 active bit，不应被 Spirit Evasion 复制；
5. PlayerController 与 GameMode 现有输入/安装路径体量较大，本阶段不在内容 authority 未确定时直接修改；
6. 因此本阶段只建立可挂载、可调用、无第二状态机的薄组件。

## 设计决策

1. 组件字段只持有 ProductHost 与 last-step proof；
2. active/recovery/terminal/tick requirement 全从 host 派生；
3. TryStart 只接收已冻结 action、definition、policy、trajectory 与 direction；
4. production start 要求有效 Character owner 与非 teardown World；
5. start time 使用 World game time；
6. preflight adapter 只在启动调用期存在；
7. TickGroup 为 PrePhysics；
8. Tick 默认关闭，只在 host Active 时开启；
9. Tick 每次只把 absolute World time 与 owner Character交给 P10.5；
10. Recovery/terminal/not-ready 统一停 Tick；
11. EndPlay 对未终态 host 路由 owner end；
12. 活动或 Recovery 中拒绝第二次 start；
13. 终态组件允许以新 action 重启；
14. 候选 host 完整成功后才替换旧 host；
15. component start result 保留 exact host start proof；
16. automation-only explicit time/port seam 不进入 Shipping；
17. 不直接调用 displacement mutation；
18. 不绑定 input、RNG 或 SpiritEnergy。

## 执行序列

1. 审计 PlayerHealth、Skill、Knockback 组件及 PlayerController/GameMode 接线模式。
2. 建立 `Udemo_mapShanmenSpiritEvasionComponent` 与 component start result。
3. 实现 production owner/World/start/preflight 委托。
4. 实现 Active-only Tick 与 Character execution 委托。
5. 实现 cancel、interrupt、owner end、Recovery finish 与 EndPlay 路由。
6. 实现 terminal restart 和 busy fail-closed。
7. 增加 automation-only explicit time/port seam。
8. 新增七个 focused tests。
9. regression map 增至 83 rules；self-test 增至 126 cases。
10. 静态确认一个 Host 字段、零重复状态、零直接 mutation/input/RNG/resource。
11. Editor candidate 6 actions，原生退出 0。
12. focused candidate `7/7`，未发生源码修正重跑。
13. 串行执行十一组正式 Automation，共 `539` success、`0` fail。
14. changed-file gate 以 `Changed=5 / Rules=1 / Required=11 / Logs=11` 通过。
15. `git diff --check` 与精确五文件实现暂存区通过，`+847/-0`。
16. Editor final 0 actions，Game final 5 actions，原生退出均为 0。
17. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
input/content/resource owners
  -> frozen action + definition + policy + trajectory + direction
  -> SpiritEvasionComponent.TryStart
      -> owner Character + World time
      -> transient P10.2 preflight port
      -> P10.5 ProductHost

Host Active
  -> component Tick enabled
  -> World absolute time + owner Character
  -> ProductHost.TryAdvanceCharacter
  -> P10.4 -> P10.3 -> shared swept authority

Host Recovery/terminal
  -> component Tick disabled

EndPlay / owner unavailable / World teardown
  -> ProductHost.TryOwnerEnd
  -> component Tick disabled
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Product SpiritEvasionComponent | 7 | 0 | `4C64C6C7EC6B6D1654F4F54DA34880D07CDE97FFC1C16B46B80736C6054196D4` |
| Product SpiritEvasionProductHost | 7 | 0 | `7FB4CE76AC701D54B232D33E2176F5187E77049FCF4158C2FFCB172BCA4395FB` |
| Product SpiritEvasionActionCoordinator | 7 | 0 | `18F5EB9347CAFB88BA58EB66FE2B71E37F6ADF971BAAA579EEC72A417FCA13BC` |
| Product SpiritEvasionMotionRuntime | 7 | 0 | `52901E0A4ACD50D433CB410758AE124BAF1548443A90BE98BDB754882F940DC1` |
| Product SpiritEvasionMovementAdapter | 5 | 0 | `B5297CCFF029DA3135DA099CCDAB00AE3F74A2C2CC008724B6B35E12935E95C5` |
| CombatRuntime SpiritEvasionMovement | 5 | 0 | `ADF397AE3C9BC747F7E80A824A5772FEAB7F579E97450F2D6D93BFF4FC0F7651` |
| CombatRuntime SpiritEvasion | 11 | 0 | `96211F75720A6123E92067BAD665F11A4C705D98B0C218CD1E6052ECEA4D68A5` |
| CombatRuntime ActionLifecycle | 1 | 0 | `71A5CE9A96AD8093A3F217C89AD87B5E0CF5447F15E06845B2DB1DE473BD23E7` |
| EnemySkillFramework | 44 | 0 | `679AE7AF2F2E30FCAE49788991703F929A3E823B165CC7F864B4790E6D953ADB` |
| V2RangedCompatibility | 22 | 0 | `6D16F030B9799E8C9AFC645510AEEB1CDD0087E67C5FC984872C147B547B8E10` |
| Shanmen.0_0_10 | 423 | 0 | `1A043C4226912AAF9AFF63761D409AE218FCB263026064E64B913BCC1F9E207B` |

所有正式进程原生退出码为 `0`；十一份日志均有 selected queue-empty，且没有 selected fail、fatal、unhandled 或 ensure。

focused candidate：`7/7`，SHA-256 `9A6C4A569DD2E709AD40AFA5713A7E30E26DBE1CD1309D968C9BC2E52264580A`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=83
SELF_TEST: PASS 126/126
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
OWNED_HOST_FIELDS=1
DUPLICATE_STATE_FIELDS=0
WORLD_TIME_READS=4
TICK_ENABLE_CALLS=2
OWNER_END_ROUTES=3
Editor candidate: 6 actions / 48.06s / exit 0
Editor final: 0 actions / 0.89s / exit 0
Game final: 5 actions / 33.21s / exit 0
```

mapping SHA-256：`E61A24D3E1AC008905FFE6948544783400202BC8ED6214A2699BBD8970CFC27E`；self-test SHA-256：`9BC304EBC5176EFCD46F4C06973EB9EACD9C08CFA2FF53097C6D24E618DE75F8`。

最终 `UnrealEditor-demo_map.dll`：`12345344` bytes / SHA-256 `0CA997F11DEB5B6F37040D25DD9FF921642D59BA879D1E63F6D20192A735D5A9`；`demo_map.exe`：`353809920` bytes / SHA-256 `94527874817C768E7FA2AE679ACE2D3AF6BBA86CAB33655DD37571572F7C46AF`。

## 真实异常

没有源码、Automation、门禁或构建失败；没有环境内存错误、非零原生退出、外层超时或重试掩盖。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 验证的是 explicit-time/fake-port component delegation；production Tick、真实 Character preflight 和 swept movement仅编译，未在测试 World 中执行。

## 下一步

P10.7 建立唯一 installation/command route：在玩家 Pawn 上确保恰好一个本组件，并由既有 input owner 提交 typed start/cancel command。action/content/policy/trajectory 必须由现有 authority 冻结；不得在输入层制造身份、复制状态或建立临时 SpiritEnergy。
