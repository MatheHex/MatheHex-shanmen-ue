# Dev.D.UE.0.0.10.P25.4.r0 Development Log

## 1. 目标

- 让玩家按下灵力护盾键后立即知道成功或拒绝原因；
- 复用现有 MainHUD 反馈窗口，不建立新 Widget、Actor、Component 或 Subsystem；
- 固定玩家文案只由结构化结果产生，禁止把任意产品诊断文本显示到 HUD；
- 用当前统一映射键提示玩家在动作冲突后重试；
- 按改动文件执行精确回归、双目标构建，并推送本阶段 Report 与 Log。

## 2. 基线与范围

- 基线：`969c17d2d503b2654d6e15224d642c5406cc431a`（P25.3）；
- 分支：`agent/0.0.10-p25-4-spirit-shield-feedback`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件全程保持未暂存；
- 不修改护盾成本、容量、Deadline、Impact、防御顺序、共享资源权威、存档、地图或 UI 资产。

## 3. 纯反馈投影

在既有 `demo_mapShanmenSpiritShieldHUDPresentation` 中新增：

- `Edemo_mapShanmenSpiritShieldInputFeedbackReason`；
- `Edemo_mapShanmenSpiritShieldInputFeedbackTone`；
- `Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation`。

投影入口先清空输出，随后验证完整产品结果与非空键名。成功固定为青色 `ACTIVE`；AlreadyActive、ActionConflict、CoordinatorNotReady、TimelineUnavailable 和资源不足归入琥珀色可恢复提示；其余结构化技术拒绝归入红色 `ACTIVATION FAILED`。只有 ActionConflict 文案使用当前映射键；成本从 `CanonicalSpiritEnergyCost()` 读取。

没有读取 `Result.Diagnostic`，没有自由格式错误文本进入玩家界面。

## 4. 控制器与 HUD 接线

`RouteSpiritShieldInput()` 对 UI 占用和缺少产品 GameMode 分别生成 `ActionConflict` 与 `CoordinatorNotReady`，正常路径仍委托 GameMode。所有有效结果被控制器保存 2.25 秒，沿用神识和剑气已经使用的短反馈模式。

MainHUD 在中央提示区域读取该冻结结果并调用纯投影。Success / Warning / Error 分别使用青、琥珀、红面板。World 时间只决定这条 HUD 消息何时隐藏，不影响权威护盾期限、战斗 Run Tick 或资源事务。

## 5. 新增与扩展自动化

HUD Presentation 组从 4 项扩展为 8 项：

1. `Stable`；
2. `Low`；
3. `Depleted`；
4. `Fences`；
5. `AlreadyActiveFeedback`；
6. `InsufficientFeedback`；
7. `ActionBusyFeedback`；
8. `InputFeedbackFences`。

Physical Input 的按键测试现在证明缺少产品 GameMode 时仍返回有效 `CoordinatorNotReady` 并打开 HUD 窗口。Product Session 的真实激活测试证明 accepted 结果投影为 `SPIRIT SHIELD · ACTIVE`。

首次聚焦运行 8/8 Success，日志 `P25.4_SpiritShieldHUDPresentation.log`，SHA-256 `697878F3347F846D028F2D264F9B3704237DE09D66F2416AC3570D878E950442`。最终映射重跑同组仍为 8/8 Success，SHA-256 `05D0024D9824600A6022C782F296BAF534E7A821D6DCD5FF212E431E57B952DE`。

## 6. 回归映射变更

`PlayerCombatController` 与 `MainHUDTrajectoryPresentation` 规则新增 `SpiritShieldPhysicalInput`；MainHUD 自检正向夹具同步增加该证据。

同时从 `UnifiedInputRegistryAndBindings` 和 `PlayerCombatController` 移除宽泛 `Shanmen.0_0_10` 项。本轮不使用父级全量组替代路径实际映射出的精确测试组，覆盖门必须看到全部 53 个组的独立健康日志。

映射自检最终 448/448 PASS，日志 SHA-256 `6328D811B01534276A7F15365654B6C6B2B8F2ED1B3AE91739D68B1390F6B7BE`。

