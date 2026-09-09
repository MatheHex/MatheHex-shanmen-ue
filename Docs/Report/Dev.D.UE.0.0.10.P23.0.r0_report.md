# Dev.D.UE.0.0.10.P23.0.r0 Report

## 1. 结论

P23.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P19 已完成但尚未接入玩家主循环的神识权威链落到 M01：玩家现在可用可重映射的 `V` 键发出一次神识脉冲，消耗 10 点本局原型 SpiritEnergy，并在主 HUD 上短暂显示存活敌人的位置、距离以及遮挡状态。可见目标使用青色标记，隔墙目标使用金色 `OCCLUDED` 标记；显示持续 3 秒。

```text
Focused Divine Sense:                 48 Success / 0 Fail
Changed-file mapped regression:     2590 Success / 0 Fail (2 healthy logs)
Changed-file regression coverage:     PASS (21 paths / 10 rules / 97 groups)
Regression gate self-test:             PASS 440/440
Game + Editor Development:             PASS (both native 0)
git diff --check:                       PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明的是输入、M01 Run、存活目标筛选、遮挡证据、资源扣除、HUD 数据与 teardown 的无头产品链，不宣称视觉手感、复杂场景遮挡、多人同步或人工体验验收已经完成。

## 2. 玩家侧变化

正式链路现在为：

```text
V（可重映射、仅 Press）
  -> PlayerController 共用 gameplay-input lock
  -> GameMode 持有的 Divine Sense Logical Input Adapter
  -> P19 Divine Sense Product Controller
  -> 当前 Combat Run / SpiritEnergy Authority
  -> 仅扫描已注册且仍存活的 M01 敌人
  -> Visibility trace 区分可见与遮挡
  -> Receipt 驱动主 HUD，3 秒后自动隐藏
```

每局原型资源为 `100 / 100 SpiritEnergy`，每次成功脉冲消耗 10。HUD 顶部显示本次目标数与剩余 SpiritEnergy；每个目标显示距离。输入面板打开、结算锁生效或 M01/Combat Run 未就绪时，输入失败关闭且给出明确原因。

## 3. 单一权威与目标证据

本轮没有新增第二个神识系统。`Ademo_mapGameMode` 只负责拥有并编排既有：

- `Fdemo_mapShanmenDivineSenseProductController`；
- `Fdemo_mapShanmenDivineSenseLogicalInputAdapter`；
- `Fdemo_mapShanmenCombatRunCoordinator`；
- P19 的扫描、观察、Receipt 与资源事务契约。

候选来源仅为 `M01EnemyActors`。每个候选必须实现 `Idemo_mapCombatVitalityHost`、保持相同 Combat Entity ID，并返回有效且 `CurrentVitality > 0` 的快照。Visibility trace 忽略玩家与目标本身，其余阻挡物决定 `WasOccluded()`。HUD 只读最后一次成功 Receipt，不参与资源、目标或生命权威。

## 4. 输入、迁移与生命周期

统一输入注册表从 30 项扩展为 31 项，新增 `DivineSense`，默认键 `V`，仅绑定 `IE_Pressed`。持久输入格式升级为 version 9：

- v8 配置中 `V` 空闲时，迁移后神识继续使用 `V`；
- v8 配置中用户已把其它动作设为 `V` 时，保留用户覆盖并为神识分配可用默认键；
- 运行中重映射会重建物理绑定，旧键立即失效；
- 共用 UI/结算输入锁，不绕开既有页面边界。

Combat Run 激活时创建唯一 `Resource.SpiritEnergy` 快照并启动 Controller 与 Adapter；任一步失败都会回滚既有产品链。Run 释放时先结束并重置神识，再释放 Run Coordinator，同时清空 Receipt 和 HUD 到期时间。一次失败后的 pending retry 最多消费一次物理重试，之后取消陈旧批次，避免玩家被永久卡住。

## 5. HUD 表达

主 HUD 新增轻量覆盖层：

- 青色菱形：当前可见的存活目标；
- 金色菱形并带 `OCCLUDED`：视线被阻挡但被神识揭示的目标；
- 每个标记显示米制距离；
- 顶部面板显示目标数及 `SPIRIT current / max`；
- 3 秒后由世界时间判定失效，无新增 Timer、Tick 所有者或并行状态机。

原 HUD 的移动、技能、交互和战斗提示保持原权威，只在帮助行增加当前神识键位。

## 6. 首次失败与有界修复

首次 5 项物理输入组为 `4 Success / 1 Fail`。新增诊断依次证明问题位于无头测试世界，而非神识生产链：临时 World 名称不是 M01、测试 Controller 未进入 World 的 Controller 列表，并且一次错误的 GC 清理顺序触发 CoreUObject 断言。修复分别为：使用唯一 `/Temp/..._L_M01_Expedition` 包名、显式 `World->AddController()`、GC 前释放临时包观察引用。最终生命周期单测 `1/1` 成功，且日志出现 `PulseAccepted Reveals=1 SpiritEnergy=90.0`。

完整 `demo_map` 首轮另暴露一项与 P23 无关但稳定可复现的旧测试缺陷：`RewardBossSource.48.PlanEquipmentNotBackpack` 的辅助函数只认可武器、护甲和普通饰品，遗漏既有 `SpatialRingCategory`，因此把合法空间戒指误报为背包。保留全量 `1329/1` 与聚焦失败证据后，只扩充测试认可的非背包装备类别；聚焦复测 `1/1`、完整复测 `1330/1330`。

## 7. 自动化证明

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.DivineSense` | 48 | 336,344 | `B0725D3A00DDAECC19D52B2C3EA81125B511A22BD517C6F7FE4765EDB8DB7638` |
| `Shanmen.0_0_10` | 1,260 | 1,929,902 | `1967863BFFEE8782F4CAD80ADC1DF67E8C801C5A12A5069F5B0EBA8FFBD0DBEC` |
| `demo_map` | 1,330 | 1,662,040 | `1774624E3182DDC59137B61A23D5F444840C0D35BD048501F0E549AA1D1FE026` |

