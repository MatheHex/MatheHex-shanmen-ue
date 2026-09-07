# Dev.D.UE.0.0.10.P21.1.r0 Report

## 1. 结论

P21.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.0 的 canonical “练习飞剑”从可组合的分段证据收紧为一条真实产品入口：同一个精确 `ItemInstanceId` 现在可以从旧 Profile / Code B 来源，经 Shanmen cutover、准备选择、durable Run start 与 P6 准入，原子进入当前战斗 Run 的 ControlledWeapon Host。调用方不能注入玩家实体 ID，也不能绕过已部署物品与 active-Run correlation。

```text
Canonical active-Run route:              1 Success / 0 Fail
Shanmen.0_0_10 full:                  1224 Success / 0 Fail
Required legacy groups:                123 Success / 0 Fail
Regression coverage:                    PASS (Changed=7 / Rules=3 / Required=67 / Logs=6)
Regression gate self-test:              PASS 425/425
Game + Editor Development:              PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是 canonical 物品到 P6 Host 的无头产品闭环，不宣称飞剑 Actor 自动生成、模型、动画、音效、操控手感或人工可视化验收通过。

## 2. 真实产品入口

新增 `Fdemo_mapShanmenControlledWeaponActiveRunRoute`。调用方只提供：

- 精确飞剑 `SourceItemInstanceId`；
- activation sequence；
- 本次攻击公式、控制力与标签；
- 已由产品创建的 Weapon Actor、碰撞根和运动参数。

入口从当前 `Fdemo_mapCombatRunCoordinator` 内部解析玩家 EntityId，并要求 Source Actor 就是该 Run 的 canonical player。随后顺序调用既有 `PrepareActiveRun` 与 `RunHost.TryAttach`，不创建第二套库存、部署、战斗或 Host 权威。

`Ademo_mapGameMode::StartControlledWeaponForActiveCombatRun` 已接入当前 GameInstance item authority、transient Runtime、CombatRunCoordinator、ControlledWeaponRunHost 与玩家 Pawn。旧的低层 attach seam 保留给已有调用者；新的产品入口负责完成权威准备和挂载组合。

## 3. 失败关闭与原子性

入口对以下情况明确失败关闭：依赖缺失、intent/Actor binding 非法、Coordinator 未就绪、来源不是 canonical player、物品准备被拒、Host 挂载被拒以及最终跨边界结果不一致。

准备阶段只读 durable authority；Host 仍沿用既有 copy-on-success 行为。测试确认：

1. 首次启动后 Host 仅绑定一件且处于 Orbiting；
2. Action RunId、SourceEntityId、item instance 与 definition 全部对应同一条 authority 证据；
3. SourceTags 同时包含 Player、Capability.Deploy 与 Item.Weapon.FlyingSword；
4. route 前后 durable authority snapshot 字节语义相等；
5. 精确重放会再次得到同一份准备证据，但以 `ItemAlreadyBound` 拒绝第二次挂载；
6. Run end 先中断并退休飞剑，再释放 Coordinator 身份。

## 4. 端到端贯通证据

新增 `CanonicalCutoverStart` 自动化，真实经过：

```text
Persistent Profile
  -> Profile Session + Code B warehouse source
  -> Shanmen item-authority cutover
  -> exact TrainingFlyingSword preparation selection
  -> durable StartPreparedRun + transient Runtime materialization
  -> CombatRunCoordinator with the same ActiveRunId
  -> ControlledWeaponActiveRunRoute
  -> P6 ControlledWeaponRunHost
