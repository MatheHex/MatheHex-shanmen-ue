# Dev.D.UE.0.0.10.P20.23.r0 Report

## 1. 结论

P20.23 已把 P20.20 的当前投掷武器选择投影为最薄的只读 HUD 反馈。玩家现在可在既有 HUD 热栏上方看到 `THROWN TRAJECTORY: STRAIGHT` 或 `THROWN TRAJECTORY: BALLISTIC ARC`，同时看到 P20.22 统一输入 registry 中当前生效的切换键。键位重映射无需修改显示逻辑；下一帧 HUD 绘制会直接读取最新配置。

本轮没有复制 choice state、revision、target、apex、session 或 route 状态，没有建立缓存、轮询器、定时器、第二权威或写入通道。HUD 每次绘制只调用一次既有 `ReadThrownWeaponInputChoiceInteraction()`，再执行一次纯投影和一次文本绘制。新增聚焦自动化 `6/0`，最终 0.0.10 全量由 `942` 增至 `948/0`。本轮只执行编译、纯投影/合成产品自动化和静态检查；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`c571062a70cf1fd257e33442cd06f6c9648e2c11`（P20.22）；
- 分支：`agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 只读呈现契约

新增 `Fdemo_mapShanmenThrownWeaponTrajectoryPresentation`，输入仅为一份有效 P20.20 read result 与一个调用方提供的按键显示名，输出仅保留：

- canonical trajectory kind；
- `STRAIGHT` / `BALLISTIC ARC` 模式标签；
- 去除首尾空白后的可配置按键标签；
- 由上述两个标签唯一派生的完整显示文本。

Straight 投影要求 read model 当前允许选择 BallisticArc，Arc 投影要求当前允许选择 Straight；因此无效、矛盾或不可用的读取都会失败关闭。每次投影开始先清空复用输出，失败不会遗留上一帧显示。该类型的字段均为 private，外部只有只读 getter。

## 4. 信息最小化边界

轨迹模式提示故意不读取、不保存也不显示 Arc target、apex、read-model identity、choice revision 或 session identity。两个 Arc 状态即使 target/apex 不同，也得到完全相同的模式文本；revision 0 与 revision 2 的等价 Straight 状态同样得到完全相同的显示。

生产投影文件不依赖 `UWorld`、Actor、Pawn、PlayerController、UObject 或 timer，也不调用 intent/command capture、reducer、route 或 submit。它是一个纯读模型投影，不具备修改产品状态的能力。

## 5. HUD 接入与刷新语义

现有 `Ademo_mapHUD::DrawHUD()` 本来就按帧读取当前产品状态，因此直接复用其自然刷新点：

1. 从当前 `Ademo_mapPlayerController` 调用 P20.20 read 一次；
2. 从 `Fdemo_mapInputBindingSettings` 读取 `ThrownWeaponTrajectoryToggle` 当前键一次；
3. 纯投影一次；
4. 成功时在热栏上方绘制一次，Straight 使用青色，BallisticArc 使用琥珀色；读取或投影失败时不绘制陈旧提示。

没有修改 PlayerController、GameMode、P20.20 端口或 P20.22 输入链。显示层不持有任何下一帧状态，因此 choice 或键位变化不需要 revision polling、事件订阅或手动失效缓存。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponTrajectoryPresentation` 新增 6 项：

- `StraightProjection`：初始 Straight 的精确模式、键位与文本；
- `ArcProjection`：BallisticArc 的精确模式、键位与文本；
- `TargetAndApexNeutrality`：Arc target/apex 不进入模式呈现；
- `RevisionNeutrality`：等价模式不受 choice revision 影响；
- `ConfigurableKeyProjection`：键名清理以及 T→C 显示更新；
- `FailureClearsOutput`：source unavailable、invalid state 与空键名均失败关闭并清理复用输出。

## 7. 改动文件回归映射

新增两条 changed-file 规则：

- trajectory presentation 文件必须覆盖新呈现组、P20.20 interaction port、P20.22 physical toggle 与完整 0.0.10；
- 主 HUD 接线除上述组外还必须覆盖 `demo_map.V2RangedCompatibility`。

