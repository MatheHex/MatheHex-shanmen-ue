# Dev.D.UE.0.0.10.P28.28.r0 Development Log

状态：`P28_28_AUDIT_PASS`。有界调用条件审计；源码零改动，无产品 Red/修复声明。发布基线由包含本文的 Git 提交追溯。

## 1. 基线与保护

2026-09-21T16:43:48.274Z heartbeat；分支 agent/0.0.10-p27-28-formation-scatter-gamemode-composition；HEAD `71b71dd900287fa723e784688300e94094f8ed2a`。入口读取最新 P28.27 Report/Log、P 阶段基线、有限冻结清单与 Git 状态。16:44:10.4307660UTC 的 P28.27 Published 核验通过：1617 输入、103 原用户文件、两份保护文档、六份原件 SHA、136 链接、三个健康本轮执行、17 个专项及两根复用 2760 个独立用例，暂存 0。

保护文档 OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。103 个原文件精确路径/SHA 与 1617 输入沿用 Saved/Automation/P28.26/validation-inputs.json；验证前后检查，未修改该锁或保护文件。

## 2. 静态审计与决定

Source 范围 .h/.cpp 搜索 CreateWorldItemsAtomically：除声明与实现外只有 ItemSubsystem.cpp:1720、1800 两个调用点。1695 的 CreateEnemyLoot 按旧表行数构造请求；1791 的 CreateWorldItem 使用单元素列表。ItemDefinitions.cpp:703 的三个静态表各一行，1326 的 LootTables::Validate 明确检查 Num()!=1 时拒绝。RunLifecycleTests.cpp:29–30 的既有 FixedLootTables 用例调用该验证，本轮完整复跑有成功结果。

ItemSubsystem.cpp:1805 的批量方法预检全部请求，要求数量适合单堆叠，之后才创建权威记录和投影。Rollback 仍先 RemoveWorldBinding、调用 Destroy 但不检查结果，再 RestoreState/清输出；本阶段未改动。当前两个调用点不提供多项成功前缀，但生成后的最终 ValidateInvariants 失败等边界未全部证明不可达。没有用三项人工数组夹具冒充正常产品入口，也没有为其增加恢复状态。行号对应上述未变基线。

核对 Manager 的标记物循环、敌人掉落重载及 ItemSubsystem 的 SpawnBoundWorldActor；普通标记物逐项调用单物品包装，旧三类敌人表走 CreateEnemyLoot。固定/M01 尸体容器与空间包 DiscardSpatialItemBundle 属于不同路径，后者不能因本轮单项分类而豁免多项回滚要求；尸体初始化/强制结束未本轮动态取证。普通 C++ 批量接口仍允许外部/未来多项调用；调用点或表结构改变后本分类失效，必须复核。

## 3. 本轮执行与首次拒绝

忽略目录 Saved/Automation/P28.28/run.ps1 锁定 HEAD 和输入，调用既有 Invoke-Shanmen BuildBoth 与隐藏的 UnrealEditor-Cmd 无头专项；未启动游戏程序。任务 Dev.D.UE.0.0.10.P28.28.r0.Audit，首次证据门拒绝后使用独立 Recovery1，仅续生命周期与尚未执行的拾取专项。

Editor Development：16:49:12.4311673–16:49:14.0048968UTC，原生 0、Target is up to date、零编译动作。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Audit/BuildEditor/20260921T164912392Z-eb998645/stdout.log`，SHA `5D6D15AD309FC0550E8B9DA5A03F03EEF5C83F1486243F6698F6A45458A580E7`。

Game Development：16:49:14.0207701–16:49:15.2107625UTC，原生 0、Target is up to date、零编译动作。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Audit/BuildGame/20260921T164914018Z-2875e4fc/stdout.log`，SHA `F6B13E7B7943A79785CA6354FD4A52E871356C2ACF7F4366E3DD1E8A6973FDE2`。

