# Dev.D.UE.0.0.10.P10.2.r0 Development Log

## 目标

把 P10.1 的 movement request 接到产品侧显式 displacement policy 和只读 WorldStatic preflight，同时拆除通用位移 authority 对旧敌人技能配置的反向依赖；不执行真实移动，不创建 SpiritEnergy 临时权威。

## 审计结论

1. P10.1 已冻结 action、active window、policy id 与 planar direction；
2. `Fdemo_mapCombatDisplacement` 同时服务 enemy skill、knockback 和产品自动化；
3. 通用实现为取得 2 cm clearance 直接 include `demo_mapEnemySkillTypes.h`，形成反向依赖；
4. `PreflightWorldStatic` 有五个产品调用点，`ClampPreflightDistance` 有七个 enemy framework 测试调用；
5. 旧 `MoveCharacterSwept` 是真实 mutation authority，但不适合在没有 duration/trajectory 契约时直接消费 P10.1 request；
6. 因此本阶段只冻结 policy、生成 plan，并做只读 preflight。

## 设计决策

1. policy 必须显式提供 id、requested、minimum 和 clearance；
2. 不在 adapter 内放 gameplay 默认值；
3. policy capture 拒绝非有限、零/负距离、minimum 超限与负 clearance；
4. plan 必须精确绑定 request intent 的 policy id；
5. PlanId 绑定 RequestId、policy id 和 exact float bits；
6. Plan::IsValid 重新派生 identity；
7. preflight 只读 Character/WorldStatic，不调用移动 mutation；
8. minimum 判断集中在 adapter 的纯函数 seam；
9. generic displacement 通过参数接收 clearance；
10. 旧 callers 继续传旧 config 的 WorldStaticSkin，保持行为兼容；
11. changed-file mapping 同时覆盖新 adapter、generic displacement 和 enemy runtime；
12. V3 manager 改动触发 Profile、CodeB 与 V2 回归。

## 执行序列

1. 审查 P10.1、旧 P6 displacement、五个 preflight caller 与七个 clamp test。
2. 修改 generic API，移除 enemy skill include，并增加输入失败关闭。
3. 更新 enemy runtime、V3 manager 和 enemy framework tests 的显式 clearance。
4. 新增 policy capture/snapshot、immutable plan、deterministic PlanId。
5. 新增 preflight status/result 与只读 adapter。
6. 新增五个 focused automation tests。
7. regression map 增至 79 rules；self-test 增至 118 cases。
8. Editor candidate 13 actions，原生退出 0。
9. focused candidate `5/5`，未发生源码修正轮。
10. 串行执行七组正式 Automation，共 `742` success、`0` fail。
11. gate 首轮发现 EnemySkillRuntimeComponent 未映射；补映射与 self-test。
12. self-test `118/118`，changed-file gate `10/4/7/7` 通过。
13. adapter mutation/resource 边界扫描 0 命中，generic enemy-config 扫描 0 命中。
14. `git diff --check` 通过，实现暂存区限定为 10 个文件、`+687/-22`。
15. Editor final 0 actions，Game final 10 actions，原生退出均为 0。
16. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
P10.1 MovementRequest
  + product-owned PolicyCapture
      -> finite/range validation
      -> immutable PolicySnapshot
      -> exact policy-id binding
      -> deterministic MovementPlan / PlanId

MovementPlan + Character
  -> read-only WorldStatic capsule sweep
  -> explicit clearance clamp
  -> resolved distance >= minimum ?
       yes -> Ready receipt
       no  -> InsufficientResolvedDistance

No MoveCharacterSwept / no trajectory / no SpiritEnergy mutation
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Product SpiritEvasionMovementAdapter | 5 | 0 | `FD821061D3F29427E01D5A36F0BF177AC840D9478FE485412E05366BE7EC3031` |
| CombatRuntime SpiritEvasionMovement | 5 | 0 | `C24A2B439B3305E37C1590C5995B7260D91C28DC5976315D99A65B2779DF8E78` |
| EnemySkillFramework | 44 | 0 | `8313BF4788A1F3D59C9B4E2651E2CEEAF63D4CBA0C58E45DC98CAB24833F238D` |
| V2RangedCompatibility | 22 | 0 | `CD1B6026D197B4DCAE2C75293E984AF77BBDC7AEE5160F812BE60358131A1144` |
| Profile | 211 | 0 | `B702BE5CCD604C9837E2CAB1606BAE0E4E9F19D640D2DF6FBDBE8CA52E170FAA` |
| CodeB | 60 | 0 | `AF0E9E9B0746631E75A820680A840EEA8977F41CC563248A86217AB069835426` |
| Shanmen.0_0_10 | 395 | 0 | `C97CC378907418088AACF03FE0D322694F67467BB82847311661D17A649A4A8D` |

所有正式进程原生退出码为 `0`，日志均有 selected queue-empty，且没有 selected fail、fatal、unhandled 或 ensure。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=79
SELF_TEST: PASS 118/118
REGRESSION_COVERAGE: PASS Changed=10 Rules=4 Required=7 Logs=7
git diff --check: PASS
ADAPTER_FORBIDDEN_HITS=0
GENERIC_ENEMY_SKILL_HITS=0
Editor candidate: 13 actions / 62.73s / exit 0
Editor final: 0 actions / 0.91s / exit 0
Game final: 10 actions / 46.41s / exit 0
```

## 真实异常

- Windows PowerShell 5.1 无法解析现有 PowerShell 7 脚本管线；切换到 `pwsh` 后 `118/118` 通过。
- changed-file gate 首轮发现 `demo_mapEnemySkillRuntimeComponent.cpp` 未映射；扩展 `SkillAndEnemyCombat` 规则并加入 self-test 后通过。
- 两项均为流程/门禁修正；没有源码、Automation 或构建失败。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.3 设计基于 plan/preflight receipt 的持续时间与逐帧位移执行 authority；SpiritEnergy 继续等待真实余额、revision、恢复与持久化 owner，不在 movement adapter 中临时创建。
