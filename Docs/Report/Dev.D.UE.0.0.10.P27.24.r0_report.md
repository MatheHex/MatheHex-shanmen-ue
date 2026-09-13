# Dev.D.UE.0.0.10.P27.24.r0 Report

## 1. 结论

P27.24 已在 P27.23 整批 World Publication 之上建立产品持有的 `FormationScatterWorldPublicationSession`。一个有效 Session 绑定一份精确的 P27.22 Handoff Evidence、一个具体 Actor 类和一个 `UWorld`，并持有该批次的 P27.23 Publication Ledger、Completion Evidence、既有 Formation World Adapter 与终止清理回执。

首次发布、部分发布恢复和完整重放继续委托给 P27.23 Publisher；Actor 生成、标签接管、冲突检测与销毁继续只由既有 World Adapter 完成。Session 不复制第二套 World 真值，也不回滚 P27.20 已消耗资源或 P27.21 已提交 Deployment。

本阶段同时补齐显式终止生命周期：`Cancelled` 或 `Ended` 可以触发整批 Actor 清理；同一终态重放返回稳定回执；不同终态、跨 World 操作、无效启动和终态后再次发布均失败关闭。Host 重建可通过同一确定性 Session ID 和 Deployment 标签接管既有 Actor，也可在本地 Ledger 丢失后直接恢复终止清理。

专项测试 4/4，完整 `Shanmen.0_0_10` 根组 1,400/1,400，改动驱动的 22 个证据组全部健康，映射自检 500/500，Editor 与 Game 双目标构建成功，最终覆盖门通过。

## 2. 阶段问题与范围

P27.23 能把完整 Handoff 按顺序发布到真实 World，并形成可恢复账本，但调用方仍需自行保存 Ledger、World Adapter 和 Completion Evidence；产品生命周期也没有统一的 World 所有权与终止清理入口。P27.24 闭合以下缺口：

- 同一批次的 Handoff、Actor 类与 Session 身份必须确定性绑定；
- 第一次有效 World 操作后不能迁移到另一个 World；
- Session 必须拥有并复用 P27.23 Ledger 与 Completion Evidence；
- 已完整发布的批次必须允许精确重放，不重复生成 Actor；
- 重建 Session 必须能接管 Deployment 标签标识的既有 Actor；
- 即使本地 Publication Ledger 丢失，也必须能按权威 Handoff 执行终止清理；
- 终止原因只能是 `Cancelled` 或 `Ended`，一旦完成不能改写；
- 清理回执必须绑定 Deployment、终态、规范已提交阵眼数和实际移除 Actor 数；
- 旧 Formation ProductSession 与新整批 Session 必须共享同一清理实现；
- 不在本阶段接入玩家输入、正式地图、UI、全局 Session Registry 或持久化 Host。

## 3. 产品持有的 Publication Session

新增 `Fdemo_mapShanmenFormationScatterWorldPublicationSession`，状态为：

- `Empty`：无有效会话；
- `Ready`：已绑定权威 Handoff 与 Actor 类，尚无成功发布记录；
- `Publishing`：Ledger 保存一个规范成功前缀；
- `Published`：Ledger 与完整 Completion Evidence 均有效；
- `Cancelled` / `Ended`：World 清理完成并持有稳定 Teardown Receipt。

