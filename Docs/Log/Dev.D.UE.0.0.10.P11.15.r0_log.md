# Dev.D.UE.0.0.10.P11.15.r0 Development Log

## 1. 目标

承接 P11.14 的 typed WeaponGuard terminal route，关闭 BasicSword、ThrownWeapon、SpiritEvasion 与 WeaponGuard 各自启动、但没有统一互斥与抢占语义的问题。目标是复用现有 Product Host 与 Combat Run identity，建立一条 typed、确定性、fail-closed 的动作通道，而不是在 PlayerController 或 GameMode 新建产品状态机。

## 2. 基线审计

基线存在四条独立产品路径：

- BasicSword 直接由 GameMode 调用 CombatRunCoordinator sweep；
- ThrownWeapon 有 direct hotbar intent 与 physical input adapter 两条入口；
- SpiritEvasion 由 ProductRoute capture 后 dispatch 到 Component；
- WeaponGuard 由 ProductSession 持有唯一 active Host。

各 Host 内部生命周期与 authority 已明确，但入口之间没有统一占用规则，因此 Guard、投掷飞行与闪避非终态可能在跨产品层并存。P11.14 已提供 exact HostId 与 typed terminal reason，为只抢占 Guard 提供了安全基础。

## 3. 实现

新增纯值 `demo_mapShanmenPlayerActionArbitration`：

- typed action kind：BasicSword / ThrownWeapon / SpiritEvasion / WeaponGuard；
- read-only occupancy snapshot；
- deterministic arbitration receipt；
- exact WeaponGuard preemption gate proof；
- pure policy matrix。

CombatRunCoordinator 新增独立 arbitration sequence，只签发 identity，不保存 product occupancy。GameMode 读取现有 Session/Lifecycle/Component，调用 Coordinator，并在需要时通过唯一 Guard terminal route 发送 `PlayerActionPreempted`。只有 exact HostId interruption + Session empty 才形成 authorized gate。

四条产品入口均携带 ActionGate：

- BasicSword：装备/offense preflight 后 gate；
- SpiritEvasion：ProductStart capture 后、command dispatch 前 gate；
- Thrown physical input：完整 input/intent capture 后、ordinal commit 前 gate；
- direct Thrown hotbar：结构与 Run preflight 后 gate；
- WeaponGuard：item authority preflight 后、Session start 前 gate。

默认 arbitration receipt 与 gate 是 invalid sentinel；真实 rejection 必须显式携带 typed error。该设计既允许旧隔离层单元测试没有 gate evidence，也不会把缺失证据伪装成授权。

## 4. 测试

最终 Automation：

| Group | Success | Fail | Queue | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 3 | 0 | 1 | `BE0E785F9988CFF3A577741D4F1DB29B3C42E044CA8C002D001894C46A1FFE5A` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 1 | `5C2D0D633EAA9EC0CBC7C005C46E125F66ECCE31CA8E4AEAAEB4368842D07456` |
| `Shanmen.0_0_10.Product.SpiritEvasionProductRoute` | 7 | 0 | 1 | `E9E463407160639BC6D8916296B33C162845225E25ABB6C49A4243810E0BC751` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` | 4 | 0 | 1 | `FD8C06B5F6C03DC74C8A44BE0EED66FCFE49E3DCF7AA941320CABB6C485758FA` |
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 7 | 0 | 1 | `D8CC26C7D8EAD24DA5AF582C9E2B1A50D7047CAE448DDDDF2B18C99CCE9665E0` |
| `Shanmen.0_0_10.Product.WeaponGuardImpactRoute` | 3 | 0 | 1 | `E92FF420759E75A39CA6C875B4E6C807F37AC6B3B7798611D2AE617509F8AF8D` |
| `Shanmen.0_0_10` | 540 | 0 | 1 | `B64B08577C7F3785457CBFC893574C0AF4CFDA16D38F4B389A7FD53D4F2CACCB` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `11EB930F8D26639E5F905656DFF809A3E5B62DF5263600E970890ABB600979BB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 1 | `DF8BC1228998BC61C156544244D33933A76C45E1EF70968D00495D7E46D79506` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 1 | `6A1C95D0E6CB09327C4A43816CFF3C210EF3AA4444BE51263023B2A112DC4BA2` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `541D2399D5267CF967F3C8C230961293F851AB881D39E32D14EB815702BF4AF8` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `3A4376D781DDBCDDF61A4F94E1D66C1D5F5FFF95C292BFA07F693306F1EAC576` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `0329EEBA17BE465DD798A2E18212F5DBFFE85DF912CE347EC3CB315DC6763899` |

原始 Success 合计 727、Fail 0；focused 41 项包含于 full 540，按 test identity 去重为 686。

## 5. 门禁

```text
REGRESSION_MAP_JSON: PASS Rules=103
SELF_TEST: PASS 166/166
REGRESSION_COVERAGE: PASS Changed=19 Rules=7 Required=43 Logs=13
ADDED_AUTHORITY_SCAN: PASS AddedSourceLines=492 ForbiddenHits=0
DIRECT_ARBITRATION_POLICY_PRODUCTION_CALLERS: 1
git diff --check: PASS (native exit 0)
```

`PlayerActionArbitration` mapping rule要求 focused policy + Coordinator + broad 0.0.10。Self-test 同时验证完整证据通过、只有 focused 日志时必须失败。

## 6. 构建与异常

- Editor implementation after compatibility fix：79/79，197.38s，原生退出 0；
- Game implementation：78/78，210.97s，原生退出 0；
- Editor final：4/4，9.77s，原生退出 0，日志 SHA-256 `BB79F5C3E613044179283C8775E09DAF406F7CD84E9355940BB34C5E345968C8`；
- Game final：3/3，11.31s，原生退出 0，日志 SHA-256 `4B28E96DD4DE53BADCC2BEA08C56AE682B9B0E13249ECB04BBDDB0B6CC794C35`。

首次 full suite 为 530 Success / 10 Fail、TEST COMPLETE `-1`、进程原生退出 `255`。根因是默认可选 gate receipt 被误判为有效拒绝；改成 invalid sentinel 后 Guard focused 与 full suite 全部恢复。首败日志 SHA-256：`451EBFB66BDD31E0D36689B440A126B6ED9620EA1211258D8E5B095313824DCA`。

首次 evidence gate 还拒绝了带历史 `;Quit` 的日志，因为最短 suite 缺 terminal marker。没有放宽 gate；去掉 `;Quit` 后完整重跑 13 组，最终 changed-file gate 通过。

## 7. 边界

本阶段未修改 Impact、damage formula、vitality、inventory 或 Product Host authority。Coordinator 只拥有 command identity；GameMode 只做占用投影与 exact Guard preemption；PlayerController 未增加产品状态。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
