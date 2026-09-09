# Dev.D.UE.0.0.10.P24.1.r0 Report

## 1. 结论

P24.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮没有再增加一层剑气业务封装，而是把 P24.0 的物理输入测试接入真实局内物品、属性、Combat Run、行动仲裁和投射物权威。无头产品链暴露并修复了一个生产级事务顺序缺陷：新意图原先会在路由回执生成前公开到控制器，行动授权同步读取占用状态时会把这个半完成意图判为无效，从而拒绝真实首发剑气。现在新意图先以私有局部值完成同步路由，取得回执后再原子提交。

```text
Sword Qi physical product loop:          5 Success / 0 Fail
Sword Qi controller regression:          4 Success / 0 Fail
Complete Shanmen.0_0_10 regression:  1,277 Success / 0 Fail
Complete demo_map regression:        1,330 Success / 0 Fail
Changed-file regression coverage:       PASS (3 paths / 2 rules / 35 groups)
Regression gate self-test:               PASS 442/442
Game + Editor Development:               PASS (both native 0)
git diff --check:                         PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实键盘、截图、Smoke、Cook 或 Package。本轮证明的是最终编译代码上的无头物理脉冲、真实运行时权威、投射物创建、冻结重试和清理闭环，不声明视觉、碰撞命中、动画、音效或手感已经人工验收。

## 2. 真实产品链

`IssueRetryAndLock` 现在建立真实 `Udemo_mapItemSubsystem` 与 `Udemo_mapAttributeComponent`，并执行以下链路：

1. 绑定玩家 Pawn、Health 与 Attributes；
2. 开始一个真实局内 Run；
3. 添加并装备 `TrainingBlade` 到正式 `WeaponSlot`；
4. 把 `Primary01` 设为 6，验证最终 `AttackPower` 为 8；
5. 用同一 RunId 启动 CombatRunCoordinator、SwordQiProductController 与 CommandEventOwner；
6. 通过正式 `B` 键绑定进入 GameMode 产品路由；
7. 验证授权物品实例、冻结攻击力、起点、方向、启动回执和真实在飞投射物；
8. 按产品生命周期中断、退役并最终结算 Run。

这条测试不注入合成 Controller Result，也不绕过 GameMode、行动仲裁、装备授权或属性读取。

## 3. 首发剑气缺陷与修复

原实现对新意图采用“先加入 `CapturedIntents`，再调用 `Session.TryRoute`”。`TryRoute` 在真正发射前同步调用 GameMode 行动授权；GameMode 又会通过 `CapturePlayerActionOccupancy()` 调用同一个 Sword Qi Controller 的 `TryAppendOccupancy()`。

此时公开的意图还没有 `LastRoute` 回执，Controller 的完整不变量检查因此失败，生成 `InvalidOccupancySnapshot`，真实产品路由被拒绝为 `ActionGateRejected`。此前控制器单测总是向仲裁器传入人工构造的空占用快照，所以没有覆盖这条自读取链。

修复后的顺序为：

1. 捕获装备、最终攻击力和不可变启动命令；
2. 保持新意图为局部私有值；
3. 执行同步 Session 路由与真实占用投影；
4. 得到完整 Route 回执；
5. 把意图和回执一起提交到 `CapturedIntents`；
6. 再执行提交后不变量检查。

控制器单测的授权闭包同步改为从 Controller 自身投影 Occupancy。这样若以后重新公开半完成状态，聚焦控制器测试会直接失败。

## 4. 冻结重试证明

第一次物理按键成功创建一个 `InFlight` 剑气投射物。投射物仍在飞行时第二次按键走真实产品路由并自然返回 `HostBusy`，CommandEventOwner 保存第二个事件的冻结请求；测试不再制造合成繁忙结果。

随后测试退役第一枚投射物，并同时改变：

- 玩家位置；
- 自动化瞄准方向；
- `Primary01`，使实时攻击属性不再等于首次采样值。

第三次按键由 availability router 自动选择 Retry。重试复用第二次事件的同一命令身份、冻结起点、冻结方向和攻击力 8，不重新读取已经变化的空间或属性权威，并创建第二枚真实投射物。该投射物完成中断与终态退役后，Controller 保持有效。

## 5. 输入锁与清理

两次新事件被命令所有者提交，Retry 复用第二次事件，不增加第三个事件身份。活动 UI 结算锁下再次按键会在产品访问前失败关闭：事件序号保持为 3、没有新投射物，也不改变 availability 投影。

测试最终按顺序结束 CommandEventOwner、SwordQiProductController 与 CombatRunCoordinator，再通过真实 ItemSubsystem 请求 Death settlement。结束摘要验证 2 个已提交事件、2 个已捕获意图和 2 个已处理命令，没有遗留飞行、终态回执、冻结重试或 Run 所有权。

## 6. 首次失败证据

| 阶段 | 结果 | 精确发现 | 修复 | SHA-256 |
|---|---:|---|---|---|
| 初始产品链 | 4/1 | transient preview World 未把显式 Controller 暴露给 `GetFirstPlayerController()` | 测试夹具通过 `World->AddController` 建立与产品相同的 World 权威 | `75B2E92EE43F23D42FE8267E73A1EA1F3CDCFED761D9DA7BB801FF12B54BF121` |
| 诊断复查 | 4/1 | `world_player=0`、`mode_pawn=0`，产品路由不可用 | 保留诊断日志并补齐 World Controller 注册 | `5F5FEAFBE5C0E4439AFC0FABC9CF86EDA1160ACD249EE6D1FEDD2100746A450C` |
| World 绑定后 | 4/1 | 装备和攻击力已正确为 8，但行动仲裁报告 malformed Host occupancy | 修正新意图的路由后原子提交顺序 | `14C3C1C978B977B09A7F7BBA217022543490A6C42ABB87F5FEEFFBE28C7F7085` |

失败日志保留在 `Saved/Codex/P24.1`，没有覆盖、美化或纳入 Git。

最终物理产品链为 5/0，日志 268,837 bytes，SHA-256 `619F7FF745B059ED15FA7651B432A069DA40E1A8DAC7E92B4B2400533327AD42`。控制器聚焦回归为 4/0，日志 265,456 bytes，SHA-256 `33E51799219DA8DA43839D81966D17B142ED0B3EAD07BD1BAC56E9B5F1C1E143`。

## 7. 全量回归与改动映射

| Evidence | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 72m04s | 1,947,232 | `B867E6858C2D2F79DC8AE6F60FDABB1974F4995FE48E75DF4881FACF879175DC` |
| `demo_map` | 1,330 | 0 | 约 37s | 1,655,487 | `62B33E0CFA5CFB1C48352497292ED3D84CD73C6C98AF26D8C803425B1572FE0F` |

两棵树合计 2,607/0。两份日志均包含唯一 RunTests 命令、0 Fail、0 fatal/unhandled/ensure、UE 5.8 成功终止标记和原生退出码 0。

改动映射结果为 `PASS Changed=3 Rules=2 Required=35 Logs=2`。覆盖日志 4,251 bytes，SHA-256 `C035CCA55D520C999E06E178B118119978AF5B7084FD0D96A2AB910D6D2465C4`；覆盖器自测为 442/442，日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

| Evidence | Result | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P24.1_EditorBuild_final.log` | PASS / native 0 | 4 | 2,337 | `986A3929AA83C2A5F5E5FA92AEFEB85302A6ABDFF9E28027230157E8FA19922A` |
| `P24.1_GameBuild_final.log` | PASS / native 0 | 5 | 2,340 | `D3328CC13BBBAE3DA2DC9D9D778A98DB722700B26F523E2161EFD9FDF946476B` |