映射正/反自测新增 4 项，总数从 `337` 增至 `341/341`。最终门禁对 4 个生产/测试改动路径求规则并集，结果为 `Changed=4 Rules=2 Required=5 Logs=5`，所有必跑组都有成功日志，未出现未映射生产路径。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `trajectory_presentation_first.log` | `Product.ThrownWeaponTrajectoryPresentation` | `6/0` | `B53C3F22BA46E43D93C5047CD67A12C6936B739ECB5F5199474910F0B944F103` |
| `interaction_port_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `72B593BBCD6ED25D318B07AD42F07469B0360A3B71FD3BEC32F8D1F19DCFE5AD` |
| `trajectory_toggle_final.log` | `Product.ThrownWeaponTrajectoryTogglePhysicalInput` | `7/0` | `68EC4157440D9F7309696D0204D7F7BD5ACFE81249D64322A1028B39B964C59F` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `97343C261E582812C44277F32A60528A25CC614D6504579070F74DD1D20DF2BB` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `948/0` | `F3D3096BB46E5089C4C7C80D02A40ED845C1FF7B3C629FD1CD848B0A0F5BB64C` |

日志审计：`PASS Logs=5 RecordedSuccess=990 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`，SHA-256 `41A13C32D16E2D5870C6D84AC1B8BC9E9F0D9DD5C4565C3BDE7F44468D5F6913`。

## 9. 流程、静态边界与构建

- regression self-test：`341/341`，SHA-256 `7AB39691796F2F84DB758050E62971DC48CD59F1734FA35D7FDD8B7F8DDD6794`；
- static audit：`PASS ExactTests=6 ForbiddenRuntime=0 ChoiceDetailReads=0 MutationCalls=0 HardcodedPhysicalT=0 HudCurrentReads=1 HudProjections=1 HudDraws=1 HudRegistryKeys=1 HudChoiceDetailReads=0 StraightLabels=2 ArcLabels=2`，SHA-256 `EF52A42B3B1D8906244DABD82BC832EF18FAE21B82A29E7E9D128FD7F9163778`；
- changed-file gate：`PASS Changed=4 Rules=2 Required=5 Logs=5`，SHA-256 `34267398EF6643A33B3E7A6A866655CC1EB49561C91609DBF963B05BA9A69401`；
- `git diff --cached --check`：PASS / native 0 / 8 个精确暂存文件，SHA-256 `E29A74B8CCD98CD6354A0ED41F7A564835D7B036E08D7C37210745B2C013A061`；
- 首次 Editor 编译：6 actions / native 0 / 22.37 秒，SHA-256 `A11E50F2654CE53631E22018FD68C8370343A5102C243E01202529840F594B1E`；
- 新组首轮即为 `6/0`，且产品/测试源码此后未修改；
- final Editor：0 actions / native 0 / 1.32 秒，SHA-256 `38E1E5981E805B91206D6E1A7022FCD1E0CA87854CCC754D2786C53579F67BAC`；
- final Game：5 actions / native 0 / 19.75 秒，SHA-256 `A5E3F1477DFA8C59CB5D96136FD3232AE766A42681D73F5E705595D5E6BE7797`。

产物：

- `Binaries/Win64/demo_map.exe`：357,574,656 bytes，SHA-256 `5AB24125DF041F68716ED867BEA7EDBD2D69C8564109654D27814CA4ED5FE453`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,314,880 bytes，SHA-256 `F0DD48CF2A28A036527E1BF538F941C20E21EDF09DE7921448D561B98C209117`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：当前 Straight/Arc 模式的只读纯投影、失败输出清理、target/apex/revision 信息最小化、实时配置键标签、HUD 单次读取/投影/绘制、P20.20/P20.22 兼容，以及由改动路径推导出的完整回归。

未验证：真实可见 HUD 布局、真实键鼠/手柄/触控输入、Arc target/apex UI、轨迹线/落点预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。无头自动化与 Development 构建不能描述为可见产品验收。

建议 P20.24 增加设备无关的 Arc 编辑呈现：只投影 P20.20 已公开的 target/apex 与当前 capability，先形成可测试的提示/数值模型，再决定鼠标、手柄或触控的具体输入映射；仍不在显示层复制 choice state 或发出写命令。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.23.r0_log.md>
