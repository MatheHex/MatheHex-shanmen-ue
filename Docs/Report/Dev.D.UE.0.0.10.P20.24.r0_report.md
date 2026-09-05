# Dev.D.UE.0.0.10.P20.24.r0 Report

## 1. 结论

P20.24 已在 P20.23 的投掷轨迹模式提示上补齐 Ballistic Arc 的设备无关编辑呈现。处于 Arc 模式时，HUD 现在会基于同一次 P20.20 权威读取额外投影并绘制当前目标意图、弧高调整值，以及 set / clear / increase / decrease 是否可用；处于 Straight、读取不可用或状态无效时，Arc 详情失败关闭，不显示陈旧值。

本轮没有选择鼠标、键盘、手柄或触控映射，没有发出 intent、request、command 或 route，也没有复制 choice state、revision、session 或 read-model identity。HUD 每次绘制仍只调用一次 `ReadThrownWeaponInputChoiceInteraction()`：同一份 immutable read result 先供 P20.23 模式呈现，再供 P20.24 Arc 详情呈现。新增聚焦自动化 `7/0`，最终 0.0.10 全量由 `948` 增至 `955/0`。

本轮只执行编译、纯投影/合成自动化和静态检查；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`fba673f47977c5d20a2c26e57736a44dda54700f`（P20.23）；
- 分支：`agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Arc 编辑呈现契约

新增 `Fdemo_mapShanmenThrownWeaponArcEditingPresentation`。它只接受一份有效 P20.20 interaction read result，并只在当前 trajectory 为 BallisticArc、且 read model 同时允许 `SelectStraightTrajectory` 与 `SetArcTargetIntent` 时投影成功。输出字段均为 private，外部仅有只读 getter：

- canonical `HasArcTargetIntent` 与二维 target 值；
- canonical `ArcApexAdjustment`，范围 `[-1, 1]`；
- set、increase、decrease、clear 四个 capability 布尔值；
- 由上述值唯一派生的 target 与 apex 显示文本。

投影入口先清空复用输出；source unavailable、invalid state、Straight 模式或 capability 矛盾均失败关闭。`IsValid()` 重新核对 finite、target 单位圆、空 target 规范、apex 边界、四项 capability 和两条派生文本，避免形成可显示但自相矛盾的对象。

## 4. 设备无关显示语义

Arc 无目标时显示：`ARC TARGET: UNSET  |  SET AVAILABLE`。有目标时显示规范化坐标，例如：`ARC TARGET: X +0.60  Y +0.80  |  SET / CLEAR AVAILABLE`。

弧高显示固定为两位小数，并只列出当前仍允许的方向：中间值显示 `ADJUST: +/-`，上界 `+1.00` 只显示 `ADJUST: -`，下界 `-1.00` 只显示 `ADJUST: +`。显示层不含具体按键提示，因此后续物理设备映射可以独立演进。

生产呈现文件不依赖 `UWorld`、Actor、Pawn、PlayerController、UObject、timer 或 input registry；不调用 intent/command capture、reducer、submit 或 route；不读取 revision、session 或 identity。

## 5. HUD 接入与单读语义

现有 `Ademo_mapHUD::DrawHUD()` 的投掷武器区域保持一个权威读取点：

1. 调用 `ReadThrownWeaponInputChoiceInteraction()` 一次；
2. P20.23 使用该结果投影并绘制模式与当前切换键；
3. P20.24 复用同一结果投影 Arc target/apex；
4. Arc 成功时在模式行上方绘制 target 与 apex 两行，Straight 或失败时绘制 0 行 Arc 详情。

因此这一帧的模式、目标、弧高与 capability 来自同一份 read snapshot，不会因重复读取产生跨帧撕裂。HUD 不缓存该对象，下一帧自然刷新。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcEditingPresentation` 新增 7 项：

- `NeutralArcProjection`：无目标、零弧高及双向调整；
- `TargetProjection`：`(3,4)` 规范化为 `(0.6,0.8)`，并开放 clear；
- `ApexCapabilityBoundaries`：`+1/-1` 上下界只开放反向调整；
- `DetailRefresh`：新 read 同步刷新 target、apex 与文本；
- `RevisionNeutrality`：Arc→Straight→Arc 后等价可见状态不泄漏 revision；
- `StraightClearsOutput`：Straight 不属于 Arc 详情并清理复用输出；
- `InvalidReadClearsOutput`：source unavailable 与 invalid state 均失败关闭。

首轮为 `6/1`：`DetailRefresh` 夹具用恰好位于两位小数中点的 `0.625`，却把特定舍入方向写入复合断言。修正只把测试样本换成无舍入歧义的 `0.5`；生产实现未因该失败修改。复编译后新组为 `7/0`，首次失败日志完整保留。

## 7. 改动文件回归映射

新增 `ThrownWeaponArcEditingPresentation` 规则，要求新组、P20.23 trajectory presentation、P20.20 interaction port 与完整 0.0.10。主 HUD 规则同步增加新 Arc 呈现组，同时保留 P20.23、P20.20、P20.22 toggle、完整 0.0.10 与 `demo_map.V2RangedCompatibility`。