```

测试 fixture 使用 canonical `Prototype.Item.Weapon.TrainingFlyingSword`，不再以 `Item.Test.FlyingSword.*` 代替产品内容。全量套件由 1223 精确增加至 1224 项。

## 5. 按改动路径回归

新生产路径首次出现时尚未登记在 regression map；本轮没有放宽 unknown-path gate，而是增加 `ControlledWeaponActiveRunRoute` 映射，要求 P6 adapter/session/world/controller/host/lifecycle、CombatRunCoordinator、Items、WorldGameplay 与 CombatRuntime 证据。

同时新增两条门禁自测：full suite 可覆盖该路径；仅有 Coordinator 证据必须失败。自测由 423 增至 425 项并全部通过。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=67 Logs=6
```

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Active-Run route focused | 1 | 0 | `6C14BAB6A36F531A433FD35FB08956D9B6A2B475D7001E2BA6EC761B90DBFB61` |
| `Shanmen.0_0_10` full | 1224 | 0 | `86522678F4A7A10CEE2D771F519841FA9B901242CC291985EB14F39DE95A3C1B` |
| `demo_map.V3.Attributes` | 4 | 0 | `4FCC02CCA10E33B5BB914662914D87036CE3E00F36D6B06D7F65B004DDEC88F4` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `4728AFC6084F703D0EFA8A44CC37638B5597D28BD3DEB873BAF5C034F9986C1F` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `52CEEAF46B05550C0CC24CF4CAB303F934B0E7A4AEF202EEB21A33FC93A8D4CA` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `75A7C72B2475D5F12852C6E857AFC50070B865689A3ED91E08B19452F6DB3714` |
| `demo_map.P4.Hotbar` | 7 | 0 | `E5E9B72C05FB7D659B24789F9B1CDBE303DE43C64DFFAED625AF5A4EEFD191E4` |

七份自动化日志合计 1348/0（focused 与 full 有意重叠）。完整套件从 `2026-09-07 13:49:34.953` 到 `15:01:28.263 UTC`，单一 UnrealEditor-Cmd 实例自然清空，Fatal / Unhandled / Ensure 为 0。

## 6. 静态审计与构建

- 7 个实现、测试和流程文件，`+471 / -1`；
- 新 route 中 SpawnActor、World/Tick、RNG、ApplyDamage 与物品 mutation 调用命中 0；
- `git diff --check` 原生退出码 0，仅有工作树 LF→CRLF 提示；
- 103 份用户原有 untracked 文件保持未暂存；
- 自动化结束后无项目 UnrealEditor、UnrealEditor-Cmd 或 demo_map 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 33 / 162.36s | `503E81737FF042AED02618CB650A3F8B6CB9DFE4937B98B980CE9311C50B5C55` |
| Game Development final | Succeeded / native 0 | 32 / 144.00s | `66AABB47938DDDBDE0ADCC7C7A87243AA7F87502F4885C2CF5C9BB8930FF786F` |
| Editor Development final | Succeeded / native 0 | 0 / 1.02s | `125A9190F6CBC443585403486304E02810F108DA11977A6E54AE7E19BED04389` |

最终产物：

- `demo_map.exe`：359,314,432 bytes，SHA-256 `6EDAA4DD3E374CD97A6FA96A2E193525E4AFA0F0D2618EAE8B18D937D3ED88BD`；
- `UnrealEditor-demo_map.dll`：18,456,064 bytes，SHA-256 `8EF87CE563C80655E54393A3E04568DD4E2DB63EEB999582680B27E9C8B87EE1`。

## 7. P/F 边界与后续

PASS：同一 canonical 飞剑身份贯穿 Profile、Code B、cutover、准备、durable Run、transient Runtime、CombatRunCoordinator、P6 preparation 与 Host；玩家身份不可注入；重复绑定失败关闭；durable authority 无旁路写入；Host 有序 teardown；路径回归、全量自动化和双目标构建通过。

未声明：自动生成飞剑 Actor、Run 开始时自动挂载、真实按键或 AI 指令、飞剑运动/碰撞/伤害的产品可视链、素材与表现验收。

下一步应由唯一的产品生命周期拥有 Weapon Actor 的创建/回收，并在 CombatRun 已建立、durable item 已部署后调用本轮入口；不得把 Actor 存在性反向当成物品或 Run 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-1-canonical-flying-sword-active-run>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-1-canonical-flying-sword-active-run/Docs/Report/Dev.D.UE.0.0.10.P21.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-1-canonical-flying-sword-active-run/Docs/Log/Dev.D.UE.0.0.10.P21.1.r0_log.md>
