# Dev.D.UE.0.0.10.P22.3.r0 Development Log

## 1. 基线与目标

- base：`5dbb16bd73791598ce5edaca6013fd60e6f303fd`（P22.2 thrown-weapon terminal feedback）；
- branch：`agent/0.0.10-p22-3-thrown-weapon-flight-cue`；
- 目标：让现有标准投掷物在真实飞行阶段具有低成本、轨迹可区分的可见提示；
- 边界：不新增 Actor、轨迹、计时、碰撞、库存、动作、伤害或输入权威，不启动 UI/PIE/产品 executable。

## 2. 审计与方案

P22.1 已把真实飞刀 Carrier、可见长方体和精确碰撞闭合，P22.2 已把终态反馈投影到 HUD；但飞刀本体只有 `30 × 4.5 × 1.2 cm`，在高速飞行中缺少持续可读提示，也无法仅凭本体区分直线与抛物线。

现有 Projectile 已拥有规范的 `Empty -> Staged -> InFlight -> Spent` 生命周期和不可变 Launch Receipt。最小实现是给同一个 Actor 增加一个附着式 Point Light：由 State 控制显隐，由 Receipt 中的 Trajectory Kind 控制颜色。无需新 Actor、Tick、Timer、Spline、Trail 或第二份飞行状态。

## 3. 组件实现

`Ademo_mapShanmenThrownWeaponProjectile` 新增私有 `FlightCueLight`：

- `UPointLightComponent`，附着于既有 Collision 根；
- Intensity `1600`，Attenuation Radius `140 cm`，不投射阴影；
- Straight 为 `Linear(1.00, 0.48, 0.08)` 暖金；
- Ballistic Arc 为 `Linear(0.20, 0.72, 1.00)` 青蓝；
- 默认不可见，且外部没有组件写访问器。

只读接口 `IsFlightCueVisible()` 和 `GetFlightCueColor()` 用于验证引擎真实组件状态。`RefreshFlightCue()` 是唯一内部更新入口。

## 4. 生命周期接线

| Transition | Cue behavior |
|---|---|
| constructor -> Empty | straight 默认色，隐藏 |
| Empty -> Staged | 根据冻结 Receipt 预置色，隐藏 |
| Staged -> InFlight | 保持规范色，显示 |
| Staged -> Empty / cancel | 清理 Receipt，隐藏 |
| InFlight -> Spent / contact or miss | 保留审计 Receipt，隐藏 |

`IsStagedFor` 新增组件存在、父组件和隐藏状态校验；`IsInFlightFor` 新增显示状态校验。原网格、碰撞和移动激活顺序保持不变。

## 5. 测试扩展

扩展既有真实 World 测试，没有新建绕过生产路径的 fixture：

- Durable Launch Gate：Staged 隐藏、authority reject 隐藏、Straight publish 暖金且显示；
- Arc Receipt Motion：Staged 隐藏、Arc publish 青蓝且显示、terminal 隐藏；
- Contact to Vitality：有效接触提交生命值并 Spent 后，网格和光同时隐藏；
- Fail Closed and Miss：无法解析的接触保持 InFlight 和光可见；显式 miss 后熄灭。

颜色断言读取 `UPointLightComponent::GetLightColor()` 的实际值，避免只验证一份与渲染组件脱离的枚举映射。

## 6. 首次失败与修正

首次 Editor 构建通过 119 actions。首个 World Delivery 专项完成 `4 Success / 2 Fail`，原生退出码 `255`；Straight 与 Arc 的显隐均正确，但两条实际颜色断言失败：

| Evidence | Observable result | SHA-256 |
|---|---|---|
| `P22.3_focused_world_delivery_initial.log` | 4/2；Straight 与 Arc 组件颜色不等于预期线性值 | `4561DDE4B2E9814EA2CD48A9635F7D68066AEC01200C82E2CF332BED0C5ED3A4` |

初版把线性颜色传给 `SetLightColor(Color, false)`。UE 将其直接量化为 8-bit `FColor`，但 getter 从 `FColor` 返回时按 sRGB 恢复，造成数值空间不对称。修正为默认 sRGB 编码，并用 `0.01` 容差覆盖 8-bit 量化。修正没有改轨迹色值，也没有放宽 State、显隐或 durable gate 断言。

随后补强接触与 miss 终态断言，重新编译，并重新生成全部最终回归日志，未复用变更前日志。

## 7. 改动文件映射回归

3 个改动路径由既有 1 条规则推导 6 个精确组：

| Group | Success | SHA-256 |
|---|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` | 6 | `9A096A415EA87E94BBB5EADEA91F4922150E28B6A579ABFDC6AF0BC928C66B11` |
| `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 5 | `81B872A6AA3366D5FD106F0C7BFEFC28B9E93ED21FBE981EB01059587F2D011D` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 18 | `9B914720DA1BC9807D3E1BE8C023F2E8F194EBB2E8EC60B4484F488CB5CB96C9` |
| `Shanmen.0_0_10.Items` | 77 | `053021F391B369C4EAADABBD5E3BBFE628CCE70E7705EC902CFDF404EF08CA0D` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | `4E34DD787018892B6F5F615321C170179C373B503CC9D4055F705ECC05840AD7` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | `2C1EF9C72A3935D2EED3D6DAB2D910E9B905CD980FE83EE8564AADB2A0E7B155` |

合计 `262/0`；6 份日志均为单一命令、自然终止、零 Fatal/Unhandled/Ensure。

## 8. 覆盖门

既有 `ThrownWeaponWorldDelivery` 规则完整覆盖本轮 3 个源码/测试路径，不需要扩大或修改映射：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=6 Logs=6
```

门禁日志 SHA-256 为 `D62978B6677C116DFA95F3ED39493E5D8E96DF8E23F7A0882613C9B3C9859138`。门禁自测保持 `439/439`，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 9. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.3_game_build_final.log` | 118 actions / PASS / native 0 | 11,489 | `76A09A96F2BEF9B4B6B4537DC6C03754F06F1EC5EE9EBA681821D9FA37B95098` |
| `P22.3_editor_build_final.log` | 0 actions / PASS / native 0 | 974 | `24E98CC34A18F8251E23F1E329B2E2AED2D98CA7DD1D3250D77211A5A7DD0C99` |

`demo_map.exe` 为 359,557,120 bytes，SHA-256 `F2E04A117C9494BE91A4A4FB1982C806E7EE4BC192202E066A7BB7CFB59B5D78`。`UnrealEditor-demo_map.dll` 为 18,754,560 bytes，SHA-256 `96333DD9C62A7EF32A833945C19729CAA8059EB5F1929914DC97D64F3B57572B`。

实现/测试 diff 为 3 files / 84 insertions / 5 deletions。新增生产行 64；Timer、SetTimer、Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 3 个实现/测试文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.3` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-3-thrown-weapon-flight-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-3-thrown-weapon-flight-cue/Docs/Report/Dev.D.UE.0.0.10.P22.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-3-thrown-weapon-flight-cue/Docs/Log/Dev.D.UE.0.0.10.P22.3.r0_log.md>
