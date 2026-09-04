# Dev.D.UE.0.0.10.P20.19.r0 Report

## 1. 结论

P20.19 已在 P20.18 controller route 前增加设备无关的投掷武器 choice-edit intent 捕获层。新的 `Fdemo_mapShanmenThrownWeaponInputChoiceIntent` 表达 trajectory、arc target、arc apex 与 clear 四类逻辑编辑；同值输入先复用 P20.10 规则规范化，再得到稳定且不含 revision 的确定性 intent identity。

一个有效 intent 只读取当前权威 choice state 一次，把该 state 的 revision 冻结进一条 P20.10 command，再只委托 P20.18 controller route 一次。新层不保存第二份 choice state、不直接提交 P20.11 session，也没有新增物理键位、UI、World/Actor 所有权、重试或 fallback 写路径。

聚焦自动化新增 `7/0`，最终全量为 `921/0`。本轮只执行编译与无头自动化；未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`f0f1ffcca1ccdf75cd07454e18db891ba2a446cf`（P20.18）；
- 分支：`agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 逻辑 Intent 契约

Intent 只有 `SelectTrajectory`、`SetArcTargetIntent`、`AdjustArcApex` 与 `ClearArcTargetIntent` 四种有效 shape。trajectory 恢复 Straight 与选择 BallisticArc 共用第一种 shape；target 归一化与 apex clamp 完全复用冻结的 P20.10 command factory，不在输入层复制规则。

确定性 ID 使用命名空间 `demo_map.ShanmenThrownWeapon.InputChoiceIntent.r1`，由 kind 与 canonical payload 派生。等价 target（例如 `(3,4)` 与 `(0.6,0.8)`）得到同一 ID；NaN、零 target、零 apex delta 或非法 trajectory 失败关闭并清空输出。

Intent 本身不携带 expected revision。只有路由时读取到当前 state 后，`TryCaptureCommand` 才把该 revision 冻结到 P20.10 command，因此逻辑输入与并发控制保持分离。

## 4. 权威读取与唯一委托

`Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter::Route` 固定顺序为：intent validity → choice source resolve 一次 → choice state read 一次 → command capture 一次 → P20.18 route 一次。每个短路点都返回 typed status、diagnostic 与精确计数。

`Ademo_mapPlayerController::RouteThrownWeaponInputChoiceIntent` 只为当前 state snapshot 解析 auth GameMode，并将生成的 command 交给既有 `RouteThrownWeaponInputChoiceCommand`。后者继续拥有 gameplay/surface/mode gate 与唯一 P20.11 session submit；新 adapter 内直接 `.Submit` 为 0。

返回结果保留原 intent、读取到的 state、冻结 command、完整 P20.18 controller result，以及 source/read/capture/route 四组计数。controller 未返回同一 command 的有效证据时，本地转为 `ControllerProtocolRejected`，不会误报接受。

## 5. 重放、NoChange 与并发冲突

- 新鲜等值编辑保留 P20.11 `NoChange/NoChange`；
- 同一 command 在读取后被并发应用时保留 `Replay/Replay`，不会二次修改；
- 不同 command 抢先推进 revision 时保留 `ReductionRejected/RevisionMismatch`；
- gameplay、Combat Run 与 product lifecycle 拒绝继续由 P20.18/P20.11 typed 证据表达；
- intent 层不重读 state、不重建第二条 command，也不自动重试。

这使“用户逻辑意图是什么”“它针对哪个权威 revision”“controller/session 如何裁决”成为三段可分别审计的不可变证据。

## 6. 自动化覆盖

新增 `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter` 7 项：

- `IntentCanonicalization`：规范化、稳定 ID 与非法原始值；
- `SourceOrder`：source/read/capture/route 的固定短路顺序；
- `AllIntentKinds`：Arc、target、apex、clear、Straight 五次编辑共用唯一 session；
- `NoChangeReplayConflict`：no-op、精确重放与并发 revision conflict；
- `ControllerRejections`：gameplay、Run 与 lifecycle typed 拒绝；
- `ControllerProtocol`：缺失或错 command 的 controller 证据失败关闭；
- `PlayerControllerBoundary`：无 World/GameMode 时停在 source，后续计数均为 0。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `choice_intent_final.log` | `Product.ThrownWeaponInputChoiceIntentAdapter` | `7/0` | `5407212A8916344F9E036C5C5711A2D55E731AB6869B01FAB185B39203E69C4B` |
| `choice_controller_final.log` | `Product.ThrownWeaponInputChoiceControllerAdapter` | `7/0` | `72C63B80A50D387DD8EAC15120EFB16A4E6D1DD8DB77AE7E21E115ED6006219E` |
| `input_choice_session_final.log` | `Product.ThrownWeaponInputChoiceSession` | `4/0` | `89A82B85DBF3DBD63A81E0B0165229CEFBB4BD8A259391E792A430BED6AF5F25` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `23/0` | `D1DF83B821B57E4A080D64DA5B6CB2FAD2C77CCA0CFB78251ACC47EC61585824` |
| `hotbar_confirmation_final.log` | `Product.ThrownWeaponHotbarConfirmationAdapter` | `7/0` | `7D4A920BF3EC49C4D1F19CB75B87B72FC5960203215C53FC24AB33DFC45CA10E` |
| `arc_launch_input_final.log` | `Product.ThrownWeaponArcLaunchInputAdapter` | `6/0` | `C00743351879768F5D23FD2EF056F3CF55E6CA8831DE5233AEA0F540E36C19C5` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `AEE369F2C208DE8A02929F1D48BF46A1A0C8A65529F215720C7763A90E8B8D2C` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `792427F1BBCA78534034122C74973D50DFA97BE1861C253649AD5BF23305D0D0` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `0A2A90807882BE145777582C10EF8B3558B6D0A5750AEEECDD03C86B4872EFB6` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `921/0` | `55BBEE40D2C13F38F434032CAF4EEA685B0E4916630C167BE4EAE08EABED4A95` |

有效日志审计：`PASS Logs=10 RecordedSuccess=1107 Invalid=0`，SHA-256 `F4F37DBE5EC71376B1EC8C88AD492495B5A7D9DF67E2BF7723BD245F0C42EEF8`。流程映射自测从 329 增至 `331/331`，SHA-256 `014668A39366A4F78997D3E62478966238246B8D960BDB9C5477619ED10F9EBC`。

## 8. 首次验证、环境恢复与门禁

首次 Editor 构建直接通过：22 actions / native 0 / 49.12 秒，SHA-256 `66CA15EDE0D4D11AA36D71D37E71F149B517FEE09AB07D130783B8155B3FBC2A`。首次 intent exact 直接为 `7/0`，SHA-256 `B2D19DC271163BC04DF0B54CD1B8F4F36F88E8CDA99F9A351C0EA3A8E6578245`；没有源码、编译或断言返工。

首次全量在 `697/0` 时被 UE 5.8 EOS 配置更新与 Home Panel 的 `google.com/generate_204` 探测反复拖住，保留中止日志 SHA-256 `FD351CF964C3F3AE01025642CA1D3382380D0134F589915EFDE084F26D99321A`。加入官方 `-NoEOS` 后仍确认 Home Panel 探测存在，第二份 `673/0` 中止日志 SHA-256 `3700038A947C19FECD8DB8993B23779A69044288CA57150E63FD4260304DFD1A`。最终运行额外使用命令行临时覆盖 `HomeScreen.EnableHomeScreen=0`，外网探测为 0，完整 `921/0` 且 native 0；没有修改引擎、Windows 或项目持久配置。

静态审计：`PASS ForbiddenProduction=0 DirectSessionSubmit=0 AddedPhysicalBinding=0 P2018DelegateCalls=1 AddedChoiceStateReads=1`，SHA-256 `878E50263556C0AB88FC41E3C6D358BB8FEE4A33770AD4E5D10694DD7B5F7726`。

最终 changed-file regression gate：`PASS Changed=9 Rules=2 Required=18 Logs=10`，SHA-256 `7691B6805EEE458535F7505732EBF1B48F16F72D49A06D20142E345605A1776A`。

暂存区格式检查：`PASS / 9 files / native 0`。

## 9. 构建与产物

- final Editor：0 actions / native 0 / 1.41 秒，SHA-256 `AE23E2C2A3965DEEBF25679AE46C0B12D784434D7E446D7D951D5790BC859030`；
- final Game：21 actions / native 0 / 54.73 秒，SHA-256 `E40ED68C72BFFF9B1E77146D31FB3C097DE4611180533FC86B13AB18466BF3C6`；
- `Binaries/Win64/demo_map.exe`：357,477,888 bytes，SHA-256 `1A2AC48CA2C805C499242996B6AE07E10EB79071A98EF2F30FB28ABDE24005B3`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,195,072 bytes，SHA-256 `5321A1AF2631475D4DB629DAD41EA3EA8775A29FECDC68B68762FE8DA6265187`。

## 10. P/F 边界与下一步

P20.19 证明的是：一个设备无关的 choice-edit intent 能先规范化并稳定标识，再对唯一当前 state 读取一次 revision、冻结一条 P20.10 command，并只委托 P20.18 route 一次；typed no-op、重放、冲突与拒绝证据可完整追溯。

它没有证明物理设备或 UI 已产生 intent，也没有证明预览、真实投掷、World trace、碰撞、命中、库存扣减或伤害。无头自动化与 Development 构建不能描述为产品运行验收。

建议 P20.20 增加 device/UI-neutral choice-edit interaction port/read model：只投影当前 canonical choice 与允许操作，并输出本轮 intent；不拥有第二份 state，不直接生成 revision，不绑定具体键位或控件。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent/Docs/Report/Dev.D.UE.0.0.10.P20.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-19-thrown-weapon-choice-edit-intent/Docs/Log/Dev.D.UE.0.0.10.P20.19.r0_log.md>
