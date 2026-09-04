# Dev.D.UE.0.0.10.P20.19.r0 Log

## 阶段

- 任务：P20.19 thrown-weapon choice-edit intent capture；
- 基线：`f0f1ffcca1ccdf75cd07454e18db891ba2a446cf`；
- 分支：`agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 `demo_mapShanmenThrownWeaponInputChoiceIntentAdapter.h/.cpp`。
2. 新增四类 immutable logical intent；规范化规则只复用 P20.10 command factory。
3. 使用独立命名空间和 canonical payload 派生稳定 intent ID；revision 只在 route 时进入 command。
4. 固定顺序：intent validity → source resolve 1 → state read 1 → command capture 1 → P20.18 route 1。
5. 结果保留 intent/state/command/controller result、typed status、diagnostic 与四组调用计数。
6. `Ademo_mapPlayerController` 新增逻辑 intent 入口，不新增按键/UI，不直接提交 session。
7. 新增 7 项 exact 测试，覆盖规范化、顺序、全部操作、no-op/replay/conflict、拒绝、protocol 与 controller boundary。
8. 更新 changed-file regression map 与正/反 self-test，总数 `331/331`。

## 首次证据

- `editor_build_first.log`：22 actions / native 0 / 49.12s，SHA-256 `66CA15EDE0D4D11AA36D71D37E71F149B517FEE09AB07D130783B8155B3FBC2A`；
- `choice_intent_first.log`：7/0 / terminal 1 / fatal 0，SHA-256 `B2D19DC271163BC04DF0B54CD1B8F4F36F88E8CDA99F9A351C0EA3A8E6578245`；
- 没有源码、编译或测试断言失败与返工。

## 最终聚焦回归

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `choice_intent_final.log` | `7/0` | `5407212A8916344F9E036C5C5711A2D55E731AB6869B01FAB185B39203E69C4B` |
| `choice_controller_final.log` | `7/0` | `72C63B80A50D387DD8EAC15120EFB16A4E6D1DD8DB77AE7E21E115ED6006219E` |
| `input_choice_session_final.log` | `4/0` | `89A82B85DBF3DBD63A81E0B0165229CEFBB4BD8A259391E792A430BED6AF5F25` |
| `input_choice_final.log` | `23/0` | `D1DF83B821B57E4A080D64DA5B6CB2FAD2C77CCA0CFB78251ACC47EC61585824` |
| `hotbar_confirmation_final.log` | `7/0` | `7D4A920BF3EC49C4D1F19CB75B87B72FC5960203215C53FC24AB33DFC45CA10E` |
| `arc_launch_input_final.log` | `6/0` | `C00743351879768F5D23FD2EF056F3CF55E6CA8831DE5233AEA0F540E36C19C5` |
| `input_adapter_final.log` | `9/0` | `AEE369F2C208DE8A02929F1D48BF46A1A0C8A65529F215720C7763A90E8B8D2C` |
| `legacy_input_restore_final.log` | `101/0` | `792427F1BBCA78534034122C74973D50DFA97BE1861C253649AD5BF23305D0D0` |
| `legacy_v2_ranged_final.log` | `22/0` | `0A2A90807882BE145777582C10EF8B3558B6D0A5750AEEECDD03C86B4872EFB6` |
| `full_0_0_10_final.log` | `921/0` | `55BBEE40D2C13F38F434032CAF4EEA685B0E4916630C167BE4EAE08EABED4A95` |

日志审计：`PASS Logs=10 RecordedSuccess=1107 Invalid=0`，SHA-256 `F4F37DBE5EC71376B1EC8C88AD492495B5A7D9DF67E2BF7723BD245F0C42EEF8`。

## 环境恢复记录

- 原始全量在 `697/0` 时因 EOS 配置更新及 Home Panel `generate_204` 探测持续拖慢而中止保留，SHA-256 `FD351CF964C3F3AE01025642CA1D3382380D0134F589915EFDE084F26D99321A`；
- 加 `-NoEOS` 后在 `673/0` 保留第二份中止日志，仍观察到 Home Panel 探测，SHA-256 `3700038A947C19FECD8DB8993B23779A69044288CA57150E63FD4260304DFD1A`；
- 最终以 `-NoEOS` 和命令行临时 `HomeScreen.EnableHomeScreen=0` 重跑，外网探测 0、`921/0`、terminal 1、native 0；
- 两份中止日志均为 0 test failure，但因无 terminal marker 未列入有效日志审计；没有修改引擎、Windows 或项目持久配置。

## 流程与静态证据

- regression self-test：`331/331`，SHA-256 `014668A39366A4F78997D3E62478966238246B8D960BDB9C5477619ED10F9EBC`；
- static audit：`PASS ForbiddenProduction=0 DirectSessionSubmit=0 AddedPhysicalBinding=0 P2018DelegateCalls=1 AddedChoiceStateReads=1`，SHA-256 `878E50263556C0AB88FC41E3C6D358BB8FEE4A33770AD4E5D10694DD7B5F7726`；
- changed-file gate：`PASS Changed=9 Rules=2 Required=18 Logs=10`，SHA-256 `7691B6805EEE458535F7505732EBF1B48F16F72D49A06D20142E345605A1776A`；
- staged diff：`PASS / 9 files / native 0`。

## 最终构建

- Editor：0 actions / native 0 / 1.41s，SHA-256 `AE23E2C2A3965DEEBF25679AE46C0B12D784434D7E446D7D951D5790BC859030`；
- Game：21 actions / native 0 / 54.73s，SHA-256 `E40ED68C72BFFF9B1E77146D31FB3C097DE4611180533FC86B13AB18466BF3C6`；
- Game artifact：357,477,888 bytes，SHA-256 `1A2AC48CA2C805C499242996B6AE07E10EB79071A98EF2F30FB28ABDE24005B3`；
- Editor artifact：16,195,072 bytes，SHA-256 `5321A1AF2631475D4DB629DAD41EA3EA8775A29FECDC68B68762FE8DA6265187`。

## P/F

PASS 范围：逻辑 intent 规范化、稳定身份、一次权威 state read、一次 P20.10 command capture、一次 P20.18 delegate，以及 typed no-op/replay/conflict/rejection 证据。

未验证：物理设备、UI、预览、真实投掷、World trace/碰撞/命中、库存扣减、伤害或产品启动。

## 下一步

P20.20：增加 device/UI-neutral choice-edit interaction port/read model，只暴露当前 canonical choice 与允许操作并输出 P20.19 intent；不拥有第二份 state，不直接生成 revision，不绑定具体设备或控件。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent/Docs/Report/Dev.D.UE.0.0.10.P20.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent/Docs/Log/Dev.D.UE.0.0.10.P20.19.r0_log.md>