## 7. 53 组最终原生证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `demo_map.FullSystemLoop.41` | 1 | 0 | `EA23A208156575DB40D5476775A0B9980CBD45F5F85199D25D90D55BA7683C16` |
| `demo_map.FullSystemLoop.47` | 1 | 0 | `7D12BF9493EDFF648F7CA1A5134A8B31D0DBA3F185C013AC3EBA46AA44CE934A` |
| `demo_map.InputRestore.32` | 1 | 0 | `39233DCEAF477785848A8AD98FE96DFC7EE29A98015164165CFF4062E91BCF90` |
| `demo_map.InputRestore` | 101 | 0 | `FF3E04CD064B52397AD66D69208F6D64109B7AF036682CB2BF3458E6FD4209CB` |
| `demo_map.P5RuntimeInterface.06` | 1 | 0 | `1F7F3A944B69176F660E500562E7827DC22D9F4693F681E2EB133B86051AE447` |
| `demo_map.P7Integration` | 9 | 0 | `DF85E6538DB0473B0303CA8D4E3A214C9A8D24EC32C793378D82F9BBCF7813B6` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `2599FF79DDBA6C35A8CC270F5AB00AA451E78336F52B50DFAAD416A0ED5803D6` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `6AD9D76E79ED8A7CEF1BDD45155CA2E3CBFCE58AA1F1FB8567EC84BF5C566B26` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `9C76536290AC5CC782E07597BD9DAA870AA2D042446336D609DF03C719A36B4C` |
| `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `1F08CE1FDDB6E688BFCE9018121EDCAF0917B4304B01A8B5E75BDDDECD8B796A` |
| `Shanmen.0_0_10.CombatRuntime.SpiritShieldSession` | 8 | 0 | `35F138BF9A7A253651377D0F3AA1D7F5E719DB0A313F99ADF13CD6D5BFE661FC` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 19 | 0 | `373026798F83DBEC551DDE405ADFBE3B4B8BD7A15C63771E3B510A3E0E4A752E` |
| `Shanmen.0_0_10.Product.CombatRunFixedTimeline` | 5 | 0 | `F62639834554E1185AB917890A958FFFD69019CDD1C2215924FC2548972D6A72` |
| `Shanmen.0_0_10.Product.ControlledWeaponInputAdapter` | 4 | 0 | `1FE90ACB17BE67332F33E4BD89E2D130F227FB1A2212611EBCAD3876E88D3BB5` |
| `Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput` | 7 | 0 | `2C4B15A1CCCD483F8F033FCDE60EE46662B18D2FD3373C44017741EB6E5218A5` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1 | 0 | `2D4191B56BA113BF252527235708801773E8FD78243D39164F641F0B80D65915` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 5 | 0 | `D79B94486A4D8F8B27CC0091D65CDF3692D195B0735A25E61346FDE213F4B5D2` |
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 12 | 0 | `69FD4A8EB2EE223080B8FC22B4DB042C895BE6ECF643FDD2057B903DC2E9CC65` |
| `Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter` | 7 | 0 | `C92CF2BD85B226207A19AD8BE70BD5EFB3C66DBB2440765A24041F9DF46F9E76` |
| `Shanmen.0_0_10.Product.DivineSensePhysicalInput` | 5 | 0 | `34DAD0F526D17016E61A898AE56AFAC2F23BB6F0FA64C99C040706C4C27F7800` |
| `Shanmen.0_0_10.Product.DivineSenseProductController` | 5 | 0 | `78179E11D9D7FC636501BC3177D01F0A9A18DB2234587B949C40821ABADA9476` |
| `Shanmen.0_0_10.Product.MeridianShockTreatment` | 32 | 0 | `700058BC077777BE167DD9BCA471F269BA7E6F19352DDCA1F1ECAAE5A6793F6A` |
| `Shanmen.0_0_10.Product.PlayerActionArbitration` | 4 | 0 | `5618448C0595DCDBA94803A8CA6446BD4E040C59678502E714FC695C7AB6880A` |
| `Shanmen.0_0_10.Product.SpiritEvasionInputAdapter` | 6 | 0 | `474D976084441BD5E39BCF0E17B24B2A637BD77F417892675FE76535D82A98BE` |
| `Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput` | 6 | 0 | `322349A18E6A95A30013F61009C74734CA14037282A7B69395E575A1DCCAED10` |
| `Shanmen.0_0_10.Product.SpiritShieldHUDPresentation` | 8 | 0 | `05D0024D9824600A6022C782F296BAF534E7A821D6DCD5FF212E431E57B952DE` |
| `Shanmen.0_0_10.Product.SpiritShieldPhysicalInput` | 3 | 0 | `180C3A1F7C7BACA94B2FFE07C91BD065F451A4B3C63763A9839780D5CF074E2A` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 8 | 0 | `1F808DF356D618B2660F5D91877F5B1FC35C04980CDD927A8EFA90CBD3759546` |
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | `C73B98CB169AC7F5C4385EE4083DBB455C980A642DD0E476893C35525DCEBF8D` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner` | 6 | 0 | `9C42AC09F34446795B1D693618D0B08AFB1C7B945070F1B0BF5BA35DC7A7C506` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute` | 7 | 0 | `D9319FE1687B9373695F19FD7A6DCA0259E48ACFB4967A5B7226DF76CEF19846` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation` | 6 | 0 | `2F67B5C215C173D136B3557F874E987727ECDA5051706CED01556E3844A93C56` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition` | 7 | 0 | `88E2F6B92D350CD0D391C081FF07C95DE9742058ECFE8B1259328A9B516AF971` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` | 10 | 0 | `6967085AA4C0EFB05DC8B589F0EFA39E2D331697BBCD08EA2F467B3E4D9E0D78` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation` | 7 | 0 | `E73C9191C9B48135B3D992D55E5E2DE5A9C844231F1CFB03C6699A9D11DFB6F0` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter` | 6 | 0 | `1B3E63DFADEFF8482E2FA9F29F11DA02BC7066B3C482F8ECF0D8CD1190ADCFE4` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation` | 3 | 0 | `4AA8C8ED6CC9A6EA523439301C08E48B3E197876593F86FDCC1C1AA2502541CF` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchPreviewContext` | 3 | 0 | `7ABA53B6305C1ACA3FE0F0AAB85DE739C7F93740104F7101ED01B4DA3CB3F655` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter` | 5 | 0 | `C8C20D36D101FDBF85D2F74828B050DF3FD08D85935D86710EAD0449B0F688D7` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5 | 0 | `B5C88129FA931A0DB56FDFF5F3A7E563C9BBF5D959792F806DE35A21EAB4225B` |
| `Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter` | 7 | 0 | `B4E4E5422173A9A6691EA035720EDBB7662FCB6F7557B047632385E6A08CE467` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` | 11 | 0 | `39FE8629ACE6FDA1A6CD081EEC01FA78E384A34DE7D3543E710E6CF636944768` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter` | 8 | 0 | `8C8FB8E4B2C83F8C49AB994CB1DB82C7CBC32F134633EE2BE685A886CFC3E1FB` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter` | 8 | 0 | `0D97E277DF99519C7CB3326C75D6BA9AA3612F1A23BB2CB30C98A6C421444F35` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` | 7 | 0 | `3061D2ADB94DFC30588656FA99C86E5F4C877E59B0757BA230D87721A472FC05` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` | 7 | 0 | `804859D63C3179195EBAB021209B2498F9CFA6DF1ABF6E355CE4E3957C73D290` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy` | 3 | 0 | `2FBAAB27BE4C1867544DD7E9DE8C2773E8EA1B8F3BFB245B1EAEA25F45EA20E9` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation` | 3 | 0 | `63894AECB275821723110FD1C201729DEF5556535D15ACE67613951E0B6594CA` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation` | 6 | 0 | `077515896C10A6C350AA6AC9C450D7C6F23A1FD31C9808998CAB19E969EFC22C` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 | 0 | `C3E806CD7232DFD6DE1B68A2B31165FA77C1A3E2DC79B1C9BFB0FB3E8295E3E7` |
| `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 6 | 0 | `BEB652B7A89409DA7572453DE5DE390320D84DAA582E348D48E254D4294C21C0` |
| `Shanmen.0_0_10.Product.WeaponGuardPhysicalInput` | 6 | 0 | `8A3C754FE70EC9E2FDBD6093ED1F95FB0B6CC91CF8F1559EFC26610BC6C95B2B` |
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 7 | 0 | `380C81E9936A5B5A1E26051A2ECCFF090D048DC8B83A1E9E807DFC3C7A8AB4A0` |

