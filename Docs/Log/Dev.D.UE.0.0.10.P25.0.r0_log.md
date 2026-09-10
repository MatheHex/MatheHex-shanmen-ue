# Dev.D.UE.0.0.10.P25.0.r0 Development Log

## 1. 目标

- 为灵力护盾接入产品层前建立唯一 Run 级 `SpiritEnergy` 权威；
- 让神识与外部战斗动作共享同一余额和同一有序审计历史；
- 保证 callback 失败、Reservation 未提交、身份冲突和状态不同步全部失败关闭；
- 保证精确重放不二次执行 mutation、不二次扣费；
- 使用真实 `FShanmenSpiritShieldSession` 证明护盾事务可复用该权威；
- 不在本轮接物理输入、HUD、敌方 Impact 或 World 生命周期；
- 按改动文件映射完成回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线：`924deda74dd72abceaee9c868467122aba8c6c41`（P24.6）；
- 分支：`agent/0.0.10-p25-0-shared-spirit-energy-ledger`；
- 开始时 tracked tree clean；
- 用户 103 个未跟踪文件保持原样；
- 本阶段不新增另一份玩家灵力、不更改平衡值、不改输入、伤害、存档或物品权威。

## 3. 缺口审计

现有 `Fdemo_mapShanmenDivineSenseProductHost` 已独占神识 Run 资源，并通过 `FShanmenActionResourceAuthority` 完成 Reserve/Commit。但 Router 的不变量假定：资源事务数严格等于已处理神识脉冲数，且每次资源变化都是一条神识命令。

若护盾直接复用 Host Authority 而 Router 不知情，下一次神识就会因 Revision 与事务数变化而判定状态损坏；若护盾另建余额，则玩家拥有两份互不一致的灵力。

因此本轮首先扩展账本，而不是直接堆按键和 HUD。

## 4. Host 外部事务实现

在现有 Product Host 内增加：

- `Fdemo_mapShanmenSharedSpiritEnergyTransactionReceipt`；
- 类型化 Status、Error 与 Result；
- `ApplySharedSpiritEnergyTransaction()`；
- 外部事务计数与 Receipt 查询；
- 以 TransactionId 为键的不可变 Receipt 保留表。

入口先复制完整 Host，再让 caller 对副本内的 Authority 执行事务。只接受恰好一笔已 Finalize、Revision `+2`、无 Pending Reservation、余额非递增且资源形状不变的结果。全部验证成功后才移动发布 Candidate。

ReceiptId 由 HostId、TransactionId、CommandId、外部顺序号和前后 SnapshotId 确定性派生。精确身份重放直接返回保留 Receipt；同 TransactionId 的不同 CommandId 返回 `TransactionConflict`。

## 5. Router 与 Controller 原子发布

Router 为每条神识命令补充全局 `ResourceSequence`，并新增外部事务记录。`IsValid()` 重建混合资源序列，逐笔验证快照连续性、Revision、资源形状、Reservation、余额方向和最终快照。

`IsConsistentWithHost()` 进一步要求：

- 神识脉冲数一致；
- 外部事务数一致；
- 当前资源快照一致；
- 每个外部 Receipt 在 Host 与 Router 中完全匹配。

Controller 复制完整自身，在 Candidate Host 执行事务，再把 Receipt 记录到 Candidate Router；只有 Session 与 Controller 全部重新通过验证后才整体发布。Session 只增加 Controller 友元访问，没有建立新的包装链或第二个运行时。

## 6. 自动化过程

新增 `SharedSpiritEnergyLedger` Controller 测试，使用真实 Shield Session、资源成本和期限定义。验证结果：

- 未 Commit 的护盾 Reservation：拒绝，余额 100，账本 0/0；
- 护盾 Begin + Commit：成功，余额 80，Host/Router 外部事务 1/1；
- 精确重放：mutation 不再调用，余额保持 80；
- 身份冲突：mutation 前拒绝；
- 旧神识命令：`ResourceProjectionStale`，Provider 调用数不变；
- 新神识脉冲：成功，余额 70；
- 最终 Router 为 1 条神识命令 + 1 条外部事务。

首次 Editor 构建在 Host Receipt `Matches` 定义中出现重复 `const`，MSVC C4114，UBT `OtherCompilationError`。通过源码修正后再构建成功。该失败明确归因于本轮源码，原始日志未覆盖。

## 7. 改动文件回归

精确改动路径：

- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseCommandRouter.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseCommandRouter.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductSession.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductController.h`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductController.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSenseProductControllerTests.cpp`。

回归表：

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `13263D67430C22620CEDC2C3319C4FCEB982D21C72548C669E53C71BC1B68E63` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `3477431425F36B84744144E08068F6282D86E41458FD7F64FF74C6CAEAB91442` |
| `Shanmen.0_0_10.CombatRuntime.DivineSense` | 4 | 0 | `E3FB208FDBF59BF6B277F476FC383A509D317CAF8EC110E287672CDDD740E7FE` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18 | 0 | `937510BB0946760E8F4191805FC751373C8DCA3087B6B1CFDBCA20DC26522AEA` |
| `Shanmen.0_0_10.Product.DivineSenseCommandRouter` | 4 | 0 | `531DFD5F7D6C58B130F9072271347C756BE47E0EE52B7CADACB8CA0C939B0426` |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 | `54D0399BE77A17F695CF07B59C37F4C1D492E4F36DD087094282D2E614A5BBD9` |
| `Shanmen.0_0_10.Product.DivineSenseProductHost` | 4 | 0 | `D18E69B61A92B33FAF47A3777B55A78309D113C28D5B1911F8E6EC8022D471DA` |
| `Shanmen.0_0_10.Product.DivineSenseProductSession` | 4 | 0 | `C746F041F33034F4C0A2F204611059844AF6F839F61FF6F419DDE47F87D82D72` |
| `Shanmen.0_0_10.Product.DivineSensePulseCoordinator` | 4 | 0 | `28C528127228F716BE96AAE137B2849AC6A209E2BD9861A6A874D35BDF6E436E` |
| `Shanmen.0_0_10.Product.DivineSenseWorldObservation` | 4 | 0 | `0CE5B8C3B3D1E6D24C7D7323452F7FB6B94A87EBB29330155BBEBA9CC3CB6C27` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `2C59810DEB7E7D2944407C202CDD35B343D3BCA3CE3AC2853703F4EFFE5C016C` |

合计 65/0。覆盖器：`PASS Changed=8 Rules=4 Required=11 Logs=11`。

- 覆盖日志：3,948 bytes，SHA-256 `814C718982292627286C3302EB0AA05DD4CB1C846C3F00DA68C0EC91EBEA735E`；
- 覆盖器自检：442/442，43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建、静态与 P/F 边界

| Target | Result | Native exit | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---:|---|
| `demo_mapEditor Win64 Development` first | C4114 / `OtherCompilationError` | 6 | 64 planned | 16,971 | `9DA98BDDFC48710999945AB8BECCCE9A70D62784B853409F3E8BDA7C7772C4E3` |
| `demo_mapEditor Win64 Development` final | Succeeded | 0 | 4 | 2,651 | `1EDD324297456D0CA639192CCDBD040810468ACF62A631D99BAF9BCE5D6E332C` |
| `demo_map Win64 Development` final | Succeeded | 0 | 63 | 6,457 | `4AB1F5476CD5E9EB57983376F44231DE46B0C8461840D82E75BC7B39DA7C87FE` |

最终 `demo_map.exe` 为 359,734,784 bytes，SHA-256 `2B0A4C888192CE19499F3FE13C08469ED1030C36401BA080E8D04929BB5A87AB`；`UnrealEditor-demo_map.dll` 为 19,008,000 bytes，SHA-256 `8CD5B6F5DD3E58791E307BC9635A4C9596D18311304FCD80E77FF82624293AD5`。

- 非文档增量：8 files，`+879/-15`；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、World 查询、Actor 操作、存档和物品权威：0；
- 最终项目相关进程：0。

P 阶段证明共享资源事务和混合回放账本成立，不声明可玩护盾已完成。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 精确提交范围

只提交第 7 节列出的 8 个源码/测试文件，加：

- `Docs/Report/Dev.D.UE.0.0.10.P25.0.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P25.0.r0_log.md`。

用户 103 个未跟踪文件保持未暂存；`Saved/Codex/P25.0` 的失败、构建、测试和覆盖原始证据保持本地忽略。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-0-shared-spirit-energy-ledger>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-0-shared-spirit-energy-ledger/Docs/Report/Dev.D.UE.0.0.10.P25.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-0-shared-spirit-energy-ledger/Docs/Log/Dev.D.UE.0.0.10.P25.0.r0_log.md>
