# Dev.D.UE.0.0.10.P20.5.r0 Development Log

## 1. 目标与基线

- 基线：`ad63daca1adae692cd2b1fdb4118e7d3107e12a4`（P20.4）；
- 分支：`agent/0.0.10-p20-5-thrown-weapon-arc-product-controller`；
- 目标：让既有 ProductController 接受显式 Arc selection，在 Run identity 已知后生成 plan，并复用唯一 RunCommand/item/Host/ledger；
- 约束：不接 session/lifecycle/input/UI，不建立第二 Action sequence、inventory transaction、Actor、damage、collision、planner 或 replay authority。

## 2. 接入前审计

P20.4 已让 RunCommand 与 RunHost 完整执行不可变 Arc plan，但 ProductController 仍只有 Straight selection：origin、normalized aim direction、maximum distance；Coordinator reservation 也把 ActionDefinitionId 固定为 Straight。

直接要求上层提交一个 Arc plan 不成立：plan identity 绑定完整 Action snapshot，而 Action ActivationId 又依赖 ProductController 向 Run 申请的下一 sequence。上层在 reservation 前无法安全构造最终 plan。若 Controller 接受并重新绑定外部 plan，就会破坏 P20.0/P20.4 的身份契约。

因此采用分层所有权：selection 只拥有选择几何，product capture 拥有平衡参数，Run 拥有 Action identity，Controller 在三者齐备后调用唯一纯 planner。

## 3. Selection 与 Product 契约

Selection 新增 `TrajectoryKind`、Arc target 与 apex clearance。旧 `TryCapture` 保持 Straight；新 `TryCaptureArc` 明确构造 BallisticArc。互斥字段、有限值、正距离/弧高、Run/item/selection identity 均由 `IsValid` 验证。

Product capture 新增 immutable Arc policy：technique tier、gravity magnitude、maximum flight time。Arc definition 必须使用规范 Arc action definition；Straight product 不得携带有效 Arc policy。`Matches` 比较完整 definition、offense、source tags 与 policy，SelectionId 冲突不能绕过产品载荷差异。

## 4. Coordinator 双轨迹身份

Reservation 新增冻结的 `ActionDefinitionId`。原三参数 API 委托到 Straight overload，现有调用无需迁移；新 overload 仅接受规范 Straight/Arc definition。

两条轨迹从同一个 `NextPlayerThrownWeaponActivationSequence` 取号。Reservation validation 用实际 definition 重算 ActivationId，保持 sequence 单调、命名空间一致、身份不可伪造。

## 5. Controller Arc 管线

共同前置 gate 完成后，Controller 先向 Coordinator 取得 Action identity，再捕获 immutable Action snapshot。

Straight 分支原样调用旧 RunCommand capture。Arc 分支从 Action、selection 与 product policy 构造 `FShanmenThrownWeaponArcRequest`；maximum launch speed 取自 weapon definition，调用 P20.0 planner 后把 plan 交给 P20.4 `TryCaptureArc`。

命令成功后继续进入原 Router、持久 item transaction 与 Host。没有在 Controller 内复制 prepare/commit/cancel、spawn 或 terminal ledger。

## 6. 不可达计划与幂等恢复

Arc request 有效但 solver 返回 `Unreachable` 时，不能让相同 SelectionId 每次重试都消耗新 sequence。为此 Controller 保存 deterministic rejection record：selection、product、sequence、Action 与 planner diagnostic；不触碰 item authority、Router 或 Host。

精确重放返回同一 ActivationId/sequence/diagnostic 并标记 reused；同 ID 异 payload 返回 conflict。Controller `IsValid` 重新构造 request 并求解，要求结果仍为 Unreachable、身份和 diagnostic 可重算，且 rejection 与成功 selection 的 ID/sequence/activation 集合互斥。

最终源码复核把成功捕获失败回滚中的 RunId 清理条件改为共同 `IsEmpty()`，确保存在 rejection record 时不会误清 Run identity；随后重编并重跑最终验证。

## 7. 自动化扩展

ProductController exact 新增 Arc submit/replay/conflict、Arc pre-launch cancellation、Arc unreachable rejection/replay 三项，并扩展 capture contract。原 Straight submit/retry/recovery 保持通过，总数 7。