合计 454 Success / 0 Fail。每份日志仅含一个目标组、至少一个原生成功终止标记、0 Fatal / Unhandled / Ensure / 联网探针。

覆盖门：`PASS Changed=10 Rules=5 Required=53 Logs=53`；日志 `P25.4_RegressionCoverage.log`，SHA-256 `2D24AEA9E8576D1F89C70A1A4051D5BEB73E75FE25BEF4E3DF0B5FA981DFE4A7`。

## 8. 首错与修复记录

| Evidence | 观察 | 修复 | SHA-256 |
|---|---|---|---|
| `P25.4_RegressionCoverageSelfTest_attempt-1.log` | Windows PowerShell 5 无法解析项目既有换行管道 | 使用随 Codex 提供的 PowerShell 7；规则不放宽 | `CD715414D92DF2CDE105820340DEC4592560E8AE686CFEF62ACBD8BFA521C1EA` |
| `P25.4_EditorBuild_attempt-1.log` | C4456：新增 HUD 局部变量遮蔽同函数后段旧变量 | 仅将新增局部变量改名为 `HUDController` | `81F8FF6AE589229BB7855B7837B4840A6CB4EE6555BA536ED8035321FD8E647D` |
| `P25.4_Mapped_demo_map_P5RuntimeInterface_06_attempt-1.log` | 1 Success、原生 0，但 `Quit` 竞争导致终止标记缺失 | 去除尾随 `Quit`，让 `-TestExit` 在队列结束时关闭；重跑完整通过 | `88A09A91F2A0D93A124139D6428743B7E1D89EECB312012D94735CABA7B805E4` |

