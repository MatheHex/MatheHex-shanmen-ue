# Dev.D.UE.0.0.10.P20.66.r0 Report

## 1. 结论

P20.66 已把 P20.65 combat-hint stack 的固定横向锚点、底部锚点、行距与字号倍率收束为一个纯 viewport-safe layout policy。MainHUD 不再自行持有这些布局常量，只提交当前 Canvas 尺寸与完整提示行数，然后按布局计划绘制。

策略不读取提示文字、字体、Widget、输入、World、Actor 或产品可变状态。标准视口保持 P20.65 原布局；窄视口或高度受压时确定性进入 compact；当全部提示无法同时保留时整组失败关闭，不按优先级静默裁掉关键行。

正式自动化 1348/0：布局策略 3/0、InputRestore 101/0、V2RangedCompatibility 22/0、0.0.10 全量 1222/0。全量较 P20.65 的 1219 增加 3 项，正好对应本轮新增测试。映射自检 423/423；changed-file regression 对 4 个本轮改动路径推导出 14 个必跑组，全部有健康日志覆盖。Game 构建 5/5 actions，Editor 构建 up to date，均 native 0。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：47bbff2d278b3c886d69aea6b0b2cbb09fb35816（P20.65）；
- 分支：agent/0.0.10-p20-66-mainhud-combat-hint-layout-policy；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 布局行为

标准模式：

- Canvas 宽度至少 960 px，且完整行栈可使用 22.5 px 行距；
- 左边距 28 px，底部锚点 112 px，字号倍率 1.0；
- 1920×1080 的五行栈位置保持为底行 `(28, 968)`、顶行 `(28, 878)`。

紧凑模式：

- Canvas 宽度为 640–959 px，或高度不足以维持 22.5 px 行距；
- 左边距 16 px，字号倍率 0.86；
- 行距可确定性压缩，但不得小于 16 px；
- 640×200 的五行栈仍完整保留，底行 `(16, 88)`、顶行 `(16, 24)`。

失败关闭：

- 宽度小于 640 px；
- 顶部安全边距无法保持 24 px；
- 行数不在 1–5；
- Canvas 尺寸非有限值；
- 任何失败都会把复用输出恢复为 canonical invalid plan。

## 4. 纯布局契约

新增 `Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan`：

- 输入只有 `CanvasSize` 与 `LineCount`；
- 输出只有 mode、Canvas 快照、行数、边距、锚点、行距与字号倍率；
- 最多接受 P20.65 规定的五行；
- `IsValid` 重查模式、数值范围、安全边距与标准/紧凑一致性；
- `TryGetLinePosition` 只接受计划内索引，失败先清零输出；
- `Matches` 对相同输入提供确定性重放核验。

该类型不包含 UObject、Canvas、Widget、Font、presentation、输入绑定或 HUD 成员状态，不会成为第二套 UI 或产品权威。

## 5. MainHUD 接线

`Ademo_mapHUD::DrawHUD` 保留 P20.65 的唯一 stack composer。组合成功后，HUD 用当前 Canvas 尺寸和 `Stack.NumLines()` 请求一次 layout plan；计划成功才遍历原 stack。

每条文本的位置来自 `TryGetLinePosition`，既有语义 tone 仍决定颜色和基础字号；布局策略只追加 standard/compact 倍率。HUD 没有新增持久字段，也没有改变 trajectory、Arc target、Arc edit、input hint 或 pre-launch gesture 的事实来源。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.66.r0_layout.log | Product.ThrownWeaponMainHUDCombatHintLayoutPolicy | 3/0 | AA07BC58B2C3DB9AF04CE7B643347809871BA3ADB654AB77DEEC99B24D918667 |
| P20.66.r0_input_restore.log | demo_map.InputRestore | 101/0 | 62C60960B22C41CB86024988722B7E28AC360CD7C3A952D4476409415BFDDC01 |
| P20.66.r0_ranged.log | demo_map.V2RangedCompatibility | 22/0 | 4D2E3CD82BD7746711B5D198023EC51F8FE196D6E02109820A939DA5625B65E8 |
| P20.66.r0_full.log | Shanmen.0_0_10 | 1222/0 | DBEF93FA74BD0555EE090CEA6D98039CE81CDC1554A56735A2A31BBA4B116597 |

