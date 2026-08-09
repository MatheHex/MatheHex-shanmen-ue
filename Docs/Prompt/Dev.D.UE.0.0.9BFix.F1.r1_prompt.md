# Dev.D.UE.0.0.9BFix.F1.r1

## 任务身份

- 项目：Dev.D.UE.0.0.9B（同一既有工程；不得新建项目）
- 修复线：0.0.9BFix
- 阶段：F1.r1——宗门出战、M01 激活与仓库回退的真实验收返工
- 任务编号：Dev.D.UE.0.0.9BFix.F1.r1
- 前置：已接受 `0.0.9B.P1—P21`、`0.0.9BFix.P1—P3`；`0.0.9BFix.F1` 的结论为 `NEEDS_0_0_9BFIX_F1_REWORK`
- 执行文件：`Dev.D.UE.0.0.9BFix.F1.r1_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9BFix.F1.r1_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：保留 F1 已完成的范围内修复，补齐 F1 尚未完成的真实运行验收；不是新功能、不是新项目、不是 P 阶段返工。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效依据

仅以最新 `0.0.9B` 大纲、活动工程、已接受的 `0.0.9B.P1—P21`、`0.0.9BFix.P1—P3`，以及本次直接前置的 `0.0.9BFix.F1_report.md` 为依据。

任何早期项目文档、旧规则、旧页面壳、旧启动链、旧库存语义或旧版本命名均不具备本任务的架构、范围、验收或命名效力。不得读取、采用、恢复或重新接入它们。

### 2. F1 已取得但尚未最终验收的事实

以下事实必须保留并在本次复核；它们不等于 F1 已通过：

1. F1 已用真实 Editor 运行复现并修复一处 P5 锁定根因：历史终局 `ActivationFailure` tombstone 保留其历史 `ActiveRunId`，但 `ReadyForPreparation` 才是 P5 可整理性的唯一 liveness 判据。不得删除、清空、伪造或重写该历史记录以取得通过。
2. 该修复只涉及 `CodeBOutOfRaidProfile` 的 P5 入口 gate、窄回归断言和只读审计日志；必须保留 Code B 为物品唯一可变真值，且不得新增第二库存、Code A mirror、P5/P6/P8 双写或 Widget 真值。
3. F1 已真实启动新的默认宗门 Host 并验证 `AtSect` 下可打开 owner-matched P5 投影；但尚未真实完成任何 P5 整理、Submit Expedition、`StartAttemptId`、`RuntimeReady`、`InRun`、P5→P6 bridge 或新的技术失败场景。不得把 F1 的静态证据或历史 tombstone 当作这些场景的替代证据。
4. 未 Cook 的 `demo_map.exe` 因缺少 Global ShaderCodeLibrary 无法进入地图／UI。这是未 Cook 开发可执行文件的运行前提限制，不是本次 0.0.9B 启动链的产品结论；本任务不得为了启动它而 Cook、Package 或修改项目去规避 shader 机制。

### 3. 固定框架与产品边界

1. `demo_map0909BFramework`、`demo_map0909BRunStartCoordinator`、`demo_map0909BM01RuntimeAdapter`、`demo_map0909BCodeBItemBridge`、`demo_map0909BSectWidget` 与 `demo_map0909BEditorSupport` 是唯一正式顶层路径。
2. 顶层状态只能是：

       AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                              ↘ TechnicalStartFailure → AtSect

3. Code B P1 Repository 是唯一可变物品真值。P5 是局外仓库／战备同图；P6 只属于已确认的 `OwnerId + RunInstanceId` 局内携行图；P8 只在 Code A 已提交真实终局后结算。
4. 战备与仓库以塔科夫式结构为参考：它们是同一 P5 图的不同位置与投影；明确提交出战前不得移动、锁定、复制、预扣或预建 P6；只有 M01 world、Player 和 input 真实就绪后，既有 bridge 才可一次性交接。
5. P1—P21 的稳定 `ItemId`／`ContainerId`、空间完整图、装备、普通容器、尸体、地面物品、快捷引用、消耗品、确定性 Loot 与 P8 终局语义全部保留。

### 4. 本任务的唯一验收目标

通过真实 UMG 输入完成并留证以下两条链；范围内出现缺陷时可做最小修复后重编译、重跑受影响场景：

    AtSect 宗门 Host
      → P5 仓库／战备同图且可整理
      → 同一用户意图的快速重复提交
      → 唯一 StartAttemptId
      → M01 World／Player／Input RuntimeReady
      → InRun 与一次 P5→P6 bridge

    同一新框架中的受控技术失败
      → TechnicalStartFailure
      → AtSect
      → P5 等价、无 P6／P8／活动 Run 残留
      → 无需重启即可再次整理仓库／战备

本任务不验收完整物品搜取、装备与空间图全覆盖、尸体／普通容器／地面、快捷使用、P8 三种终局、recovery、全量回归、Cook、Package 或最终版本封包。

---

## 下半部分：授权执行内容

### 5. 真实输入与运行路径要求

1. 所有页面点击、拖拽、双击和出战提交，必须由真实运行窗口收到的正常鼠标／键盘输入完成。允许使用人工操作，或能够将真实 OS 输入送入前台 UE 窗口的本地桌面自动化；UMG 必须实际处理命中测试与输入事件。
2. 严禁以直接调用 `StartM01Run`、Widget `OnClicked`、Coordinator／Adapter 方法、反射、控制台强制状态、手工存档编辑、测试 fixture、硬编码 `RuntimeReady`／`RunInstanceId` 或直接写 P5/P6/P8 来替代 UI 操作。
3. 先证明输入通道有效：聚焦真实 UE 窗口，使用实际点击打开一个无副作用的宗门页面并保存窗口截图／日志。此前的 `node_repl exec context not found` 是执行输入通道故障，不得为适配它而修改游戏代码、UI 或状态机；应改用可操作的本地桌面输入方式。
4. 真实运行必须包括：
   - 至少一次默认入口的 Editor PIE／New Editor Window 路径；
   - 至少一次由 Editor 发起的 Standalone Game 窗口路径。

   这两者均须是可见、可输入的真实游戏界面。`demo_mapEditor` 和 `demo_map` 仍须分别原生编译成功；未 Cook 的 `demo_map.exe` 不要求再次作为 UI 验收路径启动，且不得为此引入 Cook／Package。
5. 如果在确认窗口前台后，经过一条正常桌面输入路径和一条允许的替代桌面输入路径仍无法让真实 UMG 收到无副作用导航点击，停止。不得伪造 A—D 证据；报告使用 `BLOCKED`，原因写为 `EXECUTION_HARNESS_UNAVAILABLE`，并附窗口与输入失败的真实证据。

### 6. 保护、快照与测试数据恢复

1. 开始前创建一个完整可恢复快照，至少覆盖 `Source`、`Config`、`Content`、`.uproject`、`Docs` 和当前所有 Code A／P5／P6／P8 持久数据；同时单独记录运行前数据基线的 owner、P5 revision／graph revision／digest、P6 session 数、P8 receipt 数、活动 Run 诊断和测试根 ItemId／完整空间闭包（如存在）。
2. 使用当前真实 P5 数据，禁止创建 Profile、starter、fixture、fake ItemId、复制品、第二库存、Code A inventory mirror 或手工篡改持久数据。
3. C 场景的真实 P5→P6 会改变测试存档时，可在该场景的完整证据已记录、所有相关进程已退出后，仅恢复开始前的持久数据基线，再开始 D 场景。该恢复只用于保护测试数据，不能充当 C 或 D 的产品逻辑证据；恢复后必须证明数据与基线一致、代码修复仍保留。
4. 不得以重启 Editor／游戏作为技术失败后仓库解锁的证明；D 场景必须在同一真实运行会话中完成 TechnicalStartFailure 返回、打开仓库和一次可逆 P5 整理。

### 7. 必须执行的真实场景

#### A. 默认启动与 AtSect 可整理基线

1. 使用默认 0.0.9B 入口运行，确认可见页面是新的宗门 Host、Coordinator 状态为 `AtSect`，且没有历史启动页壳、旧错误文案、活动 Run 假提示或第二库存。
2. 通过真实 UI 打开 Warehouse 和 Loadout，证明它们是同一 P5 图的不同投影。记录同一真实根 ItemId／ContainerId（如可见）及 P5 revision／digest 的对应关系。
3. 使用当前已有且合法的真实物品执行一项可逆的正式 P5 事务：优先装备后卸下，或移动后移回。不得为凑测试而创建物品或修改 schema。记录输入动作、前后 revision、digest、ItemId、ContainerId、空间闭包与 P6/P8 零副作用；图内容、唯一 parent 与持久所有权必须恢复等价。
4. 以真实 UI 验证至少一种无效选择／无效提交被结构化拒绝，且仍保持 `AtSect`、P5 可整理、P6/P8 无写入。按钮禁用、结构化拒绝或界面错误都可作为结果，但不得通过直接函数调用制造它。

#### B 与 C. 同一用户意图的去重与真实 M01 成功链

1. 在有效 P5 LoadoutSelection 下，使用真实窗口对同一 Submit Expedition CTA 做一次快速重复点击／双击。该一次操作同时承担 B 与 C 的真实输入起点。
2. 记录可见 `PreparingStart`／`ActivatingWorld` 状态、同一 `StartAttemptId`、实际 M01 地图／世界与窗口。此阶段 P5 不得提前移动、锁定、复制或预建 P6，P8 必须无写入；仓库只可显示明确的“出战尝试处理中”，不能称为已有活动 Run。
3. 验证只保留一个有效 StartAttempt、一次激活链和最多一次 bridge；不得出现第二个 RunInstanceId、第二 P6 session、重复 P5 扣除、永久 pending 或 UI 重建后的旧 loading token。
4. 只有同一 StartAttempt 的 M01 World、GameMode／World Settings、匹配 Owner 的 PlayerController、Pawn、输入和稳定 Run identity 全部真实就绪后，才可进入 `InRun`。记录实际 `RunInstanceId`、状态顺序、真实 InRun 窗口截图与可定位日志。
5. 验证既有 P5→P6 bridge 只在 `RuntimeReady → InRun` 后执行一次，并从 durable P5 snapshot 复核后建立精确 P6 session。记录 P5/P6 的一次性差异；相同 ItemId／ContainerId 的 parent-child 空间闭包不得 clone、镜像、第二 parent 或被 flatten。
6. 如果 M01 已真实成功但 bridge 因 selection stale、冲突或保存错误拒绝，保留真实 Code A Run 与结构化审计；不得把它写成 TechnicalStartFailure、不得清除真实 Run、不得造 P6 fallback。本任务仅可修复明显的 F1 correlation／时序问题；P5/P6/P8 内部既有缺陷需如实报告。
7. 在成功链证据完整后，按第 6 节保护测试数据。不得为避免清理而把 C 场景伪称为 P8 终局，也不得扩大为 P8 测试。

#### D. 真实技术失败、幂等清理与即时仓库解锁

1. 从恢复后的干净基线，以真实 UI 再次发起新的出战意图，并在新的 Coordinator／Adapter 正式关联链中触发一次相关联的技术失败。
2. 优先使用当前工程中安全的真实 descriptor／配置／world／player／input readiness 失败。若没有安全的既有方式，允许仅在 Development／Editor 中使用一次性、默认关闭的受控 failure injection，且必须同时满足：

   - 它位于新 Coordinator／Adapter 的正式关联边界，并携带真实 UI 提交生成的 StartAttemptId；
   - 它在 RuntimeReady／P5→P6 bridge 前产生一个明确分类的技术失败回执；
   - 它不伪造成功、Run identity、P5/P6/P8 数据或任何产品存档写入，不来自历史路径；
   - 它不含无限 timer、全局 pending flag、自动重试或用户可达的产品 UI／配置／命令；
   - 运行后移除；若保留为本地测试路径，必须完整编译排除于正式入口且静态复查确认不可达。

3. 记录该 attempt 的状态顺序和失败分类。验证 cleanup 只处理同一 StartAttempt：解绑 delegate／timer／loading token，丢弃 transient runtime 数据，最终回到 `AtSect`，没有 P6 active session、P8 receipt、终局分类、活动 Run 或仓库假锁。
4. 不重启 Editor／游戏，立刻通过真实 UI 打开 Warehouse／Loadout，并再执行一项可逆的正式 P5 整理事务。证明 P5 内容与失败前等价，且 P6/P8 零副作用。
5. 若可由同一安全注入或真实 callback 产生晚到／重复回执，验证其被忽略且不会覆盖新的 attempt、重新锁仓库或修改 P5/P6/P8。不得通过直接调用 Coordinator 伪造此结果。

### 8. 仅限本任务的修复范围

1. 仅可修复 F1.r1 真实复现的默认入口、M01 descriptor／配置、World lifecycle callback、delegate 解绑、StartAttempt correlation、Controller／Pawn／input ready 判定、TechnicalStartFailure cleanup、Coordinator gate、UI 只读投影、P5→P6 bridge 调用时序、P5 terminal-liveness gate 或 Editor 诊断问题。
2. 每一处代码／配置修复必须记录复现症状、根因、修改文件、为何不改变 Code A 战斗／生命／死亡／终局权威，以及为何不改变 Code B P5/P6/P8 物品权威。
3. 禁止新增或改造战斗、地图、美术、敌人、装备数值、物品效果、随机 Loot、商店、经济、制作、自动拾取、第二地图、多人、网络同步、第二库存、P6 preview、Widget authority、fixture、starter 或历史系统复接。
4. 若修复需要重置、迁移、重掷、删除或重建 P1—P21 的历史物品／容器／世界掉落／尸体／快捷绑定，或需重写 Code A／Code B 权威，停止并报告 `BLOCKED`。

### 9. 构建、静态复核与证据

1. 所有范围内修复完成后，以及任何临时注入被移除或编译隔离后，必须分别编译：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

   两个命令均须记录最终 native exit code `0`。
2. 保存并引用来自真实运行的可读截图／日志：默认 AtSect、Warehouse／Loadout 可整理、出战处理中、真实 InRun、技术失败回宗门后的可整理状态。截图不得来自 mock、手工编辑、文本替换或 debug-only 假页面。
3. 静态复核新的 Host／Coordinator／Adapter 仍是唯一正式路径：一个 StartAttempt creator、一个产品 StartM01Run caller、一个 bridge observer；无第二库存、Code A mirror、P5/P6/P8 双写、未清理测试注入或历史路径重新可达。

### 10. Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix.F1.r1_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. F1 的部分修复如何被保留，以及最终复核结果；
2. 完整快照、测试数据基线、恢复动作与最终恢复结果；
3. 使用的真实窗口输入路径及其无副作用导航证明；
4. Editor PIE／New Editor Window 和 Standalone Game 的实际运行证据；
5. A 的同图投影、可逆 P5 事务、无效提交与 P5/P6/P8 差异；
6. B/C 的双击输入、唯一 StartAttempt、状态顺序、RuntimeReady、RunInstanceId、一次 bridge 与空间图一致性；
7. D 的真实失败触发、关联回执、幂等 cleanup、AtSect 返回与同一会话即时整理证据；
8. 所有范围内修复、根因、文件、重新编译与复测；
9. bridge 拒绝、空 P5、输入控制器或其他未闭合事实；
10. 两个构建命令、关键结果与最终 native exit code；
11. 仍未执行的后续 F 债务：完整仓库／装备／空间图操作，普通容器／尸体／地面搜取与转移，快捷使用，P8 三种终局，recovery，全量回归，Cook、Package 与最终版本验收；
12. 确认未新建工程、未建立第二库存、未重新接入历史规则，且未丢弃、重置或迁移 P1—P21 成果。

仅当 A—D 均以真实运行通过、输入证据完整、成功链只在 RuntimeReady 后一次 bridge、失败后无 P6/P8／活动 Run 残留并即时恢复仓库整理、所有范围内修复重编译复测通过时，使用：

    READY_FOR_0_0_9BFIX_F2_PLANNING

若真实运行仍暴露本任务范围内可修复的启动／回退问题，使用：

    NEEDS_0_0_9BFIX_F1_REWORK

若真实 UMG 输入无可用执行通道，或必须通过范围外权威重写、历史系统复接、物品重置／迁移或第二库存才能继续，使用：

    BLOCKED

完成后不得自动开始 `0.0.9BFix.F2`、常规 P、其他 F 或任何未授权任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.F1.r1","file":"Dev.D.UE.0.0.9BFix.F1.r1_report.md"}
