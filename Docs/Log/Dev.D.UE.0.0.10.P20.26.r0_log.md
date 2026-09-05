# Dev.D.UE.0.0.10.P20.26.r0 Log

## 阶段

- 任务：P20.26 thrown-weapon Ballistic Arc editing controller route；
- 基线：`f6a2ba33da6df4e198452bab2a0ce17ad4a666db`；
- 分支：`agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 在现有 `Ademo_mapPlayerController` 增加 target、apex adjustment、target clear 三个设备无关入口。
2. 三个入口共享一条 helper：初读 P20.20 一次、调用所选 P20.25 compose 一次、调用既有 P20.21 route 一次。
3. P20.21 对有效 request 再读一次当前 model 作为 stale fence，并至多路由一次 P20.19 intent。
4. compose 失败时只把 invalid request 路由一次，得到既有 `RequestInvalid`；P20.21 下游 read/route 计数均为 0。
5. 不新增 protocol、identity、revision、state、registry action、物理设备映射、timer、loop 或 retry。
6. 合成器 include 下沉到 `.cpp`，公开控制器头文件不承担额外传递依赖。
7. 新增 7 项 transient World/PlayerController/GameMode 自动化，完整 0.0.10 总数 `962→969`。
8. 覆盖 fresh read/no-op、三种成功路径、Straight/无 target capability、apex 边界、无效 payload 与 gameplay lock。
9. PlayerController 与新测试路径的 changed-file 规则已补齐；正/反 self-test `345→347`。
10. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_editing_controller_route_final.log` | `7/0` | `CDA322F714B0FCE19D6A224907BD72EC81B92C03CB48A3A391F70841E8303453` |
| `arc_editing_interaction_composition_final.log` | `7/0` | `74968D1B6DB9AEDCE6AB1D5450EB8A1C6D590307FA905C228505C834053D7C5F` |
| `interaction_request_coordinator_final.log` | `7/0` | `020EF33898F2721AB0DE0FF583D8C77AF6AE87BE157455185720D5C8643BDC86` |
| `interaction_port_final.log` | `7/0` | `3D1B648BAF5FB2803E4403EF0AF2AA26E528D4AE671B79AE6FE82BE12088CD46` |
| `intent_adapter_final.log` | `7/0` | `DE882C5983CC595773B09963DA21A544E42DC0F7597857C6A9BE40BC18C2EB3B` |
| `full_0_0_10_final.log` | `969/0` | `9161A0037A3E718E3FB68DD35083ED15B2848AB6C10661B1D133570DC8F5D7B5` |
| `legacy_input_restore_final.log` | `101/0` | `6ED011C3C926E625D77CBC3D4CA1168E0C67417E70EC10323D44A1D1FFEA5B3F` |
| `legacy_v2_ranged_final.log` | `22/0` | `0F499DAF3906E2935004F60B766ECAC63B3FE0165FCF3DD763C5650EE30BBEA2` |

聚焦、依赖、全量与旧回归合计 `1127/0`。日志审计：`PASS Logs=8 RecordedSuccess=1127 Failed=0 Fatal=0 MissingTerminal=0 BadCommands=0 Invalid=0 ExternalProbe=0`；SHA-256 `71ADC4D7C7D5F7324323AEC580333D4C987D9BC4520C87FA9560CAAA308852E7`。

完整组以一个连续进程运行；最慢的 SwordRhythm retry/checkpoint/journal/codec 区间持续观测成功数、末项与 CPU，未重启、未并行重复。终点为 `WorldGameplay.SweepAdapter`。

## 流程与静态证据

- regression self-test：`347/347`；SHA-256 `E4F3561C70453C0787184BE52EFB31A67510D084804935B56193143DAC9354F5`；
- static audit：`PASS ExactTests=7 PublicEntries=3 Definitions=3 InitialReads=1 RequestRoutes=1 Delegations=3 InvalidFallbacks=1 PhysicalDevices=0 PhysicalDispatchTests=0 NewProtocol=0 DirectAuthority=0 LoopsOrRetries=0`；SHA-256 `16D8C3C3838445509242721BD4547BE969A6BB9E564E5B015C9216D0076DF969`；
- changed-file gate：`PASS Changed=5 Rules=2 Required=23 Logs=8`；SHA-256 `2232D6DA2C081A0EF9EBB5F2D439293A954EF5C63D607D7CDCDB2ABED68BA9AA`；
- `git diff --cached --check`：PASS / native 0 / 7 个精确暂存文件。

## 构建与产物

- initial Editor：26 actions / native 0 / 32.08 秒；SHA-256 `8997E415F280F5E2A0282E9140D8695962CACDE3CB6677349598BCA657CD0575`；
- final Editor：26 actions / native 0 / 22.57 秒；SHA-256 `9108D0F060AA85719552843FFB4376A9F00539618E8B0739FCDC662734677F69`；
- final Game：25 actions / native 0 / 30.07 秒；SHA-256 `2195D71A51BA4C9B629283ED073B8F6B51C7D89932F1F12D80C6DAAF578FBC6B`；
- Editor artifact：16,387,584 bytes，SHA-256 `5B5B32E71C27E0BA4BE3E420906017815C368223765C41FEAA3E9F46E2912B64`；
- Game artifact：357,636,096 bytes，SHA-256 `522EF6B1662EB6C22CAA91823B6D23F6AD4BD214AB1DE5714C1DB61F8BE5725B`。

## P/F

PASS：三个设备无关控制器入口、固定 read→compose→stale-safe route、typed failure、fresh-read no-op、apex 双向边界、锁定拒绝，以及完整新旧回归。

未验证：真实 UI/设备映射、轨迹预览编辑、真实投掷、World 碰撞/命中、库存、伤害或产品启动。

## 下一步

P20.27：确定第一种可重映射 Arc 编辑设备策略，优先复用现有顶视角 pointer/aim 语义产生 target intent，并为 apex 增减与 clear 选择无冲突 action；物理层只能调用 P20.26 三个入口。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route/Docs/Report/Dev.D.UE.0.0.10.P20.26.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-26-thrown-weapon-arc-editing-controller-route/Docs/Log/Dev.D.UE.0.0.10.P20.26.r0_log.md>
