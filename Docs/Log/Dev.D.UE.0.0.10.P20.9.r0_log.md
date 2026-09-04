# Dev.D.UE.0.0.10.P20.9.r0 Development Log

## 1. 目标与基线

- 基线：`047ee273b0c1e85d53a081ae1907b4d45043aa98`（P20.8）；
- 分支：`agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition`；
- 目标：让既有 GameMode 在 Run 边界显式冻结 Straight/BallisticArc 生命周期配置，并公开复用 InputAdapter 的 Arc composition route；
- 约束：不修改 PlayerController、输入绑定、UI、预览、产品平衡参数、库存事务、planner、controller、command、host 或 world delivery。

## 2. 接入前审计

P20.8 已完成 InputAdapter 的 Straight/Arc typed route，但 GameMode 仍通过隐含 Straight 的兼容 `TryBegin` 启动生命周期，公开入口也只有 aim-direction Straight route。因此 Arc 能力存在于下层契约，却无法由既有产品组合边界选择。

审计确认 GameMode 已持有唯一 item authority、Run coordinator、ThrownWeapon lifecycle、InputAdapter、projectile class 与 Action gate。本轮只在这一处完成配置与委托，不新建第二条产品权威。

## 3. Typed trajectory 配置

GameMode 新增 `ConfiguredThrownWeaponTrajectoryKind`，兼容默认是 `Straight`。新增配置方法只接受 canonical `Straight`/`BallisticArc`，非法值失败关闭并保留原值。

变更只能发生在 coordinator 非活跃且 lifecycle 空态时。active Run 或 stale lifecycle 均拒绝配置，防止 Run 中策略漂移。相同合法值重复设置保持幂等。

## 4. Run 启动与审计

`TryActivateCombatRun` 将配置值传入 lifecycle typed `TryBegin`。其余 Run 激活、失败回滚与后续产品链不变。

成功绑定日志新增 `ThrownWeaponTrajectory`，使一次 Run 的实际 Straight/Arc 选择可以从同一 `RunBound` 事件审计。

## 5. Arc composition route

新增 GameMode `RouteThrownWeaponArcHotbarInput`，装配现有 authority、lifecycle、coordinator、world、canonical projectile、source Actor、hotbar slot、target/apex callbacks 与 `ThrownWeapon` Action gate，然后单次委托 InputAdapter。

该方法不处理设备输入，不主动采样世界，不直接改库存或伤害，不复制 trajectory planner、session、controller、command、host 或 delivery。

## 6. 自动化扩展

InputAdapter exact 新增两项，从 7 增至 9：

- `GameModeTrajectoryConfiguration`：证明默认 Straight、非法枚举失败且不变更、空态 Arc 配置、同值幂等，以及空态恢复 Straight；
- `GameModeArcRouteFailClosed`：在 transient GameMode 缺少 GameInstance authority 时，Arc route 维持 `PassThrough`，target/apex sampler 均为 0。

测试放入既有 InputAdapter automation 文件，使 changed-file regression 映射直接覆盖生产组合边界和新增证明。

## 7. 失败透明度

首次 Editor 构建失败来自测试代码引用不存在的 `AuthorityUnavailable` 状态。既有无 authority 契约是 `PassThrough`；生产代码已编译，整体 native exit 6。原日志 `editor_build_initial.log` 保留，SHA-256 `85C350DDB8254601805D7E6D7EE57CAD203D44B3AE457B0656B8FD777A22CF06`。

只修正测试期望后，Editor 构建 4 actions / native 0，SHA-256 `4E70D9A61022FDCD810425D202CDFE7C0493AC29A1F8D6EF441BC7B8E54B9F2F`。随后 exact 首次 9/0，SHA-256 `D85C697B3560D1FC3B6E9AAAFE846516D35B44A5EA29572EC79EC3089069BB78`。

首次 `demo_map.V3.Attributes` 得到 4/0/native 0，但 `;Quit` 对极短测试产生终止竞争，没有 queue-empty/TEST COMPLETE；该日志 SHA-256 `9A2770100C87B4239BA5FAC7693432310BBB0E948E8F2269AC26B32935164138`，不计入有效证据。去掉 `;Quit` 后单进程重跑取得 canonical 终止，未重启或拆分测试组。