最终 `demo_map.exe` 为 359,678,464 bytes，SHA-256 `BADB820D2DA1B82762CAA6C9677BCB58744B1C6C72A609B622E4B3B5311B3580`；`UnrealEditor-demo_map.dll` 为 18,942,976 bytes，SHA-256 `AD13A4B8A920F13C9B5558E80FB5424AFE6C22FCCC1838A93FE0AE7CA63EB142`。

非文档增量为 3 个文件、`+208/-66`：生产代码 1 个文件 `+14/-3`，测试 2 个文件 `+194/-63`。`git diff --check` 通过；没有新增模块、资产、UCLASS/USTRUCT、Actor、Subsystem、输入动作或持久玩法状态。

## 9. P/F 边界

PASS：真实 ItemSubsystem Run；真实 TrainingBlade 装备实例；最终 AttackPower 8；正式物理绑定到 GameMode；真实行动仲裁；真实投射物创建；自然 HostBusy；冻结 Retry 不重采样；UI 锁不消费身份；完整清理；聚焦 9/0；两棵全树 2,607/0；改动映射、自测与双构建。

未声明：真实键盘设备、玩家现场装备流程、渲染、碰撞命中、敌人伤害、动画、特效、音效、帧时序、HUD 布局或手感已经验收。P24.2 若继续剑气，应先审计现有 WorldDelivery 是否已经从这条物理产品链覆盖到实际目标 Impact；只补最短缺口，不新建 Sword Qi Manager。

## 10. 提交边界与 GitHub

基线提交为 `dd0667942f27ac87370826ad9ecd67ecc9166a12`，工作分支为 `agent/0.0.10-p24-1-sword-qi-product-loop`。本阶段只提交 1 个生产文件、2 个测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.1` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-1-sword-qi-product-loop>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-1-sword-qi-product-loop/Docs/Report/Dev.D.UE.0.0.10.P24.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-1-sword-qi-product-loop/Docs/Log/Dev.D.UE.0.0.10.P24.1.r0_log.md>
