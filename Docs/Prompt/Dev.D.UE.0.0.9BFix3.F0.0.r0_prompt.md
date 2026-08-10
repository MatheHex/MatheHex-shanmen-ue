# Dev.D.UE.0.0.9BFix3.F0.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不得新建项目、分支工程或测试副本。
- 修复线：`0.0.9BFix3`。
- 阶段：F0——`F0-001` 修复后的独立真实集成重测。
- 任务编号：`Dev.D.UE.0.0.9BFix3.F0.0.r0`。
- 前置：`Dev.D.UE.0.0.9B.P1—P50`、已接受的既有 Fix 链、`Dev.D.UE.0.0.9B.F0.0.r0` 的真实失败证据，以及结论为 `READY_FOR_F0_RERUN` 的 `Dev.D.UE.0.0.9BFix3.P1`。
- 执行文件：`Dev.D.UE.0.0.9BFix3.F0.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9BFix3.F0.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 引擎：`C:\Program Files\Epic Games\UE_5.8`。
- 任务性质：真实 F 阶段验收；只验证 Fix3 后的唯一 BasicCache 物理投影与原 F0 的 P49/P50/P31 闭环。不得修改生产源码、资产、配置、测试、数据表、存档或项目文件。发现问题只记录，不修复。
- 后续治理：回传本 Report 后立即停止；不得自动开始 `0.0.9BFix3.P2`、其他 Fix、P51、F1、自动化、Cook、Package 或其他任务。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效依据

唯一有效依据是当前 `0.0.9B` 活动工程、已接受任务链、本任务书、`Dev.D.UE.0.0.9B.F0.0.r0_report.md` 中的 `F0-001` 证据，以及 `Dev.D.UE.0.0.9BFix3.P1_report.md`。

`0.2 final report.docx`、所有 `0.2`／V2／V3、旧 I／IPF、历史页面壳、旧 CTA、旧库存与旧物品规则均已废止。不得读取、引用、恢复或用于测试、验收、命名、实现或范围裁决。

### 2. 已修复但尚未被真实运行验收的前置

原 F0 已在 PIE 与 Standalone 的正常宗门部署中共同复现 `F0-001`：

- target：`M01.CodeBNormalContainer.BasicCache.01`；
- anchor：`M01.Resource.TIER_1.Cluster.01`；
- 失败：`ResolveSafeWorldLocation` 在原锚点局部偏移 `(260,-220,0)` 周围找不到安全位置，因此 P10 Actor、P18 materialization、P49/P50 source 与 P31 record 均未出现。

`Fix3.P1` 已作窄范围生产修复：唯一 BasicCache 仍使用既有 anchor safe-projection、同一 exact static identity、同一候选／碰撞／ground／visibility 合约和同一生命周期；仅把稳定 anchor-local Desired 改为 `(0,-1600,0)`。它没有创建 pre-authored Actor、第二 BasicCache、第二库存、fixture、debug spawn、P9/P18/P31 预建内容或任何 durable 写入。

Fix3 的静态审计和 Editor／Game 编译通过不等于产品通过。本任务必须以真实 UI、真实鼠标键盘和真实运行面重新验证该结论。

### 3. 不可改变的真值与交互边界

1. Code B P1 Repository 与 durable Store 是唯一可变物品真值。P5 为局外图，P6 为精确 `OwnerId + RunInstanceId` 的 active carry 图，P9 为 BasicCache 的 Run-local target 图；Widget、Actor、WorldDrop、地图适配层和 Code A 只可投影或传递瞬时意图。
2. P10 的唯一生产普通容器 identity 固定为 `M01.CodeBNormalContainer.BasicCache.01`，定义为 `CodeB.NormalContainer.BasicCache`，provenance 固定为 `M01.Resource.TIER_1.Cluster.01`。`SearchTargetId` 只能从该静态 identity 稳定推导，不能来自 Actor pointer、世界临时 GUID、坐标、显示名、Widget 或随机值。
3. 每一有效 Owner／Run／M01 生命周期最多一个 exact BasicCache Actor／adapter。预置发现与动态投影若均存在，必须是互斥路径，绝不能形成第二 Actor、prompt、registration 或 P10 target。
4. P10 的 `Closed → Opening → Open`、Hidden → Searching → Revealed、一次性 materialization、P9 receipt／digest 与 P9↔P6 既有事务均保持。仅正常 P10 open/search 可以首次 materialize P9；Actor 生成、地图加载或本次测试不得预建 ItemId、P9 record、receipt、空间图或 WorldDrop。
5. P18 r2 history、真实 optional spatial utility、P31 multi-record identity、P49/P50 及 P19/P26—P30 拾回链、P13/P15/P17 一层无嵌套规则、P5/P6/P8 生命周期与 Code A Run/input/terminal authority均为只读约束。

### 4. 本次 F0 的产品承诺

| 路径 | 必验结果 |
| --- | --- |
| Fix3 唯一物理投影 | PIE 与 Standalone 均在正常 M01 部署后产生恰好一个可见、可聚焦、可正常交互的 exact BasicCache；不得复现 `no safe projection`、重复 Actor／prompt 或 stale registration。 |
| P49 simple root | 已揭示 `SpiritDust` 或 `IronShard` 经普通拖拽落为独立 WorldDrop，再由用户明确普通拖拽拾回；只有 exact record 被清理。 |
| P50 complete graph | 已真实 materialize 的 `WindTalisman` 或 `BackpackLevel1` parent 连同唯一 child closure 经普通拖拽落地、关闭／重开并整体普通拾回；不得分裂、复制、嵌套或自动装备。 |
| P31 与持久化 | simple 与 spatial record 并存时身份隔离；保存后重开／合法恢复和一个明确拒绝操作均不产生 phantom root、duplicate Actor／record、silent fallback 或错误回填。 |

本任务不是新功能开发，不扩展 P18 掉落、不强制空间样本、不以测试名义创建 fixture／debug spawn／第二库存／假 ItemId／假 record／存档编辑／随机种子／Console 或 UI injection。

---

## 下半部分：授权执行内容

### 5. 单一授权目标

在不改动工程内容的前提下，以真实运行环境验证：

    正常宗门部署
      → Fix3 后恰好一个 M01 BasicCache
      → 正常 `[G]`／正式交互打开与揭示 P10
      → P49 或 P50 的 BasicCache → WorldDrop → player
      → P31 exact-record 隔离与持久化

若发生失败，只保留最小复现证据并准确分流；不得实施、尝试或建议未验证的修复细节。

### 6. 运行前保护与真实样本纪律

1. 记录当前 commit、`git status --short`、测试开始时间、地图、每个运行面的 OwnerId／RunInstanceId、使用的 profile 身份和 BasicCache 当前 materialization 状态。已有无关改动只能记录，不得清理、覆盖、还原或提交。
2. 可使用通过应用正常流程创建的干净本地测试 profile，或现有有效本地 profile；不得读写、替换、复制或手工恢复 Save／JSON／INI／DataTable。若测试正常操作会改变持久状态，只允许让它按产品正式生命周期保留，不能用文件回滚伪造结果。
3. 一切页面点击、`[G]`、拖拽、关闭／重开及保存／恢复均须由真实 UE 窗口接收真实鼠标键盘。禁止 Console、自动化测试、函数直调、Blueprint／C++ 调用、反射、脚本、UI injection、直接 Actor interaction、内存修改或任何跳过玩家路径的方式。
4. 至少分别在 PIE 和 Standalone 完成 BasicCache 可用性检查与一个完整 P49 simple-stack 闭环。P50 可在任一真实运行面完成，但报告必须写明运行面和真实来源。
5. 只能从正常 P10/P18 打开／揭示路径使用真实 materialized sample。不得 reroll、补料、重置、迁移、替换 receipt／digest／profile／item graph；不得将地图普通掉落冒充 P18 parent。

### 7. 必须执行的真实场景

#### A. Fix3 物理投影复测（PIE 与 Standalone 各一次）

1. 从正式宗门入口正常部署 M01，进入真实 InRun；记录日志中的 exact Owner／Run、target／anchor identity 和投影结果。
2. 在实际世界中定位 `M01.CodeBNormalContainer.BasicCache.01`，确认它可见、可获得正式 focus／prompt，且不存在第二个同 identity Actor、第二个 prompt 或 `CODEB_P10_BASIC_CACHE: no safe projection`。
3. 使用正常距离、真实 `[G]`／正式 UI 输入打开该 exact BasicCache。确认它进入既有 P10 open/search 路径，并只在正常流程中生成或读取 P9 内容；不得通过日志、Actor 存在或静态代码审计替代交互证据。
4. 若任一运行面仍失败、缺失、重复、超距离可交互、无法打开或出现 stale prompt，立即停止该路径并按第 9 节报告为可复现产品失败。

#### B. P49 simple-stack 主路径（PIE 与 Standalone 各完成一次）

对同一真实 P10 流程已打开、已揭示的 `SpiritDust` 或 `IronShard` root：

1. 以 normal Drag 从 exact BasicCache Cell 拖到现有 GroundDropZone；记录 source root identity 和新 WorldDropId／Ordinal／Actor。
2. 确认 source 不再显示该 exact root，地面只新增一个可见、可打开的 exact WorldDrop record／Actor。
3. 打开该 exact record，以用户明确 normal Drag 将 root 拾回空 `BaseQuick` ordinary cell；不得让系统自动选择、自动装备或静默投向 child／其他格。
4. 确认 exact record、derived container 与对应 Actor 被清理；任何先前存在的其他 WorldDrop record 均保持可用，身份不变。
5. 在其中一个运行面额外保留两个 simple record，操作其中一个，证明 P31 exact-record 清理不误伤另一个。此项不能代替第 C 节要求的 simple／spatial 并存验证。
6. 关闭并重开相关 UI，且在一个已接受操作后执行合法的保存后重开或当前运行恢复；确认没有 duplicate Actor、phantom empty slot、root 副本、ordinal 跳变或错误回填 BasicCache。

#### C. P50 complete-spatial-graph 主路径（仅真实样本存在时）

若任一正常 P10/P18 session 真实 materialize `WindTalisman` 或 `BackpackLevel1` parent：

1. 在 identity-valid BasicCache 中确认 parent 已 Revealed，并以现有 UI／日志证据识别其唯一 child closure。
2. 用 normal Drag 从 exact BasicCache parent Cell 拖到 GroundDropZone。确认只产生一个新的 spatial WorldDrop record／Actor，而不是 simple record。
3. 不触碰该 spatial record 的内容，先关闭并重新打开它；确认 record identity 不变，且未出现 child-only record、duplicate parent 或自动装备。
4. 保留一个 B 节 simple record，同时打开／关闭／拾回 spatial record，确认双方的 WorldDropId／Ordinal、root、Actor 和可打开性彼此独立。
5. 从 exact spatial record 用 normal Drag 整体拾回：可进入用户明确的空 `BaseQuick` ordinary cell，或兼容的空装备位（`WindTalisman → SpatialRing`；`BackpackLevel1 → Backpack`）。不得接受自动选择或 fallback。
6. 确认 parent 与其唯一 child closure 同时完整离开该 record，exact record／Actor 清理；simple record 不受影响；不得发生 child orphan、clone、split、flatten、嵌套或进入空间 child。

若在合理且有限的正常测试 session 内未得到任何真实 P18 canonical parent，必须完成 A 与 B 所有可执行项，逐项记录取得样本的正常尝试边界，并以 `F0_PARTIAL_NO_GENUINE_P18_SAMPLE` 如实结束。不得把 P50 标记为通过，也不得伪造样本。

#### D. 明确拒绝路径

对一个当前 exact source／record，使用真实 UI 尝试一个可明确判定为非法的操作：目标已满、错误 equipment slot、关闭／失焦后继续 Drop 或 stale source／record 任一项均可。

确认操作被拒绝，且不会重扫目标、静默改投 `BaseQuick`／child／其他装备位、生成新 record／Actor、自动装备、自动打开或发生未授权保存。记录 source、attempted target、运行面、预期与实际。

### 8. 不可执行或失败时的分流

1. 若 F0-001 或任何产品故障在真实 UI 中复现，立刻停止受影响路径，保留必要截图与日志，并记录第一次异常、运行面、Owner／Run、target／anchor／record identity、预期与实际、是否发生持久化或重复／丢失，以及不改文件的第二次真实复现结果。不得写 Fix。
2. 若真实 UI、PIE、Standalone 或安全的 profile 创建本身不可用，记录客观阻塞点；不得以脚本、直接调用、伪造输入或修改工程来继续。
3. 不得把“本次正常 P18 未产出空间 parent”误报为产品失败；它只适用第 C 节的部分完成状态。

### 9. 明确禁止

- 禁止修改、格式化、重置、清理、提交或回滚任何源码、资产、蓝图、地图、配置、项目文件、测试、存档、用户原有改动或物品图。
- 禁止新增／删除 BasicCache、pre-authored Actor、anchor、WorldDrop、fixture、debug spawn、Fake ItemId／record、第二 P5/P6/P9、Widget truth、Code A mirror、直接 P6 grant 或 P18 强制命中。
- 禁止重新读取、引用、恢复或采用 `0.2 final report.docx`、任何 `0.2`／V2／V3／旧规则。
- 禁止测试或裁决战斗、敌人、死亡、撤离、经济、制作、网络、多人、terminal/recovery 全链、Cook、Package、全量回归或任何未列路径。
- 禁止自行实施 Fix、启动 P51／F1／其他 P／F／Fix／自动化任务。

### 10. 证据、Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix3.F0.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地包含：

1. Fix3 后 commit、git status、profile 来源、地图、PIE／Standalone、Owner／Run 与未修改工程内容的结论；
2. 一张逐项测试表：用例、真实前置样本、操作、预期、实际、截图／日志证据、PASS／FAIL／BLOCKED；
3. PIE 与 Standalone 的 Fix3 physical projection 证据：target／anchor identity、实际可见／focus／正常交互、exactly-one 结论，以及是否出现 no-safe-projection／duplicate／stale registration；
4. P49 在两个运行面的完整 normal Drag → WorldDrop → normal pickup、exact-record cleanup、two-simple-record 隔离及持久化／重开证据；
5. 若真实 P18 parent 存在：definition、unique child closure、normal ground drop、close/reopen、simple/spatial multi-record isolation、normal pickup、graph integrity 与 record cleanup 证据；若不存在：样本获取方式、正常尝试边界和所有未测项；
6. 一个明确拒绝路径的 source／target／identity、零副作用与无 silent fallback／自动装备／自动 child entry／duplicate Actor／record／BasicCache 回填结论；
7. 每个缺陷或阻塞的最小复现、影响范围和准确分流；没有缺陷也必须明确说明；
8. 所有未测项及原因，尤其是第二种 P18 definition、P47/P48/P30 附加分支、full/wrong target、terminal/recovery、Cook、Package 与全量回归；
9. 确认未修改工程、未使用 fixture／假目标／第二库存／直接调用／Console／UI injection，且未读取或采用废止文件；
10. 本任务结束后的准确状态。

仅当以下条件同时满足时，使用：

    F0_PASS
    READY_FOR_F1_PLANNING

- Fix3 后 BasicCache 在 PIE 与 Standalone 中均以 exactly-one、可正常打开／揭示的正式路径通过；
- P49 在 PIE 与 Standalone 中均完成真实闭环；
- 至少一个真实 P18 canonical parent 完成 P50 normal ground-drop → close/reopen → normal pickup 的完整图闭环；
- P31 simple/spatial multi-record isolation、保存／重开与一个明确拒绝路径均无异常；
- 没有未分流的可复现产品失败。

若 A 与 B 已通过，但没有可用的真实 P18 spatial parent，使用：

    F0_PARTIAL_NO_GENUINE_P18_SAMPLE

若存在可复现产品失败，使用：

    NEEDS_FIX

若工程无法启动、无法使用真实 UI、或测试 profile 无法安全建立，使用：

    BLOCKED

完成后不得自动开始任何后续任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix3.F0.0.r0","file":"Dev.D.UE.0.0.9BFix3.F0.0.r0_report.md"}
