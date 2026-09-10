# Dev.D.UE.0.0.10.P24.6.r0 Development Log

## 1. 目标

- 从 P24.5 的真实剑气发射路径门禁继续；
- 把 `LaunchPathBlocked` 转换为玩家可理解、可行动的 HUD 提示；
- 必须依据完整类型化错误链，禁止靠 Diagnostic 字符串推断；
- 复用现有输入反馈寿命、颜色与绘制位置；
- 不改变剑气发射、重试、碰撞、伤害、装备、属性或动作权威；
- 按改动文件映射完成精确回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线：`6a9369bc214f2b04821d9ac5937b4479a67fcc05`（P24.5）；
- 分支：`agent/0.0.10-p24-6-sword-qi-launch-feedback`；
- 开始时 tracked tree clean；
- 用户 103 个未跟踪文件保持原样；
- 本阶段不新增平衡、资源、穿透、伤害、输入或本地化规则。

## 3. 缺口审计

P24.5 已在实际 World 中返回 `Edemo_mapShanmenSwordQiLaunchError::LaunchPathBlocked`，上层 Session、Controller、Input、CommandEvent 与 Availability 结果也保留嵌套证据。

现有 HUD 只识别：成功、忙碌重试、活动 UI 阻挡、装备拒绝、攻击力不可用。所有其它 Diagnostic 都显示 `SWORD QI · UNAVAILABLE`。

因此底层知道“墙挡住了”，玩家却只看到“不可用”。缺口位于最后一层显示投影，不应通过修改发射系统或复制一条反馈链解决。

## 4. 实现决策

把已有匿名 `TryBuildSwordQiFeedback()` 移为 `Ademo_mapHUD` 的公开静态 C++ helper：

- 实际 `DrawSwordQiFeedback()` 继续调用同一 helper；
- 自动化可直接验证最终文字和颜色；
- 不创建新的 Presentation、Manager、Subsystem 或 Actor；
- 既有成功、忙碌、UI、装备、攻击力与 generic fallback 分支原样保留。

专用 `SWORD QI · LAUNCH BLOCKED` 需要完整满足：

`Dispatched → InputRejected → ProductRejected → RouteRejected → LaunchRejectedInterrupted → LaunchRejected → LaunchPathBlocked`

并要求 HostStart 未开始。输出使用现有失败提示琥珀色 `FLinearColor(1.0, 0.72, 0.18)`。

## 5. 自动化升级

升级已有 `Shanmen.0_0_10.Product.SwordQiWorldDelivery.LaunchCorridorGate`，保留全部 P24.5 断言，并增加：

1. 使用发射点真实 `UBoxComponent` 阻挡得到 `Occupied` WorldAdapter 结果；
2. 把该真实结果嵌入完整 Availability 证据链；
3. 断言 HUD 返回精确专用文本与颜色；
4. 把 HostStart 的类型化原因改为 `AdoptionRejected`；
5. 断言 HUD 只能返回 generic `UNAVAILABLE`，不能声称路径受阻。

首次 Editor 构建 7 actions，native 0。聚焦 WorldDelivery 自然达到 queue-empty，4/0，日志 SHA-256：

`7D82BC6382689314E9FA032B951E6234D1F9EAFE72F51B645A9E201DC7D4C303`

本阶段没有测试、构建或源码首次失败，因此没有失败日志需要另行保留。

## 6. 改动文件回归

改动文件：

- `Source/demo_map/demo_mapHUD.h`；
- `Source/demo_map/demo_mapHUD.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapterTests.cpp`。

