# Dev.D.UE.0.0.10.P21.2.r0 Report

## 1. 结论

P21.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.1 已经贯通到 P6 Host 的 canonical “练习飞剑”投射为真实 World Actor，并把创建、准入、运动载体与回收接入现有 Combat Run 生命周期。物理 Actor 只承担世界碰撞与表现，不成为库存、部署或 Run 的第二权威。

```text
Canonical World lifecycle focused:       1 Success / 0 Fail
Shanmen.0_0_10 full:                  1225 Success / 0 Fail
Required legacy groups:                123 Success / 0 Fail
Regression coverage:                    PASS (Changed=9 / Rules=3 / Required=68 / Logs=6)
Regression gate self-test:              PASS 427/427
Game + Editor Development:              PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是 canonical 飞剑 Actor 的无头 World 生命周期、权威绑定、运动和回收，不宣称模型、美术、动画、音效、操作手感或人工可视化验收通过。

## 2. World Actor 与唯一权威

新增 `Ademo_mapShanmenControlledWeaponActor`，提供：

- `UBoxComponent` 世界碰撞根与引擎基础网格可视载体；
- 精确 `RunId`、`ItemInstanceId` 与 Source Actor 绑定；
- P21.1 route 接受前保持 `NoCollision`，完成 Host 准入后才发布为 `QueryOnly`；
- 禁用 Actor Tick，不自行推进运动、伤害或资源事务。

Actor 的存在性不代表物品已拥有、已部署或 Run 有效。上述真值仍分别来自 Shanmen item authority、durable active-Run correlation、transient Runtime 与 `Fdemo_mapCombatRunCoordinator`。

## 3. 自动生命周期

新增 `Fdemo_mapShanmenControlledWeaponWorldLifecycle`，固定执行：

1. 读取 durable active-Run correlation；
2. 交叉验证 Runtime 与 Coordinator 的精确 Run 身份；
3. 从 immutable authority snapshot 找到同一 deployed weapon instance；
4. 仅当 DefinitionId 为 `Prototype.Item.Weapon.TrainingFlyingSword` 时创建 Actor；
5. 以 canonical phase-0 环绕位置生成，绑定 Run/item/source 身份；
6. 调用 P21.1 active-Run route，让同一 Actor 原子进入 P6 Host；
7. Host 接受后才开放 QueryOnly 碰撞。

其它武器返回成功的 `NotApplicable`，不会误生成飞剑；重复开始返回 `LifecycleBusy`，不会产生第二个 Actor。

`Ademo_mapGameMode::TryActivateCombatRun` 已在既有产品生命周期成功后自动调用该 World lifecycle。若 World 准入失败，GameMode 走现有 Run rollback；Run 结束时先完成 P6 Host → Coordinator 的逻辑 teardown，再销毁物理 Actor。孤儿或异常回收使用有界 reset，不保留碰撞载体。

## 4. 真实 World 测试

新增：

```text
Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.CanonicalTrainingFlyingSword
```

测试使用真实 `GamePreview UWorld`，执行：

```text
Persistent Profile
  -> Code B + Shanmen cutover
  -> exact TrainingFlyingSword preparation
  -> durable Run + transient Runtime
  -> CombatRunCoordinator
  -> World lifecycle + Actor spawn
  -> P21.1 active-Run route
  -> P6 Host
