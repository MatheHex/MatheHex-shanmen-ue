# Dev.D.UE.0.0.10.P20.63.r0 Report

## 1. 结论

P20.63 已建立投掷武器 BallisticArc 的唯一发射前预览上下文，并把它接入现有热栏输入、运行期 Arc 编辑、MainHUD preview binding 与 combat Run 生命周期。

现在同一个已绑定暗器热栏槽具备明确的两段手势：第一次按下只选中该槽并显示发射前弧线预览；同槽第二次按下才委托既有确认/发射链。选择另一个有效暗器槽会移动预览选择。Arc target 或 apex 的已接受编辑会即时重算已选预览；清除目标或切回 Straight 会隐藏并取消选择；发射被拒绝时保留可修正预览，发射接受后才清除上下文与显示。

新上下文只拥有 RunId、已选热栏槽与 revision，不拥有物品、库存、trajectory choice、launch command、Actor/World 或 presentation state。第一次按下、实时编辑、取消与拒绝路径均不捕获一次产品发射选择，也不推进投掷 activation sequence。

正式自动化 1438/0：新上下文 3/0、MainHUD runtime binding 5/0、五组旧系统兼容 217/0、0.0.10 全量 1213/0。映射自检 417/417；changed-file regression gate 对 11 个源码/脚本文件推导出 78 个必跑组，全部有健康日志覆盖。Game 构建 34/34 actions、Editor 构建 up to date，均 native 0。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`dcd28717464e1ac130f88b5a5c2dd0e35cd72833`（P20.62）；
- 分支：`agent/0.0.10-p20-63-thrown-weapon-prelaunch-arc-preview-context`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 产品手势与权威边界

BallisticArc 热栏输入现在按下列有限状态机工作：

1. 第一次按下有效暗器槽：更新既有 preview owner，并将该槽 `Armed`；
2. 按下另一个有效暗器槽：更新预览并将选择 `Rearmed` 到新槽；
3. 再次按下同一槽：上下文不变，只产生 `ConfirmationRequested`；
4. 既有发射链拒绝：选择、revision 与可见预览保持不变；
5. 既有发射链接受：清除选择并通过 runtime binding 隐藏预览；
6. 清除 Arc target 或切回 Straight：隐藏预览并取消选择；
7. combat Run 结束：匹配 Run 的上下文与 preview binding 都必须释放。

Straight 模式继续委托原热栏链；BallisticArc 下非暗器槽继续 pass-through。未知 trajectory、失配 Run、非法槽位、产品 trajectory 不一致、无有效 source basis 或 preview 更新失败均失败关闭。

## 4. 实现

新增 `Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext`：

- 只记录一个 active Run、1–9 的可选热栏槽和单调 revision；
- same-slot confirm 不推进 revision；
- cancel、re-arm 与 accepted completion 各只推进一次；
- rejected completion 不改变状态；
- begin/end/mutate/complete 均执行精确 Run fence；
- revision 耗尽、非法状态和跨 Run 操作失败关闭。

GameMode 新增三个有界入口：

- `RouteThrownWeaponArcPreLaunchHotbarInput` 只做 read-only 产品槽识别、预览更新与 arm/re-arm/confirm 路由；
- `RefreshThrownWeaponArcPreLaunchPreview` 只在已有 armed slot 时响应 accepted target/apex/clear/trajectory edit；
- `CompleteThrownWeaponArcPreLaunchConfirmation` 只根据既有 launch 结果保留或清除预览。

PlayerController 在热栏输入中先询问该入口；只有 same-slot confirmation 才继续原有 thrown confirmation route。choice intent 仍由既有 reducer/session 处理，成功后才请求预览刷新；presentation 刷新失败会记录诊断，但不会伪造 choice edit 失败或另建 choice 权威。

## 5. Preview 清理与生命周期

MainHUD runtime binding 增加显式 `TryClear`。它从调用方 choice 复制出本地 hidden choice，并继续通过唯一 composition owner 更新 surface；不会修改调用方权威 choice。Run teardown 复用同一内部 clear 路径，避免完成清理与 teardown 各维护一套算法。

combat Run 启动会绑定空的 pre-launch context；绑定失败时回滚已启动产品。Run 释放先验证 context 与 coordinator 的 Run 一致，再清理 preview binding 和 context。旧 Run、第二 active Run、错误槽完成与重入操作均不被接受。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `P20.63.r0_context.log` | `Product.ThrownWeaponArcPreLaunchPreviewContext` | 3/0 | `C199544FA0C931C3D269B7232EBEF58591BDD99E697100DEA629E646930195EF` |
| `P20.63.r0_runtime_binding.log` | `Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5/0 | `D8F112261E0468A82D3B7D67298270868DB503BB03D2B5E2C7B38CA8DC504081` |
| `P20.63.r0_input_restore.log` | `demo_map.InputRestore` | 101/0 | `56DF4982317FCF454E7AD2F9D9C31FB783638D1AB422CAA391825F8E1EC49650` |
| `P20.63.r0_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `4A7ECAA49E8ADC678E1922D3F4FE1F79888143E3A1BFE7A4EF0278F949C92F7E` |
| `P20.63.r0_item_use.log` | `demo_map.ItemUseAndArmor` | 46/0 | `94C1350C8B810424BF4DB440D0CDB14D15BAF92ED9FD0F8D3626C3EC93CDDABE` |
| `P20.63.r0_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `E5E2A6F6221685A4ABB49F9BCE4269BC8D418DF59AB08E714CB647532A690B75` |
| `P20.63.r0_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | `A4F14FEBD05AAC0A30C464373071F50D0B8F257357A60550CB93AA6359F89997` |
| `P20.63.r0_full.log` | `Shanmen.0_0_10` | 1213/0 | `CBBA1F6BBE67F77B9F34FE119DF0D2FF5B8FACCE8D9EEA66231C24056515DDCF` |