最终 `P5RuntimeInterface.06` 日志为 1 Success / 0 Fail / 完整终止标记，SHA-256 `1F7F3A944B69176F660E500562E7827DC22D9F4693F681E2EB133B86051AE447`。53 组运行摘要 SHA-256：`CBA0EA63D7BDDC997D4682DB5029D4251CC06872C33A9AE6F349E6C0301B0A0D`。

## 9. 构建与静态证据

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.4_EditorBuild_final.log` | Editor Succeeded | 0 | `D6A8BF8FB767678BA38AB8F0A6A6D6590FAB08661D905E79C42BACFECA7EA0C4` |
| `P25.4_GameBuild_final.log` | Game Succeeded | 0 | `E241631373E94A09C814016A684155EAB884470C9A3F4962D5C58AE581B2C2AB` |

- `git diff --check`：PASS；
- Regression Map JSON：PASS；
- 纯投影禁用依赖扫描：0 命中；
- `UnrealEditor-demo_map.dll`：19140608 bytes / `6043605D8F1F4C9F9B7792C75BE3823A808420CB5712B477E32D07D8E6AE79DA`；
- `demo_map.exe`：359844864 bytes / `9338576B354A1705F07289E3DEE11C06B9651F269052843071D4B2C3D862CAFD`；
- 最终项目相关 Unreal 进程：0。

## 10. P/F 与精确交接

P 阶段完成：结构化结果、固定文案、统一键名、2.25 秒 HUD 生命周期、控制器路由、失败关闭、19 项护盾链验证、53 组路径映射回归、Editor/Game 构建与静态边界均通过。

F 阶段未执行：未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，不声明目视/手感验收。

只暂存本阶段 10 个实现/测试/流程文件、本 Report 与本 Development Log。103 个用户未跟踪文件不暂存；`Saved/Codex/P25.4` 原始日志仅本地保留。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-4-spirit-shield-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-4-spirit-shield-feedback/Docs/Report/Dev.D.UE.0.0.10.P25.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-4-spirit-shield-feedback/Docs/Log/Dev.D.UE.0.0.10.P25.4.r0_log.md>
