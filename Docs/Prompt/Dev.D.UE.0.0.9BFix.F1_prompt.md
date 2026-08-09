# Dev.D.UE.0.0.9BFix.F1

## 任务身份

- 项目：Dev.D.UE.0.0.9B（同一既有工程；不得新建项目）
- 修复线：0.0.9BFix
- 阶段：F1——宗门出战、M01 激活与仓库回退的真实验收及范围内修复
- 任务编号：Dev.D.UE.0.0.9BFix.F1
- 前置：已接受 0.0.9B.P1—P21、0.0.9BFix.P1、P2、P3
- 执行文件：Dev.D.UE.0.0.9BFix.F1_prompt.md
- 报告文件：Dev.D.UE.0.0.9BFix.F1_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 任务性质：0.0.9B 修复线的首个真实运行验收；允许只为本任务发现的启动／回退链缺陷进行最小修复，不是新功能开发。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效依据

仅以最新 `0.0.9B` 大纲、活动工程、已接受的 `0.0.9B.P1—P21` 和 `0.0.9BFix.P1—P3` 为依据。

不得读取、采用、恢复或接回任何历史项目、历史规则、历史页面壳、历史启动链或历史库存语义。它们不具备本任务的架构、范围、验收或命名效力。

### 2. 已接受的运行框架

1. `demo_map0909BFramework`、`demo_map0909BRunStartCoordinator`、`demo_map0909BM01RuntimeAdapter`、`demo_map0909BCodeBItemBridge`、`demo_map0909BSectWidget` 与 `demo_map0909BEditorSupport` 是唯一正式顶层路径。
2. 顶层状态只能是：

       AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                              ↘ TechnicalStartFailure → AtSect

3. Code B P1 Repository 是唯一可变物品真值：P5 为局外仓库／战备图，P6 仅属于已确认的 `OwnerId + RunInstanceId` 局内携行图，P8 只在真实终局后结算。
4. 战备与仓库遵循塔科夫式结构参考：两者是同一 P5 图的不同位置与投影；提交出战前不移动、锁定、复制、预扣或预建 P6；只有实际世界、玩家与输入均确认就绪后才可由既有 bridge 一次性交接。
5. P1—P21 已有的稳定 `ItemId`／`ContainerId`、空间道具完整图、装备、尸体、普通容器、地面物品、快捷引用、消耗品、确定性 Loot 和 P8 终局语义均必须保留。

### 3. 本次必须给出真实证据的故障

此前界面暴露了两项同源风险：出战激活没有正确完成时，宗门未可靠回到可整理状态；仓库可能错误呈现为仍有活动局内会话。

`BFix.P1—P3` 已完成静态框架修复和双目标编译。本任务必须通过真实 Editor／Game 运行证明：

- 默认启动确实落入新的宗门 Host；
- 出战成功只在真实 M01 RuntimeReady 后进入 `InRun`；
- 技术失败只回到 `AtSect`，不创建／残留 P6、P8 或活动 Run；
- 回到 `AtSect` 后仓库和战备立即恢复正常整理能力；
- 页面不再由任何历史启动壳或历史活动状态支配。

### 4. 本任务不是全版本回归

F1 只验收顶层启动、仓库／战备 gate、M01 成功交接与技术失败回退。完整物品搜取、装备操作、空间图转移、尸体／普通容器、地面、快捷使用、全部 P8 终局、恢复、Cook、Package 和全量回归仍留给后续 F 任务。

---

## 下半部分：授权执行内容

### 5. 单一授权目标

在同一工程内，真实运行并验收以下一条完整链：

    AtSect 宗门 Host
      → P5 仓库／战备读取与可整理状态
      → 单次 Submit Expedition
      → StartAttemptId
      → M01 真实 World／Player／Input RuntimeReady
      → InRun 与一次 P5→P6 交接

并真实验证其对偶链：

    同一启动链中的受控技术失败
      → TechnicalStartFailure
      → AtSect
      → P5 不变、无 P6／P8／活动 Run、仓库立即可整理

如发现缺陷，只可在这条链、其启动配置、Coordinator／Adapter 的 callback 关联、UI 的只读状态投影、或 Code B bridge 的既有窄接口中进行最小修复。完成修复后必须重编译并重新运行受影响场景。

### 6. 运行前保护与可重复性