WorldFocused：16:49:15.6540190–16:49:56.0342357UTC，4 成功/0 失败、4 独立名字、精确结束队列 4、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Audit/WorldFocused/20260921T164915642Z-fcb9d9eb/UnrealEditor.log`，SHA `B6C3B5BBE4AFAF82D208B850EC8F8BE8DD183C93D62D5FFF272B44CFBFEE4E5C`。该组为值类型 World 归属/拾取契约，不动态证明生成 Actor 回滚。

**首次 LifecycleFocused 不完整，不计通过：** 16:49:56.5136510–16:50:16.8482105UTC，run-state 记录原生 0/SUCCEEDED，但仅 14 Success、0 Fail、没有精确队列结束，最后 O 项没有结果；Fatal/Ensure/Unhandled 指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Audit/LifecycleFocused/20260921T164956511Z-fd644d7a/UnrealEditor.log`，SHA `DD87D27335310A3316B44EFA8CA02EFB1C749BCEF03D78271562318BE17AD91D`。exec12908 外层 1，证据门明确拒绝；未确定缺尾原因，不称为产品 Red、源码失败或已查明环境故障。

Recovery1 LifecycleFocused：16:51:21.9224180–16:51:42.2868578UTC，15 成功/0 失败、15 独立名字、精确结束队列 15、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Recovery1/LifecycleFocused/20260921T165121881Z-b690401f/UnrealEditor.log`，SHA `B93BCBE81A05240782A0ADEC5BB228BAA2F3A7779BF938E7E843C958552DA7A9`。仅此完整执行用于生命周期通过结论，不与首次 14 条相加。

Recovery1 PickupFocused：16:51:42.8329447–16:52:03.2411812UTC，PreparedWorldPickupIdentity 1 成功/0 失败、精确结束队列 1、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.28.r0.Recovery1/PickupFocused/20260921T165142830Z-7677225f/UnrealEditor.log`，SHA `E8BABD2EABD0B134D266FCFD79927CB9F48CA4A74C49D857162109623F434521`。exec47870 外层 0；16:52:03.7126669UTC 后置输入锁通过。五个健康执行为双构建加三专项，20 个独立成功用例；没有重跑原已健康的构建与 World 组。

精确三份文档的回归分类重新执行 PASS Changed=3 Rules=0 Required=0 Logs=0。原件 `Saved/Automation/P28.28/regression-coverage.log`，SHA `92637BA9A2DD35B80B09B9D8046527FA74138760EFFB9D7069542C3A7D50A845`。这是文档改动分类，不是产品回归结果；映射/脚本未改动，未宣称重跑其 537 项自检。

## 4. 明确复用的完整根

以下两份来自 **P28.26 Final**，不是本轮新跑；1617 产品/验证输入一致。三个本轮专项的 20 个独立用例属于这 2760 项的子集，不累加。

旧根：2026-09-21T13:18:30.6573583UTC 完成，1330 成功/0 失败、精确队列 1330、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/LegacyFullRoot/20260921T131715213Z-f5b5d741/UnrealEditor.log`，SHA `306CE55AFDABD29E890EB351F093EA5CE46BC4048D72C7429335C1FB590A140F`。

新根：2026-09-21T14:48:50.0391969UTC 完成，1430 成功/0 失败、精确队列 1430、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/ShanmenFullRoot/20260921T131830947Z-45d20b29/UnrealEditor.log`，SHA `8A0640A07E8785FE2C7F3A287B97A45E396718E2350742C5B89820C7EC68A19B`。仍不是 FZ-1/2 关闭后的最终冻结验证。

## 5. 精确交接

仅有限冻结索引和新增本 Report/Log 三路径进入本阶段提交；原件只保留本地，不宣称已上传 GitHub。发布门检查输入/用户文件哈希、九份原件 SHA、相对链接、五个健康本轮执行、首次不完整证据、两套完整复用根、精确路径集合与 git diff --check。不改用户两份 OverallReadiness，也不暂存 103 个原未跟踪文件。

17:00:15.5804114UTC 发布前核验实际通过：1617 输入、103 原文件、两份保护文档、九份原件 SHA、141 个相对链接、五个健康本轮执行、首次不完整原件及两套复用根；本轮 20 项均属于完整根的 2760 个独立用例。暂存 0、105 个未跟踪路径精确匹配原 103 加新增 Report/Log，差异空白检查通过且无残留 UE-Cmd。LF/CRLF 提示不是空白错误。

FZ-1/2 仍开放；下一步只核对既有清单的真实入口，不因“继续”新增通用系统。未进入玩法/UI/F 阶段，未宣布整体完成或暂停自动化。

- [Report](../Report/Dev.D.UE.0.0.10.P28.28.r0_report.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
