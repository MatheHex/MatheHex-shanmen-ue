# Dev.D.UE.0.0.10.P20.66.r0 Development Log

## 基线与目标

- base：47bbff2d278b3c886d69aea6b0b2cbb09fb35816；
- branch：agent/0.0.10-p20-66-mainhud-combat-hint-layout-policy；
- 目标：把 P20.65 提示栈的 viewport 布局收束为纯计划，标准视口保持原位，小视口完整压缩或整组失败关闭；
- 边界：不读取文字、字体、Widget、输入、World、Actor 或产品状态，不启动产品界面。

## 调查

P20.65 已统一提示行的来源、顺序、tone 与文本，但 MainHUD 仍直接写死 `Left=28`、`BottomAnchor=112`、`LineSpacing=22.5`。在 640×200 这类小而仍可完整容纳五行的视口中，原位置会把顶行推到负安全区域；继续在 HUD 过程代码里增加分支会重新分散布局权威。

本轮保留底部锚点语义，把标准/紧凑选择、顶部安全边距和完整行预算都移入 renderer-neutral value plan。策略只接受 Canvas 数值快照和行数，因此可以无头测试，不持有 UI 生命周期。

## 布局模型

新增：

- `Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode`：Invalid / Standard / Compact；
- immutable plan：Canvas 快照、行数、左边距、底部锚点、行距与 scale；
- `TryPlan`：验证输入、计算可用垂直跨度、选择模式并返回完整计划；
- `IsValid`：重算 top-line 安全边距和模式一致性；
- `TryGetLinePosition`：只投影计划内索引，失败清零；
- `Matches`：核验确定性重放。

五行最小完整边界为 640×200：`BottomY=88`、`TopY=24`、`Spacing=16`。宽度低于 640、高度低于完整预算、行数为 0 或大于 5、非有限 Canvas 都拒绝。策略不裁行，因此不会把 target-required、confirm 或 trajectory 等语义提示悄悄丢弃。

## MainHUD 接线

`DrawHUD` 仍先组合唯一 P20.65 stack，再用当前 `Canvas->SizeX/SizeY` 和 `Stack.NumLines()` 请求计划。计划失败时不绘制该 stack；成功时原 stack 的顺序、tone、颜色与基础字号不变，位置和 standard/compact 倍率只来自 plan。

HUD 未新增成员、缓存或生命周期。产品 choice、Arc context、physical preview、input 和 trajectory 权威均未改变。

## 首轮验证与修正记录

第一次构建调用把项目名误写成不存在的 `demo_map.upLiteralPath.uproject`；UnrealBuildTool 在查找项目阶段以 `OtherCompilationError` 退出，未进入任何源码编译。这是命令拼写错误，不是源码首败。

改正为真实 `demo_map.uproject` 后，首次 Editor build 为 6 actions，UBA 21.38 s / total 23.47 s，Succeeded。首次焦点日志 `P20.66.r0_layout_first.log` 为 3/0、257,559 bytes、SHA-256 01C2A5A32E2BAEC7748B705EBFDC843E7B038819B8FFBB06C46FAE313243038D；没有测试修复轮。

## 回归映射

`ShanmenRegressionMap.json`：

- MainHUD 规则新增布局焦点组；
- 新布局规则要求布局焦点、P20.65 stack、0.0.10 全量、InputRestore 与 V2RangedCompatibility。

`Test-ShanmenRegressionCoverageSelfTest.ps1`：

- 新增布局健康 fixture；
- 新增布局路径正例；
- 新增“单独布局焦点不能替代 stack 与兼容证据”反例；
- MainHUD 正例补入布局日志。

映射 JSON 解析成功；self-test 423/423，41,964 bytes，SHA-256 B8155642B3B98FFBC7D0B185E0011DEB0C4795F3417BAFEB62E45C9905843315。

## 正式自动化

| Log | Success/Fail | Bytes | SHA-256 |
|---|---:|---:|---|
| P20.66.r0_layout.log | 3/0 | 257,559 | AA07BC58B2C3DB9AF04CE7B643347809871BA3ADB654AB77DEEC99B24D918667 |
| P20.66.r0_input_restore.log | 101/0 | 387,845 | 62C60960B22C41CB86024988722B7E28AC360CD7C3A952D4476409415BFDDC01 |
| P20.66.r0_ranged.log | 22/0 | 278,227 | 4D2E3CD82BD7746711B5D198023EC51F8FE196D6E02109820A939DA5625B65E8 |
| P20.66.r0_full.log | 1222/0 | 1,827,013 | DBEF93FA74BD0555EE090CEA6D98039CE81CDC1554A56735A2A31BBA4B116597 |

合计 1348/0。四份日志均有 native terminal-success marker，Fatal/Unhandled/Assertion=0。全量运行跨 heartbeat 保持同一个 UnrealEditor-Cmd 实例，自然清空，没有重启或拼接结果。

changed-file gate：`REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=14 Logs=4`；2,346 bytes；SHA-256 CABE68EB8FBE97E87B84D09F995E29B803C2BA62B65A6120E46D131F99724356。

## 静态与差异审计

- 新生产布局策略：2 files / 193 lines；
- World/Actor/RNG/damage/item/profile authority hits：0；
- begin/end/cancel/update/clear/route/commit/reserve/reduce/apply mutation hits：0；
- static log：114 bytes / SHA-256 49870042493FAFCE13A2AF138FE7CDCECF07AF12B120E3C4B5960E2D88679B19；
- source/test/mapping：6 files / +365 / -9；
- `git diff --check`：PASS，仅 LF→CRLF 提示；
- 103 份用户原有 untracked 文档保持未暂存；
- raw logs 不进入 Git。

## 最终构建与产物

- Game：5 actions / 21.56 s / Succeeded / native 0；log 2,323 bytes / SHA-256 633D8991B0BE54D0590EDA9DB6843E9F3A0DD5288ABF4F2E17CD1BF7BEBC3462；
- Editor：0 actions / 0.95 s / Succeeded / native 0；log 1,021 bytes / SHA-256 9ADFF00052F9EA624C68A65AC4FD93450A39F3D3A45269676CD1683F63253FF0；
- `demo_map.exe`：359,293,952 bytes / SHA-256 D08CE487947B531DADB58F43E352CBBE5D5CEF524830A55FF1298641A382C096；
- `UnrealEditor-demo_map.dll`：18,431,488 bytes / SHA-256 DCAB382B75F006ECBE529931F824CB2A18CE1F59B1E46152DCB29D787D450C74。

## P/F 边界与后续

PASS：标准/紧凑纯布局、完整行预算、安全边距、非法输入与越界失败关闭、重放一致性、MainHUD 接线、3 项焦点、旧输入、旧远程武器、全量、映射门禁、Game/Editor build。

未声明：真实文字宽度、safe-zone/DPI、其它 HUD 遮挡、肉眼可读性、真实输入、Editor UI、PIE、Standalone、产品 executable、截图、Smoke、Cook、Package。

本提示切片在无界面边界内已经闭环。后续不再增加包装层；获得视觉运行授权后再做一次聚焦 viewport/DPI/safe-zone 验收，否则转向下一项真实战斗功能。