命中 `MainHUDTrajectoryPresentation` 与 `SwordQiWorldDelivery` 两条规则，要求并完成 23 个独立组：

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `demo_map.InputRestore` | 101 | 0 | `9F5EEE379C0E0AD81978E51F920AAA379AC4F82E00CA1D68BC4CF1373E2D665D` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `BBAD24B6A2B43A720CE402C214545EC86511567BDA7D0C9B12186B2A13272884` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `FD48A1DD5A9211B1E13C24F291D865AEF8CDE6CF6D60B3F719D640609ADADF26` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 0 | `53F9138C8810BD862165586437BFDD4E99DDC15BA95BBFD3E110DD639A7E7322` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18 | 0 | `86FCE2ED8F2C290C9D267684959AA1CD8A27A338D4BF95EF64ABA89FDD54AE4D` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1 | 0 | `AF6693CCE4B5363A0273D73C5226A3F81225E89CC4F83647F9DA99D925C0206D` |
| `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 5 | 0 | `BC296F9C114F8E5E23E93268FF327B93EA0AAAC3125F43B557B7CF87EFE62D37` |
| `Shanmen.0_0_10.Product.DivineSenseHUDPresentation` | 12 | 0 | `383F0244C099930987E1F3FBB22D0C8D8737F41FB5E9063A70D9F8B0CE2424C4` |
| `Shanmen.0_0_10.Product.DivineSensePhysicalInput` | 5 | 0 | `4D58DF9A8BEE0E9BE4F9857C17699A340370D79F82CD8CB5D16DFC45156D5C8D` |
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | `4AA18E5F87B8FF834C80165150D25D70D283166E408C7BCECA6615D92BDC0EF8` |
| `Shanmen.0_0_10.Product.SwordQiWorldDelivery` | 4 | 0 | `7D82BC6382689314E9FA032B951E6234D1F9EAFE72F51B645A9E201DC7D4C303` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation` | 6 | 0 | `6B1B8740219351A644FDE0B028430095756477B5736100D510121407592C9748` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` | 10 | 0 | `C527237A1648B4226AA48F6F8930C605391EA0D23E9B23C3AC4F3D4081FFBEAE` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation` | 7 | 0 | `31F12C10F1372461B3B6669A5CF31F08B0B84C5AD94635B8795C7197B1B850A6` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchGestureFeedbackPresentation` | 3 | 0 | `8F2A14407DDFD40543C3E212D7E5FC211FE1866DD6AA0A6820346385BCC175AF` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter` | 5 | 0 | `9E9A069DC8E180FCD2B1469F6AB83D845FE9778D5FE6D2FF9658AF1990361A08` |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding` | 5 | 0 | `96202CEB747D448C0D5DF9AE62EEF2B8AB74B6A6C6C15AEA05F564D91E3C6D81` |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` | 7 | 0 | `FF9DB47CAFD7BAC7DB9A1B5E5901B7C71DC58BC805F39732E8203CC45340BE44` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy` | 3 | 0 | `43EF2AFE387BD826DC3B49D66926E177FB5B4C4AE284C5BF5BF8FAAE41AAA5E1` |
| `Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintStackPresentation` | 3 | 0 | `C78011B888E65A29DB9B74D556207F5FAE54F073F1031D828D93E8AFC793ECB5` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation` | 6 | 0 | `03A893E94C681A97A828C9EB24D26F94C09A25140FC77A2BA0E9FFF7179FA532` |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 | 0 | `11137C57B5CD7CA862ADE8CBBA7DAD280DFB222A2EAD00769A2EEC1EE7BAC368` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `6B60A96DEC040BECED023A155BD91DCCFA894E8FA4FE10780570ECCD8268AEE9` |

合计 400/0。覆盖器结果：`PASS Changed=3 Rules=2 Required=23 Logs=23`。

- 覆盖日志：8,757 bytes，SHA-256 `2946DB997BFB3E40B54E9A60F6CE7C7124BACF3AA4276723E5D02ED6DCCC7559`；
- 覆盖器自检：442/442，43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 7. 构建证据

| Target | Result | Native exit | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---:|---|
| `demo_mapEditor Win64 Development` initial | Succeeded | 0 | 7 | 2,493 | `7CE81E043182ADB71C19B932B128D5DD0B5F02C9D94E9CE0838247CA142A85DC` |
| `demo_mapEditor Win64 Development` final | Succeeded / up to date | 0 | 0 | 1,021 | `D7F98335CA43B7E7F1ECB6550037183A46298C9329664F5B88330BE0D9542AB1` |
| `demo_map Win64 Development` final | Succeeded | 0 | 6 | 2,280 | `AE7B8F3FE89A0671E7A8B8C9015516002471DFAA0DA9D845C530CEE5EE0EF0B2` |

最终 `demo_map.exe` 为 359,708,160 bytes，SHA-256 `774D33AD6C4C770B7362BB0CAC675DF32C4A3AD0429967D474120CAD37CB8A4A`；`UnrealEditor-demo_map.dll` 为 18,974,208 bytes，SHA-256 `8F26C2F7214F07AE503FB3525441EE7EA10775206958B7158EFFA15F7BF38FF8`。

## 8. 静态与 P/F 边界

- 非文档增量：3 files，`+118/-49`；
- `git diff --check`：native 0；
- 生产新增行对 Timer、RNG、`ApplyDamage`、`SpawnActor`、`Destroy`、Manager、Subsystem：0；
- 没有新增反射类型、Actor、输入、存档、Config 或 Content；
- 最终项目相关进程：0。

P 阶段证明真实 World 阻挡结果可准确抵达 HUD，并且完整类型链防止错误归因。真实关卡视觉、持续时间、本地化与玩家手感未做 F 阶段声明；未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 精确提交范围

- `Source/demo_map/demo_mapHUD.h`；
- `Source/demo_map/demo_mapHUD.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapterTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.6.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.6.r0_log.md`。

用户 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.6` 原始证据保持本地忽略。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-6-sword-qi-launch-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-6-sword-qi-launch-feedback/Docs/Report/Dev.D.UE.0.0.10.P24.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-6-sword-qi-launch-feedback/Docs/Log/Dev.D.UE.0.0.10.P24.6.r0_log.md>
