# Dev.D.UE.0.0.9B.F0.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：F0——P31—P50 物品交互闭环的首轮真实集成验证，重点验证 P49 BasicCache simple stack 与 P50 BasicCache complete spatial graph。
- 任务编号：Dev.D.UE.0.0.9B.F0.0.r0。
- 前置：已接受 0.0.9B.P1—P50 与 0.0.9BFix.P1—P4。P50 的 P 阶段报告已具备通过条件。
- 执行文件：Dev.D.UE.0.0.9B.F0.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.F0.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- Report 必须生成到：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.F0.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：这是 F 阶段真实运行验证。允许启动产品、PIE、Standalone、真实鼠标键盘、截图、保存/重开与最小 Smoke；不得修改任何生产源码、资产、项目配置、测试代码、数据表、存档文件或既有任务范围。发现问题时只记录可复现证据，不自动修复、不自动开始 Fix、P51、F1 或其他任务。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程与已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定测试、验收或范围。

Code B P1 Repository 与 durable Store 仍是唯一物品真值。Widget、Cell、DragOperation、Workspace Context、NormalContainerTarget、WorldDrop Actor、地图放置适配层和 Code A 只允许投影、显示或瞬时意图；真实运行验证不得以它们的显示状态替代真实持久化结果。

P49 已使已打开、已揭示的 P10 BasicCache ordinary SpiritDust/IronShard simple root 能以 normal Drag 落为独立 P31 WorldDrop record，并只接入既有 P26—P29 simple-world pickup 链。P50 已使已打开、已揭示的 P10/P18 canonical WindTalisman 或 BackpackLevel1 complete graph 能以 normal Drag 整体落为独立 P31 WorldDrop record，并只接入 P19 normal pickup 与 P30 BaseQuickOnly Ctrl pickup。

P31 的多 record 身份、P17 一层无嵌套限制、P18 实际 receipt/profile/history、P8 terminal/recovery、P13 binding、P15 use、P5/P6 bridge 与 Code A authority 都是只读约束。F0 只验证，不重写这些规则。

### 2. F0 产品裁决

P49/P50 已完成静态闭合，现需要在真实 UI 与真实持久化生命周期中证明以下最小产品承诺：

| 路径 | F0 必验结果 |
| --- | --- |
| P49 BasicCache simple root → WorldDrop → player | 已揭示 SpiritDust 或 IronShard 经普通拖拽落地后产生一个独立可打开 record/Actor；从该 exact record 正常拾回后，只有该 record 被清理。 |
| P50 BasicCache complete graph → WorldDrop → player | 已揭示、真实物质化的 WindTalisman 或 BackpackLevel1 parent 连同唯一 child closure 经普通拖拽落地；从该 exact record 正常拾回时整个 graph 保持完整，不出现 child 分离、复制、嵌套或自动装备。 |
| P31 多 record 隔离 | simple 与 spatial 分别落地后，各有独立 WorldDropId/Ordinal/Actor；操作一个 record 不删除、重写或错误打开另一个。 |
| 生命周期与拒绝 | close/reopen、保存后重开，以及一个明确错误/无效目标都不产生 phantom root、duplicate record、silent fallback 或额外保存。 |

F0 不是新功能开发，不是扩展 P18 掉落表，不是以测试理由添加 fixture、debug spawn、存档编辑、强制随机种子或世界交互捷径。

### 3. 真实样本纪律