映射正/反自测新增 2 项，总数从 `341` 增至 `343/343`。最终门禁对 4 个生产/测试改动路径求规则并集，结果为 `Changed=4 Rules=2 Required=6 Logs=6`，所有必跑组均有成功日志，没有未映射生产路径。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_editing_presentation_final.log` | `Product.ThrownWeaponArcEditingPresentation` | `7/0` | `4EF5FB64D98F788573663F2E5234CD69156D6B820E5A2FC3EA30D63547203124` |
| `trajectory_presentation_final.log` | `Product.ThrownWeaponTrajectoryPresentation` | `6/0` | `83F9C44FA8AB4AF278B10EE68DB4BD7D7B2A4570CFF3DBD75234815A71D7A0E9` |
| `interaction_port_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `5D90BC16FE186677EF86CC871177627ECADBF1E72D2F0D8367A1794673DB37F7` |
| `trajectory_toggle_final.log` | `Product.ThrownWeaponTrajectoryTogglePhysicalInput` | `7/0` | `D8C0A1D934B01A106B940D5387A541F9F7913EBA685FB6E77F89E93E03B9392B` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `58570F8FC21095AB3AD35E4ACB1A82D4A366632AC4465AE547290124210F366A` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `955/0` | `26FB6CB7A8FD9BC9D0478A0ADCFFFAED975BE4A6A4E0F60CADED5429832FC715` |

首次失败证据：`arc_editing_presentation_first.log` 为 `6/1`，SHA-256 `C675B38DE0D675C8F20E3C905E3D8E34A8D871ED562E71F4EA61F355D2CA21FC`。

最终日志审计：`PASS Logs=6 RecordedSuccess=1004 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `A485D87E9B035DF9EDC5B99F112D36C8F2A98E9A2353DE01906FF0794F195F50`。

## 9. 流程、静态边界与构建

- regression self-test：`343/343`，SHA-256 `ADE286A44863DC9010E9A6277A5EB1858B2485A7FC249980D634576F034BA112`；
- static audit：`PASS ExactTests=7 ForbiddenRuntime=0 PhysicalDevices=0 MutationCalls=0 IdentityReads=0 HudCurrentReads=1 HudModeProjections=1 HudArcProjections=1 HudDraws=3 HudRegistryKeys=1 TargetDisplayDraws=1 ApexDisplayDraws=1`，SHA-256 `DF57F52CBAA31D9CF8991024B199DA553FB36432DDEDBBF735BDB6213071153F`；
- changed-file gate：`PASS Changed=4 Rules=2 Required=6 Logs=6`，SHA-256 `510C70C3A064A3C29E6029D1540F2D8E2829CCDF05D5E0E441C3991F1F044112`；
- `git diff --cached --check`：PASS / native 0 / 8 个精确暂存文件，SHA-256 `3C5CD2B356225300861B66BDB6CB06EBE8B421C1C7EF0817DD8118DE4DC82908`；
- 首次 Editor 编译：6 actions / native 0 / 26.10 秒，SHA-256 `015D2972CDAD0755F13ABA5EF26FE95E68AE39181D56E54119C50276338A5E6A`；
- 测试夹具修正后 Editor 编译：4 actions / native 0 / 5.87 秒，SHA-256 `E2BFD1B10D45FA2D76161D40D8ECFC363608315D26E5C0C40992ED54481C0856`；
- final Editor：0 actions / native 0 / 0.96 秒，SHA-256 `48DA0673F5252316121E2761652A2F22E379379B8B001E474FBF9F00063B23B2`；
- final Game：5 actions / native 0 / 24.69 秒，SHA-256 `0BE75D9143743E45F6DA5AF7C400DB2AF6A9E1F5B00AF68A208E7D8BA3E0F166`。

产物：

- `Binaries/Win64/demo_map.exe`：357,596,160 bytes，SHA-256 `038686F7EBB39D3D18ED765F9D6B0C12E740DB596816BD9D8EAE45EC37F77400`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,340,480 bytes，SHA-256 `C56B7114593D0C4F353AAE1D9DF256BF7ABFC5E97BFFF61D7A4F06E844CDDDD3`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：BallisticArc target/apex/capability 的 immutable 纯投影、两位小数文本、上下界方向提示、Straight/无效读取失败清理、revision/identity/device 信息最小化、HUD 单次读取并组合三行文本，以及由改动路径推导的完整回归。

未验证：真实可见 HUD 布局、任何鼠标/键盘/手柄/触控操作、Arc 编辑命令发出、轨迹线/落点预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为可见产品验收。

建议 P20.25 建立设备无关的 Arc 编辑交互合成层：把规范化 target、apex delta 与 clear 操作和一次 P20.20 read 合成为既有 P20.21 stale-safe request，再由现有权威 route 执行；仍将具体物理设备绑定留给后续独立阶段。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.24.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.24.r0_log.md>
