# Dev.D.UE.0.0.10.P22.12.r0 Development Log

## 1. 基线与目标

- base：`921ae0cf560a52c278070bd25c503cbe7afc08bb`（P22.11 release-path feedback）；
- branch：`agent/0.0.10-p22-12-thrown-weapon-hand-release`；
- 目标：让正式角色的 Straight 飞刀实投与 Ballistic Arc 预览共用实时右手释放中心；
- 边界：保留旧代理点作安全回退，不改库存、动作、轨迹、伤害、碰撞、HUD 或持久协议，不启动 UI/PIE/产品 executable。

## 2. 审计结论

P22.0–P22.11 已把飞刀推进到可见飞行、真实碰撞、贴墙拒绝和 HUD 原因提示，但 `InputAdapter` 仍只接收 Actor Transform，并以 `Forward*55 + Right*28 + Up*50` 计算释放点。正式 `BP_TopDownCharacter` 使用 Manny/Quinn 骨骼资产且包含 `hand_r`，但既有链路从未读取它。

Arc 预览通过 `ThrownWeaponArcSourceBasisAdapter` 获取同一个代理点。因此最小修复不是增加 Socket 资产或第二个轨迹系统，而是为既有解析入口增加可信骨骼来源，并让 Straight 与 Arc 共同调用。

## 3. 实现

`Fdemo_mapShanmenThrownWeaponInputAdapter` 保留 `MakeLaunchOrigin(FTransform)` 为规范代理回退，并新增 `ResolveLaunchOrigin(AActor*)`：

- 只接受有效 `ACharacter::GetMesh()`；
- 要求 Mesh 有资产、已注册、`hand_r` 索引可落入 component-space transform 数组；
- 检查 Source、Forward、Hand 和最终 Origin 均为有限值；
- 手距 Actor 超过 200 cm 即拒绝该姿态；
- 成功时返回 `HandLocation + ActorForward*18 cm`，否则返回旧代理点。

返回结果增加 `bUsedSkeletalHandOrigin` 作为只读证据，不驱动任何第二状态机。

## 4. Arc 同源

`ThrownWeaponArcSourceBasisAdapter` 改用同一 `ResolveLaunchOrigin()`。采样结果记录骨骼/代理来源，并保持既有 Basis、Forward、Right、一次采样与失效栅栏契约。

Straight 的 `RouteTypedHotbarInput()` 在构造 Run Command 前使用相同解析入口。因此预览和实际命令不会分别维护发射点公式。

## 5. 测试增量

输入测试 Fixture 的正式 Source 改为 `ACharacter`，默认无 Skeletal Mesh 资产时继续证明代理回退。新增 `SkeletalHandReleaseOrigin`：

- 加载项目正式 `SKM_Manny_Simple`；
- 刷新真实组件姿态并验证 `hand_r`；
- 比较实时手部 + 18 cm 与解析结果；
- 证明该结果不同于代理点；
- 路由 Straight 热栏输入并核对捕获命令 Origin 与来源标记。

Arc Source Basis 既有普通 Actor 测试补充代理来源断言。

## 6. 首次失败与修复

首次 Editor 构建 `52 actions`，在约 16 路并行、系统提交内存 `74.76/89.74 GB` 时失败：

- 多个编译进程报告 `C3859` / Windows `1455` 页面文件不足；
- 生产代码与测试各有一处 `C2248`，因为 UE 5.8 的 `USkeletalMeshComponent::AreBoneTransformsValid()` 覆盖接口为 protected；
- 原生日志最终为 `OtherCompilationError / 63.09s`，SHA-256 `7E648C9E07C455F52219DA104B9F426C0A2370C13045FEA3D6C3ECBAB3B12F73`。

有界修复：移除 protected 调用，改由公开的注册状态与 component-space transform 数组边界验证；后续构建固定 `-MaxParallelActions=2`。未修改 Windows 系统设置。Editor 重试 `10 actions / native 0 / 21.13s`，日志 SHA-256 `B94EC7F7E09FA367863CCE383BB36ED8B67CD6687058DDBFAFD561958E325BF1`。

## 7. 聚焦验证

两组聚焦测试均一次通过：

- `ThrownWeaponInputAdapter`：11/0，包含真实 Manny 右手与 Straight 命令同源；
- `ThrownWeaponArcSourceBasisAdapter`：6/0，包含代理回退与只读 Basis；
- 合计：`17 Success / 0 Fail`。

日志 SHA-256 分别为 `3D4DC3...A959F` 与 `6E11DB...A305`，均有原生完成标记且无 fatal/unhandled/ensure。

## 8. 映射回归与构建

6 个改动路径命中 2 条规则并产生 17 个必跑组。两份健康日志覆盖全部要求：

- `Shanmen.0_0_10`：1,255/0；
- `demo_map.ItemUseAndArmor`：46/0；
- 合计：`1,301/0`。

覆盖门：`PASS Changed=6 Rules=2 Required=17 Logs=2`；门禁自测：`PASS 439/439`。

最终构建：

- Game：51 actions / native 0 / 101.03s；
- Editor：up-to-date / native 0 / 0.97s；
- `demo_map.exe`：359,574,016 bytes / SHA-256 `FB801A8E11E1865D6DEEB93D6E371132826206628397AB4CD6CB006B472557E0`；
- `UnrealEditor-demo_map.dll`：18,814,976 bytes / SHA-256 `A47D75D5FD8386856A97891919B27589BBD7F5CE71B46EDCBB923372EC71AC9C`。

## 9. 静态边界

非文档改动：生产 `+87/-7`，测试 `+74/-1`。新增生产代码无 Timer、SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、UPROPERTY、UFUNCTION、SpawnActor、ApplyDamage、SaveGame 或 OpenLevel。

`git diff --check` 为 0；无新增资产、模块依赖、Actor、Subsystem、持久数据或第二套玩法权威。首次失败证据、最终测试与构建日志保存在 `Saved/Codex/P22.12`，不进入 Git。

## 10. 提交边界与 GitHub

精确提交 4 个生产文件、2 个测试文件、本 Report 与本 Development Log，共 8 个文件。103 个用户原有 untracked 文件不暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-12-thrown-weapon-hand-release>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-12-thrown-weapon-hand-release/Docs/Report/Dev.D.UE.0.0.10.P22.12.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-12-thrown-weapon-hand-release/Docs/Log/Dev.D.UE.0.0.10.P22.12.r0_log.md>
