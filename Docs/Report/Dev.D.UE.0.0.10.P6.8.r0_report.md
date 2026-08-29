# Dev.D.UE.0.0.10.P6.8.r0 Report

## 1. 结论

P6.8 已完成受控武器的显式、确定性 Orbit 物理位姿层，结论为 **PASS**。

每把 exact item 在附着到 Run Host 时冻结自己的世界空间环绕中心偏移、平面法线、参考轴、半径、角速度、初相位和最大采样步长。逻辑状态为 `Orbiting` 时，Controller 以纯确定性公式计算下一位姿并生成可审计收据；Host 按稳定 item 顺序推进多把武器，并在任一武器的步长超限时于移动前拒绝整批。

本轮只实现规划中“飞剑环绕”作为攻击、近身威胁、防御与发射的共同准备位姿。没有自行加入近身伤害、防御判定、扫掠接触、编队、输入键位、选剑策略、默认数值或正式内容资产。

## 2. 功能性

- `Fdemo_mapShanmenControlledWeaponMotionCapture` 新增完整 Orbit 参数；所有参数由外层显式注入，不存在产品默认值；
- 捕获边界拒绝非有限中心、非单位或非正交平面基、非正半径、零角速度和非法最大步长；初相位在启动时规范到 `[0, 2π)`；
- Orbit 中心使用“当前 Source Actor 世界位置 + 冻结中心偏移”，因此 Source 移动后下一采样自动跟随新锚点；
- 下一相位使用冻结角速度与显式 `DeltaSeconds` 推导，并规范到 `[0, 2π)`；负角速度同样受契约支持；
- `TryAdvanceOrbiting` 只接受 active `Orbiting` 状态，先验证候选控制器相位，再以非 sweep、`TeleportPhysics` 的 `SetActorLocation` 放置 Actor；
- `Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt` 记录 exact activation / item identity、中心、平面、半径、角速度、起止相位、请求与实际位置及是否移动，并重算公式校验自身；
- Launch 后状态进入 `Directed`，Orbit 入口立即拒绝；既有 Directed sweep、接触窗口、WorldDelivery 与 Impact 路径不变；
- `TryAdvanceOrbitingInOrder` 只收集 active Orbiting items，沿既有 GUID 稳定顺序推进；超大步长先对整批预检，避免部分相位或变换提交；
- `Ademo_mapGameMode::AdvanceControlledWeaponOrbit` 只是唯一 Run owner 的薄委托；本轮没有绑定 Tick、定时器或输入 cadence。

## 3. 完整性

新增 2 个产品级自动化：

1. `ControlledWeaponController.OrbitMotion`：验证单剑确定性相位与位姿、Source 锚点移动、超步长不变性、Launch 后 Orbit 拒绝；
2. `ControlledWeaponRunHost.OrbitOrderAndPreflight`：验证逆序附着后的稳定 GUID 顺序、不同初相位、多剑超步长全批预检，以及一把 Launch 后仅推进其余 Orbiting item。

`AtomicFences` 另补非正交 Orbit 平面拒绝；P6.6 Lifecycle 与 P6.7 CommandRouter 的运动夹具补齐显式 Orbit capture，证明新契约没有绕过既有 Run 生命周期和命令路由。

全量 `Shanmen.0_0_10` 从 P6.7 的 `153` 个增加至 `155` 个，最终 `155/155` Success。

## 4. 兼容性

- 未建立第二个 Run、entity、inventory、deployed-item、command、impact 或 movement authority；
- 未改变 P6.1–P6.3 Adapter / Session / WorldDelivery、P6.6 Lifecycle、P6.7 CommandRouter、CombatCore、CombatRuntime、Items 或 WorldGameplay 的生产语义；
- Orbit 路径没有 sweep、`FHitResult`、接触窗口、WorldDelivery、Impact、`ApplyDamage` 或 `TakeDamage`；
- 新增生产差异没有 Spawn、物品 Reserve / Commit / Consume、RNG、输入绑定或 input mapping；
- 未修改 Profile schema、存档格式、item definition、GameplayTags 或资产；
- 未把 legacy projectile 解释为飞剑，也未冻结飞剑数量、编队策略、正式半径/速度、视觉、音效或碰撞规则；
- 世界空间平面与中心偏移是本轮显式 capture 契约；是否改为角色局部空间属于未来 presentation / policy 决策。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponProductController.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponProductControllerTests.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycleTests.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunCommandRouterTests.cpp`；
- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponController` | 4 | 0 | 0 | `E37BF0B063EE87E81638C0E2D1DA2A91A20D57EDCF1AC7C18B8656040459117A` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 4 | 0 | 0 | `2E11F3333ACA7CDA9A435DF2ECF552C5EDCB1C3F32C0EC05A735292ECC897317` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `44F953AD8B0D176F4E8D3D59C94BAF3C04B61AC0BD4E5023D41C9DCC0C8ABE50` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter` | 4 | 0 | 0 | `A35CD32FDFEE779FE8716E76000E8932D5999CD78D76BEC749C2D641BEDF67E0` |
| `Shanmen.0_0_10` | 155 | 0 | 0 | `624CF984D0F242D899DEEDD365C62C7012C553FDF41E70E21C362F009C291E16` |

- 五份日志均有唯一实际 `Cmd: Automation RunTests`、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- changed-file gate：`PASS Changed=11 Rules=5 Required=12 Logs=5`；完整套件覆盖 12 个文件映射必跑组，另保留 4 个直接受影响组的 focused 证据；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=13 Rules=5 Required=12 Logs=5`；
- regression coverage self-test：`14/14 PASS`；
- regression JSON parse：PASS；
- 新增生产差异的 spawn / damage / inventory transaction / RNG / input binding 扫描命中 `0`；Orbit 函数的 sweep / hit / resolve / delivery / damage 扫描命中 `0`；
- 最终源文件时间早于已测试 Editor DLL 与 Game executable；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `30/30` actions、`Result: Succeeded`、native exit `0`、`131.04s`。

新增 Controller 与 RunHost focused 自动化均首轮成功，分别为 `4/4` 与 `4/4`。随后完整 `Shanmen.0_0_10` 首轮即为 `155/155` Success；Lifecycle `3/3` 与 CommandRouter `4/4` 的下游夹具复核同样首轮成功。

构建前的契约复审将 Orbit 状态提交收紧为候选控制器先验证、Actor 放置成功且收据自校验后再提交相位。没有源码、测试或编译失败，也没有发生重试性修复。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `30/30` actions；
- `Result: Succeeded`；
- native exit `0`；
- `131.04s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `29/29` actions；
- `Result: Succeeded`；
- native exit `0`；
- `120.88s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-8-controlled-weapon-orbit-motion>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-8-controlled-weapon-orbit-motion/Docs/Report/Dev.D.UE.0.0.10.P6.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-8-controlled-weapon-orbit-motion/Docs/Log/Dev.D.UE.0.0.10.P6.8.r0_log.md>
