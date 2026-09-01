# Dev.D.UE.0.0.10.P15.7.r0 Development Log

## 1. 目标

收敛 legacy `EnemyRouteLoot.42.EnhancedCorpseCapacity` 的陈旧固定容量断言，并给该 fixture 建立精确的改动—回归映射。

## 2. 基线与根因

- focused baseline：`43 Success / 1 Fail / 44 Total`，SHA `63F62A3D3185A191E659A6903B0E16AB68A99A5326C8C4D94E376CB0D70D1531`；
- `GetEquipmentSlotIds()` 与 `CorpseEquipmentCapacity` 从 foundation baseline 起都为 5；
- legacy case 42 同期硬编码容量 4，与 Weapon、Armor、Accessory、SpatialRing、Backpack 五角色权威冲突；
- Body capacity 2 仍正确。

## 3. 实现

- Equipment section capacity 改为与独立 canonical equipment-role count 比较；
- Body section 继续独立断言为 2；
- 新增 `demo_mapEnemyRouteLootTests.cpp -> demo_map.EnemyRouteLoot` exact mapping；
- 增加该映射的成功覆盖与“全量日志不能替代 exact legacy suite”失败自检；
- production C++ 零修改。

## 4. 验证

```text
EnemyRouteLoot: 43/1 -> 44/0
legacy demo_map: 1322/8 -> 1323/7, Total=1330
Shanmen.0_0_10: 712/0
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 258/258
JSON_PARSE: PASS Rules=154
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| focused baseline | 43 Success / 1 Fail | `63F62A3D3185A191E659A6903B0E16AB68A99A5326C8C4D94E376CB0D70D1531` |
| focused after | 44 Success / 0 Fail | `0D0589510E463A2AA5E61FCE478DEC4E2C7C2293671CBC2037004BBE9B3B9477` |
| legacy after | 1323 Success / 7 Fail | `3C852352DC3B8FBE8DEB26B0FC938876C719321FC02726CD11F60F09BDD5688C` |
| full 0.0.10 | 712 Success / 0 Fail | `12A38A998DFA2AFB464D8ED6FF3C535C7DFDB81D08E7F6B013A0F375FD7EC853` |

流程日志 SHA：gate `1CF9FBEF35DF927278F2C1A95BE3054AB5D759A470CF40780EC72436B260A9C6`；self-test `CF20634D090F333994902E3AFF5A09E3580880E7DDD03B9B7185FD3928122966`；JSON parse `96A857163C1797625DF85180BE8C8F618B2AAFE0480666577320822FEFEE0C56`。

## 5. 构建

- initial Editor：4 actions / 17.73s / status 0 / SHA `49956417B4AD3A1A6228C61FF702746987DF9254AD04E28D7426C04BB4B658F4`；
- final Game：3 actions / 32.68s / status 0 / SHA `872F57090A915316CE11B1482682B24427AE6351571255EEBEF0D58A44B1B395`；
- final Editor：up to date / 0 actions / 0.97s / status 0 / SHA `A02053D862AFC1F5E3928FDD2307B9DD7626E92CCA518F3141D9BD92ABCD834E`；
- Game EXE：355,744,256 bytes / SHA `4551C70742EE9A94E773C2DA69BD5F6F3AFD50F7DEFFB5747AB4CE0FB3F89358`；
- Editor DLL：14,330,368 bytes / SHA `5C24AA0EC1D1DA056243FF9B027AFF32079312083A9C81256246DBC3BBE49781`。

## 6. 剩余失败与范围

legacy 父组剩余 7 项：P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。它们未被跳过或归入本阶段。

本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation、静态检查与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression/Docs/Report/Dev.D.UE.0.0.10.P15.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-7-enemy-route-loot-capacity-regression/Docs/Log/Dev.D.UE.0.0.10.P15.7.r0_log.md>