```

核心断言：

- World 中恰有一个 canonical weapon Actor；
- 第一帧前已位于 phase-0 防御就绪位置；
- Actor 的 Run、item、source、owner 与 P6 receipt 完全一致；
- 只有 route 成功后碰撞才为 QueryOnly；
- 重复开始失败关闭且 Actor 数仍为 1；
- 既有 orbit pump 推进的是同一个 Actor，而非替代实例；
- Host 未清空时禁止提前销毁物理 Actor；
- 逻辑 teardown 完成后 Actor 才销毁；
- 整个过程前后 durable item authority snapshot 不变。

## 5. 按改动路径回归

为两个新生产文件登记 `ControlledWeaponWorldLifecycle` 映射，并新增两条门禁自测：完整 0.0.10 证据可覆盖；仅 Coordinator 证据必须失败。门禁自测由 425 增至 427 项并全部通过。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=68 Logs=6
```

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| World lifecycle focused | 1 | 0 | 263,643 | `41682FC8B959EA7DBE1E4D08235688572AEC521AE2EF73CC5B36BB79D2B47E10` |
| `Shanmen.0_0_10` full | 1225 | 0 | 1,878,548 | `094A71B4827AEFEA29FE5EF6DF5D7E185536A051A36E2C6DCBCDA5779446A6F4` |
| `demo_map.V3.Attributes` | 4 | 0 | 265,508 | `7BD5584F0B62B14F1B25D3F758EC8D78D88E5A1BF670D7F65673F840141DA1E1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 306,165 | `C540B4F70BB283CD2640C2A26A3317DA87FB3F1ECDAAB48DD923F48FAFBA811B` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 285,721 | `FA24F921B5AFEDC3DAD83C6CB07A303B174D9A68C6FAEDACF3A9608117CB3B6E` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 310,198 | `353EA7F63669EB79B8289CECC87F7AF5A9C809B6F17E27D76A851F38B78BE231` |
| `demo_map.P4.Hotbar` | 7 | 0 | 268,017 | `4F35C143207EE20C73F8C61CF13E0CFF14EC7F54C5B0C7879AB45C27003EBD38` |

七份自动化日志合计 1349/0（focused 与 full 有意重叠）。完整套件从 `2026-09-07 16:06:07.561` 到 `17:16:15.886`，单一 UnrealEditor-Cmd 实例自然清空，原生退出码 0，Fatal / Unhandled / Assertion / Ensure 为 0。

## 6. 静态审计与构建

- 9 个实现、测试和流程文件，约 `+1021 / -2`；
- 新 World 生产层中 `ApplyDamage`、资源 Reserve/Commit、`StartPreparedRun`、RNG 与 Actor Tick override 命中 0；
- `git diff --check` 退出码 0，仅有工作树 LF→CRLF 提示；
- regression self-test：427/427，SHA-256 `B700C8931E7C2DCA8F7947F3D88C2564B450246BDD6E7DEF59E33B396A6F04A2`；
- regression gate：PASS，SHA-256 `015020F0F57125561E27EA0C4F69C9BA424439467BDD02DE5669236293CF90CC`；
- 4 个本轮新增文件与 103 个用户原有 untracked 文件已区分，后者保持未暂存；
- 自动化与构建结束后无 UnrealEditor / UnrealEditor-Cmd 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 34 / 164.85s | `58BEFE59C11C26E90C4EBFB12A4B9839CD769FC0941CE2EC9953560D132D1B9E` |
| Game Development final | Succeeded / native 0 | 33 / 190.66s | `C6AB1099C89EF37E09CA25740FAAFDDB5369C823EEBEFDF03C38951812899F76` |
| Editor Development final | Succeeded / native 0 | 0 / 1.15s | `6D5E98CEF369478FF9724438499818AF5AC19746FCDF341B5E974ECFD477C762` |

最终产物：

- `demo_map.exe`：359,344,128 bytes，SHA-256 `1A8E0C21592C2581D911BC9BC8AB9E23386A57D8CBE7AEB86453846FBB45532D`；
- `UnrealEditor-demo_map.dll`：18,498,560 bytes，SHA-256 `66666A47B4785E569362057B9E4B0235BFB4FA4075625C0EA4FB4131DF5FDD94`。

## 7. P/F 边界与后续

PASS：同一 canonical TrainingFlyingSword 的 durable item、active Run、World Actor、P21.1 route 与 P6 Host 身份一致；仅一件 Actor；初始位置确定；现有 orbit pump 可移动该 Actor；碰撞发布有序；重复开始和提前回收失败关闭；逻辑权威先于物理载体结束；全量、旧回归、门禁和双目标构建通过。

未声明：人工观察下的外观、动画、VFX、音效、输入手感、敌人接触反馈或最终伤害体验。World 测试直接验证生命周期；GameMode 自动接线已编译并通过相关全量/旧回归，但本轮未启动可视产品流程。

下一步应让已有产品运动/碰撞链在真实游戏帧中消费该 Actor，继续复用 P6 Host、现有 detector 与 Impact authority，不为飞剑另建库存、Run、运动或伤害系统。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P21.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P21.2.r0_log.md>
