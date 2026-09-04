# Dev.D.UE.0.0.10.P20.0.r0 Development Log

## 1. 目标与基线

- 基线：`8a4f0c32bfa182017e707c3cf8fe5098897d0856`（P19.9）；
- 分支：`agent/0.0.10-p20-0-thrown-weapon-arc-planner`；
- 目标：为“暗器百解”中阶操作建立纯值、确定性、固定顶点的弹道规划契约；
- 约束：不接物理输入、World、Actor、ProjectileMovement、碰撞、自动寻路、库存修改或最终数值调优。

## 2. 设计审计

审计了既有 P7 暗器执行与其 product item/world/Run/input 链。现有实现已经关闭初阶直线投掷、exact item、Run identity、World delivery、impact 与产品输入边界，但没有一个可供中阶操作使用的弧线解算权威。

本轮没有修改既有直线飞行，也没有在 product adapter 中临时计算轨迹。新契约位于 `ShanmenCombatRuntime`，只产生可验证的几何计划；后续适配器消费该计划，避免 World 层与输入层各自实现一套弹道公式。

## 3. 不可变请求

捕获输入包含 action snapshot、技术层级、起点、目标点、重力、顶点净空、最大初速度和最大飞行时间。

成功捕获后，所有字段转入 private `BlueprintReadOnly` 请求。请求必须绑定 canonical action definition 与 exact source item，并把 signed zero 规范化后派生 `RequestId`。`IsValid()` 重新计算身份，不能用任意 GUID 绕过输入一致性。

初阶请求可以作为合法身份证据保存，但 `IsArcUnlocked()` 为 false。这样“操作未解锁”与“请求损坏”保持两个不同的失败状态。

## 4. 纯值弹道求解

求解器以较高端点加净空作为顶点 Z：

1. 根据上升高度和重力求垂直初速度；
2. 分别求上升与下降时间；
3. 用总时间求水平速度；
4. 检查 launch-speed/time envelope；
5. 重构顶点与目标点；
6. 只有全部不变量成立才生成 plan identity。

它不寻找障碍、不扫描 Actor、不改变重力、不夹取超限输入。调用方若要求不可达的弧线，会得到合法的 `Unreachable` 结果。

## 5. 计划、采样与结果状态

计划保存 request、初速度、世界向下重力、顶点、速度、上升时间和总飞行时间。`PlanId` 绑定请求与完整求解结果；`IsValid()` 会重新求解并比较全部字段。

采样函数只处理合法计划和飞行时间范围。所有拒绝路径先将输出归零，防止调用方误用旧值。

结果区分：

- `RequestRejected`：输入或不可变请求无效；
- `TechniqueLocked`：初阶尚未解锁弧线；
- `Unreachable`：请求有效，但超出其速度/时间 envelope；
- `Planned`：获得完整、自校验方案。

`Unreachable` 的结果自校验会再次调用求解器，防止可达请求伪装为不可达。

## 6. 确定性身份

请求身份纳入 Run、owner、activation、source entity/item、动作定义、内容版本与摘要、技术层级、起终点和全部解算参数。

浮点以规范化后的 64-bit pattern 编码；`-0.0` 与 `+0.0` 不会产生不同 ID。相同输入可重放出相同 RequestId/PlanId；更改顶点净空或 content digest 会产生新身份。

## 7. 测试开发

新增 6 项 exact contract：

- `TechniqueGate`：初阶锁定、中阶解锁、高阶保留中阶操作；
- `LevelTargetPlan`：同高目标的起点、顶点、终点与顶点垂直速度；
- `HeightVariants`：升高和降低目标的非对称弧线；
- `EnvelopeFences`：速度/时间超限与伪造 Unreachable 失败关闭；
- `DeterminismAndIdentity`：重放、signed zero、弧高和内容来源身份；
- `FailureAndSamplingFences`：退化、非有限、错误 action、缺 item 与采样范围。

## 8. 首次失败与修正

首次 exact：

```text
Success=5 Fail=1 NativeExit=255
Fail=Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc.EnvelopeFences
Assertion=A content envelope can admit a taller manual arc
```

根因是测试对浮点重构后的 apex Z 使用 `== 500.0`。求解器的终点重构与其它断言均正确；失败只来自测试采用了 bitwise equality。改成统一的相对/绝对容差后 exact 6/6。首次失败日志保留于 `automation_exact_initial.log`，SHA-256 `84DB70BA7A24ABAF3F5C5F7A4A1E0103F7B81ECFF92F60B61FE7F245D189E0DE`。

