# Dev.D.UE.0.0.10.P27.13.r0 Report

## 1. 结论

P27.13 已把 P27.4 的“设备无关锚点输入 → 任意生命周期回调”接入真实产品依赖，形成唯一的具体阵法锚点入口 `Fdemo_mapShanmenFormationAnchorProductRoute`。

调用方不再提交 RunId，也不能替换锚点生命周期函数。入口只接收现有 ItemAuthority、CombatRunCoordinator、FormationRunLifecycle、显式 World、显式 ActorClass、稳定 InputEventId 与锚点定义；Run 与玩家身份只从 ready Combat Run 获取，最终调用固定进入 `Lifecycle.TryExecuteAnchorOperation(...)`。

本轮同时修复了一个已存在的一致性缺口：同一已提交材料 AttemptId 不能再绑定到另一锚点。专项路由 2/2、FormationSession 4/4、完整 `Shanmen.0_0_10` 1,355/1,355、属性交叉组 4/4、Editor/Game 两目标构建、映射自测与改动驱动覆盖门全部通过。

## 2. 阶段问题与范围

P27.12 已封闭阵法开始入口，但阵法开始后的锚点输入仍允许调用方填写 RunId 并传入任意生命周期回调。错误接线可能把一个 Combat Run 的输入交给另一 Run，或绕开唯一 FormationRunLifecycle、材料事务与 World 部署链。

本轮只封闭该产品接线，并修复其测试暴露的幂等漏洞：

- gameplay gate 必须早于产品依赖检查、锚点采样和生命周期调用；
- RunId 与 PlayerEntityId 只取自 ready CombatRunCoordinator；
- FormationRunLifecycle 必须 active、valid 且绑定同一 Run；
- World 必须显式传入、有效且未进入 teardown；
- ActorClass 必须显式传入，为非抽象、非废弃的 AActor 子类；
- 固定回调只能调用现有 `Lifecycle.TryExecuteAnchorOperation(...)`；
- 返回后再次核对 Coordinator、Lifecycle、World 与 ActorClass 绑定；
- 已提交 AttemptId 若尝试改绑另一锚点，在接触材料权威前返回 `AttemptConflict`。

没有绑定物理按键、UI、PlayerController、GameMode、地图或正式阵图内容，也没有新增第二份阵法、物品、Run、材料或 World 权威。

## 3. 固定路由顺序

`RouteAnchor()` 的顺序冻结为：

1. gameplay gate；
2. 一次产品依赖验证；
3. 捕获 ready Combat Run 的 RunId 与 PlayerEntityId；
4. 核对 Formation 生命周期 active/valid/Run 一致；
5. 核对显式 World 与显式 ActorClass；
6. P27.4 InputAdapter 惰性采样锚点并派生唯一 AttemptId；
7. 固定回调进入唯一 FormationRunLifecycle；
8. 生命周期依次经过 ProductController、ProductHost、Session、材料事务与 World 部署；
9. 返回后重新核对全部产品绑定。

gameplay 被阻止时 `DependencyValidationCount=0`，即使 Coordinator、Lifecycle、World、ActorClass、事件与锚点都无效，也不会检查依赖、采样或调用生命周期。依赖失败时 InputAdapter 保留 `LifecycleUnavailable` 的有效零采样证明。

## 4. 可审计结果契约

顶层结果保留状态、诊断、依赖验证次数、RunId、PlayerEntityId、LifecycleRunId、ActorClassPath，以及完整的 P27.4 输入证明。

- `GameplayBlocked`：输入门先于依赖完成；
- `CoordinatorUnavailable`：Combat Run 未 ready；
- `LifecycleUnavailable` / `LifecycleRunMismatch`：生命周期无效或属于另一 Run；
- `WorldUnavailable`：显式 World 缺失、失效或正在 teardown；
- `ActorClassUnavailable`：类缺失、抽象、废弃或不是 AActor 子类；
- `Routed`：具体依赖已核对，现有输入/生命周期链产生合法接受或拒绝结果；
- `StateDesynchronized`：嵌套证据无效，或调用期间任一产品绑定发生变化。

`Routed` 不等于业务接受：无效锚点等合法拒绝仍是有效审计结果；只有嵌套输入与生命周期都接受时，顶层 `IsAccepted()` 才为真。

## 5. 幂等与一致性修复

真实产品 fixture 证明：