八份正式自动化日志合计 1438 Success、0 Fail，均含 native terminal-success marker，Fatal/Unhandled/Ensure 为 0。全量由 P20.62 的 1210 增至 1213，增量正好对应本轮新增的两项纯上下文测试与一项生命周期组合测试。

## 7. 关键行为证据

自动化分别证明：

- arm、跨槽 re-arm、同槽 confirm、reject preserve、accepted clear；
- cancel no-op、非法槽、跨 Run、错误槽完成与 lifecycle teardown fence；
- 首按预览、apex 实时更新、target clear 取消、再次选择与成功完成的完整组合；
- 整个发射前过程不推进 `GetNextPlayerThrownWeaponActivationSequence()`；
- 整个发射前过程不增加 lifecycle captured selections；
- `TryClear` 隐藏 surface 但保留调用方 choice 中的 target intent；
- 切回 Straight 时先隐藏旧 Arc preview，再取消 armed slot，避免以后切回 Arc 误触发旧选择确认；
- 旧 InputRestore、ranged、item/armor、attributes 与 enemy framework 契约保持通过。

## 8. Changed-file Regression 与静态边界

- mapping self-test：417/417，SHA-256 `C940658BF0A03CF1EAEE3B8A80FCE55E0FA75CA0249E2A76BBE1D15FCAE00863`；
- changed-file gate：`PASS Changed=11 Rules=5 Required=78 Logs=8`；
- gate SHA-256：`9D1B80EE3AE3F9D1FDBA4F228AA4072BED702F1C266F15D25A2D0E94D4B92AA7`；
- 新 context 生产边界：2 files / 405 lines / 0 matches；
- 扫描项：World/Actor/UObject、Tick/timer/async、inventory、damage、spawn、trace/sweep、RNG、TODO/FIXME/HACK；
- 静态日志 SHA-256：`CB0CCBC6903A9E9BD63EC4C7BC3FCEDD9EB47A69B2F07337070410FE7F3B4611`；
- 主体源码/测试/映射改动：11 files / 1282 additions / 32 deletions；
- `git diff --check`：PASS，仅有工作区既有 LF→CRLF 提示；
- 用户已有未跟踪文件未暂存、未修改。

## 9. 构建与产物

- final Game：34 actions / Succeeded / native 0 / 37.48 s；构建日志 SHA-256 `34CE631457287E739E296E89C9BB7D648DD399DA9C1EFA24659E2D8831D773A6`；
- final Editor：up to date / 0 actions / Succeeded / native 0 / 0.94 s；构建日志 SHA-256 `2F32A3F18136D6EF4E1083B1EEE456207858EF3F4579420C86D45398AD8BC18E`。

产物：

- `Binaries/Win64/demo_map.exe`：359,246,336 bytes，SHA-256 `AA55DEFD3017E21C1C24D32FD9C2EEEFD8006065FC836F1B07C6F9F364DA8E22`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,374,144 bytes，SHA-256 `FA8A5497A79F419FAEC997D32EF94AEBA220378C7B75668BD1DF05610AE9C4BD`。

## 10. P/F 边界与下一步

PASS：唯一 pre-launch context、first-press arm、different-slot re-arm、same-slot confirm、live target/apex refresh、clear/Straight cancel、reject preserve、accepted cleanup、Run fencing、调用方 choice 不变、发射前零资源/序号消耗、旧系统兼容、全量回归、Editor/Game 构建。

未声明：真实热栏按键手感、HUD 中“已选槽/再次按下确认”的文字或图标反馈、viewport/DPI/遮挡、真实鼠标键盘输入、Unreal Editor UI、PIE、Standalone、截图、Smoke、Cook 或 Package。

建议 P20.64 在不复制 context 或 launch 权威的前提下，增加只读的 pre-launch gesture feedback projection，使 MainHUD 能明确展示 armed slot、可编辑状态与同槽二次确认提示；真实 UI/输入验收继续作为单独授权阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-63-thrown-weapon-prelaunch-arc-preview-context>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-63-thrown-weapon-prelaunch-arc-preview-context/Docs/Report/Dev.D.UE.0.0.10.P20.63.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-63-thrown-weapon-prelaunch-arc-preview-context/Docs/Log/Dev.D.UE.0.0.10.P20.63.r0_log.md>
