# Dev.D.UE.0.0.10.P20.63.r0 Development Log

## 基线与目标

- base：`dcd28717464e1ac130f88b5a5c2dd0e35cd72833`；
- branch：`agent/0.0.10-p20-63-thrown-weapon-prelaunch-arc-preview-context`；
- 目标：建立唯一、Run-scoped 的 BallisticArc 发射前热栏预览选择，并把 P20.62 已开放的 live Arc edit 接到既有 MainHUD runtime binding；
- 边界：不复制 item、inventory、Run、choice、launch、World 或 presentation 权威；不启动产品界面。

## 调查记录

1. P20.62 允许 combat Run 中修改 Arc target/apex/clear，但 P20.61 preview 仍只在最终确认链内同步更新，因此没有可编辑的发射前展示窗口。
2. 热栏确认链已拥有暗器识别、reservation、snapshot capture、activation sequence 与发射结果；新阶段不能再建一套 launch authority。
3. product lifecycle 可以只读查询某热栏槽是否绑定当前暗器及其冻结 trajectory，足以判定 Arc 预览资格，不需要读取或修改库存。
4. MainHUD runtime binding 已拥有唯一 preview composition owner，但缺少“隐藏当前预览且不改调用方 choice”的公共完成入口。
5. choice intent 路由成功后，PlayerController 同时持有 GameMode 与 Pawn source basis，可在不改变 reducer/session 结果的情况下请求展示刷新。

## 状态机设计

- state：RunId、ArmedHotbarSlotNumber、Revision；
- first eligible press：`Armed`，revision +1；
- different eligible press：`Rearmed`，revision +1；
- same eligible press：`ConfirmationRequested`，revision 不变；
- cancel：有选择时清除并 revision +1，空态为 no-op；
- rejected confirmation：保持选择与 revision；
- accepted confirmation：清除并 revision +1；
- begin/end 与所有 mutate/complete 均要求精确 Run identity；
- 槽位限定 1–9，revision exhaustion 失败关闭。

## 生产实现

新增：

- `demo_mapShanmenThrownWeaponArcPreLaunchPreviewContext.h/.cpp`：纯 Run-scoped 选择与审计结果；
- GameMode `RouteThrownWeaponArcPreLaunchHotbarInput`：Straight 委托、非暗器 pass-through、Arc arm/re-arm/confirm；
- GameMode `RefreshThrownWeaponArcPreLaunchPreview`：accepted target/apex/clear/trajectory edit 的有界刷新；
- GameMode `CompleteThrownWeaponArcPreLaunchConfirmation`：根据既有 launch result 保留或清除；
- runtime binding `TryClear`：在本地 choice copy 上归约 target clear，通过唯一 owner 隐藏 surface；
- PlayerController 热栏路由：first press 消费为 preview，same-slot press 才调用既有确认链；
- PlayerController choice route：accepted edit 后请求 presentation refresh，失败只记录诊断。

生命周期接入：

- combat Run activation 在产品链成功后绑定空 context；
- context bind 失败会调用既有 product rollback；
- release 前先检查 context/coordinator Run 一致；
- preview binding 清理后结束 context；
- target clear 与 Straight trajectory 都会隐藏预览并取消 armed slot；
- accepted launch 清理 context 与 preview，rejected launch 保留两者。

## 自动化改动

新增纯上下文测试：

- `ArmRearmConfirmAndComplete`；
- `CancelAndRunFences`。

新增生命周期组合测试：

- `LiveEditCancelAndConfirmation`：首按可见、apex edit 即时替换、reject 保留、target clear 取消、重新选择、同槽确认、成功清理；
- 每一步都断言 activation sequence 与 captured selections 不变，证明发射前状态不提前消耗产品资源。

扩展 runtime binding teardown 测试，显式证明 `TryClear` 隐藏 surface、binding 仍 active、调用方 authoritative choice 未被修改，随后 matching teardown 可结束已隐藏 binding。

## 编译与正式自动化

- integration Editor build：4 actions / native 0 / 9.56 s；
- post-fix Editor build：5 actions / native 0 / 17.45 s；
- context focus：3/0 / native 0；
- MainHUD runtime binding focus：5/0 / native 0；
- InputRestore：101/0 / native 0；
- V2RangedCompatibility：22/0 / native 0；
- ItemUseAndArmor：46/0 / native 0；
- V3.Attributes：4/0 / native 0；
- EnemySkillFramework：44/0 / native 0；
- full `Shanmen.0_0_10`：1213/0 / native 0；
- formal automation total：1438/0；
- terminal-success markers：8/8；
- Fail/Fatal/Unhandled/Ensure：0/0/0/0。

