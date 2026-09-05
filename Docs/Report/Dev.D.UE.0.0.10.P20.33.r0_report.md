# Dev.D.UE.0.0.10.P20.33.r0 Report

## 1. 结论

P20.33 已把 P20.32 的 live product Arc preview 捕获结果投影为 renderer-agnostic presentation snapshot。新增展示值只消费已经捕获的产品桥结果与不可变 source basis，在内部恰好调用一次既有纯几何组合器，随后输出确定性线段、最高点、计划落点和飞行时间。

展示状态由消费者持有，不拥有产品会话、库存、Run coordinator、World、Actor、组件、计时器、输入或 renderer。revision-aware reducer 支持首次安装、相同快照幂等、较新修订替换，以及清除后的 hidden tombstone；旧异步 preview 因修订号较低而不能重新出现。

新增 8 项自动化，完整 0.0.10 由 1013 增至 1021/0。按 changed-file 映射执行 8 份最终自动化证据，合计 1108/0；Editor 与 Game Development 构建均成功。

本轮没有连接 HUD/UI、World renderer、真实设备输入、trace、collision、projectile、launch、库存 reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：804b776d84b86456440623bc3042d8e746d1d2fc（P20.32）；
- 分支：agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Renderer-neutral presentation snapshot

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project() 接收：

1. 已自验证的 P20.32 captured product bridge result；
2. 不可变 Fdemo_mapShanmenThrownWeaponArcChoiceBasis。

投影器先验证两个输入，再使用桥中冻结的 choice/config 与 source basis 调用一次 P20.30 composition。组合成功后必须再次对齐：

- composition config 与 capture config；
- composition choice 与 bridge choice；
- action Run 与 lifecycle Run；
- action owner/source 与 player；
- action source item 与 hotbar item；
- action activation 与 preview activation。

任何不一致都返回 typed rejection，不输出可见状态。成功时，preview 的相邻采样点被转成 renderer-neutral line segments；presentation 不执行 draw，也不持有任何渲染对象。

## 4. 确定性身份与不可变线段

每个 segment 记录 source preview ID、顺序 index、start/end，并通过独立 r1 命名空间和 IEEE-754 坐标位模式派生 SegmentId。segment 自验证会重算该 ID，并要求有限坐标、合法 index 和有效来源。

PresentationStateId 由 mode、Run、player、item、choice state/revision，以及可见状态的 product request、preview activation、preview 和全部有序 segment ID 派生。状态字段均为私有，只允许 projector/reducer 构造；消费者只能读取和按值复制。

Visible 状态还逐段验证：segment 数量等于 preview segment count、每段严格对应 positions[i]→positions[i+1]，apex、landing 与 flight time 与 source preview 相同，并且 source action 与状态 scope 一致。

## 5. Revision-aware replace/clear

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer 是无状态纯值 reducer：

- 空 previous state 可安装首个 visible candidate；
- 完全相同 candidate 返回 Duplicate，不产生第二状态；
- Run/player/item scope 不同返回 IdentityMismatch；
- 较旧 revision 返回 StaleRevision；
- 相同 revision 但不同状态返回 RevisionConflict；
- 只有严格较新 revision 可以替换。

Clear() 要求一个有效 previous state 和严格较新的有效 choice。若新 choice 仍是带 target 的 BallisticArc，必须走 Replace 而不是 Clear；否则生成 Hidden tombstone。

Hidden tombstone 只保留 Run/player/item scope 与较新 choice/revision，清空 product request、preview activation、source preview、segments、apex、landing 和 flight time。它因此既不泄露旧几何，也能拒绝清除前迟到的旧修订。

## 6. Typed 状态

Project 区分 Invalid、BridgeRejected、BasisRejected、CompositionRejected、IdentityMismatch、StateRejected 与 Projected。

Reducer 区分 Invalid、Replaced、Cleared、Duplicate、CandidateRejected、PreviousStateRequired、PreviousStateInvalid、IdentityMismatch、StaleRevision、RevisionConflict、ChoiceRejected、ClearNotRequired 与 StateRejected。

拒绝结果不携带伪造的可见状态；应用结果和 duplicate 结果都要求其状态重新通过自验证。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation 新增 8 项：

1. VisibleSnapshot：真实 P20.32 bridge 投影到精确线段，产品权威与 sequence 不变；
2. DeterministicDuplicate：相同输入逐值重放，duplicate 幂等；
3. NewerRevisionReplace：较新 choice revision 可替换旧快照；
4. ClearTombstone：非 preview choice 清除全部几何并保留较新修订；
5. RetargetAfterClear：清除后更高修订的 Arc target 可重新显示；
6. ScopeFence：foreign Run/player/item candidate 不能污染消费者状态；
7. SourceFences：非法 bridge、basis、不可达 geometry 均 typed fail-closed；
8. ClearFences：空/非法 previous、非法/旧/等修订 choice 与不必要 clear 均被拒绝。

