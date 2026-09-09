# Dev.D.UE.0.0.10.P22.12.r0 Report

## 1. 结论

P22.12 在 P 阶段边界内完成，结论为 **PASS**。

本轮把飞刀的释放中心从固定角色代理点提升为“实时右手骨骼优先、安全代理点回退”。默认 Manny/Quinn 角色存在可信 `hand_r` 姿态时，Straight 实投与 Ballistic Arc 预览现在共同从右手前方 18 cm 取点；骨骼不可用、未注册、越界、非有限或离角色超过 200 cm 时，继续使用既有 `55/28/50 cm` 代理点，不阻断玩家输入。

```text
Initial Editor Development build:       FAIL (保留：内存环境 + C2248)
Bounded source repair + Editor retry:    PASS (10 actions / native 0)
Focused hand-origin and Arc basis:       17 Success / 0 Fail
Changed-file mapped regression:        1301 Success / 0 Fail (2 healthy logs)
Changed-file regression coverage:       PASS (6 files / 2 rules / 17 groups)
Regression gate self-test:               PASS 439/439
Game + Editor Development:               PASS (both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明骨骼来源、代理回退、Straight 命令取点和 Arc 预览共用规则，不宣称动画手感、刀模 Socket、网络复制或全部角色资产已完成人工体验验收。

## 2. 玩家侧变化

此前飞刀从角色中心偏移出的固定点释放，人物姿态变化时刀与右手可能视觉脱节。现在正式链路为：

```text
Character + 已注册 SkeletalMesh + 有效 hand_r
  -> 实时右手世界位置 + 角色前方 18 cm
  -> Straight 实投 / Ballistic Arc 预览共用

任何骨骼证据不可信
  -> 既有 Actor 代理点（前 55 / 右 28 / 高 50 cm）
  -> 输入继续可用，不生成第二套发射流程
