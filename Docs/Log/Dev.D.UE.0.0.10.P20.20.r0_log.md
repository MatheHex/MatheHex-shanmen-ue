# Dev.D.UE.0.0.10.P20.20.r0 Log

## 阶段

- 任务：P20.20 thrown-weapon choice interaction read model/port；
- 基线：`f5f3f6ee0867168a9f5aab6a171aae940948392d`；
- 分支：`agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 `demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h/.cpp`。
2. 增加 revisionless immutable read model，只投影 P20.11 state 的 canonical 可见值。
3. 增加 typed capability mask，表达当前 choice 下有意义的 trajectory/target/apex/clear 操作。
4. read-model ID 只由可见 choice 与 capability 派生，不携带 revision/state ID。
5. 固定只读顺序：source resolve 1 → state read 1 → projection 1；结果不保存完整权威 state。
6. 四个 emit helper 只生成既有 P20.19 intent，不读取 revision、不 route、不 `.Submit`。
7. `Ademo_mapPlayerController` 新增 auth GameMode read seam，不新增按键/UI/重试/写路径。
8. 新增 7 项 exact 测试；更新 changed-file regression map 与正/反 self-test，总数 `333/333`。

## 首次证据

- Editor 首次编译：23 actions / native 0 / 55.05 秒；
- `choice_interaction_first.log`：7/0 / terminal 1 / fatal 0 / external probe 0，SHA-256 `D7CB030B98257FAE9B339A651D1BEAE063C0D444BDA1749E3D28FDE28C6E96C5`；
- 没有源码、编译或测试断言失败与返工。

## 最终聚焦与全量回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `choice_interaction_final.log` | `7/0` | `D7CB030B98257FAE9B339A651D1BEAE063C0D444BDA1749E3D28FDE28C6E96C5` |
| `choice_intent_final.log` | `7/0` | `5EC5C84D825D1CBB25F883C9ACF5B5B3A7038A03712ADDDE7B6A4F3DE1B59E49` |
| `choice_controller_final.log` | `7/0` | `E199B8A5CBEA957AABD82804F9ED5C2531590E8698F7DAEF6D128242EEE5E6C3` |
| `input_choice_session_final.log` | `4/0` | `9E6631A7FE69135FE0FB1EC26B67774615BEE3535A84C3B3EE120E8D3F3D09F1` |
| `input_choice_final.log` | `30/0` | `5761F85FEF31BECBD7B5569F27FC06F1270E6419530BB0C6C75DEB1B535888E1` |
| `hotbar_confirmation_final.log` | `7/0` | `EBB492F30A08ECE12939391011344DEF3D19783D295BD2AF9CAD27E5B66541DF` |
| `arc_launch_input_final.log` | `6/0` | `BBB9942316DC1C1735E33710A03B2762E1CB5AAD8F4083EC7C9BDEDAC96C7F21` |
| `input_adapter_final.log` | `9/0` | `1AEA1E52A064A6CCD07869925C37645E3E969D456439C30F787A8079F4021160` |
| `legacy_input_restore_final.log` | `101/0` | `D0E3AEA63D6D615EF19B6B8F13409A6E02267554A878EB319676A9D1A8B027C4` |
| `legacy_v2_ranged_final.log` | `22/0` | `67C5AF957E49167E8DFA5492CC3489D5B2680A4796A4E5DB7D7B1241775BF2FE` |
| `full_0_0_10_final.log` | `928/0` | `5FC58C307B898DE26F46277DED78A60E8405B0454AB9E4533D22CC07B85C508B` |

日志审计：`PASS Logs=11 RecordedSuccess=1128 Invalid=0 ExternalProbe=0`，SHA-256 `B7B9943FA12A0239C68C48D1A9A0F5F8DBBCA0366362BC98954CC9011BE4FE3F`。

全量中的既有 retry/checkpoint/manifest 测试具有长计算窗口，但从 651 到 928 持续推进，最终 native 0；未发生重启、重复计数或网络恢复操作。

## 流程与静态证据

- regression self-test：`333/333`，SHA-256 `A1D5C5D4C186419A0F2AD7925F18DF21E29DD563E26C901EA49C8E6D0F2CF37F`；
- static audit：`PASS ForbiddenProduction=0 RevisionReads=0 DirectSessionSubmit=0 DirectIntentRoute=0 IntentFactoryCalls=4 AddedPhysicalBinding=0 ControllerChoiceReads=1 ControllerPortReads=1`，SHA-256 `64F0F64BFA374A5A7F575585FEAA75DD832E2CE772E4938F4A77273D7BCA7B13`；
- changed-file gate：`PASS Changed=9 Rules=2 Required=19 Logs=11`，SHA-256 `CC21105D56616C7C7CC81A2BF8E257A88E2ACE51B6C3A147AA0075582021C9F6`；
- `git diff --check`：PASS / native 0，SHA-256 `0E0E5C876FB4EDCBCCAD8B4AB94F5C8486F820BAC2EDF430FE8DF81D2746D5C7`。

## 最终构建

- Editor：0 actions / native 0 / 1.15 秒，SHA-256 `40C73EBF3A06128DD715F756E75C77AE5CDDBCD94F1B391BEC41D756439B2C1F`；
- Game：22 actions / native 0 / 57.03 秒，SHA-256 `139FC90764F679438A2799B8F0D0C4C9FE1C4F9A9B2BD7812CDE412392F3DC2F`；
- Game artifact：357,501,440 bytes，SHA-256 `B4FECFDE7801F701E0DE1CD2DFE0406878679F4DFFC754B172CB1AF7AFAEC507`；
- Editor artifact：16,223,232 bytes，SHA-256 `EA3E2AAFCC651A0A277700716885D6A4458BDCCB1F5C05448164DA2AE45FD8C2`。

## P/F

PASS：唯一 choice state 的 revisionless 投影、typed capability、稳定 identity、一次只读顺序、四类 P20.19 intent 输出、饱和边界与缺源拒绝。

未验证：物理设备/UI、stale-view 交互事务、预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.21：device/UI-neutral interaction request coordinator；冻结 read-model ID 与请求，重读当前 projection 后 typed 拒绝 stale view，再只输出/路由一条 P20.19 intent，不绑定具体设备或控件。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port/Docs/Report/Dev.D.UE.0.0.10.P20.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-20-thrown-weapon-choice-interaction-port/Docs/Log/Dev.D.UE.0.0.10.P20.20.r0_log.md>