Session ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationSession.r1`，绑定 P27.22 Evidence ID、P27.21 Deployment ID 与具体 Actor Class Path。重建相同输入会得到相同 Session ID；外来 Evidence 或 Actor 类得到不同身份。

这一级身份不宣称已经存在全局注册中心。当前保证的是单个 Session 对象内部的确定性绑定和只前进状态；把它接入唯一玩家 Product Host、处理同时存活的调用方，是下一阶段的产品路由工作。

## 4. 发布、恢复与重放

`TryPublish` 在接触 World 前验证 Session、终态、World 有效性和既有 World 绑定。随后构造具体 P27.23 Adapter Publication Port，并把权威 Handoff 和 Session 自有 Ledger 交给既有 Publisher。

- 首次完整成功映射为 `Published`；
- 从规范成功前缀补齐映射为 `Recovered`；
- 完整账本精确重放映射为 `Replayed`；
- 失败前没有记录时回到 `Ready`；
- 失败后存在规范前缀时保持 `Publishing`；
- 已经完整发布的会话若重放被 World 拒绝，保留此前有效 Completion Evidence，不伪造回滚。

World Adapter 仍是 Actor 层唯一执行缝：Publisher 只处理顺序、回执和 Ledger；Session 只处理所有权与生命周期。重建会话向同一 World 重放时，Adapter 按稳定 Deployment/Placement 标签接管既有 Actor，Actor 指针集合、Actor 数和发布账本均不增长。

## 5. 终止清理与恢复

既有 `Fdemo_mapShanmenFormationWorldAdapter` 新增通用、不可变值请求 `Fdemo_mapShanmenFormationWorldTeardownRequest`。请求只接受：有效 Deployment ID、`Cancelled` 或 `Ended`、非负规范已提交阵眼数。

原 `TryTeardownTerminal(ProductSession)` 保留旧产品审计，然后委托给新的 `TryTeardownDeployment`。因此旧单阵眼产品流程和新整批发布流程共享以下实际实现：

1. 验证 World、Deployment 与 Adapter 所有权；
2. 合并 Adapter 已记录 Actor 与 World 中带稳定 Deployment 标签的 Actor；
3. 只在既有 Adapter 中执行销毁；
4. 验证没有残留 Deployment 标签 Actor；
5. 生成确定性 Teardown Receipt；
6. 同一请求重放返回同一回执；终态或数量漂移返回冲突。

新 Session 从有效 Handoff 推导 Deployment ID 与规范阵眼数。即使重建实例尚无本地 Ledger，也能依靠 World 标签移除既有批次；该恢复只清理 World 表现，不修改资源 Authority 或 Deployment 的 `Active` 事实。

## 6. P27.24 专项证明

四条真实无头瞬态 World/Actor 自动化覆盖：

1. `PublishReplayAndTerminalTeardown`：发布 2 个具体 `ACharacter`，完整重放仍为 2 个；`Ended` 清理移除 2 个，重复终止返回同一回执；物品 Authority 快照、Deployment 状态和 Receipt 数不变。
2. `ReconstructionAdoptionAndDirectTeardown`：相同证据重建得到相同 Session ID；第二个 Adapter 接管原 2 个 Actor 且指针集合不变；第三个无本地 Ledger 的恢复实例可按 Deployment 标签执行 `Cancelled` 清理。
3. `WorldAndTerminalConflicts`：已绑定 World A 后，向 World B 发布或清理均被拒绝；`Ended` 后改为 `Cancelled` 被拒绝；终态后再次发布被拒绝，原回执不变。
4. `InvalidStartWorldAndTerminalState`：缺失 Handoff、缺失 Actor 类、空 World 与非终态清理均失败关闭，合法控制会话保持 `Ready`、未绑定且 Ledger 为空。

最终专项：4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0；日志 SHA-256 `C1C9ACA6F8053BA173C84059CFD6BBEA4552F7C45B6D0836ABF075E24F1220A0`。

## 7. 自动化与回归证据

改动驱动覆盖门要求 22 个唯一测试组。关键结果如下：

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationSession | 4 | 0 | C1C9ACA6F8053BA173C84059CFD6BBEA4552F7C45B6D0836ABF075E24F1220A0 |
| FormationScatterWorldPublication | 8 | 0 | 32099CAFAEC8A368E8052FD229A1766D207AA744BDAAD50038F30C34F66F2E2C |
| FormationScatterWorldPlacementHandoff | 4 | 0 | 71724B34DC0294FEEC71957C9594467A8CB34059D2553595D5A2E5657FAA0B38 |
| FormationWorldDelivery | 4 | 0 | 45ECB5C7559A608F84E5B4FE42BAF4527A4A43F3D2BBCD7B6144F9E9D1F62B70 |
| Items | 77 | 0 | 9618DBCA7620AD23A1064EBD61810B8AFCDBCE1E1FFF36F48101C0DCAF3CBB20 |
| WorldGameplay | 10 | 0 | 3BA33AA16CD8EF84578ADC64D04EB377489FBD392C5C1DE37AA9CD1A6C9D26DB |
| CombatCore | 9 | 0 | 4496AEBFF5D2318B19D3C82B7C95A63889381F7E8D7D7B2CD11301C9DF9E94EF |
| Shanmen.0_0_10 完整根组 | 1,400 | 0 | A9A55E70D1C99ACB7AF8C56281A6AEBF81446000F37F4D4FA7FFE1A0C52897F9 |

`FormationScatterWorldPublication` 的 8 项包含原 P27.23 四项及其命名空间下新增的 P27.24 Session 四项；独立 Session 日志仍精确证明新增四项。其余 Deployment、Resource、Mastery、Formation Session 和 Influence Lifecycle 依赖组的完整逐组结果及哈希见同名 Development Log。

完整根组从 2026-09-13 18:03:41.447 UTC 运行至 19:11:16.240 UTC，持续 1 小时 7 分 34.793 秒；1,400 项全部成功，原生退出 0。日志中的联网可用性探测超时只作为测试事件 Warning 记录，不构成测试失败；没有 Fatal、Unhandled Exception 或 Ensure condition failed。

## 8. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 新增 `FormationScatterWorldPublicationSession` 规则，要求新 Session、P27.23 Publication、P27.22 Handoff、World Delivery、完整资源/Deployment 链、Formation Mastery/Session、Items、WorldGameplay、CombatCore 与完整根组。Adapter 改动同时触发既有三个 Influence Lifecycle 清理消费者组。

- 映射自检：`SELF_TEST: PASS 500/500`，SHA-256 `8114748657B53F0A856AE6814934FC490BFA1A182144E5E248869F56720FCBDA`；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=9 Rules=5 Required=22 Logs=22`，SHA-256 `5AE82AB75058C68A22AFB7B7B81FF3B5045050EC8F0F7BD60E1454609DF83D94`；
- Editor：0 actions，Result Succeeded，原生退出 0，总计 4.12 秒；日志 SHA-256 `B83F4F6D89FFA0A41812A7F5EE4511551B02A2268DB13030B870B2FB68BB247F`；
- `UnrealEditor-demo_map.dll`：20,093,952 bytes，SHA-256 `C70852C327FDAADC88E976F157F0A15DC25A8F1EA4C9ABC96B9DE54C5AFD2C5A`；
- Game：89 actions，Result Succeeded，原生退出 0；UBA 78.68 秒，总计 81.56 秒；日志 SHA-256 `17BD1778E6B0B204B5D3274F03A9589E711D12D73FF63D3160B007BA966276FD`；
- `demo_map.exe`：360,658,432 bytes，SHA-256 `3007A87A22A7C9F8C964F93C3C7819083239146FB2498BDE69C2691A27E2A90E`。

