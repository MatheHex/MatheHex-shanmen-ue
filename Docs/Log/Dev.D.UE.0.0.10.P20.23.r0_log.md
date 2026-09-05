# Dev.D.UE.0.0.10.P20.23.r0 Log

## 阶段

- 任务：P20.23 thrown-weapon trajectory read-only presentation；
- 基线：`c571062a70cf1fd257e33442cd06f6c9648e2c11`；
- 分支：`agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 immutable `Fdemo_mapShanmenThrownWeaponTrajectoryPresentation`，只持有 trajectory、mode label、当前配置键标签与唯一派生显示文本。
2. 投影仅接受有效 P20.20 read result；Straight/Arc 分别核对可切往另一模式的 capability，矛盾状态失败关闭。
3. 投影入口先清空输出；source unavailable、invalid state 或空键标签不会遗留陈旧显示。
4. target、apex、read-model identity、choice revision 与 session 不进入呈现类型；等价可见模式跨 target/apex/revision 得到相同文本。
5. HUD 在既有 `DrawHUD()` 刷新点读取当前 P20.20 model 一次，并从 input settings 读取 P20.22 action 当前键。
6. 成功投影后只绘制一次：Straight 青色，BallisticArc 琥珀色，位置在现有热栏上方。
7. 未修改 PlayerController、GameMode、interaction port、physical toggle 或 choice state；未增加缓存、timer、poll、retry、route 或写通道。
8. 新增 6 项纯投影测试；完整 0.0.10 总数 `942→948`。
9. 新增 presentation 与主 HUD 两条 changed-file 映射规则，正/反 self-test 新增 4 项，总数 `337→341`。

## 首次证据与修正记录

- 首次 Editor 编译即成功：6 actions / native 0 / 22.37 秒。
- 新组首次执行即为 `6/0`；此后没有修改产品或测试源码，因此该首轮日志即为冻结 final exact evidence。
- 第一次启动流程映射自测时误用了旧 Windows PowerShell 5；该宿主无法解析脚本既有的行首管道语法，因此自测器尚未执行。改用项目要求的 PowerShell 7 后一次通过 `341/341`。这不是产品、源码或自动化用例失败。
- 完整 0.0.10 单进程在既有 retry/checkpoint/journal 纯值测试区间进行长时间 CPU 计算；通过测试计数、日志末项、CPU、内存与进程响应持续核验，未重启或并行重复。

## 最终聚焦与回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `trajectory_presentation_first.log` | `6/0` | `B53C3F22BA46E43D93C5047CD67A12C6936B739ECB5F5199474910F0B944F103` |
| `interaction_port_final.log` | `7/0` | `72B593BBCD6ED25D318B07AD42F07469B0360A3B71FD3BEC32F8D1F19DCFE5AD` |
| `trajectory_toggle_final.log` | `7/0` | `68EC4157440D9F7309696D0204D7F7BD5ACFE81249D64322A1028B39B964C59F` |
| `legacy_v2_ranged_final.log` | `22/0` | `97343C261E582812C44277F32A60528A25CC614D6504579070F74DD1D20DF2BB` |
| `full_0_0_10_final.log` | `948/0` | `F3D3096BB46E5089C4C7C80D02A40ED845C1FF7B3C629FD1CD848B0A0F5BB64C` |

日志审计：`PASS Logs=5 RecordedSuccess=990 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`；SHA-256 `41A13C32D16E2D5870C6D84AC1B8BC9E9F0D9DD5C4565C3BDE7F44468D5F6913`。

## 流程与静态证据

- regression self-test：`341/341`；SHA-256 `7AB39691796F2F84DB758050E62971DC48CD59F1734FA35D7FDD8B7F8DDD6794`；
- static audit：`PASS ExactTests=6 ForbiddenRuntime=0 ChoiceDetailReads=0 MutationCalls=0 HardcodedPhysicalT=0 HudCurrentReads=1 HudProjections=1 HudDraws=1 HudRegistryKeys=1 HudChoiceDetailReads=0 StraightLabels=2 ArcLabels=2`；SHA-256 `EF52A42B3B1D8906244DABD82BC832EF18FAE21B82A29E7E9D128FD7F9163778`；
- changed-file gate：`PASS Changed=4 Rules=2 Required=5 Logs=5`；SHA-256 `34267398EF6643A33B3E7A6A866655CC1EB49561C91609DBF963B05BA9A69401`。
- `git diff --cached --check`：PASS / native 0 / 8 个精确暂存文件；SHA-256 `E29A74B8CCD98CD6354A0ED41F7A564835D7B036E08D7C37210745B2C013A061`。

## 最终构建

- 首次 Editor：6 actions / native 0 / 22.37 秒；SHA-256 `A11E50F2654CE53631E22018FD68C8370343A5102C243E01202529840F594B1E`；
- final Editor：0 actions / native 0 / 1.32 秒；SHA-256 `38E1E5981E805B91206D6E1A7022FCD1E0CA87854CCC754D2786C53579F67BAC`；
- final Game：5 actions / native 0 / 19.75 秒；SHA-256 `A5E3F1477DFA8C59CB5D96136FD3232AE766A42681D73F5E705595D5E6BE7797`；
- Game artifact：357,574,656 bytes，SHA-256 `5AB24125DF041F68716ED867BEA7EDBD2D69C8564109654D27814CA4ED5FE453`；
- Editor artifact：16,314,880 bytes，SHA-256 `F0DD48CF2A28A036527E1BF538F941C20E21EDF09DE7921448D561B98C209117`。

## P/F

PASS：当前 Straight/Arc 的只读纯投影、失败清理、target/apex/revision 信息最小化、实时配置键显示、HUD 单次读取/投影/绘制，以及映射出的 P20.20、P20.22、V2 ranged 与完整回归。

未验证：真实可见 HUD 布局、真实设备输入、Arc target/apex UI、轨迹预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.24：基于 P20.20 已公开值与 capability 建立设备无关的 Arc target/apex 编辑呈现模型；先冻结纯显示语义，再单独选择鼠标、手柄和触控输入映射。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-23-thrown-weapon-trajectory-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.23.r0_log.md>
