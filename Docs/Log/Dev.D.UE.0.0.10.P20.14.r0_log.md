# Dev.D.UE.0.0.10.P20.14.r0 Development Log

## 1. 目标与基线

- 基线：`7f062f618f29cdab1b7c47eb1d173ccf9c86fc59`（P20.13）；
- 分支：`agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter`；
- 目标：在 P20.13 真正请求 basis 时，从 caller-supplied canonical source Actor 采样一次 transform 并委托既有 Arc route；
- 约束：不复制产品权威，不做设备绑定、UI/preview、world trace、库存、projectile 或伤害开发。

## 2. 接入前审计

P20.8 已定义“先 hotbar/item/Run/lifecycle 分类，再按轨迹一次性采样几何”的唯一 InputAdapter；P20.9 把 Arc target/apex callback 暴露到 GameMode；P20.11 保存唯一冻结 choice state；P20.12 把 choice+basis+policy 投射成纯值 target/apex；P20.13 负责只在 InputAdapter 认领后惰性调用 basis sampler。

剩余缺口是 caller 仍需手工构造 basis，尚没有一个受限世界边界把 source Actor 的 transform 快照转换成 P20.12 值类型。本轮只填这一层，不移动 item、Run、source 或 world delivery 所有权。

## 3. Sample result 契约

新增只读 `Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult`，记录状态、诊断、transform sample count 与 canonical basis。有效成功必须恰好一个 snapshot 且 basis 有效；source unavailable 必须零 snapshot；transform/basis 拒绝必须恰好一个 snapshot 且不得携带有效 basis。

source 空、无效或销毁中时失败关闭。有限性检查覆盖 location、forward、right；最终规范性继续委托 P20.12 `TryCapture`，不在适配器复制向量正交化或 identity 规则。

## 4. 惰性 Route 实现

新增 route result，记录 route invocation、source sample request、sample result 与完整 P20.13 composition result。`RouteWithBasis` 只调用一次；其 basis callback 第一次调用才采样 Actor，后续调用返回空 basis且不重采样。

P20.13 提前结束时，source sample request 为 0。有效 source 可进入 `Composed`；无效 source 为 `SourceSampleRejected`；重复/乱序 callback 或次数不一致为 `CompositionProtocolRejected`。最终接受权仍取决于 P20.13 与既有 InputAdapter。

## 5. 单快照与共享 origin

`Sample` 只读取一次 `GetActorTransform()`，location 与 X/Y unit axes 均来自该局部快照。origin 高度读取 InputAdapter 新暴露的 `GetLaunchOriginHeight()`；InputAdapter 原实现也改为读取该函数，因此 50uu 不再双写。

结果只保留纯值 basis，不保留 `AActor*`。适配器不调用 `GetWorld()`、不查询玩家、不做 trace/sweep，不触碰 item/Run/inventory/damage/product begin/commit API。

## 6. GameMode 委派

新增 `RouteThrownWeaponArcChoiceFromSourceHotbarInput` 与一个无状态 source adapter 成员。新入口把 slot、同一个 source Actor 与按值 policy 交给 adapter；下游仍调用 P20.13 的 `RouteThrownWeaponArcChoiceHotbarInput`，继而进入现有 P20.8 route。

旧 direct Arc route 与 choice route 都未删除。transient GameMode 测试证明未认领路径不会访问 source transform，并保持冻结 choice identity/revision。

## 7. 新增自动化与首次验证

新增 exact 6 项：sample contract、lazy classification、canonical project/delegate、null source failure、protocol failure、GameMode boundary。

首次 Editor build：31 actions / native 0，SHA-256 `CFDC1BDB6108B38E01320B46549E353E9C111AABFEA40730ADB72261961695DA`。首次 exact：`6/0`、fatal/unhandled/ensure 0，SHA-256 `5A7D6C31D63694AB4198E3CF55767229356EDC4F131A990B635DFDF435291766`。没有源码或断言修复。

最终 composition 验证第一次命令遗漏组名中的 `Input`，产生 0 tests/no-match；准确重跑后为 `5/0`。该纠正未修改源码或测试，记录 SHA-256 `37CA468A44276159D85E8850BE31E1351A62B372687C3563CE5AD2305B9CD932`。