新 Session 头/实现共 477 行、444 个非空行。静态扫描未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG 或直接 Spawn/Destroy Actor；Adapter 新增行也未增加这些边界。Regression Map JSON、`git diff --check` 和最终暂存差异检查通过。

## 9. P/F 边界

P 阶段完成：权威 Handoff → Session 启动 → 单 World 发布 → 精确重放/重建接管 → 显式 `Cancelled`/`Ended` 清理 → 稳定终止回执。真实无头 World 中执行了 Actor 生成、接管和销毁验证。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实玩家输入、截图、Smoke、Cook 或 Package；没有修改正式地图或内容资产；没有把 Session 接入玩家可触发的唯一 Product Host/Command Route；没有声明正式游戏流程已经可见或可玩。

## 10. 下一阶段与 GitHub

建议 P27.25 建立唯一的 Formation Scatter World Publication Command/Host 路由：从玩家产品流程接收完整 P27.22 Evidence，持有一个活动 Session，将发布和终止命令绑定到 Run/Deployment 生命周期，并明确 Host 重建时旧实例退出与新实例接管顺序。所有 World 操作仍只经现有 Adapter，资源与 Deployment 事实继续只前进。

基线提交：`61c5c8035d39880a50a1ab5e4d57eed26d44b787`（P27.23）。分支：`agent/0.0.10-p27-24-formation-scatter-world-publication-session`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-24-formation-scatter-world-publication-session>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-24-formation-scatter-world-publication-session/Docs/Report/Dev.D.UE.0.0.10.P27.24.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-24-formation-scatter-world-publication-session/Docs/Log/Dev.D.UE.0.0.10.P27.24.r0_log.md>
