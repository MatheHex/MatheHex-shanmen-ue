# Dev.D.UE.0.0.10.P21.2.r0 Development Log

## 1. 基线与目标

- base：`87a3689d9385d23da627767dbef2874ff9488c6a`（P21.1 canonical flying-sword active-Run route）；
- branch：`agent/0.0.10-p21-2-training-flying-sword-world-lifecycle`；
- 目标：为 exact canonical TrainingFlyingSword 建立由 Combat Run 自动拥有的 World Actor 生命周期；
- 边界：不建立第二套 item/Run/Host/伤害权威，不启动可视产品，不修改 Windows 或 Engine。

## 2. 起始审计

P21.1 已证明同一个 TrainingFlyingSword item instance 可从 Profile / Code B / Shanmen cutover 贯通 durable Run、Runtime、Coordinator 与 P6 Host，但 Weapon Actor 仍要求调用方预先创建和传入。因此产品 Run 尚不能自行投射、发布或回收飞剑的世界载体。

既有 P6 controller 已拥有确定性 orbit/return 状态机与 sweep/contact 入口。本轮选择补齐产品组合层，不复制运动数学、detector、Impact、库存或 Run 状态。

## 3. 实现

新增：

- `demo_mapShanmenControlledWeaponActor.h/.cpp`；
- `demo_mapShanmenControlledWeaponWorldLifecycle.h/.cpp`；
- `Fdemo_mapShanmenControlledWeaponWorldStartResult` 与明确状态枚举。

World lifecycle 的固定顺序：

1. 要求空且有效的生命周期、同一 World 的 Source Actor 与可用 item authorities；
2. 重建 durable correlation，并与 Runtime / Coordinator exact RunId 交叉验证；
3. 从 immutable snapshot 找到 correlation 指向的 deployed weapon；
4. 非 canonical TrainingFlyingSword 返回 accepted `NotApplicable`；
5. canonical 飞剑在 phase-0 orbit 位置 transient spawn；
6. 绑定 exact Run/item/source identity，保持 NoCollision；
7. 委托 P21.1 route 进入既有 P6 Host；
8. route 成功后发布 QueryOnly collision；
9. Run end 时要求 Host 已空，再销毁物理 Actor。

GameMode 已把该入口接在既有 combat-product activation 之后，并在 activation rollback、正常 Run end、孤儿清理和失败回收路径中保持逻辑 Host/Coordinator 先于物理 Actor 的顺序。

## 4. 测试实现

新增 `CanonicalTrainingFlyingSword` World 测试。fixture 创建真实 GamePreview World、Pawn 与健康组件，并复用 Profile、Code B、cutover、prepared durable Run、Runtime、Coordinator、P21.1 route 与 P6 Host。

验证：唯一 Actor、初始防御位置、精确身份、QueryOnly 发布、重复启动拒绝、同 Actor orbit 位移、提前退休拒绝、Host → Coordinator → Actor teardown，以及 durable authority 前后不变。

全量测试由 1224 精确增加至 1225 项。

## 5. 首次构建与 focused 证据

首次 Editor build：34 actions / 164.85s / Succeeded，一次通过，没有源码修复轮；日志 4,584 bytes，SHA-256 `58BEFE59C11C26E90C4EBFB12A4B9839CD769FC0941CE2EC9953560D132D1B9E`。

focused test 首次运行即通过：

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.2.r0_focused_first.log` | ControlledWeaponWorldLifecycle | 1/0 | 263,643 | `41682FC8B959EA7DBE1E4D08235688572AEC521AE2EF73CC5B36BB79D2B47E10` |

日志自然清空，进程退出 0，Fatal / Unhandled / Assertion / Ensure 为 0。本轮没有需要保存的源码首败。

## 6. 全量与旧回归

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.2.r0_full.log` | `Shanmen.0_0_10` | 1225/0 | 1,878,548 | `094A71B4827AEFEA29FE5EF6DF5D7E185536A051A36E2C6DCBCDA5779446A6F4` |
| `P21.2.r0_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 265,508 | `7BD5584F0B62B14F1B25D3F758EC8D78D88E5A1BF670D7F65673F840141DA1E1` |
| `P21.2.r0_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | 306,165 | `C540B4F70BB283CD2640C2A26A3317DA87FB3F1ECDAAB48DD923F48FAFBA811B` |
| `P21.2.r0_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 285,721 | `FA24F921B5AFEDC3DAD83C6CB07A303B174D9A68C6FAEDACF3A9608117CB3B6E` |
| `P21.2.r0_item_use.log` | `demo_map.ItemUseAndArmor` | 46/0 | 310,198 | `353EA7F63669EB79B8289CECC87F7AF5A9C809B6F17E27D76A851F38B78BE231` |
| `P21.2.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 268,017 | `4F35C143207EE20C73F8C61CF13E0CFF14EC7F54C5B0C7879AB45C27003EBD38` |

七份日志合计 1349/0（focused 与 full 有意重叠）。完整 0.0.10 套件由一个 UnrealEditor-Cmd 实例从 `16:06:07.561` 运行至 `17:16:15.886`，正常收到 queue-empty 终止信号并以原生状态 0 退出；所有日志的 Fail、Fatal、Unhandled、Assertion 与 Ensure 计数均为 0。

## 7. Regression map

新增 `ControlledWeaponWorldLifecycle` rule，要求 full 0.0.10 与直接 ControlledWeapon authority seam。自测增加：

- full suite 必须覆盖 Actor / World lifecycle；
- coordinator-only 证据必须被拒绝。

最终：

- self-test `PASS 427/427`；42,266 bytes；SHA `B700C8931E7C2DCA8F7947F3D88C2564B450246BDD6E7DEF59E33B396A6F04A2`；
- changed-file gate `PASS Changed=9 Rules=3 Required=68 Logs=6`；7,881 bytes；SHA `015020F0F57125561E27EA0C4F69C9BA424439467BDD02DE5669236293CF90CC`。

## 8. 静态与最终构建

- 5 tracked changed + 4 owned new production/test/process files，约 `+1021 / -2`；
- forbidden boundary scan：ApplyDamage、Reserve/Commit、StartPreparedRun、RNG、Actor Tick override 共 0；
- `git diff --check` exit 0；
- 103 个用户既有 untracked 文件未暂存；
- residual Unreal process 0。

| Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 34 / 164.85s | 4,584 | `58BEFE59C11C26E90C4EBFB12A4B9839CD769FC0941CE2EC9953560D132D1B9E` |
| Game final | Succeeded / native 0 | 33 / 190.66s | 4,361 | `C6AB1099C89EF37E09CA25740FAAFDDB5369C823EEBEFDF03C38951812899F76` |
| Editor final | Succeeded / native 0 | 0 / 1.15s | 1,019 | `6D5E98CEF369478FF9724438499818AF5AC19746FCDF341B5E974ECFD477C762` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,344,128 bytes / SHA `1A8E0C21592C2581D911BC9BC8AB9E23386A57D8CBE7AEB86453846FBB45532D`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,498,560 bytes / SHA `66666A47B4785E569362057B9E4B0235BFB4FA4075625C0EA4FB4131DF5FDD94`。

## 9. 提交边界

计划提交 9 个实现、测试和流程文件、本 Report 与本 Development Log，共 11 个文件。103 份用户原有 untracked 文档保持未暂存；`Saved/Codex/P21.2` raw logs 不入 Git。

未修改 Content、地图、项目资源、配置、Engine、Windows、save schema、既有 P6 运动数学、库存写路径或 Impact resolver。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P21.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-2-training-flying-sword-world-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P21.2.r0_log.md>