## 8. 最终自动化与 changed-file gate

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `source_basis_adapter_final.log` | `6/0` | `BF2862643BED00FCF1CCDA2048AC92DB2DFA063270911668F208E1AAAA9E5C57` |
| `arc_choice_composition_final.log` | `5/0` | `1A9795E23A958281A7698990B4FF4ABDF111BAF93332927983A4B3253290F91C` |
| `arc_choice_projection_final.log` | `5/0` | `855CD4506F67499B76D13EE43DD8506C04BBA196FB54197BB78EF91974A06C3C` |
| `input_choice_final.log` | `9/0` | `23473A923D22A5FBDD56993C7B252B0AF38C7B1201B723CB86788A250DE1F284` |
| `input_adapter_final.log` | `9/0` | `60535AA1E6E46A05D84D76BB326798525D317F2C885CF93FF7AE10243A134068` |
| `full_0_0_10_final.log` | `888/0` | `CCE15337047CBB0C4EDA162FBCB371F1440DB88337F64A161E968EACA169427C` |
| `legacy_v3_attributes_final.log` | `4/0` | `0E7394E1DB8F368192EB5D41B488F16E77343D0B545CDD653B4FE98BAF1F3DCA` |
| `legacy_enemy_skill_final.log` | `44/0` | `5F4F36C2C66CF597E28AC8D41BB0DC1A3EF60A8F6173F60D289260998DEEB5BE` |
| `legacy_v2_ranged_final.log` | `22/0` | `49B66DFCE767BB402C694129AF2D0B49879C88B68F4FEBB55253BFFB969978AA` |
| `legacy_item_armor_final.log` | `46/0` | `154E6D154563C3D85E0FAC38F2A84FBD0A02EA9CC40C2B56FFD785347C79F812` |

日志审计：每份 1 个 canonical RunTests、1 个 queue-empty、1 个 TestExit，合计 success 1038、fail 0、no-match 0、fatal/unhandled/ensure 0；SHA-256 `DF45BB0B0691DEC42AD1FBB0950C6B6277D7FEF138852F1098E952EC04B7197D`。

映射新增 source-basis rule，并把 source exact 加入 M01 GameMode required groups。流程自测为 `321/321`，SHA-256 `02A26ABA3E87F0683C275037EF511841D725EC547298FCD9B67BBACC34311754`。

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=58 Logs=10
```

- gate SHA-256：`333F43E959408643F8D9F398D38F1991A27BB6DAD443940F807491277096F249`；
- production boundary：source adapter 332 行 + GameMode 31 条新增行 + InputAdapter 9 条新增行，0 forbidden matches；
- boundary SHA-256：`AD4979670824EF4466D62D0F166D849C0A38958F80F8F2EB80AF24E1F4E69FDB`；
- placeholder scan：0 matches；JSON mapping parse：PASS / 185 rules；
- `git diff --cached --check`：精确暂存 11 个文件后 PASS / native 0；证据 SHA-256 `F68675108CE8C5CAF5BDCABC704BF99BF6221D1F28288669A5192C1D725A8780`；
- 长期未跟踪文件不纳入暂存、提交或推送。

## 9. 最终构建与产物

- final Editor：4 actions / native 0，SHA-256 `CBC425BE80A8C6A632975B2B6FA8427858FFC1721CDE7699345691CFA2567A31`；
- final Game：30 actions / native 0，SHA-256 `1C8B262CB0E91F0818CCF21EA1591FB099E348574B58D95A704FAB68C3B20B9B`；
- `demo_map.exe`：`357315584` bytes / `6119778BD32532F152C74FD293216C983BAF47832FC4DDA5645766AB888CEDF9`；
- `UnrealEditor-demo_map.dll`：`16002560` bytes / `305275352E352344D3EEFD4954F0D8F0F9C780A296E91A90C4EEB9ED1147ED86`。

只执行编译与无头自动化，没有启动产品。

## 10. P/F 边界与后续判断

本轮证明 source Actor 的一个 transform 快照可按惰性、单次、可审计协议进入 P20.13，且 launch origin 与既有 InputAdapter 使用同一权威常量。它不证明物理输入、焦点、UI/preview、真实产品成功路径、trace、命中或伤害。

P20.15 应增加 device-independent Arc launch command seam：从既有 PlayerController 输入边界提供 slot 与 canonical pawn，只委托本轮 GameMode source route；先冻结焦点/输入模式栅栏，不接 UI 或预览。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-14-thrown-weapon-arc-source-basis-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.14.r0_log.md>
