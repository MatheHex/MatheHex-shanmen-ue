# Dev.D.UE.0.0.10.P27.30.r0 Report

## 1. 结论

P27.30 完成 Run 结束流程的一项结构修复：逻辑 Host/Coordinator 已结束，而 World 销毁拒绝时，保留既有成功结果和剩余 owner，重试只继续未完成的清理。不再把 World/Timeline 拒绝或无完成证明的 orphan 直接 Reset 成空状态。

旧实现上的两项新测试先复现失败；修复后 ControlledWeaponWorldLifecycle 全组 3/3、完整 Shanmen.0_0_10 根组 1,418/1,418、五组相关旧系统 123/123 均通过。Editor/Game 原生构建退出码均为 0，回归映射自检 504/504。

这是一项底层生命周期闭合，不是整体架构冻结或可玩版本验收。没有新增物理输入、UI、玩法数值、地图或内容资产。

## 2. 基线与真实缺口

- 实现承接 P27.29：`58e696483119137cd4ed3b267b615af812cc7f39`。
- 本阶段提交父基线：`b3d7c026c513d8b3e03a56e47674569eb06f880d`；中间提交只增补总体报告，没有改变本阶段五个源码/映射文件。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。

旧顺序先结束逻辑 Run，再清理 World 和 Timeline。World 拒绝销毁后会被 Reset；World 生命周期还在调用 Destroy 前关闭碰撞/表现，使本应保留的 owner 失去再次校验的条件。Timeline 拒绝也被 Reset。下一次结束看到 inactive Coordinator 时，orphan 分支又会清空残留，既丢失失败证据，也可能把重复尝试变成表面成功。

单纯删除 Reset 仍不能恢复已经结束的 Coordinator，必须区分“逻辑结束已成功”和“剩余清理尚未完成”。

## 3. 最小实现

1. 在清理前检查非空 Timeline 和御器 World owner 的有效性及 Run 身份；不匹配时先拒绝，避免消耗清理前缀。
2. 使用一个私有 optional 保留原有 `Fdemo_mapShanmenControlledWeaponRunEndResult`，不新建 Authority、身份工厂或持久协议。
3. 私有 `TryFinishCombatRunRetirement` 校验该成功结果，依次确认 Formation、清理 World、结束 Timeline，全部成功后才清空路由/采样状态及 optional。
4. 存在待完成清理时，下一次 Release 只进入该后半段；同步 Actor 销毁回调重入会被私有 guard 拒绝。
5. 待完成期间禁止激活新 Run，并暂停既有 Run 时间线及御器表现更新块；不增加或调整玩家 UI 功能。
6. World 的 Destroy 接受后才关闭原有碰撞/表现；真实拒绝时保持原 owner 可验证。
7. 没有成功逻辑结束结果的 orphan 分支只拒绝，不再通过大范围 Reset 制造空状态。

五个实现/映射文件合计 291 行新增、71 行删除，包含移动代码、注释和 177 行测试改动，不代表同等数量的新业务逻辑。

## 4. 非零状态与恢复证明

| 情形 | 连续两次失败应保持 | 校正后的证据 |
|---|---|---|
| UE GameWorld 真正拒绝销毁飞剑 Actor | 原 Actor、Run、碰撞/表现、非零 Timeline；Host/Coordinator 已结束，成功结果仍在 | 同一结束请求继续清理；仅一个销毁回调，嵌套 Release 不重复完成 |
| 非空 Timeline 属于外来 Run | 原 Timeline 身份及 tick、原 World/表现、仍活动的逻辑 Host/Coordinator；不制造结束结果 | 恢复原 Timeline 后正常结束，重复结束成功 |
| 无活动 Coordinator、无成功结束结果，但 Timeline 非空 | 两次都拒绝；原身份与非零 tick 保留 | 不把未知来源残留当作合法成功 |

恢复测试先通过真实物品 Authority 的整备和 Run 启动获得飞剑，执行 Launch 并推进非零时间，捕获非空表现快照；最后核对持久物品快照前后完全相等。不是以全新空对象的相等断言替代保留证明。

Destroy 拒绝通过瞬态测试 Actor 的 simulated-proxy role 触发 UE 既有销毁规则，再恢复原 role；没有修改引擎、Windows 权限或实际网络功能。外来 Timeline 的恢复同样使用夹具已知原值，不声称生产代码可以自动修正任意错误身份。

## 5. 准确边界

- optional 只在同一个 GameMode 内保留，不是跨进程、跨地图重建或独立磁盘 checkpoint。
- 更早已成功的护盾、治疗、剑法等清理前缀不回滚；不是整函数原子事务。
- 新测试直接覆盖真实销毁拒绝、外来 Timeline 和非空 Timeline orphan。源码还加入 World owner 预检，但不把所有身份损坏或 orphan 组合计作已单独测试。
- Actor 意外消失等未知状态仍失败关闭，不自动生成替代对象或伪造完成结果。
- 正常立即完成保留原 RunReleased 汇总；恢复后完成使用 RunRetirementCompleted。没有重建此前局部变量中的完整前缀计数，不宣称两条路径的旧汇总完全相同。

