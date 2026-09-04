# Dev.D.UE.0.0.10.P20.9.r0 Report

## 1. 结论

P20.9 已把 P20.8 的 Straight/BallisticArc typed 输入能力接入既有 `Ademo_mapGameMode` 组合边界。

GameMode 现在以 `Straight` 作为兼容默认值，并允许在 combat Run 之外显式选择 `Straight` 或 `BallisticArc`。Run 激活时，选定的轨迹类型会传入唯一 `Fdemo_mapShanmenThrownWeaponProductLifecycle`；Run 绑定日志同时记录实际轨迹类型。GameMode 还新增一条 Arc hotbar route，以 target/apex 惰性采样回调委托既有 InputAdapter，不复制物品、Action、生命周期或世界执行逻辑。

配置入口拒绝非法枚举，并在 combat Run 活跃或旧生命周期仍未清空时失败关闭，避免一次 Run 中途切换轨迹策略。新增自动化证明兼容默认、合法/非法空态配置，以及缺少权威时 Arc route 保持 hotbar 透传且不采样 target/apex。

本轮未修改 PlayerController、设备输入、按键映射、UI 或轨迹预览；没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`047ee273b0c1e85d53a081ae1907b4d45043aa98`（P20.8）；
- 分支：`agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. GameMode 轨迹配置

`Ademo_mapGameMode` 持有唯一 typed trajectory 配置，初值固定为 `Straight`，因此所有旧调用方仍维持原行为。`TryConfigureThrownWeaponTrajectory` 只接受 `Straight` 或 `BallisticArc`：非法枚举返回诊断且不修改当前配置；合法的同值重复设置是幂等操作。

配置只允许在 `CombatRunCoordinator` 非活跃且 `ThrownWeaponProductLifecycle` 已清空时发生。任一条件不满足都会拒绝变更并保留旧配置。这将内容选择固定在 Run 边界，而不是让单次 hotbar 输入临时改变产品策略。

## 4. Run 启动绑定

`TryActivateCombatRun` 不再调用隐含 Straight 的兼容 lifecycle 入口，而是把 GameMode 当前配置显式传入 typed `TryBegin`。后续 ProductSession、Controller、RunCommand 与 Host 继续使用同一条 P20.7/P20.8 权威链。

若生命周期绑定失败，原有 Run 激活回滚保持不变；成功时 `0_0_10_COMBAT_RUN Event=RunBound` 增加 `ThrownWeaponTrajectory` 字段，便于从运行日志区分 Straight 与 BallisticArc 配置。

## 5. Arc 组合入口

新增 `RouteThrownWeaponArcHotbarInput` 只承担 GameMode 依赖装配：取得既有 item authority、product lifecycle、combat Run coordinator、world、canonical projectile class、source Actor 和同一 `ThrownWeapon` Action gate，然后委托 P20.8 的 `RouteArcHotbarInput`。

GameMode 不读取或改写库存，不计算弹道，不生成伤害，不做 trace/sweep，不绑定输入，也不产生第二套 session/controller/host。target 与 apex 仍由调用方以惰性回调提供；InputAdapter 的失败关闭顺序继续决定它们是否被采样。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | 9/0 | `3F07DA11342F33284E77A90B572BEC96B2D81069A7E317EF35E98C9CE513E833` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 863/0 | `BDCA1BB7BB026F74A458DA3A7509C9BBC85A6364288C8106932E7014A1ADFA6A` |
| `legacy_v3_attributes_retry.log` | `demo_map.V3.Attributes` | 4/0 | `FEB193B26F28356121D1CB128C1FDEE7561F46EEE10C658A12F69A050CB23009` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | 44/0 | `CED12288463945A7F775313C78EEA29BC4E005F89DA479C5D498D3E7E36B0640` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | 22/0 | `D5B9B0163A890EC51A81FAB3DE9A019A0D72F67EAEF529D919F0A8185C8EDCD2` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `067843E0DA7A51B9053C7D4FCFC79A1283F693C7795374AD54540E552852D2BD` |

六份有效日志均只有一个 canonical `RunTests` command、一个成功终止信号、至少一个 Success、0 Fail，并且无 Fatal/Unhandled/Ensure：`PASS Logs=6 RecordedSuccess=988 Invalid=0`。审计 SHA-256：`5020F4CD6EDED3AFEF92F136D0AFCA60620EF81A85FEEE6B83627D4902C2CDD1`。

InputAdapter exact 从 P20.8 的 7 增至 9。首次 exact 同样为 9/0，SHA-256 `D85C697B3560D1FC3B6E9AAAFE846516D35B44A5EA29572EC79EC3089069BB78`；0.0.10 全量从 861 增至 863。

## 7. 失败与重试证据

首次 Editor 构建暴露的是新增测试中的枚举期望拼写错误：测试写成不存在的 `AuthorityUnavailable`，而既有无权威契约实际为 `PassThrough`。生产代码已经完成编译，整体构建 native exit 6；原始日志保留为 `editor_build_initial.log`，SHA-256 `85C350DDB8254601805D7E6D7EE57CAD203D44B3AE457B0656B8FD777A22CF06`。修正测试期望后 Editor 构建 4 actions / native 0，SHA-256 `4E70D9A61022FDCD810425D202CDFE7C0493AC29A1F8D6EF441BC7B8E54B9F2F`。

首次 `demo_map.V3.Attributes` 运行得到 4/0 和 native 0，但超快 `;Quit` 与 TestExit 发生竞争，日志没有 queue-empty/TEST COMPLETE 终止证据；该日志不计入有效证据，仍保留并记录 SHA-256 `9A2770100C87B4239BA5FAC7693432310BBB0E948E8F2269AC26B32935164138`。移除 `;Quit` 后单进程重跑得到有效 4/0 终止日志。

## 8. 改动门禁与静态边界

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=53 Logs=6
```