首次 Editor 编译另有测试局部输出可能未初始化的 C4701 警告。把所有采样输出显式初始化为 `FVector::ZeroVector` 后，后续编译无该警告。

## 9. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 6/0 | `2573B9AFE129BCEDA75BD737BBAE16F86AB4602573D55CD87F215A612EC9C144` |
| `automation_full.log` | 841/0 | `01D6D7DACEE5383ACC11342E979A208132EE53AB2BD4C758A0ABE230939D97D9` |
| `automation_legacy_attributes.log` | 4/0 | `FAC8F7F6FAF8E94129956BF1E1337A771DFF196E3D8918CB9556EFAB435C4E56` |
| `automation_legacy_enemy.log` | 44/0 | `B8453E177DE27D57BCF86C4A1332A3284BAEA539E98AF4D8086250C6D369D06F` |
| `automation_legacy_v2_ranged.log` | 22/0 | `77590ABD888B1B937891F24483239CFA6A08873C9D5E8360E2216BFD8C55024D` |
| `automation_legacy_item_use_armor.log` | 46/0 | `DC642B6635BCC6540746507A975BC953520530E18066FA9CC40499FF0EAC997D` |

每份正式日志通过唯一 command、精确 Success、Fail 0、唯一 native terminal、Fatal/Unhandled/Ensure 0 检查：

```text
EVIDENCE_AUDIT: PASS Logs=6 RecordedSuccess=963
```

审计日志 SHA-256：`C2ED946CBAB954490B069371E59D71A3876CE9C2655BBD9FC7E072BCD913C417`。

## 10. Changed-file regression gate

新增 `ThrownWeaponArcPlanner` mapping，要求：

- `Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc`；
- `Shanmen.0_0_10.CombatRuntime.ThrownWeapon`；
- `Shanmen.0_0_10.CombatCore`；
- 通用 `Shanmen.0_0_10.CombatRuntime` 规则。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=4 Logs=6
```

- gate SHA-256：`1BEB292C6DF8A39D5DD794562E4B411ACF197A3852CB576A24862AF5117C972B`；
- self-test：`309/309 PASS`；
- self-test SHA-256：`7B02180C8205D4A9FEBCFD93AA0C47F16A3CAF9C9DD712212971AC34426FA8CC`。

## 11. 静态边界与构建

生产 header/cpp 扫描禁止 demo_map、World/Actor、spawn/destroy、ProjectileMovement、trace/sweep/overlap、输入/UI、Tick/Timer、RNG 与 inventory mutation：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。`git diff --check` native 0。

最终构建：

- Game：5 actions / 36.96s / native 0，log SHA-256 `02E4C3B24D953D6AA5375C93CFF7B6CA0E3DD3F28FD1EFF691827311DB024AD0`；
- Editor：up to date / 1.15s / native 0，log SHA-256 `20D6B9E419EEC99C30C408A7CBAC3843EEAAF60FF3CF8CF05BB2D146B3D4F765`。

产物：

- `demo_map.exe`：357,046,784 bytes / `C26616E9E3CBE7DCE0802B244D513FC279ADA13C65FCB0CFF5092E5B39035D18`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：1,980,416 bytes / `75146588F78A6FDF51F8137A39B4B582C4081481198B9AA851A1E4F4ED93366C`。

## 12. P/F 边界与后续判断

全量 841 项从 canonical command 到 native terminal 用时约 34 分 50 秒。既有 Sword Rhythm checkpoint/envelope 慢段保持 responsive、CPU 与 Success 持续推进，最终未中断或缩小范围。

未运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P20.0 只完成中阶暗器弧线的 pure planning seam。下一 P-stage 可把 plan 组合进既有暗器产品链；物理滚轮、可视轨迹、场景碰撞和手感属于明确 F-stage。高阶自动辅助路径仍是独立能力，不由当前 `Master` 枚举值冒充完成。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-0-thrown-weapon-arc-planner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-0-thrown-weapon-arc-planner/Docs/Report/Dev.D.UE.0.0.10.P20.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-0-thrown-weapon-arc-planner/Docs/Log/Dev.D.UE.0.0.10.P20.0.r0_log.md>
