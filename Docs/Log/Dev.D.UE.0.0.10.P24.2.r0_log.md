# Dev.D.UE.0.0.10.P24.2.r0 Development Log

## 1. 目标

- 审计 P24.1 物理剑气入口与现有 P18 Impact 交付链之间是否已有同事务证据；
- 在不新增生产封装的前提下补齐物理 `B` 到真实 M01 生命值权威的最短闭环；
- 保留自然 HostBusy、冻结 Retry、输入锁和清理验证；
- 按改动文件映射完成聚焦测试、两棵完整回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`5119d4a484fc7b74fd435a3c04082180cfd2ab53`；
- 工作分支：`agent/0.0.10-p24-2-sword-qi-impact-loop`；
- 基线阶段：P24.1 已证明物理 `B`、真实装备/属性、Combat Run、行动仲裁、投射物、HostBusy 与冻结 Retry；
- 开始时 tracked tree clean，保留用户 103 个未跟踪文件；
- P18 既有 RunHost 与 WorldDelivery 已负责接触、Impact 和敌人生命值提交。

## 3. 审计结论

现有生产链完整，不需要新的 Sword Qi Manager、桥接 Subsystem 或重复生命值服务。证据缺口只存在于测试组合：

- WorldDelivery/RunHost 测试证明“接触事件 → M01 生命值”；
- P24.1 PhysicalInput 测试证明“物理 `B` → 真实产品投射物”；
- 没有单个用例从同一次物理输入产生投射物，再由该投射物提交真实敌人 Impact。

本轮因此只修改既有物理输入测试，不改生产代码。

## 4. 测试实现

### 4.1 M01 目标权威

新增测试辅助函数从 `Fdemo_mapM01EnemyConfig` 选择 `StandardSkirmisher`，构造正式 encounter identity，并为目标生成指向根 PrimitiveComponent 的 `FHitResult`。

`IssueRetryAndLock` 在 preview World 中生成 transient `Ademo_mapEnemyCharacter`，安装 `Udemo_mapM01EnemyIdentityComponent`，执行 definition 与 encounter 配置，并通过当前 `CombatRunCoordinator` 注册目标。任一步缺失均失败关闭。

### 4.2 同事务 Impact

首个正式 `B` 输入仍通过真实 ItemSubsystem、TrainingBlade、最终 AttackPower 8、GameMode 产品路由和行动仲裁生成真实投射物。第二个 `B` 在投射物仍在飞行时自然形成 HostBusy 与冻结请求。

测试随后从敌人 vitality host 捕获前快照，向首枚产品投射物的 `OnContact` 生产事件广播目标命中，再捕获后快照并退役终态。断言同时检查：

- Terminal Kind 为 Impact；
- Delivery 已提交；
- `NewlyCommittedDamage` 在 `KINDA_SMALL_NUMBER` 容差内为 `0.58`；
- 前后生命值差在同一容差内为 `0.58`；
- `AuthorityRevision == before + 1`。

### 4.3 原有行为保持

Impact 退役后继续原用例的空间、方向和属性变更，再通过第三个 `B` 执行冻结 Retry。重试仍使用第二个事件的同一命令身份和冻结值；第二枚投射物、UI 结算锁、命令计数、Owner/Controller/Coordinator 结束及 Item Run settlement 均沿用既有清理断言。

## 5. 首次运行证据

| Evidence | Result | 说明 | Bytes | SHA-256 |
|---|---:|---|---:|---|
| `P24.2_EditorBuild_initial.log` | PASS / native 0 | 4 actions，编译并链接修改后的测试 | 2,269 | `71AD631F022CC76AB7DA9F8876A2BB3111D7D5CD56EAC48299DD1ACCD85AFCE9` |
| `P24.2_SwordQiPhysicalInput_initial.log` | 5/0 | 首次聚焦运行即通过 | 270,140 | `229C9BF985582C5F5D13E71B2F304956A2369906CF8EA3F2748E3D535EB812F0` |

