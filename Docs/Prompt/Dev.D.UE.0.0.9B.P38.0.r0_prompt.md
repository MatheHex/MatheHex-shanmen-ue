# Dev.D.UE.0.0.9B.P38.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P38——将 P21 已揭示尸体装备的**普通显式拖拽拾回**从“只能先进入 BaseQuick”扩展为可直接落到明确的 P6 储物或兼容装备位置。
- 任务编号：Dev.D.UE.0.0.9B.P38.0.r0。
- 前置：已接受 0.0.9B.P1—P37 与 0.0.9BFix.P1—P4。Fix 仅用于已确认、已验收功能的缺陷修复；P38 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P38.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P38.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动产品、PIE、Standalone、真实输入验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P39、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、WorldDropTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 `BasicCorpse` 的死亡回执、首次物质化、Hidden → Searching → Revealed、已打开 BodyTarget 与 P6/P11 单次 durable transfer。P21 在未来未 materialize 的 `BasicCorpse.r3` 中建立了 `Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0` 三个固定尸体装备位；最多一件真实、非空间、Quantity=1、MaxStack=1 的 standard root 可出现。P21 原任务将该 root 的显式取得限定为“已 Revealed 尸体装备格 → 空 P6 BaseQuick”，并没有授予直接装备、自动装备、快捷领取或尸体回存能力。

P17 已建立真实空间道具的一层 ChildContainer 图及 P7 工作台投影。P19/P30 只处理空间 parent 的完整图地面路径和受限快捷拾回；P26—P29 已处理 simple stack 在 BaseQuick／当前已打开合法 P17 child 与地面之间的正常、数量与快捷路径；P31 已将地面扩展为多个彼此隔离的 exact WorldDrop record。P32—P35 已补齐 standard non-spatial equipment 的显式地面拖放及明确拾回；P36/P37 只为特定 WorldDrop provenance 提供输入时冻结目标的 `Ctrl + 左键`快捷拾回。

P38 不改变 P21 的尸体来源、r3 deterministic roll、Hidden/Searching/Reveal、BodyTarget 生命周期或尸体装备本身的空间/战斗语义。它只放宽已 Revealed、已打开、身份有效 P21 尸体装备 root 的**普通真实 Drag/Drop 的明确目的地**。P38 不是 P36/P37 的快捷功能续写，不得让 P21 source 获得 `Ctrl + 左键`、自动目的地或自动装备。

### 2. P38 产品裁决

当玩家已经通过既有 P12 流程主动打开身份有效的唯一 BasicCorpse、且某个 P21 尸体装备格已经 Revealed 时，玩家可从该已挂载的 root Cell 发起既有普通 DragOperation，并把它落到一个**明确指定且通过真值验证**的 P6 target。允许的 destination 只有下表三类：

| Source | 明确 Drop destination | 唯一允许事务 |
| --- | --- | --- |
| 当前已打开、已 Revealed、identity-valid 的 P21 `Body.Weapon` / `Body.ArmorRobe` / `Body.Accessory0` root | active P6 BaseQuick 内的明确空 ordinary storage cell | 一个 P1 whole-root Move，沿既有 P11/P6 单次 durable replacement 提交 |
| 同一 source | 当前已打开、identity-valid P17 child 内的明确空 ordinary storage cell | 一个 P1 whole-root Move，沿既有 P11/P6 单次 durable replacement 提交 |
| 同一 source | Definition-compatible 的明确空 P6 Weapon、Armor/ArmorRobe 或普通 Accessory formal equipment slot | 一个 P1 whole-root Move，沿既有 P11/P6 单次 durable replacement 提交 |

这三条路径都属于显式 drag target，而不是 resolver 扫描得出的自动目标。P38 不得在 BaseQuick、child 或装备位之间自动选择、自动回退、自动换位、自动穿戴、自动打开/切换 child，亦不得因 target 已满、失效或不兼容而改变 destination。用户拖到哪里，durable transaction 就只验证那个 exact stable target；失败即完整零写入。

