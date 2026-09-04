# Dev.D.UE.0.0.10.P20.0.r0 Report

## 1. 结论

P20.0 已完成“暗器百解”中阶操作所需的确定性固定顶点弧线规划契约。

新规划器接收已冻结的战斗动作、暗器实体、起点、目标点、重力、顶点净空以及内容侧速度/飞行时间上限，输出不可变、可重放、自校验的弹道方案。初阶明确返回 `TechniqueLocked`；中阶可以规划手动弧线；高阶当前只保留中阶能力，不在本轮伪装成自动寻路。

本轮只关闭纯值几何与身份边界。没有接入鼠标滚轮、Enhanced Input、Actor、World、ProjectileMovement、碰撞、自动寻路、物品扣除或最终数值调优。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`8a4f0c32bfa182017e707c3cf8fe5098897d0856`（P19.9）；
- 分支：`agent/0.0.10-p20-0-thrown-weapon-arc-planner`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 新增与修改

新增：

- `ShanmenThrownWeaponArcPlanner.h`：204 行；
- `ShanmenThrownWeaponArcPlanner.cpp`：485 行；
- `ShanmenThrownWeaponArcPlannerTests.cpp`：346 行。

修改：

- `ShanmenRegressionMap.json`：增加弧线规划、既有直线暗器、CombatCore 与宽域 CombatRuntime 的改动路径证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加完整正例和“只有弧线测试不足以覆盖改动”的失败关闭反例。

## 4. 请求与精通边界

`FShanmenThrownWeaponArcRequestCapture` 是唯一可编辑输入。成功捕获后得到只读 `FShanmenThrownWeaponArcRequest`，并强制满足：

1. action snapshot 有效，且动作定义严格为 `Combat.Action.ThrownWeapon.Arc01`；
2. 必须携带一个有效的 exact source item instance；
3. 起点、目标、重力、顶点净空和两项 envelope 全部有限且合法；
4. 起点与目标不能退化为同一点；
5. `RequestId` 必须由完整规范输入重新推导并精确匹配。

精通层级是明确的操作门，而不是数值倍率：

- `Beginner`：请求身份可以成立，但不能进入弧线规划；
- `Intermediate`：允许手动选择固定顶点弧线；
- `Master`：本轮只继承中阶弧线能力，自动路径辅助仍未实现。

## 5. 固定顶点弹道

规划器把目标顶点固定为较高端点再加 `ApexClearance`，然后纯函数求解：

- 垂直初速度：`sqrt(2 * gravity * rise)`；
- 上升时间：垂直初速度除以重力；
- 下降时间：`sqrt(2 * fall / gravity)`；
- 水平速度：水平位移除以总飞行时间。

结果在返回前重新构造顶点和目标点，并验证速度、飞行时间、有限值及内容侧 envelope。超出速度或时长上限时返回 typed `Unreachable`，不夹取、不静默改弧线，也不改写内容参数。

`TrySamplePosition` 只在 `[0, FlightTime]` 内输出位置；负时间、非有限时间和飞行结束后的采样全部失败关闭并清空输出。

## 6. 身份、自校验与失败关闭

请求与方案分别使用命名空间：

- `Shanmen.ThrownWeapon.ArcRequest.r1`；
- `Shanmen.ThrownWeapon.ArcPlan.r1`。

确定性身份包含 Run、owner、activation、source entity、source item、action definition、content provenance、精通层级、起终点与全部弹道 envelope。浮点值按 bit pattern 编码，正负零先规范化，避免等价输入分裂身份。

`FShanmenThrownWeaponArcPlan::IsValid()` 会重新求解并比对全部输出及 `PlanId`。结果状态区分 `Planned`、`RequestRejected`、`TechniqueLocked` 与 `Unreachable`；其中 `Unreachable` 也必须重新证明请求确实不可求解，调用方不能给可达请求套一个失败状态。

## 7. 自动化结果

新增 exact tests 6 项：