## 8. 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `input_adapter_final.log` | 9/0 | `3F07DA11342F33284E77A90B572BEC96B2D81069A7E317EF35E98C9CE513E833` |
| `full_0_0_10_final.log` | 863/0 | `BDCA1BB7BB026F74A458DA3A7509C9BBC85A6364288C8106932E7014A1ADFA6A` |
| `legacy_v3_attributes_retry.log` | 4/0 | `FEB193B26F28356121D1CB128C1FDEE7561F46EEE10C658A12F69A050CB23009` |
| `legacy_enemy_skill_final.log` | 44/0 | `CED12288463945A7F775313C78EEA29BC4E005F89DA479C5D498D3E7E36B0640` |
| `legacy_v2_ranged_final.log` | 22/0 | `D5B9B0163A890EC51A81FAB3DE9A019A0D72F67EAEF529D919F0A8185C8EDCD2` |
| `legacy_item_armor_final.log` | 46/0 | `067843E0DA7A51B9053C7D4FCFC79A1283F693C7795374AD54540E552852D2BD` |

证据审计：`PASS Logs=6 RecordedSuccess=988 Invalid=0`；SHA-256 `5020F4CD6EDED3AFEF92F136D0AFCA60620EF81A85FEEE6B83627D4902C2CDD1`。

0.0.10 全量从 P20.8 的 861 增至 863。全量运行是一个连续进程；SwordRhythm 长测试和环境 `generate_204` 噪声未导致失败、拆组或重启。

## 9. Changed-file regression 与静态边界

三个改动路径命中 `M01GameMode` 与 `ThrownWeaponInputAdapter`：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=53 Logs=6
```

- regression gate SHA-256：`9832645799D0C39C74A60D3C737AC149590490D758DDB57503627B2EBA41736C`；
- production boundary：`PASS Files=2 AddedLines=74 Matches=0`；
- boundary SHA-256：`9DA35AC5CB3D36200DA71DB899BBBE5DD6EF6D4C812D94BD282D8D1D3D5D4025`；
- `git diff --cached --check`：精确暂存本轮 5 个文件后 PASS / native 0；证据 SHA-256 `622EB6D835092621585AD4AB14DAB3874939F2299FFA328CA2BEB95C9A57C835`；长期未跟踪文件不纳入。

边界扫描覆盖 GameplayStatics、direct damage、spawn、RNG、trace/sweep、input binding、PlayerController、generic item use 与 Prepare/Commit。

## 10. 构建与产物

- final Editor：0 actions / native 0，SHA-256 `79EDD957F5345D36FF060D7975674A8F6A7DD03290ED1284F934EADF297CA0F4`；
- final Game：25 actions / native 0，SHA-256 `DC38CBAD83D5D9A8E2C9BCB6809C45CE3817709335EC8D890F1F6AC5A0EA467F`；
- `demo_map.exe`：357,197,312 bytes / `45AD9EF8682306541EADFACCFCB4B4293FFBEF179CE46EA2D328381021AB0428`；
- `UnrealEditor-demo_map.dll`：15,862,272 bytes / `8B82FC1C03B979482640B29E12A3A071346DE0C0CF086EB7B6A83D3D425C33FC`。

## 11. P/F 边界与后续判断

本轮只证明 GameMode 组合边界的 typed trajectory 冻结与 Arc 委托。未增加设备输入或 UI，未执行真实玩家操作、Editor UI、PIE、Standalone、产品程序、截图、Smoke、Cook 或 Package。

P20.10 建议先建立平台无关、纯值的 thrown-weapon 输入选择契约/归约器：显式 Straight/Arc 模式，以及规范化 apex adjustment/target intent；以确定性测试冻结切换、重复输入和无效值语义。PlayerController、具体键鼠/手柄映射、world trace 与预览继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition/Docs/Report/Dev.D.UE.0.0.10.P20.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-9-thrown-weapon-arc-game-mode-composition/Docs/Log/Dev.D.UE.0.0.10.P20.9.r0_log.md>