P21 source 的稳定 provenance／slot semantic 必须从活动 Code B truth 实际读取，并在 Report 中写出实际稳定值。不得凭空增加或重命名 durable provenance，也不得用图标、显示名、Cell class、Actor、当前位置、尸体展示顺序或当前选择推断它。source 必须同时证明为：P21 future-only BasicCorpse.r3 所产生、归属于 exact P11 BodyTarget/death receipt 的 `Body.Weapon`、`Body.ArmorRobe` 或 `Body.Accessory0` 正式装备位中的 root；其 Catalog definition 必须是 standard non-spatial Weapon、Armor/ArmorRobe 或普通 Accessory，Quantity=1、MaxStack=1、无 ChildContainerId、无空间语义、无 graph closure。

“当前 P17 child”仅是第二行表格的显式画面目标条件：拖放发生时该 exact child 必须已经 opened、identity-valid、属于当前 active P6 的正式 SpatialRing/Backpack parent，并且 payload 与 commit 均能重验 exact parent ItemId、child ContainerId、open generation、P6 revision、ordinary slot semantic 和 empty target cell。child 在拖放后关闭、切换、失焦、generation 失效、parent/child 拓扑失效、容量失效或 target stale 时，一律拒绝；不得改投 BaseQuick、另一 child 或装备位。

装备位只接受玩家明确拖上的 exact target。Weapon、Armor/ArmorRobe、普通 Accessory 的 Definition compatibility 与空位性必须由既有 Catalog/P1/P4x formal slot policy 验证；不得扫描“第一个兼容装备位”，不得把 SpatialRing、Backpack、Hotbar、child root、warehouse、ordinary container、尸体容器、WorldDrop 或另一 BodyTarget 当成自动或隐式目的地。

### 3. 严格范围与持续排除

P38 source 只接受同时满足全部条件的 P21 corpse equipment root：

1. 当前 Workspace 是既有 P12 production Host，且 exact BodyTarget 已 Open；OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、ProfileId/Version、source container、source slot semantic、root ItemId、Reveal state、route、focus、target-open generation 与 P6 revision 都能在 preview 和 durable commit 前从 P1/P6/P11 truth 重新验证；
2. source 是已 Revealed 的 exact P21 Body equipment slot root，而不是 P12 普通 body item、Hidden、Searching、P9 container、P14/P31 WorldDrop、P21 空 slot、玩家物品、另一尸体、Code A object 或 UI-only projection；
3. P21 r3 receipt、candidate-set digest、Catalog、P1 与 P11 snapshot 共同证明 item 是实际 candidate-set 内的 single standard root；不得通过播放层、显示文本、old Code A loot 或随机 Actor state 认定资格；
4. destination 的 ContainerId、SlotIndex、Owner/Run、revision、scope、active session、Prepared/terminal gate 与 Host validity 在一次 durable commit 前持续完整成立。

以下对象或动作持续不属于 P38：

1. `Ctrl + 左键`、QuickTransfer、双击、右键 Take、交互键领取、Take All、Actor direct pickup、距离自动拾取、快捷丢弃、自动装备、auto bind、auto use、自动打开/切换 child、自动目标扫描、fallback、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=0、WorldPickupDraft、PlayerSplitDraft；
2. P12 普通 corpse root 的既有 Move/Merge/Swap 产品语义、P21 source 的 P6 → 尸体回存拒绝、P9 ordinary container、P14/P31 WorldDrop、P19/P30 complete graph、P26—P29 simple stack、P32—P37 ground source families、P5 warehouse、P8 receipt、P13 binding、P15 Use、P17 graph construction、P20/P21 deterministic loot source；
3. 任意空间 parent、child graph、nested container、空间道具本体、空间道具内部内容的 graph Split/Merge、尸体/玩家之间的 partial transfer、map drop 或第二个尸体；
4. Code A 的敌人死亡、尸体 Actor、交互距离、地图、战斗、生命、HUD、Run、Run Save、终局、旧库存、旧 Loot、经济、网络或多人；
5. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 尸体状态机或 P21 r3 来源、不改变 P17 graph、不中断 P19/P26—P37 与 P31 边界、也不增加任何自动行为的前提下，扩展 P21 已 Revealed corpse-equipment root 的既有 `NativeOnDrop` policy：允许它经单一、可回滚的 P1 whole-root Move，从 exact P11 body equipment slot 直接移动到玩家明确指定的空 BaseQuick ordinary cell、当前有效 P17 child empty ordinary cell 或 compatible empty formal player-equipment slot。