1. 优先使用一个已经通过生产 P10/P18 流程真实物质化的 BasicCache。若无可用空间图样本，可仅通过应用内正常创建测试会话/运行与正常打开 BasicCache 的方式取得样本；不得重写已物质化 receipt、profile、digest、候选权重、存档、随机种子或 item graph。
2. 可以使用干净的本地测试 profile，但必须由应用内正常流程创建；不得直接编辑 profile、save、JSON、INI、DataTable、源码、蓝图、资产或项目配置。
3. 对同一已经物质化的 BasicCache 不得 reroll、补料、重置或替换结果。若在合理、有限的正常测试会话内没有得到任何真实 P18 spatial parent，则完成所有可执行 P49 用例，并以 F0_PARTIAL_NO_GENUINE_P18_SAMPLE 如实结束；不得把 P50 标记为通过，也不得伪造样本。
4. 每一次测试操作都必须使用真实鼠标键盘和正式 UI；禁止调用 C++/Blueprint 函数、Console command、自动化测试、UI injection、脚本、直接 Actor interaction、内存修改或文件写入来跳过玩家路径。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不修改工程内容的前提下，使用真实运行环境验证 P49/P50 的 BasicCache → WorldDrop → player 闭环、P31 多 record 隔离和最小失败保护，并留下足以定位后续 Fix 的可复现证据。

### 5. 执行步骤与验收

#### 5.1 开始前检查

1. 记录当前 commit、git status、当前 save/profile 身份、使用的地图/运行模式和启动时间。若工作区已有与本任务无关的改动，只记录，不清理、不覆盖、不提交。
2. 启动实际工程。至少在 PIE 和 Standalone 各验证一个完整 P49 simple-stack 循环；P50 完整图验证可以在其中任一真实运行面完成，但必须明确写明所在运行面。
3. 截取每个关键成功或失败状态的证据。截图、日志或现有 UI 读数必须能关联到具体用例、运行面与 record，而不是只给一张无上下文的画面。

#### 5.2 P49 simple-stack 主路径

对一个通过真实 P10 流程已打开、已揭示的 SpiritDust 或 IronShard root 执行：

1. 用 normal Drag 从 exact BasicCache Cell 拖到现有 GroundDropZone。
2. 确认玩家原 BasicCache source 不再显示该 exact root，且地面只新增一个可见、可打开的 WorldDrop Actor/record。
3. 打开该 exact record，以用户明确 normal Drag 将完整 root 拾回一个空 BaseQuick ordinary cell。
4. 确认被拾回的 record、其 derived container 与对应 Actor 被清理；任何先前存在的 other WorldDrop record 均保持可用且身份不变。
5. 保存并关闭/重开相关 UI 或重新加载当前有效运行面；确认没有 duplicate Actor、phantom empty slot、根副本、ordinal 跳变或错误回填 BasicCache。
6. 在另一运行面重复一个完整 simple-stack 循环。该重复可以使用 SpiritDust 或 IronShard 中另一种；若只能取得一种，必须如实记录，不能把另一种伪造成已验证。

#### 5.3 P50 complete-spatial-graph 主路径

仅当存在真实 P18 materialized WindTalisman 或 BackpackLevel1 parent 时执行：

1. 打开 identity-valid BasicCache，确认该 parent 已 Revealed，并在现有 UI/日志证据中能识别其唯一 child closure。
2. 用 normal Drag 从 exact BasicCache parent Cell 拖到 GroundDropZone。确认只出现一个新的 spatial WorldDrop record/Actor，且不是 simple-stack record。
3. 在不触碰该 spatial record 的情况下，先关闭并重新打开它；确认仍是相同 record，未出现 child-only record、duplicate parent 或错误自动装备。
4. 从该 exact spatial record 用 normal Drag 拾回到一个用户明确的空 BaseQuick ordinary cell，或对应的空兼容 formal equipment slot：WindTalisman 只能进入 SpatialRing，BackpackLevel1 只能进入 Backpack。不得接受自动选择或 fallback。
5. 确认 parent 与其唯一 child closure 同时、完整地离开该 record；该 exact record/Actor 清理，P49 或任何 earlier record 不受影响；空间 parent 不进入任何 child，不发生嵌套、flatten、clone、split 或 child orphan。
6. 若同一测试会话中还真实出现第二个 canonical P18 parent，可额外验证 P47/P48/P30：BasicCache Ctrl 只能进 BaseQuick；WorldDrop Ctrl 只能进 BaseQuick；explicit normal Drag 才可进入对应兼容 equipment slot。没有第二样本时不得人为复制样本，也不得把这些附加项标记为已验证。

