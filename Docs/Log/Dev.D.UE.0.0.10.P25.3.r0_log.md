# Dev.D.UE.0.0.10.P25.3.r0 Development Log

## 1. 目标

- 把 P25.2 已进入真实敌方 Impact 的短时灵力护盾投影到现有 MainHUD；
- 显示活动状态、剩余容量、固定期限和统一 `H` 键名；
- HUD 只读消费产品会话与 Run 固定时间线，不复制状态或计时；
- 为有效、低容量、耗尽与非法输入建立纯自动化证明；
- 按改动文件映射执行精确回归，而不是按任务主题猜测测试范围；
- 构建 Editor/Game，生成 Report/Log 并精确推送本阶段文件。

## 2. 基线与范围

- 基线：`94e8145ca8e9ca4ff0d5592ae55c5199965aeff2`（P25.2）；
- 分支：`agent/0.0.10-p25-3-spirit-shield-hud`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件全程保持原样；
- 不修改护盾成本、容量、期限、输入路由、Impact、防御顺序、生命值、存档、地图或 UI 资产。

## 3. 纯 HUD 投影

新增 `Fdemo_mapShanmenSpiritShieldHUDPresentation`。入口接受一次冻结读取：

- Session 是否活动；
- Available / Maximum Capacity；
- Current / Deadline Tick；
- Ticks Per Second；
- 统一输入映射产生的按键标签。

投影先清空输出，再验证有限数值、容量边界、严格未到期时间窗、正 Tick Rate 与非空 Key。有效值分为 `Stable`、`Low`、`Depleted`；Low 阈值为 25%。剩余时间按 0.1 秒向上显示，避免最后一个 Tick 被显示为零。

该类不 include Product Session 或 GameMode，不持有 World/Actor/Timer/Clock/RNG，也不向产品层写入。

## 4. MainHUD 读取链

`Ademo_mapHUD::DrawHUD()` 的新辅助绘制函数直接读取：

- `ActiveMode->GetSpiritShieldProductSession()`；
- `ActiveMode->GetCombatRunFixedTimeline()`；
- Product Session 内只读 Runtime Session 的最大容量；
- Input Binding Settings 中 `SpiritShield` 的当前显示键。

只有纯投影成功才绘制。稳定/低容量/耗尽分别使用青、琥珀、灰色面板。顶部帮助行新增 Shield 操作提示。HUD 不调用 Activate、ObserveTimeline、CommitImpact 或容量 Authority 写方法。

## 5. 新增自动化

`demo_mapShanmenSpiritShieldHUDPresentationTests.cpp` 新增：

1. `Stable`：满容量、3.0 秒与重放一致；
2. `Low`：25% 临界值、非整数容量和 61/30 秒向上显示；
3. `Depleted`：活动会话的零容量仍可见，单 Tick 显示 0.1 秒；
4. `Fences`：未激活、已到期、零 Tick Rate、容量越界、NaN 和空 Key 均拒绝，复用输出不残留。

聚焦测试首次运行即 4/4，通过日志：`P25.3_SpiritShieldHUDPresentation.log`，SHA-256 `D9D199E063C290B3166005CFF2799E2B6D672BF82A82351B33EA6F2A7C050CC3`。

## 6. 路径驱动回归

新建 `SpiritShieldHUDPresentation` 路径规则，要求 HUD Projection 与 Product Session 两组；现有 MainHUD 规则同步增加这两组。自检增加相应正向与失败关闭样例。

正式 20 组证据如下：

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SpiritShieldHUDPresentation` | 4 | 0 | `D9D199E063C290B3166005CFF2799E2B6D672BF82A82351B33EA6F2A7C050CC3` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 8 | 0 | `0825599A78FE2366ECFF124726F44624C0694B11717FC600E9D79A71908DFBB7` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1 | 0 | `596FF7095C34691FC2245EEA83ABD1E4884BDE974E8EAFC560CEA85175162520` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 5 | 0 | `3C712ABA4846C37F495F7ECDD6B3F2A20B861D1904C7C06D7A513D7902FECC7D` |
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 12 | 0 | `B041FABCB05AD25F951CFFC3C6CDAD59818D67FFC276A4C1F029817452883BA6` |
| `Shanmen.0_0_10.Product.DivineSensePhysicalInput` | 5 | 0 | `A475BAD0E0D2E895284E162C585CED0C393154D5B8ED5C10AD906D6F6869BEC4` |
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | `B45343448336BEEE04B385DB17262D49627FF560C5D6D797309691D0767D2732` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation` | 6 | 0 | `B7258444743B496823CAF47662E1DD1C5D2ED90D248EC38925FDEFF17220D428` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` | 10 | 0 | `5C619BC32C6960380DF495CFA58A2D8FEAB8D0F85CE0D99CC13CC1372F68B8E7` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation` | 7 | 0 | `6002BAB7F36101F86E922B0831CBDF72F8726C7751FFC8848A0194E77332ABC0` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation` | 3 | 0 | `F3F059BBE284E26C718C2D22A011ED58E8B3E076EDFC0D88CEC090971BC4C986` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter` | 5 | 0 | `D87B21107CF98C6A16B3C97208DBBF69B4A77447D43ADBB2DF7FFB3BF11CA328` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5 | 0 | `67981058F44F4FA95421981B99E34A973616945F7C0733F489E84787A65385FE` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` | 7 | 0 | `6182725D50870BAECA130C9E2276AD8C42763398C64F44185D58BBF45B14830B` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy` | 3 | 0 | `BFF857B535F0E0659169EADFB4859B7E3A9A263088448D99E8FE3A011FA4322C` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation` | 3 | 0 | `D9D49815C798DCF5147845D69DD252D9F90EF9EEF065467AF0130EC46C097F92` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation` | 6 | 0 | `ECD634AEDA0AB21C221130B2D84575091ABDA9B64D2DB6717228F8C4E2F51A2F` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 | 0 | `4CF84E2A1B5674057A3495455574AB2D5A226746E532E27D86640B9A79430552` |
| `demo_map.InputRestore` | 101 | 0 | `8D185C4FF72F83D54D2EB8A092FFB51752E115F229CC4E1E9BA80F69F9509B16` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `4F729716303174AE04834ED82B1E1E903E19F4E6B28BF8058C0A3DF147F23C39` |

