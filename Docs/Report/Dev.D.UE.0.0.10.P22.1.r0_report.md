# Dev.D.UE.0.0.10.P22.1.r0 Report

## 1. 结论

P22.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮把标准投掷物的命中轮廓从半径 `12 cm` 的球体改为与 P22.0 可见飞刀一致的 `30 × 4.5 × 1.2 cm` 盒体，并把接触通知接到既有 `UProjectileMovementComponent::OnProjectileStop` 终止事件。玩家看到的刀身、真实扫掠轮廓和飞行方向现在由同一 Actor、同一尺寸常量和同一运动组件共同约束。

```text
Thrown-weapon World focused final:         6 Success / 0 Fail
Changed-file mapped regression:          266 Success / 0 Fail (7 healthy logs)
Changed-file regression coverage:        PASS (4 files / 2 rules / 7 required groups)
Regression gate self-test:               PASS 437/437
Game + Editor Development:               PASS (Game 117 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明碰撞几何、真实扫掠、运动停止和接触事件闭合，不宣称最终美术、地图手感或玩家体验已人工验收。

## 2. 玩家可见结果

P22.0 的细长飞刀不再由一个远宽于刀身、又短于刀尖的球体判定命中：

- 飞刀完整尺寸为 `30 × 4.5 × 1.2 cm`；
- 根碰撞盒半尺寸为 `15 × 2.25 × 0.6 cm`；
- 偏离刀身宽度的窄障碍不会误触；
- 位于飞行正前方的障碍会在刀尖到达时阻挡运动；
- 阻挡会产生一次既有 `OnContact` 事件，并携带实际障碍 Actor；
- 可见网格继续禁用碰撞，不会成为第二命中来源。

## 3. 单一事实链

本轮没有增加第二套命中、运动或结算权威：

```text
ThrownWeaponPrototypeFullSize
  -> root UBoxComponent extent
  -> collisionless Engine Cube visual scale

durably published launch
  -> existing UProjectileMovementComponent swept movement
  -> OnProjectileStop(actual FHitResult)
  -> existing native OnContact seam
  -> existing Run Host / World Adapter impact route