- `TechniqueGate`；
- `LevelTargetPlan`；
- `HeightVariants`；
- `EnvelopeFences`；
- `DeterminismAndIdentity`；
- `FailureAndSamplingFences`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `CombatRuntime.ThrownWeaponArc` | 6/0 | `2573B9AFE129BCEDA75BD737BBAE16F86AB4602573D55CD87F215A612EC9C144` |
| `automation_full.log` | `Shanmen.0_0_10` | 841/0 | `01D6D7DACEE5383ACC11342E979A208132EE53AB2BD4C758A0ABE230939D97D9` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `FAC8F7F6FAF8E94129956BF1E1337A771DFF196E3D8918CB9556EFAB435C4E56` |
| `automation_legacy_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | `B8453E177DE27D57BCF86C4A1332A3284BAEA539E98AF4D8086250C6D369D06F` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `77590ABD888B1B937891F24483239CFA6A08873C9D5E8360E2216BFD8C55024D` |
| `automation_legacy_item_use_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `DC642B6635BCC6540746507A975BC953520530E18066FA9CC40499FF0EAC997D` |

全量由 P19.9 的 835 增加到 841。六份正式日志均只有一个 canonical `RunTests` command、精确预期 Success、Fail 0、一个 native terminal、Fatal/Unhandled/Ensure 0。证据审计：`PASS Logs=6 RecordedSuccess=963`，SHA-256 `C2ED946CBAB954490B069371E59D71A3876CE9C2655BBD9FC7E072BCD913C417`。

## 8. 门禁、边界与构建

真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=6
```

- gate SHA-256：`1BEB292C6DF8A39D5DD794562E4B411ACF197A3852CB576A24862AF5117C972B`；
- self-test：`309/309 PASS`，SHA-256 `7B02180C8205D4A9FEBCFD93AA0C47F16A3CAF9C9DD712212971AC34426FA8CC`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：native 0。

构建：

- Game final：5 actions / 36.96s / native 0，SHA-256 `02E4C3B24D953D6AA5375C93CFF7B6CA0E3DD3F28FD1EFF691827311DB024AD0`；
- Editor final：up to date / 1.15s / native 0，SHA-256 `20D6B9E419EEC99C30C408A7CBAC3843EEAAF60FF3CF8CF05BB2D146B3D4F765`。

产物：

- `demo_map.exe`：357,046,784 bytes，SHA-256 `C26616E9E3CBE7DCE0802B244D513FC279ADA13C65FCB0CFF5092E5B39035D18`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：1,980,416 bytes，SHA-256 `75146588F78A6FDF51F8137A39B4B582C4081481198B9AA851A1E4F4ED93366C`。

## 9. 异常与修正

首次 exact 自动化为 5 Success / 1 Fail / native 255。唯一失败是 `EnvelopeFences` 对理论顶点 `500.0` 使用 bitwise 浮点相等；弹道重构值在容差内正确，但测试表达过严。改为与规划器一致的数值容差后，exact 6/6 与全量 841/841 均通过。首次失败日志已保留，SHA-256 `84DB70BA7A24ABAF3F5C5F7A4A1E0103F7B81ECFF92F60B61FE7F245D189E0DE`。

首次编译还暴露测试局部 `FVector` 可能未初始化的 C4701 警告；显式初始化所有采样输出后，后续 Editor 与 Game 构建不再出现该警告。

全量从 canonical command 到 native terminal 用时约 34 分 50 秒。既有 Sword Rhythm checkpoint/envelope 段包含长时单例；进程始终响应，CPU 与 Success 持续推进，最终未重启、未裁剪，也没有用部分结果替代完成证据。

## 10. P/F 边界与下一步

P20.0 关闭的是“一个已授权中阶弧线选择如何变成确定性弹道证据”。它没有声称暗器已经在场景中按弧线飞行，也没有声称滚轮调弧、碰撞命中或高阶辅助寻路可玩。

建议下一 P-stage 把该计划作为只读输入接到既有暗器 product route/world delivery 之前，并继续复用已有 item、Run 与 impact 权威；实际滚轮输入、场景碰撞、轨迹反馈和手感验收应留给明确的 F-stage。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-0-thrown-weapon-arc-planner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-0-thrown-weapon-arc-planner/Docs/Report/Dev.D.UE.0.0.10.P20.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-0-thrown-weapon-arc-planner/Docs/Log/Dev.D.UE.0.0.10.P20.0.r0_log.md>