合计 225 Success / 0 Fail；所有正式日志均有唯一成功终止标记，Fatal/Unhandled/Ensure/联网探测均为 0。

覆盖门禁：`PASS Changed=6 Rules=2 Required=20 Logs=20`，SHA-256 `60E7CF8B017D88D9C6789BC218514700D4C75C872A9BF212A9C00617E137E3AE`。

## 7. 覆盖器首错与修复

首次自检在既有 MainHUD 正向样例处正确失败：路径规则已要求 Shield HUD + Product Session，但样例仍只构造旧证据。首错日志：`P25.3_RegressionCoverageSelfTest_attempt-1.log`，SHA-256 `DC9E9A768615CA6CE2A5B6BE8A7FECBC1CF05D3A75CE3A3AA6B2E15A6F9FC1B1`。

修复仅更新测试夹具：

- 构造 Shield HUD 日志样例；
- MainHUD 正向样例补入 Shield HUD 与 Product Session；
- 新增 HUD 单独证据不能替代 Product Session 的失败关闭断言。

最终自检 448/448 PASS；日志 SHA-256 `6328D811B01534276A7F15365654B6C6B2B8F2ED1B3AE91739D68B1390F6B7BE`。

## 8. 全量诊断记录

第一份全量命令遗漏项目历史上已验证的 Home Screen 进程级覆盖，UE 后台 `generate_204` 探测拖慢队列。该进程在 756 Success / 0 Fail 时有界停止，终止标记 0，日志 SHA-256 `750E476DBE9A783FBC4D8E84DE4EEF348EF5DFE129AA78056BCD8C856B94D40B`。

第二份从头加入 `-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0`，确认 743 Success / 0 Fail 且 `generate_204=0`。考虑本轮已经采用“改动文件决定回归组”的基线，该宽树在进入历史慢速恢复契约段后停止，终止标记 0，日志 SHA-256 `8F11FAEFBC3AEC76D64BFADE478E842E61FA2C7A18DD10712CB1327A4292D49E`。

两份均不作为 PASS 证据，也不用于替代 20 组正式日志。没有修改 Engine、Windows、用户全局配置或防火墙。

## 9. 构建与静态证据

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.3_EditorBuild_final.log` | Editor Succeeded | 0 | `697A73C74F9C7A6CF2C96B1055DDA8F86E8924BCE1350840CE7FF23573F106ED` |
| `P25.3_GameBuild_final.log` | Game Succeeded | 0 | `53A417FE19CBA2773C3F7DBAFBFD6F0F46956642969428D073A05995B0710A6C` |

- `git diff --cached --check`：PASS；
- 新纯投影禁止依赖扫描：0 命中；
- `UnrealEditor-demo_map.dll`：19125248 bytes / `94FF5D61A03AE50A7A1302BBF954E1A3CD4870F539AAEE3D5AB373A5AF1C135A`；
- `demo_map.exe`：359832576 bytes / `8D22F8A47B8F25D74E691B5323529040428D4E11C98A423655E1C8FD852303F5`；
- 最终项目相关进程：0。

## 10. P/F 与精确交接

P 阶段已证明：HUD 文本、容量阈值、耗尽状态、Deadline 换算、最后一 Tick、非法输入失败关闭、Product Session 读取、统一输入键与现有 HUD 编译接线成立；全部路径映射回归与双目标构建通过。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；因此不声明已目视确认 HUD 画面。

只暂存本阶段 6 个实现/测试/流程文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件不暂存；`Saved/Codex/P25.3` 原始日志仅本地保留。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-3-spirit-shield-hud>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-3-spirit-shield-hud/Docs/Report/Dev.D.UE.0.0.10.P25.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-3-spirit-shield-hud/Docs/Log/Dev.D.UE.0.0.10.P25.3.r0_log.md>
