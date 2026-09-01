# Dev.D.UE.0.0.10.P15.7.r0 Report

## 1. 结论

P15.7 完成 legacy `demo_map.EnemyRouteLoot.42.EnhancedCorpseCapacity` 的有界收敛：尸体 Equipment section 容量不再与陈旧常量 `4` 比较，而是与当前 canonical equipment-role authority 保持一致；Body section 的独立容量 `2` 仍被明确验证。

`demo_map.EnemyRouteLoot` 从 `43 Success / 1 Fail / 44 Total` 收敛为 `44 Success / 0 Fail / 44 Total`。legacy `demo_map` 父组从 P15.6 的 `1322 Success / 8 Fail / 1330 Total` 变为 `1323 Success / 7 Fail / 1330 Total`，精确消除这一项失败，没有隐藏其它失败。

本阶段 production C++ 零修改；没有把尸体容量降回 4，也没有删除 SpatialRing 或 Backpack 装备角色。

## 2. 根因

从 foundation baseline `f2e4430` 起，两个独立产品权威已经一致采用 5 个装备角色：

- `Fdemo_mapItemDefinitions::GetEquipmentSlotIds()`：Weapon、Armor、Accessory、SpatialRing、Backpack；
- `Fdemo_mapSearchContainerPrototypeConfig::CorpseEquipmentCapacity`：`5`。

但同一 baseline 中的 EnemyRouteLoot case 42 仍硬编码 `GetSectionCapacity(...Equipment) == 4`。因此该断言从版本基线开始就与产品模型冲突；当前失败不是产品容量回归，而是 fixture 没有跟随第五个 canonical equipment role。

Body section 的 `2` 与当前产品配置一致，不属于问题。

## 3. 实现

case 42 现在分别验证两个契约：

```text
Corpse Equipment capacity == canonical equipment-role count
Corpse Body capacity      == 2
```

Equipment capacity 与 role catalog 由不同产品类型拥有，因此该比较不是同字段自比较：任一侧单独增删角色或容量都会使测试失败。Body 容量继续以独立固定值验证。

## 4. 回归覆盖门禁

本轮为此前未映射的 legacy fixture 新增 exact rule：

```text
Source/demo_map/demo_mapEnemyRouteLootTests.cpp
  -> demo_map.EnemyRouteLoot
```

流程自检新增一正一反：

- exact EnemyRouteLoot 日志可以覆盖该 fixture；
- 无关 `Shanmen.0_0_10` 全量日志不能替代 legacy EnemyRouteLoot suite。

最终流程证据：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 258/258
JSON_PARSE: PASS Rules=154
git diff --check: PASS
```

## 5. Automation 证据

| Evidence | Success | Fail | Total | SHA-256 |
|---|---:|---:|---:|---|
| EnemyRouteLoot baseline | 43 | 1 | 44 | `63F62A3D3185A191E659A6903B0E16AB68A99A5326C8C4D94E376CB0D70D1531` |
| EnemyRouteLoot after | 44 | 0 | 44 | `0D0589510E463A2AA5E61FCE478DEC4E2C7C2293671CBC2037004BBE9B3B9477` |
| legacy `demo_map` after | 1323 | 7 | 1330 | `3C852352DC3B8FBE8DEB26B0FC938876C719321FC02726CD11F60F09BDD5688C` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | `12A38A998DFA2AFB464D8ED6FF3C535C7DFDB81D08E7F6B013A0F375FD7EC853` |

四份 Automation 日志都有 queue-empty terminal evidence；Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。legacy 父组 status `0` 不被当作全绿，仍以逐项结果记录剩余 7 项失败。

0.0.10 全量首末 Success 时间为 `20:15:56.984 -> 20:48:01.087`，约 `32m04.10s`，随后出现 `712 tests performed` queue-empty terminal evidence。

流程日志 SHA：gate `1CF9FBEF35DF927278F2C1A95BE3054AB5D759A470CF40780EC72436B260A9C6`；self-test `CF20634D090F333994902E3AFF5A09E3580880E7DDD03B9B7185FD3928122966`；JSON parse `96A857163C1797625DF85180BE8C8F618B2AAFE0480666577320822FEFEE0C56`。

## 6. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 4 / 17.73s | 0 | `49956417B4AD3A1A6228C61FF702746987DF9254AD04E28D7426C04BB4B658F4` |
| Game final | Succeeded | 3 / 32.68s | 0 | `872F57090A915316CE11B1482682B24427AE6351571255EEBEF0D58A44B1B395` |
| Editor final | Succeeded, up to date | 0 / 0.97s | 0 | `A02053D862AFC1F5E3928FDD2307B9DD7626E92CCA518F3141D9BD92ABCD834E` |

最终产物：

- `demo_map.exe`：355,744,256 bytes，SHA-256 `4551C70742EE9A94E773C2DA69BD5F6F3AFD50F7DEFFB5747AB4CE0FB3F89358`；
- `UnrealEditor-demo_map.dll`：14,330,368 bytes，SHA-256 `5C24AA0EC1D1DA056243FF9B027AFF32079312083A9C81256246DBC3BBE49781`。

## 7. 修改范围

- `demo_mapEnemyRouteLootTests.cpp`：用独立 canonical role authority 替换陈旧容量 4；
- `ShanmenRegressionMap.json`：新增 EnemyRouteLoot exact mapping；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 exact mapping 正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 仅包含 P 阶段 legacy test contract 修复、回归门禁、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 剩余失败与下一步

legacy 父组剩余 7 项：P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。

P15.8 可合并审查 P5RuntimeInterface 与 P6 DualLoot 中同源的五装备角色、Definition-backed 空间容量和 slot canonicalization 三项断言，避免逐个修同一模型迁移留下的表象错误。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression/Docs/Report/Dev.D.UE.0.0.10.P15.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression/Docs/Log/Dev.D.UE.0.0.10.P15.7.r0_log.md>
