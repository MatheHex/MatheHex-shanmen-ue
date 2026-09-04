# Dev.D.UE.0.0.10.P20.5.r0 Report

## 1. 结论

P20.5 已把 BallisticArc 接入既有 `Fdemo_mapShanmenThrownWeaponProductController`，并继续复用唯一 Combat Run action sequence、物品 Prepare/Commit/Cancel、RunCommand Router、RunHost、幂等回放与取消恢复权威。

产品入口现在显式区分 Straight 与 BallisticArc。Arc selection 只冻结玩家选择的 origin、target 与 apex clearance；产品快照冻结武器 definition/offense/source tags 以及 technique tier、gravity、maximum flight time。Controller 在取得 Run 所有的 Action identity 后调用 P20.0 纯求解器，再把不可变 Arc plan 交给 P20.4 RunCommand。调用方不能提交已绑定到未知 Action 的 plan，也不能从旧 `AimDirection + MaximumDistance` 隐式猜 Arc。

本轮没有把 Arc 暴露给 ProductSession、lifecycle、输入或 UI。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`ad63daca1adae692cd2b1fdb4118e7d3107e12a4`（P20.4）；
- 分支：`agent/0.0.10-p20-5-thrown-weapon-arc-product-controller`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 判别式选择与产品快照

`Fdemo_mapShanmenThrownWeaponSelectionIntent` 新增显式 trajectory kind：

- 旧 `TryCapture` 保持 Straight 兼容，只接受规范化方向与有限正最大距离；
- 新 `TryCaptureArc` 只接受有限 origin/target、正 apex clearance 与精确 Run/item/selection identity；
- Straight 必须没有 Arc target/policy，Arc 必须没有有效直线 payload；
- `IsValid` 与 `Matches` 按 trajectory kind 失败关闭，阻止同一 SelectionId 复用不同轨迹或几何。

`Fdemo_mapShanmenThrownWeaponProductCapture` 保持不可变产品快照。Arc 新增 `Fdemo_mapShanmenThrownWeaponArcProductPolicy`，由产品侧明确拥有 technique tier、gravity magnitude 与 maximum flight time；definition 的 launch speed 仍是唯一最大初速度来源。Selection 因此不拥有数值平衡参数。

## 4. Run 所有的 Action 身份

`Fdemo_mapPlayerThrownWeaponActionReservation` 现在冻结实际 `ActionDefinitionId`。Coordinator 保留旧三参数 Straight API，并新增显式 action-definition overload，只接受规范 Straight 或 Arc definition。

两种轨迹共享同一个单调递增 thrown-weapon activation sequence。Reservation 的 `IsValid` 会用 RunId、player entity、实际 definition 与 sequence 重算 ActivationId，防止 Arc 身份被 Straight canonical definition 误绑定，也没有新增第二套 sequence 或 ledger。

## 5. 单一 Controller/Router/Host 路径

共同路径保持为：selection/product/correlation gate → authority snapshot 与 item tag gate → Run action reservation → immutable action capture → trajectory-specific command capture → 原 Router 与 Host。

- Straight 继续调用原 `TryCapture`；
- Arc 由 Controller 使用 Action + selection geometry + product policy 构造 request，调用纯 `FShanmenThrownWeaponArcPlanner`，再调用 P20.4 `TryCaptureArc`；
- 两者随后共享同一个 durable item transaction、Router、Host、terminal receipt 与 cancellation recovery。

生产边界没有直接 inventory mutation、damage apply、RNG、trace/sweep、input binding、PlayerController 或 GameplayStatics 接线。

## 6. 幂等、冲突与不可达 Arc

成功 selection 继续以 SelectionId 记录完整 frozen selection、product、sequence 与 command：

- 精确重放复用原 Action/command/terminal receipt，不消耗新 sequence、不重复扣物品或生成 Actor；
- 同 SelectionId 异 payload 在 item I/O 前以 `SelectionIdConflict` 失败关闭；
- Arc pre-launch failure 继续走原持久取消与恢复路径。

不可达 Arc 也必须是确定性的终态。Controller 因此记录一次只读 rejection：Action identity、sequence、selection、product 与 planner diagnostic。精确重放复用同一 rejection，不消耗第二个 sequence且不触碰 item authority；冲突 payload 仍被拒绝。`IsValid` 会重新求解并要求结果仍为 `Unreachable`、diagnostic 与 ActivationId 均可重算。

最终源码复核另修正一个恢复边缘：成功捕获回滚后仅在成功与 rejection 两个映射都为空时才清除 RunId，避免已有 Arc rejection 时误丢 Run 身份。

## 7. 自动化结果

ProductController exact 从 4 增至 7 项：

