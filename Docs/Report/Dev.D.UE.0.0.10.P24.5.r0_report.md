# Dev.D.UE.0.0.10.P24.5.r0 Report

## 1. 结论

P24.5 已完成剑气发射路径的真实 World 阻挡门禁。现有剑气载体在发布飞行前会检查：

- 发射点的完整碰撞球体是否已被阻挡；
- 玩家源点到发射点之间的整段球形走廊是否被阻挡；
- 发射源与载体是否处于同一 World。

墙内生成或隔墙发射均以明确的 `LaunchPathBlocked` 失败关闭。失败不会修改活动动作，不会开启碰撞、移动、能量刃或光源；障碍移除后，同一未发布请求可正常暂存。

## 2. 玩家价值

P24.4 让剑气成为可见的真实飞行载体；P24.5 补齐其生成边界。玩家不再得到“剑气已经成功发射，但载体从墙体内部出现或穿过近身障碍”的错误反馈。

该行为复用现有 Sword Qi ProductController → Session → RunHost → WorldAdapter → Projectile 链路，没有创建第二套剑气运行时。

## 3. 实现边界

`Ademo_mapShanmenSwordQiProjectile::TryStageLaunch()` 现在返回细分的暂存错误：

- `None`：暂存成功或精确暂存重放；
- `ContractRejected`：身份、World 或载体契约不成立；
- `LaunchPathBlocked`：发射体积或完整走廊被阻挡。

真实 World 中使用现有 `SwordQiCollision` 的实际缩放半径建立查询形状，忽略发射者和载体自身：

1. 在精确发射点执行 blocking overlap；
2. 从发射者位置到发射点执行 blocking sweep；
3. 任一命中即在任何载体状态写入前返回失败。

纯值/无 World 合同测试仍可运行；实际产品路径新增同 World 前置条件，不能利用无 World 分支绕过物理门禁。

## 4. 权威与原子性

WorldAdapter 继续采用复制后发布：临时 Execution 可以生成发射证据，但只有载体成功暂存后才允许发布到活动 Execution。

路径受阻时：

- live Execution 保持 `Ready`；
- emission 保持关闭；
- Projectile 保持 `Empty`；
- LaunchReceipt 与 HitContext 保持无效；
- Collision 保持 `NoCollision`；
- Movement 保持 inactive；
- 能量刃与飞行光源保持隐藏。

外层 `Edemo_mapShanmenSwordQiLaunchError` 保留精确 `LaunchPathBlocked`，RunHost 与上层产品结果可以审计真实阻塞原因，不把它折叠为普通暂存失败。

## 5. 真实碰撞世界验证

新增 `SwordQiWorldDelivery.LaunchCorridorGate`，在启用 physics scene 与 trace collision 的临时 World 中使用真实 `UBoxComponent` 障碍，验证三种状态：

| 场景 | 预期 | 结果 |
|---|---|---|
| 障碍占用发射点 | 发布前拒绝 | PASS |
| 发射点清空，但源点与发射点中间存在薄墙 | 整段走廊拒绝 | PASS |
| 将同一障碍移出路径 | 同一请求可暂存并取消 | PASS |

聚焦 WorldDelivery 为 4/0；完整 Sword Qi 家族为 39/0。

## 6. 改动文件驱动回归

5 个生产/测试改动路径命中 `SwordQiWorldDelivery` 映射规则，要求 5 个测试组。全部使用各自独立的原始日志完成：

| Required group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiWorldDelivery` | 4 | 0 | `DE033DEC43625EED53CE4D0C60750882957C4D372C35E6DAB126AF9F91B90BD8` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18 | 0 | `3F38CF1A9A6D28DE151387DA734FF51DCB67639FC661234A44752DD075BEE224` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `E091AAC7EBD79A593CA2870737838214B584E7950B7245431D61ACDDF8298983` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 0 | `E06713F7E5AE89F57612569F43B3D6E61D5CDDEDED64B3849A78243DE8E8827E` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `0C16EC88BAAD86E032520F603560A4723C2CE6EA60D034F2D362654B3380620A` |

映射回归合计 187/0。覆盖门禁结果为：

`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=5 Logs=5`

覆盖日志 SHA-256 为 `3E3B36FEC3A52F293C0DE5CAB21E5FCD24A6368C5E7669255C4CBCB782876EBD`；门禁自检为 442/442，SHA-256 为 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 构建证据

| Evidence | Result | Actions | SHA-256 |
|---|---|---:|---|
| `P24.5_EditorBuild_initial.log` | Succeeded / native 0 | 62 | `D3BEE15462973A383EC0EABEBC1D5A52EF6A17FF02BBFC3FDCEA2E41B611C08E` |
| `P24.5_EditorBuild_final.log` | Succeeded / native 0 | 0 / up to date | `BE6DA44C06319AF37C98FD49B71967D1A0A8C25F30489137997C39E4224A6CF5` |
| `P24.5_GameBuild_final.log` | Succeeded / native 0 | 61 | `29E73F77F93F48F1E9960D61621A43E7C68C21F1BFBA2BAD648A0CC8153EF1AC` |

最终二进制：

- `demo_map.exe`：359,705,600 bytes，SHA-256 `A6D4209A2D1D1F831A810DD45A88C49F7CC877CAD6325F2E554ECB205AA4D0CB`；
- `UnrealEditor-demo_map.dll`：18,971,648 bytes，SHA-256 `FF9BA29191E900924C02D1A521B98AD7D07A9A3ADE3AB1702846677B16799B47`。

## 8. 静态边界

- 非文档增量：5 files，`+309/-8`；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、`SpawnActor`、`Destroy`、Manager、Subsystem：0 命中；
- 没有新增 UCLASS/USTRUCT、Actor、输入动作、存档字段、Config 或 Content 资产；
- 测试专用 World 与 Actor 只存在于自动化 fixture，不进入产品运行时；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：真实碰撞查询能拒绝发射点占用与中间薄墙；失败保持动作和载体原子性；清路后可恢复；Sword Qi 家族 39/0；改动映射 187/0；门禁、自检和双目标构建通过。

未声明：真实关卡中所有墙体材质、复杂碰撞、极端坡面、移动障碍、网络延迟、镜头观感或玩家手感已验收。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub 交接

基线提交：`750875df516e1803ce0c15addf900864fd0a1323`。

工作分支：`agent/0.0.10-p24-5-sword-qi-launch-clearance`。

本阶段仅提交 4 个生产文件、1 个测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.5` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-5-sword-qi-launch-clearance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-5-sword-qi-launch-clearance/Docs/Report/Dev.D.UE.0.0.10.P24.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-5-sword-qi-launch-clearance/Docs/Log/Dev.D.UE.0.0.10.P24.5.r0_log.md>
