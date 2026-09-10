# Dev.D.UE.0.0.10.P25.0.r0 Report

## 1. 结论

P25.0 已把神识脉冲与后续灵力护盾统一到同一个 Run 级 `SpiritEnergy` 权威，并建立可验证、可重放、按发生顺序审计的混合资源账本。

本阶段没有把护盾做成第二份灵力余额。真实 `FShanmenSpiritShieldSession` 已通过新入口在现有 Divine Sense Product Controller 所拥有的资源权威上完成一次 20 点事务；随后神识继续从同一余额支付 10 点，最终余额精确为 70。

## 2. 解决的结构风险

P19 之后，神识 Product Host 已持有可靠的 Run 级灵力权威；灵力护盾结算本身也已有容量、期限、事务与幂等语义。若直接在护盾接线中新增余额，会形成两份“玩家灵力”，并使神识 Router 原先的回放不变量把外部资源变化误判为状态损坏。

P25.0 没有复制资源系统，而是扩展既有权威，使神识和其它明确授权的战斗动作共享一条事务历史。

## 3. 共享事务契约

新增的外部灵力事务入口要求：

- 有效的活动 Run、Host、TransactionId 与 CommandId；
- 事务只在资源权威副本上执行；
- 必须恰好完成一笔事务；
- 结束时不得残留 Reservation；
- AuthorityRevision 必须精确增加 2；
- 余额只能保持或减少，不能借外部事务增发灵力；
- 成功后生成包含前后快照、Host、身份和顺序号的不可变 Receipt；
- 相同 TransactionId 与 CommandId 重放时返回原 Receipt，不再调用 mutation；
- TransactionId 被另一个 CommandId 复用时，在 mutation 前失败关闭。

任何 callback 拒绝、未 Commit 的 Reservation、快照形状变化或不完整事务都不会发布 Host 状态。

## 4. 有序混合账本

Divine Sense Command Router 现在维护统一的 `ResourceSequence`：神识脉冲和外部战斗事务都占据同一条连续序列。

验证会从 Run 打开时的资源快照开始，逐笔检查：

- 前一笔 After 必须等于后一笔 Before；
- Owner、ResourceChannel 与 MaximumAmount 必须保持不变；
- 每笔前后 Revision 必须精确连续；
- 每笔前后 Reservation 必须为 0；
- CurrentAmount 不得上升；
- 最终快照必须与 Host 当前资源快照完全一致；
- Host 与 Router 必须保留相同外部 Receipt。

因此外部护盾支付不会再破坏下一次神识命令的回放审计，也不能绕开 Host/Router 一致性检查。

## 5. 真实护盾桥接证明

新增 Controller 自动化使用真实 `FShanmenSpiritShieldSession`，而不是测试替身：

1. 先捕获一个神识命令，但让 World Evidence Provider 拒绝，使该命令保留旧资源快照；
2. 尝试只 Begin、不 Commit 的护盾事务，入口拒绝并保持余额 100、Host/Router 外部事务数 0；
3. 通过共享入口执行护盾 Begin + Commit，支付 20，余额变为 80；
4. 精确重放同一身份，mutation 调用数不增加，余额仍为 80；
5. 用相同 TransactionId 配不同 CommandId，冲突在 mutation 前拒绝；
6. 重试旧神识命令，得到 `ResourceProjectionStale`，且不访问 Provider；
7. 捕获新神识命令并支付 10，最终共享余额为 70；
8. Availability 投影同样显示 70。

聚焦 Controller 组由 4 项增至 5 项并全部通过。

## 6. 改动文件驱动回归

8 个非文档改动路径命中 4 条回归映射规则，共要求并完成 11 个独立测试组：

| Group | Success | Fail |
|---|---:|---:|
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 |
| `Shanmen.0_0_10.CombatRuntime.DivineSense` | 4 | 0 |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18 | 0 |
| `Shanmen.0_0_10.Product.DivineSenseCommandRouter` | 4 | 0 |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 |
| `Shanmen.0_0_10.Product.DivineSenseProductHost` | 4 | 0 |
| `Shanmen.0_0_10.Product.DivineSenseProductSession` | 4 | 0 |
| `Shanmen.0_0_10.Product.DivineSensePulseCoordinator` | 4 | 0 |
| `Shanmen.0_0_10.Product.DivineSenseWorldObservation` | 4 | 0 |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 |

合计 65/0。

覆盖结果：`REGRESSION_COVERAGE: PASS Changed=8 Rules=4 Required=11 Logs=11`。

覆盖日志 SHA-256：`814C718982292627286C3302EB0AA05DD4CB1C846C3F00DA68C0EC91EBEA735E`。

覆盖器自检：442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 构建与失败证据

首次 Editor 构建真实失败：`demo_mapShanmenDivineSenseProductHost.cpp` 的函数签名重复了 `const` 类型限定符，MSVC 返回 C4114，UBT 为 `OtherCompilationError`。这是本轮源码错误，不是内存、环境或外层超时；修正后重新构建成功。首次失败日志保持原样。

| Evidence | Result | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P25.0_EditorBuild_FirstFailure.log` | Failed / C4114 | 64 planned | 16,971 | `9DA98BDDFC48710999945AB8BECCCE9A70D62784B853409F3E8BDA7C7772C4E3` |
| `P25.0_EditorBuild_final.log` | Succeeded / native 0 | 4 | 2,651 | `1EDD324297456D0CA639192CCDBD040810468ACF62A631D99BAF9BCE5D6E332C` |
| `P25.0_GameBuild_final.log` | Succeeded / native 0 | 63 | 6,457 | `4AB1F5476CD5E9EB57983376F44231DE46B0C8461840D82E75BC7B39DA7C87FE` |

最终二进制：

- `demo_map.exe`：359,734,784 bytes，SHA-256 `2B0A4C888192CE19499F3FE13C08469ED1030C36401BA080E8D04929BB5A87AB`；
- `UnrealEditor-demo_map.dll`：19,008,000 bytes，SHA-256 `8CD5B6F5DD3E58791E307BC9635A4C9596D18311304FCD80E77FF82624293AD5`。

## 8. 静态边界

- 非文档增量：8 files，`+879/-15`；其中 246 行为共享账本 Controller 自动化；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、World 查询、Actor 操作、存档与物品权威：0 命中；
- 没有新增 Manager、Subsystem、Actor、输入资产、Config、Content 或存档字段；
- 只在现有 Host、Router、Session、Controller 内增量扩展；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界与下一步

PASS：共享灵力权威、混合事务顺序、Host/Router 一致性、未完成 Reservation 回滚、身份冲突、精确重放、旧命令过期拒绝和后续神识支付均有无头自动化证明；映射回归 65/0；覆盖门禁、自检和双目标构建通过。

未声明：灵力护盾已经成为可玩的产品功能。本阶段尚未接物理按键、PlayerController、敌方 Impact、期限 Tick、HUD 或真实关卡反馈。P25.1 应在这份共享账本上接入护盾启动与受击生命周期，不得新增第二份灵力余额。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub 交接

基线提交：`924deda74dd72abceaee9c868467122aba8c6c41`。

工作分支：`agent/0.0.10-p25-0-shared-spirit-energy-ledger`。

本阶段仅提交 8 个实现/测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P25.0` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-0-shared-spirit-energy-ledger>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-0-shared-spirit-energy-ledger/Docs/Report/Dev.D.UE.0.0.10.P25.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-0-shared-spirit-energy-ledger/Docs/Log/Dev.D.UE.0.0.10.P25.0.r0_log.md>