1. 开始前建立可恢复快照，至少覆盖 `Source`、`Config`、`Content`、`.uproject`、项目文档和当前 P5／P6／P8 持久数据。报告中写明快照位置与覆盖范围。
2. 记录测试 Owner、初始 Coordinator 状态、P5 owner/revision/graph revision/digest、测试根 `ItemId`／完整空间闭包（如有）、现有 P6 session 数、P8 receipt 数与活动 Run 诊断。不得输出不必要的私人数据或完整存档内容。
3. 测试不得以新 Profile、starter、fixture、fake ItemId、复制品、第二库存、Code A inventory mirror 或手工篡改 P5/P6/P8 作为前提。
4. 可使用当前实际 P5 数据；若需要通过真实既有终局返回宗门，必须走正式终局链。若测试会改变持久物品状态，必须在完成前通过正式链恢复，或从开始前快照精确恢复并验证恢复后 P5/P6/P8 与基线一致。不得把快照恢复当成产品逻辑的替代品。
5. 不得通过伪造 `RuntimeReady`、硬编码 `RunInstanceId`、静态成功 flag、绕过 Adapter、跳过 bridge 或直接改写 Coordinator 状态来取得通过。

### 7. 必须执行的真实场景

所有场景必须使用默认 0.0.9B 入口运行。可使用 Editor PIE、Standalone、Editor Launch 或 Development Game；至少包含一次真实 Editor 路径和一次真实 Game target 路径。每项记录尝试时间、`StartAttemptId`、最终 Coordinator 状态、`OwnerId`、必要时 `RunInstanceId`、P5/P6/P8 变化和可定位的日志／截图证据。

#### A. 默认启动与宗门可整理基线

1. 启动后必须进入新的宗门 Host，并显示 `AtSect` 的当前状态。
2. 打开仓库与战备页面，确认它们读取同一 P5 物品图；不得出现第二库存、临时 loadout、历史启动页壳或活动 Run 假提示。
3. 在不改变物品的前提下，确认 `AtSect` 下仓库写 gate 为可用；若当前 P5 有可操作物品，执行一项可逆的正式整理事务（移动后移回，或装备后卸下）以证明 UI 只提交 P5 正式事务。记录前后 revision、digest、`ItemId`／`ContainerId` 和空间图闭包不变。
4. 空 P5、失效选择或 Owner 不一致时，提交必须被结构化拒绝，且仍保持 `AtSect`、P5 可整理、P6/P8 零副作用。可在不修改持久数据的受控条件下验证其中至少一项。

#### B. 单次意图与重复点击

1. 在有效 P5 LoadoutSelection 下，对同一用户意图执行快速重复提交／双击。
2. 必须只产生一个仍有效的 `StartAttemptId`、一次世界激活和最多一次 P5→P6 bridge；后续点击只能显示同一 attempt 的处理中／已决状态。
3. 不得出现第二个 `RunInstanceId`、第二个 P6 session、重复 P5 扣除、仓库永久 pending，或 UI 重建后遗留旧 loading token。

#### C. 真实 M01 成功链

1. 使用有效 descriptor 和真实 M01 运行时对象发起一次出战。
2. `PreparingStart` 与 `ActivatingWorld` 期间，仓库可以显示“出战尝试处理中”，但不得把 pending attempt 呈现为活动局内会话；P5 不得提前移动／锁定，P6/P8 必须尚未产生写入。
3. 只有同一 `StartAttemptId` 的 M01 World、GameMode、World Settings、匹配 Owner 的 PlayerController、Pawn、输入与稳定 Run identity 都真实就绪后，才能转为 `InRun`。
4. 确认既有 Code B bridge 仅在该转移后运行一次，并重读 P5 durable snapshot 后才建立精确的 P6 session。验证同一批 ItemId／ContainerId 的 parent-child 关系没有 clone、镜像或第二 parent。
5. 记录进入 InRun 后的实际页面／世界、状态机顺序、P5/P6 的一次性变化和 UI 表现。不得把 bridge 拒绝伪装为 world activation failure；若发生 bridge 拒绝，保留真实 Code A Run 和结构化审计，并按第 9 节处置。

#### D. 真实技术失败与返回宗门

1. 必须在新的 0.0.9B Adapter 链内触发至少一种真实的、相关联的技术失败（例如 descriptor/config/world/player/input readiness 的实际失败），并让其携带同一 `StartAttemptId` 回到 Coordinator。
2. 若工程没有安全的既有故障触发方式，只允许在 Development／Editor 运行时采用一次性、显式的、默认关闭的受控故障注入：

   - 注入点必须位于新 Coordinator／Adapter 的正式关联边界，不得来自历史路径；
   - 不修改 P5/P6/P8、不伪造成功、不伪造 Run identity、不写入任何产品存档；
   - 只可产生一个明确分类的技术失败回执；不得使用无限 timer、重试循环或全局 flag；
   - 运行结束后必须移除该临时注入实现，或将其限制为编译排除的本地测试路径；正式默认入口不得保留可触发它的 UI、配置或指令。