专属测试首轮即为 8/0，没有生产代码修正轮。完整 0.0.10 为 1021/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewPresentation 规则，覆盖 presentation、P20.32 product bridge、P20.31 capture、P20.30 composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 19 个 required groups。

正反 mapping self-test 由 359 增至 361/361。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=19 Logs=8，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_presentation_final.log | Product.ThrownWeaponArcPreviewPresentation | 8/0 | 437FBDE5842C8E73611EE1199D8B0175A8BB6068E230BC837F58975E7D805E92 |
| arc_preview_product_bridge_final.log | Product.ThrownWeaponArcPreviewProductBridge | 7/0 | D518BAEBB7104E44138BBD5F1852B82505DEC62E40499BAF25EFECA87326AB6F |
| arc_preview_capture_final.log | Product.ThrownWeaponArcPreviewCapture | 8/0 | 31F12420E5DFF721EACA9E3F8BE3ABFB54976C404AAECD468D8F73933E2D0014 |
| arc_preview_composition_final.log | Product.ThrownWeaponArcPreviewComposition | 7/0 | AA55BE7D315A4747CF6DE13E3B32D0A752F7E1F2EE7A169C7D641C71DA5FED03 |
| thrown_product_lifecycle_final.log | Product.ThrownWeaponProductLifecycle | 5/0 | BAE98433FCF68C8D77A9C272FFE059D7D943742596D070AC39E7CB2A0AFA3E06 |
| thrown_product_session_final.log | Product.ThrownWeaponProductSession | 6/0 | 93F249FCEA515BFDC90C9B0378F50AF5CF65C065D4A25117196DE3080C3DA562 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 5BC229A22905D469716E0DCD39230BC47A14212B48668340FA56F83569F637D7 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1021/0 | 7D8AB6D4BE7CE49619F25A0A601BB0741CE6A8CF99D05E53CB61A5D41C3FE708 |

最终自动化证据合计 1108/0。每份日志均有唯一 RunTests、精确成功数、零 fail/not-run、唯一 UE 5.8 原生成功终止标记与零 fatal/unhandled/ensure/no-match。

- automation audit：PASS，SHA-256 F5CD021171FD9379BA06E57159AC91CA329843954A045B9B7F85151FD5EE08A7；
- regression self-test：361/361，SHA-256 9F50B3821680D59169C35A47206FB4C75C87924E2D6FDF9C4FC432365607C249；
- boundary scan：PASS，forbidden mutation/World call 0、composition call 1、mutable authority parameter 0、World/UI include 0、placeholder 0，SHA-256 872D0308EC904FB3E63A2617A62A84DD07D9634EA531ACCDD3EB704DEDA53B95；
- changed-file gate：PASS Changed=5 Rules=2 Required=19 Logs=8，SHA-256 2DCE69448BE743B98442A82C592238172C3A0C5B5AF52705003C442EB75E3BFE；
- staged diff check：PASS，7 个精确暂存文件、103 个历史未跟踪文件保持在外、0 个空白错误，SHA-256 751C42BD50172C022C81076AD8BB1E43A8E83B41C7D38EBD9D924D1984DDD1EF；
- final Editor：up to date / native 0 / 1.94 秒，SHA-256 328A988F7B10F1DD7AD6A0484AFF9A20DCD724D122AE325F6626301703714C88；
- final Game：4 actions / native 0 / 33.37 秒，SHA-256 C871E5E55F5471147A6E9B720803A7B833E9C8C5B303FE133EDFC3E2E58AC41C。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,640,512 bytes，SHA-256 8DF1F7CE3A3580A65526ED802E524961823565DE6DB01D23C063A769B9739871；
- Binaries/Win64/demo_map.exe：357,857,792 bytes，SHA-256 A659B1E8988AC2DEB5FF65CBE090A27541866FA0D81D0FBD6E47DEF0A19E210D。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：P20.32 captured product preview 到 renderer-neutral snapshot 的单次组合、确定性 segment/state identity、scope 与 source action 交叉验证、严格 revision replace、幂等 duplicate、clear tombstone、stale/conflict/foreign/source typed rejection、产品 authority/sequence/Host/selection 不变，以及完整 0.0.10 与 changed-file 回归。

未验证：HUD/UI presentation、真实 renderer 或可见轨迹、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.34 建立 renderer-independent Arc preview update coordinator：组合 live capture/project/replace-or-clear 为一个 bounded transition，并让调用方只持有 presentation state；仍不连接真实 HUD、World 或 renderer。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.33.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.33.r0_log.md>
