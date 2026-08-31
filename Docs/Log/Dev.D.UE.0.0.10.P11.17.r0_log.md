# Dev.D.UE.0.0.10.P11.17.r0 Development Log

## 1. 目标

关闭 P11.16 玩家动作通道在 WeaponGuard 抢占后的时间检查缺口：不能只证明 exact Guard Host 已终止，还必须重新读取所有现有 Product Host，证明终止回调完成后的动作通道仍然有效且为空，才能放行原请求。

## 2. 产品边界审计

审计了 ControlledWeapon、Formation 与 SpiritShield 的现有语义，没有把它们伪装成新的独占动作 claim：

- 飞剑 Orbit 是攻击、近身威胁、防御与发射的共同准备姿态，应与玩家即时动作共存；
- 已部署阵法是持续 World 影响，不应在其整个生命周期占用玩家动作通道；
- 已启动 SpiritShield 是短时防御产品，也不等同于持续锁死全部攻击与移动；
- 当前三者均没有一条已经落地、需要独占通道的第五种物理输入 start route。

因此本阶段不增加虚构 product kind、全局 registry 或持久 occupancy 状态，只修复已有 exact Guard preemption 的真实原子性缺口。

## 3. 实现

`FromGuardPreemption` 现在必须接收 Guard 终止后的第二份只读 occupancy projection。GameMode 在 exact terminal receipt、HostId 一致和 Guard Session 清空之后，立即再次调用现有 `CapturePlayerActionOccupancy()`。

新增 typed post-preemption observation：`NotObserved / Empty / Occupied / Invalid`。只有 `Empty` 能形成 `WeaponGuardPreempted` 授权；结构损坏返回 `PostPreemptionProjectionInvalid`，出现任何新 owner 返回 `PostPreemptionLaneOccupied`。两种拒绝都保留已成功退休的 Guard HostId，避免把“Guard 已退场”与“后续动作未获授权”混为一谈。

## 4. 测试

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 4 | 0 | `DEBA349B4F22F1A1BFC0E11A56ABBA4B23474DF3131D4552879F5CC51272E61F` |
| `Shanmen.0_0_10` | 541 | 0 | `AF2CBF504BAFA84F7B667B2BCC8A3E672204256221DDBC6CFAB7F5C91E322FE3` |
| `demo_map.V3.Attributes` | 4 | 0 | `F6647A2BB681D592EEDE8944E63F83B350F0FC4B2AB00796C7F2D33FA095D5F1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `AA1D3BF0FCE6F16A6BBE3907CB408519D354108FF58BFF0070C389A76683AC64` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `ED02D73ACCD529DE096B2012DAC79A3468BCB576E477901E5163BF2E5C3E36BE` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `8D2EA3BDB62B0C65F2AB3335F85A4B97FEF4AD6A192569B8AF94F2876D62207A` |

原始 Success 合计 661、Fail 0；focused 4 项包含于 full 541，按 test identity 去重为 657。

## 5. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=40 Logs=6
SELF_TEST: PASS 166/166
STATIC_REVIEW: PASS AddedSourceLines=124 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

coverage 日志 SHA-256：`98E338B395B01ECAFF464943DA73988F0F80775316578233AB941FC07EAB45BF`；self-test 日志 SHA-256：`15F7374CDBCDF3970528D2BBE94056B9F6B37342044CB23412F0F9ED8C275B01`。

## 6. 构建

- 首次 Editor：原生退出 `1`，UBT `OtherCompilationError`。测试夹具误用了不存在的 `ArbitrationThrownHost`；失败日志 SHA-256 `95622D14B4E7DD8E5A01CB4B774E982D7C8ABA0801C6974EEA4B45E450235765`；
- 修复后 Editor：`4/4`，原生退出 `0`，日志 SHA-256 `577A25C431838CEFD21648BB10D0433A82145016D01477F353E7AD0274E1F176`；
- Game：`78/78`，原生退出 `0`，日志 SHA-256 `1FD20940371E3323B7A51440A54E4933AB00B6378F891E9D1CA803A1EFF96594`。

首次失败是测试源码标识符错误，不是内存、页面文件或 SDK 环境故障。Win64 SDK `10.0.22621.0` 始终为 VALID。

## 7. 边界

未修改任何 Product Host authority、CombatCore、Impact、damage、vitality、inventory 或 schema；没有新增计时器、Tick、随机数、wall clock 或第二套动作状态。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