成功时，P11 source 与 P6 target 必须只通过现有 P12/P1/P6 事务共同更新，并以同一 Owner durable replacement 保存。失败时 P11、P6、P7、BodyTarget projection、P13、P14/P31、P8、Code A 与所有无关 transient state 均保持不变。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 的 BodyTarget open/reveal lifecycle、已挂载 root Cell、DragOperation、`NativeOnDrop`、preview、P11/P6 candidate、取消、close、Actor/Host invalidation 与 stale 生命周期；说明 P38 如何复用该真实 Drop 写入口，而不是新建 button、hotkey、pointer handler、Widget 或 Actor 写入口；
2. P21 r3 的 `Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0` stable slot semantic、future-only deterministic receipt、candidate definition 和 P21 source eligibility；实际 stable provenance/value 必须如实记录；
3. P7/P4x 的 P6 BaseQuick、formal equipment target、explicit Drag/Drop、Definition compatibility、empty target 与 P13 reconcile；说明 P38 只复用既有 formal target policy，不创造新的 Equip/Unequip 规则；
4. P17 current child identity、parent → child canonical topology、open generation、dynamic capacity、ordinary Cell 与 stable SlotIndex；说明 child target 如何在 input、preview 和 commit 前重新证明；
5. P1/P2/P3/P6/P11 的 whole-root Move、跨图 single candidate、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、session/Prepared/terminal gate 与 revision 调用图；
6. 所有 Widget direct Move、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 ItemId、旧 Code A Loot、按 first/last/selection 猜测目标、第二 Repository transaction、第二保存或 Code A inventory writer；它们不得成为 P38 写入路径。

#### 5.2 唯一普通 Drag 路由与 source gate

1. 既有 P12 已挂载 Cell 的普通 DragOperation 与 `NativeOnDrop` 仍是 P38 的唯一输入与提交入口。不得为 P38 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。
2. ordinary left click 继续只选择；right-click 继续只读详情；`Ctrl + 左键`继续不为 P21 source 获得 P38 写入语义；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、`Shift + 1—9`、player-side corpse-related drag 与任何 Code A interaction 都不得形成 P38 位置写入。
3. source payload 必须仅由当前 Open、Revealed、identity-valid exact P21 body equipment root 建立，并冻结 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、source ContainerId、source SlotIndex、root ItemId、source P6/P11 revision、target-open generation 与 necessary route/focus proof。不得从 Hidden、Searching、empty equipment slot、普通 corpse item、another BodyTarget、P9/P14/P31 或 player cell 建立 P38 intent。
4. Preview 与 durable Commit 前必须重新验证 source root 仍位于该 exact body equipment slot，visibility 仍 Revealed，P21 candidate/receipt/profile proof 不变，BodyTarget 仍 Open，且 Owner/Run/session/Prepared/terminal/Host/route/focus/revision gate 全部成立。close、reopen、focus loss、Actor EndPlay、map reload、recovery、terminal、record/root/container mismatch、payload cancel 或 Host invalidation 必须立刻使 intent 失效并零写入。
5. source 不得生成或携带可写 quantity cache；它只能是 Quantity=1 的 existing root。不得调用 Merge、Split、RequestedMergeQuantity、Quantity=0、WorldPickupDraft、PlayerSplitDraft、graph Move 或任何 graph closure shortcut。

#### 5.3 三类显式 target 的确定验证

1. P38 不得实现 target resolver。Drop target 的 exact stable ContainerId、SlotIndex、scope 与 revision 必须来自用户实际 drop 命中的 production Cell；任何 category 都不得用 display order、focus、selection、nearest slot 或 fixture 猜测。
2. BaseQuick target 只能是 active P6 Basic/BaseQuick root 内一个正式、可写、空 ordinary storage cell。现有 P21 BaseQuick path 保持，但须纳入与其他两类相同的 source/session/revision transaction proof。
3. child target 只能是当前 opened、identity-valid P17 child 内用户明确命中的正式、可写、空 ordinary storage cell。Preview 与 commit 均须验证 exact parent ItemId、child ContainerId、open generation、Owner/Run、P17 definition、one-layer/no-cycle topology、dynamic capacity、ordinary slot semantic、slot availability 与 P6 revision。child close、switch、失焦、generation/parent/capacity mismatch 或 cell 被占用时拒绝；绝不改投 BaseQuick、另一 child、装备位或其他容器。
4. equipment target 只能是用户明确命中的 P6 formal Weapon、Armor/ArmorRobe 或普通 Accessory equipment cell；P1/P4x/Catalog 必须证明 exact item Definition 与该 exact slot compatible、slot 属于 active P6 owner/run、且 target empty。不得扫第一个合法装备位，不得使用 SpatialRing、Backpack、Hotbar、child root、warehouse、尸体位、WorldDrop 或任意普通容器作为 equipment fallback。
5. 目标满、被占用、不可写、definition incompatible、scope/revision stale、session invalid 或任何 lifecycle 条件失败时，P38 必须零写入。不得 Swap、Replacement、挤位、Compact、Sort、先临时放 BaseQuick 再装备、先 UnEquip 目标物品、或生成中间 ItemId。