正式日志：

| File | Bytes | SHA-256 |
|---|---:|---|
| `P20.63.r0_context.log` | 261,267 | `C199544FA0C931C3D269B7232EBEF58591BDD99E697100DEA629E646930195EF` |
| `P20.63.r0_runtime_binding.log` | 264,533 | `D8F112261E0468A82D3B7D67298270868DB503BB03D2B5E2C7B38CA8DC504081` |
| `P20.63.r0_input_restore.log` | 390,634 | `56DF4982317FCF454E7AD2F9D9C31FB783638D1AB422CAA391825F8E1EC49650` |
| `P20.63.r0_ranged.log` | 280,224 | `4A7ECAA49E8ADC678E1922D3F4FE1F79888143E3A1BFE7A4EF0278F949C92F7E` |
| `P20.63.r0_item_use.log` | 305,482 | `94C1350C8B810424BF4DB440D0CDB14D15BAF92ED9FD0F8D3626C3EC93CDDABE` |
| `P20.63.r0_attributes.log` | 260,874 | `E5E2A6F6221685A4ABB49F9BCE4269BC8D418DF59AB08E714CB647532A690B75` |
| `P20.63.r0_enemy.log` | 300,552 | `A4F14FEBD05AAC0A30C464373071F50D0B8F257357A60550CB93AA6359F89997` |
| `P20.63.r0_full.log` | 1,818,670 | `CBBA1F6BBE67F77B9F34FE119DF0D2FF5B8FACCE8D9EEA66231C24056515DDCF` |

## Changed-file Regression

- mapping JSON：新增 pre-launch context 规则，并把 GameMode、PlayerController、MainHUD runtime binding 映射到该组；
- self-test：417/417，41,293 bytes，SHA-256 `C940658BF0A03CF1EAEE3B8A80FCE55E0FA75CA0249E2A76BBE1D15FCAE00863`；
- changed files：11；
- matched rules：5；
- required groups：78；
- evidence logs：8；
- gate：`REGRESSION_COVERAGE: PASS Changed=11 Rules=5 Required=78 Logs=8`；
- gate log：9,734 bytes，SHA-256 `9D1B80EE3AE3F9D1FDBA4F228AA4072BED702F1C266F15D25A2D0E94D4B92AA7`。

## 静态与差异审计

- pure context production：2 files / 405 lines；
- UWorld/AActor/UObject/Tick/timer/async/inventory/damage/spawn/trace/sweep/RNG/TODO/FIXME/HACK：0；
- static log：211 bytes，SHA-256 `CB0CCBC6903A9E9BD63EC4C7BC3FCEDD9EB47A69B2F07337070410FE7F3B4611`；
- source/test/mapping：11 files / 1282 additions / 32 deletions；
- `git diff --check`：PASS，仅 LF→CRLF 工作区提示；
- 三个测试框架生成的重复 backup raw logs 已按精确路径删除；正式日志未改；
- 约 100 个用户已有 untracked Prompt/Report/文档保持未暂存、未修改。

## 最终构建与产物

- Game：34 actions / 37.48 s / Succeeded / native 0；log 4,310 bytes / SHA-256 `34CE631457287E739E296E89C9BB7D648DD399DA9C1EFA24659E2D8831D773A6`；
- Editor：up to date / 0 actions / 0.94 s / Succeeded / native 0；log 1,014 bytes / SHA-256 `2F32A3F18136D6EF4E1083B1EEE456207858EF3F4579420C86D45398AD8BC18E`；
- `demo_map.exe`：359,246,336 bytes / SHA-256 `AA55DEFD3017E21C1C24D32FD9C2EEEFD8006065FC836F1B07C6F9F364DA8E22`；
- `UnrealEditor-demo_map.dll`：18,374,144 bytes / SHA-256 `FA8A5497A79F419FAEC997D32EF94AEBA220378C7B75668BD1DF05610AE9C4BD`。

## P/F 边界

PASS：arm/re-arm/confirm/cancel、live edit preview refresh、reject preservation、accepted cleanup、Run fence、Straight/non-thrown delegation、caller choice isolation、pre-launch zero activation/capture consumption、legacy compatibility、full regression、Game/Editor build。

未声明：真实 MainHUD 提示质量、真实热栏输入、viewport/DPI/遮挡、Editor UI、PIE、Standalone、截图、Smoke、Cook、Package。

## 下一阶段

P20.64 建议建立只读 pre-launch gesture feedback projection，把 armed slot、可编辑状态和 same-slot confirmation 提示投影给 MainHUD；继续让 context、choice session、product lifecycle 与 launch chain 保持各自唯一权威。真实 UI/输入验收另行授权。
