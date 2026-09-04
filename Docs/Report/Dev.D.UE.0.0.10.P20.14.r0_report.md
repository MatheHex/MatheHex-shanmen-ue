# Dev.D.UE.0.0.10.P20.14.r0 Report

## 1. 结论

P20.14 已在 P20.13 的 consumer-owned 惰性组合层之上增加唯一 source-Actor basis adapter。新路径继续让既有 InputAdapter 先完成 hotbar item、active Run、source、Action gate、selection ordinal、lifecycle/session/controller/host/world delivery 分类；只有该路径真正请求 Arc target 时，适配器才读取调用方提供的 `AActor` transform 快照一次，并把同一份 location/forward/right basis 交给 P20.13。

Arc launch origin 高度不再在两个适配层分别写死。P20.8 InputAdapter 暴露唯一 `GetLaunchOriginHeight()`，其原路径和本轮 source adapter 都读取同一常量。适配器不保留 Actor 指针，不查询 World，不查找玩家，不创建新的 source、item、Run 或产品权威。

最终验证为：source-basis exact `6/0`、composition `5/0`、projection `5/0`、choice `9/0`、InputAdapter `9/0`、0.0.10 全量 `888/0`，以及四组 GameMode 旧回归 `4/0 + 44/0 + 22/0 + 46/0`；十份日志合计 `1038/0`。Editor 与 Game Development 构建均成功。changed-file regression gate 为 `PASS Changed=9 Rules=3 Required=58 Logs=10`。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实设备输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`7f062f618f29cdab1b7c47eb1d173ccf9c86fc59`（P20.13）；
- 分支：`agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Source basis 契约

新增 `Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter`、不可变 sample result 与 route result。sample 状态显式区分：

- `Sampled`：一个 live source Actor 产生有效 canonical basis；
- `SourceUnavailable`：Actor 为空、无效或正在销毁，transform 读取数为 0；
- `TransformInvalid`：一次 transform 快照包含非有限值；
- `BasisRejected`：一次快照未能形成 P20.12 的 canonical basis。

route 状态区分提前完成、source 拒绝、组合完成和组合协议拒绝，并记录 route invocation、source sample request、transform snapshot 与 P20.13 composition evidence。合法 route 恰好调用一次，source 最多采样一次；`IsAccepted()` 仍由有效组合结果和既有 InputAdapter 接受结果共同决定。

## 4. 惰性采样与坐标快照

`Route` 先把惰性 basis callback 交给 P20.13。只有 P20.13 在既有 InputAdapter 认领输入后请求 basis，该 callback 才调用 `Sample(SourceActor)`。因此非暗器槽、无 Run 等提前结束路径不会访问 Actor transform。

`Sample` 只调用一次 `GetActorTransform()`，随后从该局部不可变 `FTransform` 读取 location、X 轴 forward 和 Y 轴 right。origin 为 location 加唯一共享 launch height `50uu`，再由 P20.12 `TryCapture` 规范化并生成确定性 basis identity。移动或旋转 source 会产生对应的新 origin/directions 与 identity；相同快照可重放同一 identity。

适配器只在同步调用栈内使用 `SourceActor`，结果中只保留纯值 basis 与审计字段，没有 UObject/World 所有权。

## 5. 失败关闭与协议边界

null source 在真正请求 basis 时返回 `SourceUnavailable`，P20.13 保留 `ProjectionRejected/BasisInvalid`，既有 InputAdapter 以 `TargetUnavailable` 结束且不请求 apex。未认领输入则直接为 `InputCompletedBeforeSourceSample`，source、basis、target、apex 工作全部为 0。

重复 basis 请求不会再次采样 source。apex-before-target、重复 target 或 P20.13 次数审计不一致会升级为 `CompositionProtocolRejected`。有效 source 但后续 projection 拒绝仍归 `Composed`，因为 source 适配职责已正确完成，精确投射失败原因继续由 P20.13/P20.12 保存。

## 6. GameMode 接入与权威

GameMode 新增 `RouteThrownWeaponArcChoiceFromSourceHotbarInput`。它只把 slot、同一个 `SourceActor` 与按值冻结的 policy 委托给 source adapter，再调用现有 `RouteThrownWeaponArcChoiceHotbarInput`。后者继续读取唯一 `ThrownWeaponInputChoiceSession`，并最终进入现有 Arc InputAdapter route。

旧 direct Arc route 和 P20.13 choice route均保留；本轮没有替换它们，也没有复制 item、Run、source、Action gate、selection ordinal、lifecycle、session、controller、host 或 world delivery。`Policy` 在同步 callback 中按值捕获，避免引用生命周期扩散。

## 7. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter` 6 项：

- `SampleContract`：location/yaw 到 origin/forward/right、共享 50uu 高度与 identity 重放；
- `LazyClassification`：提前 PassThrough 时 route 一次、source/basis/target/apex 全为 0；
- `ProjectAndDelegate`：canonical source 得到 target `(1100, 350, 50)`、apex `350`，各阶段一次；
- `SourceFailure`：null source 零 transform snapshot，投射与 InputAdapter 失败关闭；
- `ProtocolFailure`：apex-before-target 与 duplicate-target 不重复采样；
- `GameModeBoundary`：无 GameInstance 的未认领路径不采样，choice identity/revision 保持不变。