Coordinator exact 验证 Straight/Arc 共用 sequence 与可重算 definition identity；RunCommand exact 继续验证 P20.4 Arc transaction seam。全量测试总数由 851 增至 854。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `product_controller_final.log` | 7/0 | `1A691C3B5573EA0C2E312C2D86C6F3074B774B79AFEABC9214763DE1CE0EE46E` |
| `combat_run_coordinator_final.log` | 18/0 | `6F6095B4FE6350488ADE7815D8A123862D24E695136DF96092B17A83936FEB45` |
| `run_command_final.log` | 6/0 | `458CA4DBF167A3F3A5EE671972FEDE7AFB73F782F78D00EA229A1CDF9CB5027B` |
| `full_0_0_10_final.log` | 854/0 | `821CBC432C9ED37E2EA2FB513A8B976BFB6557E29A76E4B24E08D4374E6D957E` |
| `legacy_v3_attributes_final.log` | 4/0 | `B75E676F468D6C57F0C9BF7212F0A97245DA1A9734579E9E7026D173A2CB7036` |
| `legacy_enemy_skill_final.log` | 44/0 | `557133E68FD2D4151E81A2FBFF18A405FC681AF1F403AA611BE98F77E186BC68` |
| `legacy_v2_ranged_final.log` | 22/0 | `3B7D638425190161D1621AE138E13A79BD9F2BA3EDA93504091BFEF92BA6EF92` |
| `legacy_item_armor_final.log` | 46/0 | `9421A0732539316C4EE1DE3A4E6667AC98B9DA8E22BE639776B5C448991F97FB` |

证据审计：`PASS Logs=8 RecordedSuccess=1001`，SHA-256 `30144FA0243312085482F4623B2B76609DC9BBDC3CE055F953D8D4F148EA3750`。

## 9. Changed-file regression 与静态边界

最终改动 5 个源码/测试文件，命中 ProductController 与 CombatRunCoordinator 映射：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=22 Logs=8
```

- regression gate SHA-256：`61FFD8D4EF74A58F2D0495847146AA12FFEAD6721615F652D2D40FA70F4FEEC2`；
- production boundary：`PASS Files=4 AddedLines=529 Matches=0`；
- boundary SHA-256：`D6813CDC709DDB04A175BDE1E72FDB71292B921BB11D73236B7046596D560FB9`；
- `git diff --cached --check`：最终 7 文件 PASS / native 0，SHA-256 `84E1F7035CB544F20683DCF38F3006B20CFD3889C93520A9DBF1CE42130E570F`。

边界扫描覆盖新增 production 行中的 direct damage、direct inventory mutation、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics。结果不得用测试文件或注释命中替代生产边界判断。

## 10. 构建与产物

- final Editor build：4 actions / native 0，SHA-256 `F79E0C8441891BBA0A6DD2529D3D6CAD23A17DB30234BD147DD2FABB4B1F01AA`；
- final Game build：3 actions / native 0，SHA-256 `FFB6EFB3168FD3B3F6CF09CE61D22B046793DA2A41659EFB343C6D248BB727C4`；
- `demo_map.exe`：357,144,064 bytes / `3574BBB2CFD3ED14C61A44DA04D73290442BBD76503D7C821CD9B0F1C69ACD49`；
- `UnrealEditor-demo_map.dll`：15,805,440 bytes / `069CD7410D0890C0A08AEE9A585B37EFE9CC7E51EEC9FD6B8F9F61BD6289F818`。

本轮首次编译与自动化即成功。最终源码复核后的边缘修正也成功重编；没有失败日志被删除、改写或包装为成功。

## 11. P/F 边界与后续判断

没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

ProductSession 目前仍只构造 Straight selection。P20.6 应只扩展该现有 session seam，显式接收 Arc target/apex 与 immutable Arc product policy，并调用本轮 Controller；lifecycle、玩家滚轮输入、目标预览和视觉反馈继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller/Docs/Report/Dev.D.UE.0.0.10.P20.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-5-thrown-weapon-arc-product-controller/Docs/Log/Dev.D.UE.0.0.10.P20.5.r0_log.md>