#### 5.4 单一事务、持久化与回滚

1. Preview 成功后只建立一个 Owner/Run scoped Candidate，并只调用一次现有 P1 whole-root Move：同一个 ItemId 从 exact P11 Body equipment container slot 0 直接移动到用户明确指定的 exact P6 target。不得先落 BaseQuick、再二次 Equip/Move，也不得用 P6 local move 绕过 P11/P6 atomic transaction。
2. Commit 必须沿既有 P12 → P3/P2/P1 → P11/P6 durable callback 的实际活动链。Store 在任何 durable write 前重新验证 command intent、source/target stable address、BodyTarget/death receipt、Reveal/Open state、Owner、Run、revision、P21 eligibility、target category proof、target empty state 与 active session。
3. accepted candidate 中 DefinitionId、ItemId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、P21 candidate provenance 和 no-child/non-spatial qualification 必须完全保持。唯一合法变化是 parent/slot 从 exact P11 body equipment container 变为 exact P6 BaseQuick、child ordinary 或 compatible equipment slot。不得生成新 ItemId、ContainerId、receipt、body record、WorldDrop、ordinal、Actor、child、binding 或第二 revision。
4. P11 与 P6 必须在同一 Owner durable replacement 中共同提交；accepted proof 必须证明 source 已离开 exact P11 body equipment slot 且 target 已获得同一 root，才可刷新该 exact BodyTarget projection 和 P7 target projection。不得预清 source cell、预写 P6、预删 BodyTarget 内容、预刷 Code A Actor 或在 save 后补写另一侧。
5. P13 只按既有 accepted commit reconcile，不得自动 Bind、Use、Equip 或恢复历史 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P11 residual；P38 不改变尸体死亡、reveal、receipt、r3 materialization、终局或 recovery。
6. candidate、source gate、target gate、P1 Move、P11/P6 replacement、P13 reconcile、projection refresh 或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P11 source root、P6 target cell、body visibility/open state、P7 projection、selection 与无关 body/world record 均保持未变，且不得遗留 phantom empty slot、phantom equipment、transient item copy 或 partial corpse removal。
7. 成功后只刷新 exact BodyTarget source section、exact P6 target 和必要关联 P17 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset 或改写 P29—P37 语义。

#### 5.5 非回归与权威边界

1. P21 的 `BasicCorpse.r1/r2` 历史、已 materialized P11 records、r3 deterministic identity、Optional.EquippedLoadout、候选/权重/digest、最多一件装备、三固定尸体装备位、Hidden/Searching/Reveal 与 P6 → corpse equipment 拒绝均保持。P38 绝不重掷、补料、迁移或改写尸体来源。
2. P12 普通 corpse root 的 Move/Merge/Swap、P9 normal container、P14/P31 multi-record WorldDrop、P19/P30 complete graph、P26—P29 simple stack、P32/P33/P35 normal ground Drag、P34/P36/P37 WorldDrop Ctrl branches 均保持既有 source family、target、quantity、record 与 lifecycle 语义。P21 source 不得进入这些 branch；这些 source 也不得进入 P38 durable branch。
3. P36/P37 的 input-time frozen QuickTransfer model 仍只服务其已授权 WorldDrop provenance。P38 是 normal Drag；不得借它给 P21 corpse source 增加 Ctrl quick pickup、child-first auto target、BaseQuick fallback 或任何 quick-drop。
4. P17 graph、P19 whole graph、P31 Registry、P8 BuildP14PlayerOnlySession/full-registry exclusion、P5 warehouse/P6 bridge、P13 binding、P15 Use、P20/P21 body source 与 Code A authority 均不改变。P38 不创建 WorldDrop，不读写 NextWorldDropOrdinal，不影响 Actor projection、地图落点或 other-record isolation。
5. Code A 继续只拥有 corpse Actor、交互距离、地图、死亡、战斗、Run 与终局边缘。它不得获得 Item、Container、Quantity、P11/P6、BodyTarget、Loot、search、terminal 或 player-equipment durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12/P21 已有 body-aware drag payload、preview、target-policy 与 P11/P6 durable transaction；
- P7/P3/P4 的既有 production Cell、DragOperation、P17 child projection 或 formal equipment target 只读/validation 接入；
- P1/P2/P3/P6/P11/P13 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P21 roll/profile/schema history、P12 搜尸 state machine、P5 warehouse、P8 terminal policy、P14/P31 WorldDrop schema、P17 graph construction、P19 graph semantics、P23 局外工作台、网络或多人。