最终日志：

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `source_basis_adapter_final.log` | `Product.ThrownWeaponArcSourceBasisAdapter` | `6/0` | `BF2862643BED00FCF1CCDA2048AC92DB2DFA063270911668F208E1AAAA9E5C57` |
| `arc_choice_composition_final.log` | `Product.ThrownWeaponArcChoiceInputComposition` | `5/0` | `1A9795E23A958281A7698990B4FF4ABDF111BAF93332927983A4B3253290F91C` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `855CD4506F67499B76D13EE43DD8506C04BBA196FB54197BB78EF91974A06C3C` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `23473A923D22A5FBDD56993C7B252B0AF38C7B1201B723CB86788A250DE1F284` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `60535AA1E6E46A05D84D76BB326798525D317F2C885CF93FF7AE10243A134068` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `888/0` | `CCE15337047CBB0C4EDA162FBCB371F1440DB88337F64A161E968EACA169427C` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | `4/0` | `0E7394E1DB8F368192EB5D41B488F16E77343D0B545CDD653B4FE98BAF1F3DCA` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | `44/0` | `5F4F36C2C66CF597E28AC8D41BB0DC1A3EF60A8F6173F60D289260998DEEB5BE` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `49B66DFCE767BB402C694129AF2D0B49879C88B68F4FEBB55253BFFB969978AA` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | `46/0` | `154E6D154563C3D85E0FAC38F2A84FBD0A02EA9CC40C2B56FFD785347C79F812` |

十份日志均只有一个 canonical RunTests、一个 queue-empty、一个 TestExit，合计 1038 个 success marker、0 fail、0 no-match、0 fatal/unhandled/ensure。审计 SHA-256：`DF45BB0B0691DEC42AD1FBB0950C6B6277D7FEF138852F1098E952EC04B7197D`。

0.0.10 全量从 P20.13 的 882 增至 `888`，恰好增加本轮 6 项。

## 8. 首次验证、门禁与静态边界

首次 Editor 构建直接通过：31 actions / native 0，SHA-256 `CFDC1BDB6108B38E01320B46549E353E9C111AABFEA40730ADB72261961695DA`。首次 source-basis exact 直接为 `6/0`、无 fatal/unhandled/ensure，SHA-256 `5A7D6C31D63694AB4198E3CF55767229356EDC4F131A990B635DFDF435291766`；没有源码或测试断言修复。

最终聚焦验证中曾把 composition 组名误写为缺少 `Input` 的未注册名称，原生退出码为 0 但明确得到 0 tests/no-match；随后用准确组名重跑为 `5/0`，没有修改源码或断言。纠正记录 SHA-256：`37CA468A44276159D85E8850BE31E1351A62B372687C3563CE5AD2305B9CD932`。

新增 source-basis 映射，要求 source exact、composition、projection、choice、InputAdapter 与 0.0.10 full；M01 GameMode 映射增加 source-basis exact。流程自测新增 broad-evidence 正例与无关 item evidence 必须失败的反例，最终 `321/321`，SHA-256 `02A26ABA3E87F0683C275037EF511841D725EC547298FCD9B67BBACC34311754`。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=58 Logs=10
```

gate SHA-256：`333F43E959408643F8D9F398D38F1991A27BB6DAD443940F807491277096F249`。

对 source adapter 两个 production 文件的 332 行扫描 direct World lookup、spawn、trace/sweep、controller/device input、RNG、库存/物品权威、伤害与产品 begin/commit API：0 matches；对 GameMode 31 条新增行和 InputAdapter 9 条新增行做同类扫描：0 matches。placeholder 扫描为 0，映射 JSON 185 rules 可解析。结果 `PASS`，SHA-256 `AD4979670824EF4466D62D0F166D849C0A38958F80F8F2EB80AF24E1F4E69FDB`。

`git diff --cached --check`：精确暂存本轮 11 个文件后 `PASS / native 0`；证据 SHA-256：`F68675108CE8C5CAF5BDCABC704BF99BF6221D1F28288669A5192C1D725A8780`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：4 actions / native 0，SHA-256 `CBC425BE80A8C6A632975B2B6FA8427858FFC1721CDE7699345691CFA2567A31`；
- final Game：30 actions / native 0，SHA-256 `1C8B262CB0E91F0818CCF21EA1591FB099E348574B58D95A704FAB68C3B20B9B`。

产物：

- `Binaries/Win64/demo_map.exe`：`357315584` bytes，SHA-256 `6119778BD32532F152C74FD293216C983BAF47832FC4DDA5645766AB888CEDF9`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`16002560` bytes，SHA-256 `305275352E352344D3EEFD4954F0D8F0F9C780A296E91A90C4EEB9ED1147ED86`。

headless 日志中的非 Win64 SDK 信息、UE 自带 UnifiedErrorTest 启动输出与 `generate_204` 网络探测警告属于既有环境噪声；Win64 构建、目标测试 terminal 与原生退出码均成功。

## 10. P/F 边界与下一步

P20.14 证明的是：“既有 InputAdapter 认领 Arc 输入后，可以惰性地从调用方指定的 live source Actor 读取恰好一个 transform 快照，使用唯一共享 launch height 生成 canonical basis，并继续委托 P20.13/P20.12 与唯一现有产品 route；提前结束、source 失效和协议错误均可审计且失败关闭。”

它没有证明物理设备输入、焦点/输入模式、UI、轨迹预览、真实注册 source 的成功产品路径、world trace、碰撞、命中、库存扣减或伤害。`888/0` 不能描述为产品运行验收。

建议 P20.15 增加一个 device-independent Arc launch command seam：由既有 PlayerController 输入边界提供 hotbar slot 与 canonical pawn，只委托本轮 GameMode source route；先以纯命令/无头测试冻结焦点与输入模式栅栏，不增加 UI、预览或第二套产品路由。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.14.r0_log.md>