本轮没有失败的源码构建或自动化测试，因此不存在需要保留的首次失败日志。未通过放宽断言、直接写生命值或伪造 Delivery Receipt 获得 PASS。

## 6. 聚焦验证

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 270,140 | `229C9BF985582C5F5D13E71B2F304956A2369906CF8EA3F2748E3D535EB812F0` |
| `Shanmen.0_0_10.Product.SwordQi` | 38 | 0 | 305,848 | `908A48D03AD9D503609F74D166F22B0404E65002E6779536BAE9F5A6B37F3BA7` |

第二组覆盖完整 Sword Qi 产品族，包括 availability、命令所有权、产品权威、Controller、Session、RunHost 与 WorldDelivery。两组均有成功终止标记，原生退出码 0。

## 7. 完整回归与覆盖

| Group | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 73m09s | 1,946,376 | `335D0FB8FD5E19FD2CEC958D4E78BC4BA8C7833695169E2F3D3BEB9E4C849624` |
| `demo_map` | 1,330 | 0 | 约 1m02s | 1,656,074 | `341147292DA6D6CF00AD8FA43876707988A2761A0D043A46C6F4C3EDD27B73D1` |

完整合计 2,607/0。慢段来自既有深层 journal、checkpoint 与恢复测试；没有将耗时或引擎启动提示改写为源码失败。

改动文件映射结果：`PASS Changed=1 Rules=1 Required=20 Logs=2`。覆盖日志 SHA-256 `B15B9960375B59C29EE00C0201CAF6DB7E2A97552327F3C6E689AF097C3D0EC8`。覆盖器自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_mapEditor Win64 Development`（initial） | Succeeded | 0 | 4 | `71AD631F022CC76AB7DA9F8876A2BB3111D7D5CD56EAC48299DD1ACCD85AFCE9` |
| `demo_mapEditor Win64 Development`（final） | Succeeded / up to date | 0 | 0 | `E0780A56BDC01FF6AFE9AC188B64B9CAFDCA82607CB6169867FB1B9CBDABB39B` |
| `demo_map Win64 Development`（final） | Succeeded | 0 | 3 | `2E0E738A5955CF502F01F12CC407C8313C4D69443B9EA9EE583276F76531CFF3` |

最终二进制：

- `demo_map.exe`：359,681,536 bytes，SHA-256 `B09117ABC2CFA69DD709A557CD2EEE9AC0E4FDB243D3C0B4BC2CB58FB51FFB86`；
- `UnrealEditor-demo_map.dll`：18,945,536 bytes，SHA-256 `20878FC0284BFDA22890135C5D342A0C5DAB939A2EAA16711EC02E7625688711`。

## 9. 静态与 P/F 边界

- 非文档增量：1 test file、`+128/-5`；
- `git diff --check`：PASS；
- 生产文件改动：0；
- 新增模块、资产、UCLASS/USTRUCT、Actor、Subsystem、输入动作或持久状态：0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P 阶段证明无头物理脉冲可经真实产品权威创建投射物，并由该投射物的接触事件提交 M01 Impact。F 阶段保留真实键盘设备、场景物理碰撞、画面、动画音效、HUD 视觉和手感验收。

## 10. 提交与后续

精确提交 3 个文件：

- `Source/demo_map/demo_mapShanmenSwordQiPhysicalInputTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.2.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.2.r0_log.md`。

用户 103 个未跟踪文件保持未暂存，原始验证日志保持本地忽略。后续阶段应优先增加玩家可观察的剑气结果或填补经审计确认的责任边界，不重复现有 ProductController、RunHost、WorldDelivery 或生命值权威。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-2-sword-qi-impact-loop>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-2-sword-qi-impact-loop/Docs/Report/Dev.D.UE.0.0.10.P24.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-2-sword-qi-impact-loop/Docs/Log/Dev.D.UE.0.0.10.P24.2.r0_log.md>