三个改动文件命中 `M01GameMode` 与 `ThrownWeaponInputAdapter` 两条映射规则。53 个必跑组全部由 exact、0.0.10 全量与四组 legacy 日志覆盖；gate SHA-256：`9832645799D0C39C74A60D3C737AC149590490D758DDB57503627B2EBA41736C`。

对两个 production 文件的新增行扫描 GameplayStatics、direct damage、spawn、RNG、trace/sweep、input binding、PlayerController、generic item use 与 Prepare/Commit：`PASS Files=2 AddedLines=74 Matches=0`；SHA-256：`9DA35AC5CB3D36200DA71DB899BBBE5DD6EF6D4C812D94BD282D8D1D3D5D4025`。

`git diff --cached --check`：仅暂存本轮 5 个文件后 PASS / native 0；证据 SHA-256：`622EB6D835092621585AD4AB14DAB3874939F2299FFA328CA2BEB95C9A57C835`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：up to date / 0 actions / native 0，SHA-256 `79EDD957F5345D36FF060D7975674A8F6A7DD03290ED1284F934EADF297CA0F4`；
- final Game：25 actions / native 0，SHA-256 `DC38CBAD83D5D9A8E2C9BCB6809C45CE3817709335EC8D890F1F6AC5A0EA467F`。

产物：

- `Binaries/Win64/demo_map.exe`：357,197,312 bytes，SHA-256 `45AD9EF8682306541EADFACCFCB4B4293FFBEF179CE46EA2D328381021AB0428`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,862,272 bytes，SHA-256 `8B82FC1C03B979482640B29E12A3A071346DE0C0CF086EB7B6A83D3D425C33FC`。

## 10. P/F 边界与下一步

P20.9 证明的是“GameMode 可以在 Run 之前冻结 typed thrown-weapon trajectory，并把 Arc 请求装配到既有 InputAdapter”。它没有证明玩家设备已经能选择 Arc，也没有证明 active/stale fence 的所有状态组合、真实弹道呈现或产品输入体验。

建议 P20.10 先定义平台无关、纯值的 thrown-weapon 输入选择契约/归约器：显式 Straight/Arc 模式、规范化 target intent 与 apex adjustment，验证模式切换和重复输入的确定性；仍不接按键、鼠标、滚轮、UI、world trace 或轨迹预览。策略稳定后，再由独立阶段把 PlayerController/设备输入适配到该契约。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition/Docs/Report/Dev.D.UE.0.0.10.P20.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition/Docs/Log/Dev.D.UE.0.0.10.P20.9.r0_log.md>