- `CaptureContract`：Straight/Arc selection、product policy 与 trajectory mismatch 失败关闭；
- `ArcSubmitReplayConflict`：Arc 真实 authority/Router/Host 路径、扣减、精确回放和冲突；
- `ArcPreLaunchCancel`：Arc 发射前失败的持久取消与无副作用回放；
- `ArcPlanRejectionReplay`：不可达 plan 的单次身份记录、无 item I/O、重放与冲突；
- 原 Straight submit/retry/recovery 三项继续通过。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `product_controller_final.log` | `Product.ThrownWeaponProductController` | 7/0 | `1A691C3B5573EA0C2E312C2D86C6F3074B774B79AFEABC9214763DE1CE0EE46E` |
| `combat_run_coordinator_final.log` | `Product.CombatRunCoordinator` | 18/0 | `6F6095B4FE6350488ADE7815D8A123862D24E695136DF96092B17A83936FEB45` |
| `run_command_final.log` | `Product.ThrownWeaponRunCommand` | 6/0 | `458CA4DBF167A3F3A5EE671972FEDE7AFB73F782F78D00EA229A1CDF9CB5027B` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 854/0 | `821CBC432C9ED37E2EA2FB513A8B976BFB6557E29A76E4B24E08D4374E6D957E` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | 4/0 | `B75E676F468D6C57F0C9BF7212F0A97245DA1A9734579E9E7026D173A2CB7036` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | 44/0 | `557133E68FD2D4151E81A2FBFF18A405FC681AF1F403AA611BE98F77E186BC68` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | 22/0 | `3B7D638425190161D1621AE138E13A79BD9F2BA3EDA93504091BFEF92BA6EF92` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `9421A0732539316C4EE1DE3A4E6667AC98B9DA8E22BE639776B5C448991F97FB` |

0.0.10 全量从 P20.4 的 851 增至 854。证据审计要求每份日志只有一个 canonical `RunTests` command、一个 native terminal、声明数等于成功记录数，且 Fail/Fatal/Unhandled/Ensure 均为 0：`PASS Logs=8 RecordedSuccess=1001`，审计 SHA-256 `30144FA0243312085482F4623B2B76609DC9BBDC3CE055F953D8D4F148EA3750`。

## 8. 改动门禁与静态边界

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=22 Logs=8
```

改动路径命中 ProductController 与 CombatRunCoordinator 映射；要求组由实际 `git diff --name-only` 推导，并由 exact、0.0.10 全量及旧版兼容日志覆盖。gate SHA-256：`61FFD8D4EF74A58F2D0495847146AA12FFEAD6721615F652D2D40FA70F4FEEC2`。

对 4 个 production 文件的新增行执行边界扫描：`PASS Files=4 AddedLines=529 Matches=0`，SHA-256 `D6813CDC709DDB04A175BDE1E72FDB71292B921BB11D73236B7046596D560FB9`。

`git diff --cached --check` 对最终 7 个提交文件原生通过，记录 SHA-256 `84E1F7035CB544F20683DCF38F3006B20CFD3889C93520A9DBF1CE42130E570F`。本轮没有失败的编译或自动化日志需要隐藏或覆盖。

## 9. 构建与产物

- final Editor：4 actions / native 0，SHA-256 `F79E0C8441891BBA0A6DD2529D3D6CAD23A17DB30234BD147DD2FABB4B1F01AA`；
- final Game：3 actions / native 0，SHA-256 `FFB6EFB3168FD3B3F6CF09CE61D22B046793DA2A41659EFB343C6D248BB727C4`。

产物：

- `Binaries/Win64/demo_map.exe`：357,144,064 bytes，SHA-256 `3574BBB2CFD3ED14C61A44DA04D73290442BBD76503D7C821CD9B0F1C69ACD49`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,805,440 bytes，SHA-256 `069CD7410D0890C0A08AEE9A585B37EFE9CC7E51EEC9FD6B8F9F61BD6289F818`。

## 10. P/F 边界与下一步

P20.5 证明的是“typed product selection 如何在取得 Run-owned Action identity 后生成 Arc plan，并经唯一事务/Router/Host 执行”。它没有声明玩家入口已可用。

建议 P20.6 扩展既有 ProductSession，使 session 显式接受 Straight 或 Arc selection 并把 immutable capture 交给本轮 Controller；继续不修改 lifecycle/input/UI。后续阶段再独立接滚轮弧度意图、目标预览与可视反馈，避免把产品会话和平台输入重新耦合。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller/Docs/Report/Dev.D.UE.0.0.10.P20.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller/Docs/Log/Dev.D.UE.0.0.10.P20.5.r0_log.md>