## 6. 自动化结果

| 范围 | Success | Fail | 原始日志 SHA-256 |
|---|---:|---:|---|
| 旧实现失败复现 | 0 | 2 | `7B5C24326F6794F00028E4144890EA9C10A48122AF2549FC1119B88010B7F175` |
| ControlledWeaponWorldLifecycle 全组 | 3 | 0 | `EAC0FE6A414E04E5F21521037BD431146CEE9A7EE4FD4FEF5F107E0181C5C926` |
| demo_map.V3.Attributes | 4 | 0 | `26DE3E38906FC2F508637B3055D28DF321B7E8AB179068FC05A8FBB7D794C028` |
| demo_map.EnemySkillFramework | 44 | 0 | `BE0949DC222E1164EFEE37F1A351EC26AE0787308533CC15B2A4F5A23C018480` |
| demo_map.V2RangedCompatibility | 22 | 0 | `46404EBF6FE6A14E96B19882D54D994D9498621DA2BFE19A48C763120F0120D7` |
| demo_map.ItemUseAndArmor | 46 | 0 | `5E1196988919FB6E7BC75F9D750FB4B25C2A4A8AF52023F4A4868A0BF75BECE4` |
| demo_map.P4.Hotbar | 7 | 0 | `DD34A53BBE298F8B4254CF866A79E7241F37DDAFE1D3BB6D774EFB8EC82FE858` |
| Shanmen.0_0_10 完整根 | 1418 | 0 | `F402AF7DE96A5AFE92464D571E1426DBBD22C46E06FE667658E2E3E69F9C724C` |

最终各组原生退出 0、队列清空，Fatal/Unhandled Exception/Ensure 计数为 0。完整根测试主体从 01:40:05.815 至 02:43:40.986 UTC，约 1 小时 3 分 35 秒；相较 P27.29 增加两个注册测试。3 项专项包含在根组中，不能相加为独立测试总数。

失败复现进程原生退出也是 0，但两个 Automation Result 为 Fail；原日志完整保留，未作为通过证据。启动阶段既有 13 条 LogAutomationTest Condition failed 文本与上一阶段相同，早于本次选定测试；不宣称原日志零 Error，也未删改或屏蔽它们。

## 7. 构建与改动驱动回归

- 最终 Editor：Succeeded，0.88 秒，原生 0；SHA-256 `5BC9382A911FC01ED745222F3A0173BD65CC28C7AB35A832E5E56A8B43B010B5`。
- 最终 Game：Succeeded，139.03 秒，原生 0；SHA-256 `FAC9FD874ADD58ECACFBB8F7B2C5F924397AFBB13B50A328A161089CBA749C58`。
- 映射自检：504/504 PASS；SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。
- M01GameMode 增加完整 ControlledWeaponWorldLifecycle 组；PreparationAdapterTests 对应 ItemProductAdapters，因此额外执行旧 Hotbar 7 项，不能沿用上一阶段四组 116 项作为全部范围。
- 五文件初始覆盖通过；最终七文件覆盖：PASS Changed=7 Rules=3 Required=88 Logs=6，SHA-256 `3B1312041B97D6B31C71DF369241D0D25E1EE26286C4BE4B2572DB68F5C894DC`。88 是映射要求的组数，不是新增测试数量；原日志路径见 Development Log。
- 串行验证只运行一份；五文件在构建、各组测试和覆盖后均按原字节 SHA-256 复核，没有测试期间源码漂移。

## 8. 后续收尾

P27.29 列出的这组 World/Timeline/orphan 结束风险已得到本阶段的最小修复及具体失败证明。下一步应汇总现有入口、唯一权威与生命周期索引，做冻结前总审计，而不是因本阶段通过就添加更多恢复包装层。

整体冻结必须另行核对已批准契约与现有能力的对应关系，区分真正会导致返工的结构缺口和留给 F 阶段的地图、资产、性能与玩家体验验收。本报告不替代该总审计，也不以测试数量推算游戏完成率。

## 9. P/F 边界

仅底层源码、测试、回归映射、Editor/Game 编译和 UnrealEditor-Cmd 无头自动化。未接物理玩家输入，未修改正式地图、内容资产、数值、手感、敌人、关卡或 UI；未启动 Editor UI、PIE、Standalone、产品 exe，未做截图、Smoke、Cook、Package。

## 10. 交接范围

本阶段仅提交五个实现/映射文件与本 Report、Development Log，共七文件。103 个既有未跟踪用户文件保持不变。原始 Saved 日志留在本地，GitHub 文档提供路径与原字节哈希，不冒称原始日志目录已上传。

- [Development Log](../Log/Dev.D.UE.0.0.10.P27.30.r0_log.md)
- [总体就绪度报告](Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)
- [上一阶段 P27.29](Dev.D.UE.0.0.10.P27.29.r0_report.md)