```

18 cm 只用于让飞刀中心离开掌心；飞行方向、射程、弧高、碰撞、库存提交、动作仲裁、伤害和 HUD 反馈均保持原权威。

## 3. 骨骼优先契约

`ResolveLaunchOrigin()` 仅在下列证据全部成立时接受 `hand_r`：

- Source 是有效且未销毁的 `ACharacter`；
- Character Mesh 有 Skeletal Mesh 资产并已注册；
- `hand_r` 存在，且骨骼索引落在公开的 component-space transform 数组内；
- Actor 位置/前向量、手部位置和最终释放点全部为有限数；
- 手部位置距 Actor 不超过 200 cm。

任一条件失败都会返回既有 `MakeLaunchOrigin()` 代理点并将 `bUsedSkeletalHandOrigin` 保持为 false。无资产硬写、无 Socket 强依赖、无缓存姿态、无 Tick 或定时轮询。

## 4. Straight 与 Arc 一致性

Straight 热栏路由在构造 Run Command 前调用同一解析入口，并把来源记录到不可变结果 `bUsedSkeletalHandOrigin`。Arc Source Basis Adapter 同样调用该入口，再将得到的 Origin 与当前 Actor Forward/Right 捕获为预览基底。

因此本轮没有“预览仍从角色中心、实投却从手上飞出”的双规则。Arc 只读采样仍不扣物品、不启动飞行、不提交动作；Straight 仍由原 Product Session/World Delivery 完成事务。

## 5. 真实资产证明

新增自动化使用项目正式资产：

`/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple`

测试把该资产挂到真实 `ACharacter::Mesh`，刷新姿态并证明：

- `hand_r` 骨骼存在且 component-space transform 可读；
- 解析点等于实时手部世界位置加 18 cm 前向净空；
- 结果不同于旧角色中心代理点；
- 实际 Straight Run Command 捕获完全相同的 Origin；
- 普通 Actor/无可信骨骼时仍使用旧代理点。

## 6. 首次失败与有界修复

首次 Editor 构建失败证据完整保留。它同时暴露两个独立问题：

1. 16 路 UBA 启动时系统提交内存为 `74.76/89.74 GB`，多份编译器进程返回 `C3859`、系统码 `1455`（页面文件不足）；
2. UE 5.8 将派生组件上的 `AreBoneTransformsValid()` 覆盖声明为 protected，生产代码与测试各产生一处 `C2248`。

源码修复改为公开且可验证的 Mesh 注册状态、骨骼索引和 `GetNumComponentSpaceTransforms()` 边界；构建并行度降为 2。没有修改 Windows 分页文件或系统权限。重试 Editor 构建 `10 actions / native 0 / 21.13s`。

## 7. 自动化证明

两组聚焦测试共 `17/0`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `ThrownWeaponInputAdapter` | 11 | 276,593 | `3D4DC3F719F1C50C88266A6EFB9E760D40A0790A5EFEA75B53FB7A944E7A959F` |
| `ThrownWeaponArcSourceBasisAdapter` | 6 | 268,449 | `6E11DBB25A4B0700DDA8163C827B47318C275FFAEB98E542F77F0794EE95A305` |

聚焦覆盖真实 Manny 右手、Straight 命令同源、代理点回退、Arc 基底确定性、失效 Source 与只读采样。两份日志均有唯一 RunTests 命令、逐项 Success、终止标记、0 Fail，且无 fatal/unhandled/ensure。

## 8. 改动文件回归与构建

最终两份映射日志合计 `1,301 Success / 0 Fail`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10` | 1,255 | 1,871,113 | `F00C0B21B28530FCD438498271F65D76685EE8A6E27D388CB838D755C1DDAD21` |
| `demo_map.ItemUseAndArmor` | 46 | 305,042 | `45B2425F5033135736D505566EC5E3BDF65220944F3EB20186FDEB628D9083AF` |

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.12_regression_coverage.log` | PASS 6/2/17/2 | 2,368 | `C7FA8B306C8F61FF123625ED9DAEC05EFDB912EB42A0525EEA44FC3B7F24A7B4` |
| `P22.12_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.12_GameBuild_final.log` | PASS / 51 actions / native 0 / 101.03s | 5,419 | `706080F4204E94BCA643F1FE5988F7755C9CC5DC30E6319F25454AD86776D443` |
| `P22.12_EditorBuild_final.log` | PASS / up-to-date / native 0 / 0.97s | 965 | `D182D940DF606960456862D02FB5C039B064EB3F05CAFDF317DAA384294C8326` |

最终 `demo_map.exe` 为 359,574,016 bytes，SHA-256 `FB801A8E11E1865D6DEEB93D6E371132826206628397AB4CD6CB006B472557E0`；`UnrealEditor-demo_map.dll` 为 18,814,976 bytes，SHA-256 `A47D75D5FD8386856A97891919B27589BBD7F5CE71B46EDCBB923372EC71AC9C`。

## 9. P/F 与静态边界

PASS：真实 Manny `hand_r` 被使用；Straight 与 Arc 共用解析入口；无可信骨骼时自动回退；1,301 项映射回归零失败；覆盖门、自测和双构建通过。

未声明：刀模已绑定专用 Socket、动画蒙太奇已做手部附着、非 Character Pawn 也具有骨骼来源、网络端姿态同步或人工观感验收已经完成。

非文档改动为 4 个生产文件 `+87/-7`、2 个测试文件 `+74/-1`。新增生产代码中的 Timer/SetTimer/自定义 Tick/Sleep/Random/Rand/RNG/UPROPERTY/UFUNCTION/SpawnActor/ApplyDamage/SaveGame/OpenLevel 均为 0；无新增资产、模块、Actor、Subsystem、持久格式或第二套权威；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段只提交 4 个生产文件、2 个测试文件、本 Report 与本 Development Log，共 8 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.12` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-12-thrown-weapon-hand-release>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-12-thrown-weapon-hand-release/Docs/Report/Dev.D.UE.0.0.10.P22.12.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-12-thrown-weapon-hand-release/Docs/Log/Dev.D.UE.0.0.10.P22.12.r0_log.md>