新增的 5 项物理/产品测试覆盖：注册表默认值、v8 自由/冲突迁移、Press-only 绑定、实时重映射、共享 UI 锁，以及真实 `Ademo_mapGameMode` + Combat Run + 注册存活敌人 + SpiritEnergy + Receipt + teardown 纵向切片。最终三份日志均为唯一 RunTests 命令、0 Fail、原生成功终止，且无 fatal/unhandled/ensure。

## 8. 改动文件回归与构建

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P23.0_regression_coverage.log` | PASS 21/10/97/2 | 12,907 | `1974466A1A242784E817FDCC8C6202C699B8D2D3E5C5F3EC26D66C474DAC7C70` |
| `P23.0_regression_coverage_selftest.log` | PASS 440/440 | 43,400 | `FACB8B48B6969E6391723F6DEEAF88B8808F30EA0F55B6FC17510D7A595A0F3A` |
| `P23.0_GameBuild_final.log` | PASS / 55 actions / native 0 / 119.54s | 5,972 | `5DEA6C1A14DBB8395DEA1CD8BC4C639CD7AA3C1A5C26AC50FAA0B19ABAE6127B` |
| `P23.0_EditorBuild_final.log` | PASS / up-to-date / native 0 / 0.98s | 974 | `C4E6C07F0713813612B1FDC5892D714DA816422E657C70EE30ECC2B25CA2DFC5` |

最终 `demo_map.exe` 为 359,607,808 bytes，SHA-256 `C2997AFC0EB2C7919E60B45CF41322905D4166789D82E2FC287BD4613F17B47E`；`UnrealEditor-demo_map.dll` 为 18,856,960 bytes，SHA-256 `1CA47E8B0050E99061ED21A90F466F85202E5995D80222B6FE6110E4E1342E56`。

## 9. P/F 与静态边界

PASS：V 键可用且可重映射；v8→v9 迁移保留用户键位；真实 GameMode Run 能揭示一个注册存活敌人；成功脉冲将 SpiritEnergy 从 100 降为 90；Receipt、HUD 有效期与 teardown 一致；2,590 项映射回归零失败；覆盖门、自测和双构建通过。

未声明：神识可扫描非 M01 对象、死亡尸体仍可揭示、SpiritEnergy 已由角色成长/装备配置、遮挡颜色已经人工调色、HUD 已完成分辨率与本地化验收，或产品已进行手动游玩。

非文档改动为 8 个生产文件 `+468/-7`、11 个测试文件 `+572/-20`、2 个回归规则文件 `+30/-1`。新增生产代码中的 Timer/SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、ApplyDamage、SpawnActor 和 DestroyActor 均为 0；无新增资产、模块、Actor、Subsystem、存档字段或第二套玩法权威；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段精确提交 8 个生产文件、11 个测试文件、2 个回归规则文件、本 Report 与本 Development Log，共 23 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P23.0` 首次失败与最终成功原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-0-divine-sense-reveal>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-0-divine-sense-reveal/Docs/Report/Dev.D.UE.0.0.10.P23.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-0-divine-sense-reveal/Docs/Log/Dev.D.UE.0.0.10.P23.0.r0_log.md>