```

Actor 仍不选择目标、不修改库存、不结算伤害；它只携带既有发射事实并报告世界接触。

## 4. 碰撞与生命周期

旧球体沿侧向延伸 `12 cm`，而可见刀身半宽只有 `2.25 cm`，容易对侧面空隙误判；沿前向又只延伸 `12 cm`，比 `15 cm` 刀尖短 `3 cm`。新盒体直接由与视觉缩放相同的 `ThrownWeaponPrototypeFullSize` 生成，消除了两套硬编码尺寸漂移。

根组件继续使用 `QueryOnly` 扫掠。旧 `OnComponentHit`/刚体通知被移除，接触改由无反弹投射物的标准终止事件 `OnProjectileStop` 驱动。回调在 `PostInitializeComponents()` 中绑定到实际 Actor 实例；状态、无效 Actor、自身和来源 Actor 过滤仍然失败关闭。

## 5. 真实测试证明

新增 `CollisionProfileSweep` 使用带物理场景和 Trace Collision 的 `GamePreview` World，并进入 Actor 初始化阶段：

- 在 `X=8 cm`、前方 `Y=100 cm` 放置半宽 `1 cm` 的窄障碍；新刀身扫掠可无阻通过，而旧半径 `12 cm` 球体会误触；
- 把同一障碍对齐到 `X=0` 后，真实盒体 Sweep 返回该障碍和其根组件；
- 将标准运动组件以 `750 cm/s` 推进 `0.2 s`，验证 Actor 在障碍前停止；
- 验证 `OnProjectileStop` 的绑定对象是当前飞刀实例；
- 验证既有原生 `OnContact` 恰好触发一次并识别正确障碍。

专项组由 5 项增至 6 项，最终为 `6/0`。

## 6. 首次失败与根因修正

首次 Editor 构建同时暴露两类问题：默认并行编译达到 Windows 提交内存/PCH 上限，以及 Run Host 仍引用旧 `USphereComponent` 头导致 `UBoxComponent` 不完整类型错误。后续构建固定为最多 4 路并行，并修正精确组件头；未把环境失败描述成源码失败。

初版测试还证明：在未进入 Play 生命周期的临时 World 中，直接 `MoveComponent` 虽可返回阻挡 `FHitResult`，却不能证明 Actor 动态回调已执行。源码核查确认 UE 5.8 的 `AActor::ProcessEvent` 会在 `AreActorsInitialized()==false` 时跳过该回调。最终测试没有放宽断言，而是初始化测试 World 的 Actor 生命周期，并通过标准投射物停止事件验证真实接触。所有失败日志均保留。

一次广域诊断运行达到 `720 Success / 0 Fail` 后被主动停止；它没有自然完成，因此不计作全量通过。按项目已采用的“改动文件推导必跑组”规则，最终改跑 7 份独立健康日志，合计 `266 Success / 0 Fail`。

## 7. 自动化与覆盖门

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| Focused final | 6/0 | 263,213 | `33454346DD2A04EAE1719DB99A2A6FFDB8713F2753EBAFB2F8E9E133A2213294` |
| CombatRuntime | 146/0 | 407,588 | `187529D853B51FD3209CDF13B6E6B2AA6581F7A67154449D49DAC6AC0A229355` |
| Items | 77/0 | 347,779 | `672256647E2E2279B9A7FBBA9EF3509EE73A979D98C04A9B32B3851CB161B8C3` |
| CombatRunCoordinator | 18/0 | 283,949 | `60A19F1267DBB43424E83555FFD1ED38A44AE2168725CDD4C6415CCC2CB2A052` |
| ThrownWeaponItemAdapter | 5/0 | 262,154 | `0110A64FD43D1F695519A9D4F9C667827F91E1521EF76759F07E7B89AB37CA64` |
| ThrownWeaponRunHost | 4/0 | 260,954 | `B4E4997B9DD66A5E0BD1AFDF615FB1C32CD721D5C580C7A03FB36EF5C74F14B3` |
| ThrownWeaponWorldDelivery | 6/0 | 264,019 | `70B3819A82BB86869A2F4C5415835DA6574FF0AC76928D4103A2213CCF79D2A5` |
| WorldGameplay | 10/0 | 266,304 | `AE610FE31B496CC54D7106573488BA1D3B37E2C6B1CB04C5BF52FC515B7E2DBA` |

覆盖门结果为 `PASS Changed=4 Rules=2 Required=7 Logs=7`，SHA-256 `8088093B18D72039D328829A967A3BDFB76B30283C653B4265CB9323547167A8`。门禁自测为 `PASS 437/437`，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 117 / 207.61s | `BF9954AC686F380794C06BE920AE368B6DB3FBFC90EFB34050F69FF472D0FC7F` |
| Editor Development final | Succeeded / native 0 | 0 / 1.14s | `96B5D7AC66040D299E65ED23443EB6B92477514E4F09206E79AA257F0EC9032D` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,545,344 bytes；SHA-256 `7B439A2A0DF5DD5BDAA84E4611F27ED8787425B8B26903CD0390344DC179C63C`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,739,712 bytes；SHA-256 `D656069202CAB5A0D9AF1312D9657D013D58FFEEBC4F1E4E954767B584793005`。

实现/测试 diff 为 4 files / 270 insertions / 24 deletions。新增生产行共 26 行，其中 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor` 与 `Destroy` 均为 0；`git diff --check` 原生退出码 0；验证后相关运行进程数为 0。

## 9. P/F 边界

PASS：可见飞刀与碰撞盒共享尺寸权威；侧向窄障碍不误触；正面真实扫掠会停止标准运动并发布一次接触；现有资源、运行、命中和生命值链未复制；专项、映射回归、覆盖门、自测及双目标构建全部通过。

未声明：Cube 已替换为最终飞刀资产；真实地图中的动画、材质、碰撞容差、网络同步、镜头可读性或手感已人工验收。这些内容需要后续明确阶段与实际 UI/PIE 授权。

## 10. 提交边界与 GitHub

本阶段只提交 4 个源/测试文件、本 Report 与本 Development Log，共 6 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P22.1` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-1-thrown-weapon-collision-profile>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-1-thrown-weapon-collision-profile/Docs/Report/Dev.D.UE.0.0.10.P22.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-1-thrown-weapon-collision-profile/Docs/Log/Dev.D.UE.0.0.10.P22.1.r0_log.md>