### 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 P38 只接受 current opened/revealed/exact P21 body equipment source，且实际 stable source provenance/slot values 由 Code B truth 重建；Hidden、Searching、ordinary body item、P9/P14/P31、player source、another BodyTarget 与 unknown family 均不能进入 P38。
2. 审查三类 target 全部是 explicit exact cells：BaseQuick empty ordinary、current valid P17 child empty ordinary、compatible empty formal equipment slot；不存在 auto target、scan、fallback、swap、auto equip、temporary BaseQuick staging 或 child auto-open。
3. 审查唯一写入仍是现有 P12 normal Drag → single candidate → single P1 whole-root Move → one P11/P6 durable replacement → one save/reconcile 路径；没有新 pointer/UI/Actor writer、second transaction、second save 或 Code A inventory。
4. 审查 source/target identity、reveal/open state、BodyTarget/death receipt、P17 parent/child/open generation、formal slot compatibility、Owner/Run/revision、Prepared/terminal、Host invalidation 与 BeforeSnapshot rollback；任一失败均零写入。
5. 审查 P21 r3 determinism/history、P12 normal corpse behavior、P17/P19/P26—P37/P31、P5/P6/P8/P13/P15、Code A authority 与 stable SlotIndex/scroll/selection 不发生产品语义回归。
6. 运行 `git diff --check`，并在 Report 中列出实际修改文件、写入 call graph 与所有未修改的权威边界。
7. 只编译下列两个代码目标；编译不等同于运行测试：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 若编译失败，只修正 P38 引入的局部声明、include、payload proof、P12/P21 target policy、P11/P6 transaction、P17/formal-slot validation、序列化或调用签名问题，然后重新执行同一目标。若修复需要越过本 Prompt 边界，停止受影响部分并报告；不得自行扩展到 P39、Fix、F 或 Code A 权威。

### 8. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P38.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P21 r3 actual stable source provenance/slot semantic、BodyTarget/death receipt/profile/candidate identity 与为什么 r1/r2/history/determinism 未被改写；
3. P12 normal Drag 到 P38 的完整调用图，以及没有新 pointer、Widget、Actor、QuickTransfer 或 Code A 写入口的证据；
4. exact source gate、Reveal/Open lifecycle、P11/P6/Owner/Run/revision/session proof 与 three-target explicit policy；
5. BaseQuick、current valid P17 child 与 compatible formal equipment slot 三条 accepted path 各自的 target validation，及无 auto target/fallback/swap/auto-equip 的证据；
6. single P1 whole-root Move、单一 P11/P6 durable replacement、source/target identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P12 普通 corpse、P21 source/history、P17/P19/P26—P37/P31、P5/P6/P8/P13/P15、Code A authority 的非回归结论；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：P21 r3 future corpse 真实 materialize/reveal，各 Weapon/ArmorRobe/Accessory source 分别真实拖至空 BaseQuick、valid current child empty ordinary cell、compatible empty equipment slot；wrong/incompatible/occupied slot；Hidden/Searching/closed/reopened body；child close/switch/focus/generation/parent/capacity stale；BodyTarget/death receipt/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P21 ordinary corpse rule、P36/P37 Ctrl branches、P12 normal Move/Merge/Swap、P14/P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P38 corpse-equipment explicit direct pickup 静态闭合、P21/P12 与 P17/P19/P26—P37/P31/P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P39_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P38_REWORK

若现有 P12/P11/P6 事务无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P21 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P39、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P38.0.r0","file":"Dev.D.UE.0.0.9B.P38.0.r0_report.md"}