3. 验证失败 cleanup 只影响对应 attempt：解绑 delegate／timer／loading token，丢弃 transient Runtime 资料，确认无 P6 active session、无 P8 receipt、无终局分类、无活动 Run，最后回到 `AtSect`。
4. 返回宗门后立即打开仓库和战备，执行一次可逆的正式 P5 整理事务，证明无需重启 Editor／游戏即可解锁，且 P5 与失败前基线等价。
5. 重复发送失败回执、迟到 callback、重新打开 UI 或再次出战时，不得清理／覆盖新的 attempt，也不得让仓库重锁。

### 8. 仅限本任务的修复规则

1. 可以修复由 F1 直接复现的默认入口、M01 descriptor／配置、World lifecycle callback、delegate 解绑、StartAttempt correlation、Controller／Pawn／input ready 判定、TechnicalStartFailure cleanup、Coordinator gate、UI 只读投影、P5→P6 bridge 调用时序和 Editor 诊断问题。
2. 每处修复必须说明：复现症状、根因、所改文件、为何不改变 Code A 战斗／生命／死亡／终局权威，且为何不改变 Code B P5/P6/P8 物品权威。
3. 禁止借机新增战斗、地图、美术、敌人、装备数值、物品效果、随机 Loot、商店、经济、制作、自动拾取、第二地图、多人、网络同步、第二库存、P6 preview、Widget authority、fixture、starter 或历史系统复接。
4. 若修复需要重置、迁移、重掷、删除 P1—P21 的历史物品／容器／世界掉落／尸体／快捷绑定，或需要重写 Code A／Code B 的既有权威，停止并报告 `BLOCKED`，不得以范围外改动强行通过。

### 9. Bridge 拒绝与未闭合情形

1. 如果真实 M01 已确认而 bridge 因 selection stale、冲突或保存错误被拒绝，不得将其倒写为 TechnicalStartFailure，不得清除真实 Code A Run，也不得创建 fallback P6、clone 或第二库存。
2. 记录完整相关证据，保持当前真实 Run，并将该情形报告为独立的、可定位的后续问题；只在本任务允许范围内修正明显的 correlation／时序错误。
3. 如果桥接拒绝来自 P5/P6/P8 内部未曾授权的既有功能缺陷，保留证据、完成可安全完成的其余场景，并报告为需要单独修复的条目，不得扩大 F1。

### 10. 构建、证据与验收

1. 在所有代码／配置修复后，必须编译：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

2. 运行前后各保存最少一组可读截图／日志证据：宗门 `AtSect`、出战处理中、真实 `InRun`，以及技术失败回宗门后的可整理状态。截图必须来自真实运行，不得使用 mock、编辑图或手工替换文案。
3. 静态复查：新的 Host／Coordinator／Adapter 仍为唯一正式路径；没有第二 StartAttempt creator、第二库存、Code A mirror、P5/P6/P8 双写、未清理的测试注入或历史路径重新可达。
4. 若无修复，两个编译目标仍必须以 native exit code `0` 结束；若有修复，所有受影响的成功与失败场景必须重新运行。

### 11. Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix.F1_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. 运行前安全快照、测试 Owner／基线及恢复结果；
2. 各真实场景的操作、StartAttemptId、状态序列、OwnerId／RunInstanceId、P5/P6/P8 差异和截图／日志位置；
3. 默认宗门入口、仓库／战备同图和 `AtSect` 可整理的真实证据；
4. 重复点击如何只保留一个 attempt、一次激活和最多一次 bridge；
5. M01 成功何时满足 RuntimeReady，P5→P6 如何只发生一次，以及空间图／ItemId／ContainerId 的一致性；
6. 技术失败的实际触发方式、关联回执、幂等清理、AtSect 返回和即时仓库解锁证据；
7. 发生的任何代码／配置修复、根因、边界证明、重编译与复测结果；
8. bridge 拒绝、空 P5 或其他未闭合情况的真实事实；
9. 两个构建命令、关键结果和最终 native exit code；
10. 未执行的后续 F 债务：完整仓库／装备／空间图操作，普通容器／尸体／地面搜取与转移，快捷使用，P8 三种终局，recovery，全量回归，Cook、Package 和最终版本验收；
11. 确认未新建工程、未建立第二库存、未使用历史规则、未丢弃或重置 P1—P21 成果。

仅当 A—D 场景均以真实运行通过、失败后仓库即时恢复、无错误 P6/P8／活动 Run 残留、P5→P6 只在真实 RuntimeReady 后一次性发生、所有范围内修复均已重编译复测时，使用：

    READY_FOR_0_0_9BFIX_F2_PLANNING

若启动／回退链仍有本任务范围内可修复问题，使用：

    NEEDS_0_0_9BFIX_F1_REWORK

若必须通过范围外权威重写、历史系统复接、物品重置／迁移或第二库存才能继续，使用：

    BLOCKED

完成后不得自动开始 `0.0.9BFix.F2`、常规 P 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.F1","file":"Dev.D.UE.0.0.9BFix.F1_report.md"}