四份正式日志合计 1348 Success、0 Fail，均含 UE 5.8 native terminal-success marker；Fatal、Unhandled 与 Assertion 命中均为 0。

新增测试覆盖标准布局、宽度/高度触发的紧凑布局、最小完整五行边界、过窄/过矮/非法行数/NaN 输入、失败输出清空、越界索引和确定性重放。

## 7. 改动—回归映射

新增 `ThrownWeaponMainHUDCombatHintLayoutPolicy` 规则，并把布局焦点组加入 `MainHUDTrajectoryPresentation` 规则。布局文件必须同时具备自身焦点、P20.65 stack、0.0.10 全量、InputRestore 与 V2RangedCompatibility 证据。

- 映射自检：423/423；SHA-256 B8155642B3B98FFBC7D0B185E0011DEB0C4795F3417BAFEB62E45C9905843315；
- changed-file gate：PASS，Changed=4 / Rules=2 / Required=14 / Logs=4；
- gate SHA-256：CABE68EB8FBE97E87B84D09F995E29B803C2BA62B65A6120E46D131F99724356。

## 8. 静态审计与构建

- 新生产布局策略：2 files / 193 lines；
- World/Actor/RNG/damage/item/profile authority 命中：0；
- begin/end/cancel/update/clear/route/commit/reserve/reduce/apply mutation 调用命中：0；
- static log：114 bytes；SHA-256 49870042493FAFCE13A2AF138FE7CDCECF07AF12B120E3C4B5960E2D88679B19；
- source/test/mapping：6 files / 365 additions / 9 deletions；
- `git diff --check`：PASS，仅 Windows LF→CRLF 工作区提示；
- 103 份用户已有 untracked 文档保持未暂存、未修改；
- 原始自动化与构建日志继续只作本地证据，不进入提交。

最终构建：

- Game：5 actions / 21.56 s / Succeeded / native 0；log SHA-256 633D8991B0BE54D0590EDA9DB6843E9F3A0DD5288ABF4F2E17CD1BF7BEBC3462；
- Editor：up to date / 0 actions / 0.95 s / Succeeded / native 0；log SHA-256 9ADFF00052F9EA624C68A65AC4FD93450A39F3D3A45269676CD1683F63253FF0；
- `demo_map.exe`：359,293,952 bytes / SHA-256 D08CE487947B531DADB58F43E352CBBE5D5CEF524830A55FF1298641A382C096；
- `UnrealEditor-demo_map.dll`：18,431,488 bytes / SHA-256 DCAB382B75F006ECBE529931F824CB2A18CE1F59B1E46152DCB29D787D450C74。

## 9. P/F 边界

PASS：纯布局计划、标准保持、紧凑压缩、全栈安全边距、非法输入失败关闭、失败输出清空、确定性重放、MainHUD 单次计划接线、旧输入、旧远程武器、0.0.10 全量、回归映射、Game/Editor build。

未声明：真实文本宽度、平台 safe-zone/DPI、与其它 HUD 面板遮挡、颜色对比、真实 viewport 肉眼可读性、真实输入、Editor UI、PIE、Standalone、产品可执行文件、截图、Smoke、Cook、Package。

## 10. GitHub 与后续判断

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-66-mainhud-combat-hint-layout-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-66-mainhud-combat-hint-layout-policy/Docs/Report/Dev.D.UE.0.0.10.P20.66.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-66-mainhud-combat-hint-layout-policy/Docs/Log/Dev.D.UE.0.0.10.P20.66.r0_log.md>

不建议继续为同一提示栈增加新的包装层。下一步应在获得视觉运行授权后做一次聚焦 viewport/DPI/safe-zone 验收；若继续保持无界面边界，则关闭该提示切片并转向下一项真实战斗功能。
