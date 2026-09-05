# Dev.D.UE.0.0.10.P20.24.r0 Log

## 阶段

- 任务：P20.24 thrown-weapon Ballistic Arc editing presentation；
- 基线：`fba673f47977c5d20a2c26e57736a44dda54700f`；
- 分支：`agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 immutable `Fdemo_mapShanmenThrownWeaponArcEditingPresentation`，只投影 BallisticArc 的 canonical target、apex、四项 capability 与两条派生文本。
2. 只接受有效 P20.20 read result；Straight、不可用、无效或 capability 矛盾均失败关闭，且入口先清空复用输出。
3. target 重新核对 finite、空值规范和单位圆；apex 重新核对 finite 与 `[-1,1]`；capability 必须与值域完全一致。
4. 无目标显示 `UNSET / SET AVAILABLE`；有目标显示两位小数坐标与 `SET / CLEAR AVAILABLE`。
5. apex 中间值显示 `+/-`，上界只显示 `-`，下界只显示 `+`；负零统一显示为正零。
6. 新类型没有按键、设备、revision、session、identity、World、Actor、timer、intent、command、route 或 mutable state。
7. HUD 复用 P20.23 已有的一次 `ReadThrownWeaponInputChoiceInteraction()`；模式投影与 Arc 详情投影不重复读取权威状态。
8. Arc 成功时绘制 target、apex 两行，并保留既有 mode 一行；Straight 或失败时只保留 mode。
9. 新增 7 项纯投影自动化；完整 0.0.10 总数 `948→955`。
10. 新增 Arc 呈现 changed-file 规则并扩展 HUD 规则；正/反 self-test 总数 `341→343`。

## 首次证据与修正记录

- 首次 Editor 编译成功：6 actions / native 0 / 26.10 秒。
- 新组首轮为 `6/1`；唯一失败是 `DetailRefresh` 夹具把 `0.625` 的特定两位小数舍入方向写入复合断言。
- 修正仅把该测试样本改为无中点舍入歧义的 `0.5`；未修改生产呈现或 HUD 实现。
- 修正后 Editor 编译成功：4 actions / native 0 / 5.87 秒；新组复测 `7/0`。
- 首次失败日志已保留：SHA-256 `C675B38DE0D675C8F20E3C905E3D8E34A8D871ED562E71F4EA61F355D2CA21FC`。
- 完整 0.0.10 单进程在既有 retry/checkpoint/journal 纯值测试区间进行长时间 CPU 计算；通过成功计数、末项、CPU 与进程响应持续核验，未重启或并行重复。

## 最终聚焦与回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_editing_presentation_final.log` | `7/0` | `4EF5FB64D98F788573663F2E5234CD69156D6B820E5A2FC3EA30D63547203124` |
| `trajectory_presentation_final.log` | `6/0` | `83F9C44FA8AB4AF278B10EE68DB4BD7D7B2A4570CFF3DBD75234815A71D7A0E9` |
| `interaction_port_final.log` | `7/0` | `5D90BC16FE186677EF86CC871177627ECADBF1E72D2F0D8367A1794673DB37F7` |
| `trajectory_toggle_final.log` | `7/0` | `D8C0A1D934B01A106B940D5387A541F9F7913EBA685FB6E77F89E93E03B9392B` |
| `legacy_v2_ranged_final.log` | `22/0` | `58570F8FC21095AB3AD35E4ACB1A82D4A366632AC4465AE547290124210F366A` |
| `full_0_0_10_final.log` | `955/0` | `26FB6CB7A8FD9BC9D0478A0ADCFFFAED975BE4A6A4E0F60CADED5429832FC715` |

最终日志审计：`PASS Logs=6 RecordedSuccess=1004 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`；SHA-256 `A485D87E9B035DF9EDC5B99F112D36C8F2A98E9A2353DE01906FF0794F195F50`。

## 流程与静态证据

- regression self-test：`343/343`；SHA-256 `ADE286A44863DC9010E9A6277A5EB1858B2485A7FC249980D634576F034BA112`；
- static audit：`PASS ExactTests=7 ForbiddenRuntime=0 PhysicalDevices=0 MutationCalls=0 IdentityReads=0 HudCurrentReads=1 HudModeProjections=1 HudArcProjections=1 HudDraws=3 HudRegistryKeys=1 TargetDisplayDraws=1 ApexDisplayDraws=1`；SHA-256 `DF57F52CBAA31D9CF8991024B199DA553FB36432DDEDBBF735BDB6213071153F`；
- changed-file gate：`PASS Changed=4 Rules=2 Required=6 Logs=6`；SHA-256 `510C70C3A064A3C29E6029D1540F2D8E2829CCDF05D5E0E441C3991F1F044112`；
- `git diff --cached --check`：PASS / native 0 / 8 个精确暂存文件；SHA-256 `3C5CD2B356225300861B66BDB6CB06EBE8B421C1C7EF0817DD8118DE4DC82908`。

## 最终构建

- 首次 Editor：6 actions / native 0 / 26.10 秒；SHA-256 `015D2972CDAD0755F13ABA5EF26FE95E68AE39181D56E54119C50276338A5E6A`；
- fixture 修正后 Editor：4 actions / native 0 / 5.87 秒；SHA-256 `E2BFD1B10D45FA2D76161D40D8ECFC363608315D26E5C0C40992ED54481C0856`；
- final Editor：0 actions / native 0 / 0.96 秒；SHA-256 `48DA0673F5252316121E2761652A2F22E379379B8B001E474FBF9F00063B23B2`；
- final Game：5 actions / native 0 / 24.69 秒；SHA-256 `0BE75D9143743E45F6DA5AF7C400DB2AF6A9E1F5B00AF68A208E7D8BA3E0F166`；
- Game artifact：357,596,160 bytes，SHA-256 `038686F7EBB39D3D18ED765F9D6B0C12E740DB596816BD9D8EAE45EC37F77400`；
- Editor artifact：16,340,480 bytes，SHA-256 `C56B7114593D0C4F353AAE1D9DF256BF7ABFC5E97BFFF61D7A4F06E844CDDDD3`。

## P/F

PASS：BallisticArc target/apex/capability 纯投影、失败输出清理、边界方向提示、revision/identity/device 信息最小化、HUD 单次权威读取组合，以及映射出的 P20.20、P20.22、V2 ranged 与完整回归。

未验证：真实可见 HUD、物理设备输入、Arc 编辑写入、轨迹预览、真实投掷、World 碰撞/命中、库存、伤害或产品启动。

## 下一步

P20.25：建立设备无关 Arc 编辑交互合成层，把 normalized target、apex delta 与 clear 操作和一次 P20.20 read 合成为既有 P20.21 stale-safe request；具体物理设备映射后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.24.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-24-thrown-weapon-arc-editing-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.24.r0_log.md>