#### 5.4 多 record、持久化与失败保护

1. 在同一有效 session 中同时保留一个 P49 simple record 与一个 P50 spatial record，再分别打开、关闭、拾回其中一个。确认另一 record 的 WorldDropId/Ordinal、root、Actor 和可打开性未被改变。
2. 对一个已打开 record 尝试一个明确不可接受的操作：例如 target 已满、错误 equipment slot、关闭/失焦后继续 Drop，或 source/record 已不再是 current exact record。确认操作被拒绝，且不会重扫目标、静默改投 BaseQuick/child/其他装备位，也不会生成新 record/Actor。
3. 在上述关键状态之一执行保存后重开或合法的当前运行恢复。确认 durable 状态与 accepted operation 一致，且没有 duplicate record、ghost Actor、slot 与 record 不一致或 BasicCache 回填。

#### 5.5 禁止行为

- 不修改、格式化、还原、清理或提交任何源码、资产、配置、项目文件、测试文件、存档或用户原有改动。
- 不修复缺陷，不以测试需要临时增加日志、按钮、Console command、fixture、假 ItemId、假 record、假 P18 parent 或第二库存。
- 不测试、扩展或裁决战斗、敌人、死亡、撤离、经济、制作、网络、多人与其他未列路径。
- 不将正常游戏中一次未产出 P18 parent 当作 P50 失败；只在报告中说明样本不足。

### 6. 缺陷分流

若出现真实、可复现的失败，立即停止对应路径，保留必要截图和最小复现步骤，并在 Report 中精确标出：

1. 第一次可观察异常；
2. source/target/record 的实际身份与运行面；
3. 预期与实际差异；
4. 是否发生了持久化、记录清理、Actor 刷新、重复或丢失；
5. 是否可以在不改文件的第二次真实操作中复现。

不得自行编写 Fix。只有已成功复现且边界明确的问题，才可在报告结论中建议后续 Fix 任务。

### 7. Report 与完成信号

生成 Dev.D.UE.0.0.9B.F0.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须包含：

1. 当前 commit、git status、测试 profile 是否为新建本地测试 profile、地图、PIE/Standalone 使用情况与未修改工程内容的结论；
2. 一张逐项测试表：用例、真实前置样本、操作、预期、实际、截图/日志证据、PASS/FAIL/BLOCKED；
3. P49 两个运行面的完整闭环、P31 exact-record 清理和 other-record 隔离证据；
4. P50 实际 parent 定义、child closure、normal ground drop、close/reopen、normal pickup、graph 保持与 record cleanup 证据；若没有真实样本，明确列出样本取得方式、尝试边界和未测试项目；
5. 多 record、保存/重开或恢复、拒绝路径的证据，以及没有 silent fallback、自动装备、自动 child entry、duplicate Actor/record 或 BasicCache 回填的结论；
6. 每个缺陷的最小复现、影响范围和建议分流；没有缺陷也必须明确说明；
7. 所有未测项与原因，尤其是第二种 P18 definition、P47/P48/P30 附加分支、full/wrong target、terminal/recovery 的剩余覆盖；
8. 本任务结束后的准确状态。

仅当以下条件同时满足时，使用：

    F0_PASS
    READY_FOR_F1_PLANNING

- P49 在 PIE 与 Standalone 中均完成真实闭环；
- 至少一个真实 P18 canonical parent 完成 P50 normal ground-drop → close/reopen → normal pickup 的完整图闭环；
- P31 multi-record isolation、保存/重开与一个明确拒绝路径均无异常；
- 没有未分流的可复现失败。

若 P49 通过但没有可用的真实 P18 spatial parent，使用：

    F0_PARTIAL_NO_GENUINE_P18_SAMPLE

若存在可复现的产品失败，使用：

    NEEDS_FIX

若工程无法启动、无法使用真实 UI、或测试 profile 无法安全建立，使用：

    BLOCKED

完成后不得自动开始 P51、Fix、F1 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.F0.0.r0","file":"Dev.D.UE.0.0.9B.F0.0.r0_report.md"}