- 第一个锚点经具体路由完成材料提交与 World 放置；
- 完全相同的 InputEventId、锚点、World 与 ActorClass 重放，复用同一 AttemptId 和 PlacementReceipt；
- 相同事件改用另一 ActorClass，由既有部署幂等门拒绝，不产生第二次放置；
- 相同事件改绑另一锚点，在 Session 准备阶段返回 `AttemptConflict`，物品快照不变且 Session 仍有效；
- 新事件可继续放置第二个锚点，使阵法进入 Active；
- 最终仍由唯一 FormationRunLifecycle 删除两个 Actor 并释放 Combat Run。

修复位于 `Fdemo_mapShanmenFormationProductSession::TryPrepareAnchor()`：精确锚点重放仍优先返回原审计；若 AttemptId 已存在于另一已提交审计，则在任何新材料预留或 pending 状态写入前失败关闭。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationAnchorProductRoute` | 2 | 0 | `C485871E7F7223F356A1A1103083BFFA651C87FEDA530A5BEF9BB887027206DA` |
| `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | `9D6DEDD88677070746D51993532001BB04677F451AE3200D936A3D26F5BCCD2F` |
| `Shanmen.0_0_10` | 1,355 | 0 | `6544558EEF7D3E1CE112DA2BD2A0DC85A695BD541FC3ADF62ED3A6A0CE1344AD` |
| `demo_map.V3.Attributes` | 4 | 0 | `99A02B9146F3EDE7F39C2A91FAC65524C381A3EAD230345868BF806FBE73BFD2` |

四份日志均有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。专项路由耗时 18.048 秒，Session 组 18.215 秒，完整根组 65 分 34.358 秒，属性组 17.743 秒。

## 7. 改动驱动回归

新增 `FormationAnchorProductRoute` 映射规则，要求根组以及 InputAdapter、RunLifecycle、ProductController、ProductAuthority、CombatRunCoordinator、ProductHost、FormationSession、MaterialAdapter、Items、WorldGameplay、FormationDeployment 与 CombatCore 证据。Session 修改同时触发现有阵法影响链和 `demo_map.V3.Attributes` 规则。

- regression map JSON：PASS；
- 映射器正反自测：`478/478` PASS；
- 负向自测确认只有 P27.13 专项不能替代生命周期、材料与 World 证明；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=41 Logs=4`；
- `git diff --check`：PASS。

## 8. 构建与静态边界

- Editor：7 actions，`SUCCEEDED`，原生退出 0，23.789 秒；stdout SHA-256 `9DEE5528E3A53A66211F00681DC968E9358D35A510A4B3288F4A4215A09E3940`；
- `UnrealEditor-demo_map.dll`：19,654,144 bytes，SHA-256 `D9F5D5249A54F78A6A74CE51BB70B1F1167BDC48618937D80959253CBAEA708C`；
- Game：6 actions，`SUCCEEDED`，原生退出 0，31.246 秒；stdout SHA-256 `CB7A7D087A18780D88291FDFAEF1DC23BBDFF2C520C24C37FCE3D1CE0281E8FD`；
- `demo_map.exe`：360,276,992 bytes，SHA-256 `8FD1116357147973B0A81B8A80E2D80AD362D9FB834A2D879E5269CA7957C849`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增具体产品路由共 354 行非空行（header 76 + cpp 278）。它只接受显式 World 与 ActorClass，不进行 World/Actor 发现；没有物理键位、UI、PlayerController、Enhanced Input、GameMode、轮询、定时器、异步任务、随机 GUID/RNG、Profile/schema、伤害调用或第二权威。没有新增内容资产、地图或配置改动。

## 9. P/F 边界与下一阶段

P 阶段已经证明 gameplay-first 零访问、具体依赖同 Run 绑定、显式 World/Class 围栏、首次放置、精确重放、ActorClass 漂移拒绝、跨锚点 AttemptId 冲突失败关闭、正常完成与唯一生命周期清理，以及完整回归、覆盖门和双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。因此仍不声明玩家已有正式锚点按键或 UI。下一阶段应在正式 InputAction、键位与瞄准/摆放交互责任明确后，让现有输入所有者持有本路由的真实依赖；不得恢复调用方注入 RunId 或自由生命周期回调。

## 10. GitHub 交接

基线提交：`d05674651faa835646c8c400df8958bdc65340f2`（P27.12）。分支：`agent/0.0.10-p27-13-formation-anchor-product-route`。本阶段只提交 2 个新增生产文件、2 个既有生产/单元测试文件、1 个既有产品集成测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.13` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-13-formation-anchor-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-13-formation-anchor-product-route/Docs/Report/Dev.D.UE.0.0.10.P27.13.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-13-formation-anchor-product-route/Docs/Log/Dev.D.UE.0.0.10.P27.13.r0_log.md>
