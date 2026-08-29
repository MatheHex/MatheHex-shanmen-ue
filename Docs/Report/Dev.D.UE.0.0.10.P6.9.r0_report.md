# Dev.D.UE.0.0.10.P6.9.r0 Report

## 1. 结论

P6.9 已完成受控武器 Orbit 的真实帧 owner 接线，结论为 **PASS**。

`Ademo_mapGameMode::Tick` 现在会在既有 extraction Tick 逻辑之前，把引擎提供的本帧 `DeltaSeconds` 交给唯一 `ControlledWeaponRunHost`。Host 以结构化、自校验的 frame result 区分空闲、成功、非法 delta、Host 损坏和位姿拒绝；成功仍复用 P6.8 的稳定 item 顺序、显式最大步长与无伤害位姿公式。

本轮没有加入固定更新频率、默认数值、累计器、substep、catch-up、输入、近身候选、伤害或防御。每把武器的 `MaximumStepSeconds` 仍是唯一显式步长边界，超限帧失败关闭而不猜测补帧策略。

## 2. 功能性

- 新增 `NumOrbiting()`，只统计有效 Host 中 active 且逻辑状态为 `Orbiting` 的 exact items；
- 新增 `Fdemo_mapShanmenControlledWeaponOrbitFrameResult`，包含状态、exact `RunId`、本帧 delta、Host 绑定数、Orbiting 数与有序 batch；
- frame result 对五个状态分别自校验：`NoOrbitingItems`、`Advanced`、`DeltaInvalid`、`HostInvalid`、`MovementRejected`；
- 空 Host 或有效但没有 Orbiting item 都是无副作用 no-op，但通过 `RunId + BoundCount` 保留两者的审计差异；
- 有效推进必须具有 exact Run identity、`BoundCount >= OrbitingCount > 0`，且 batch 数量、顺序与全部收据一致；
- 超出任一 item 的显式最大步长时返回 `MovementRejected`，保留 Run / bound / orbiting 证据，且 P6.8 的全批预检保证 Actor 与相位均不变；
- 非空但内部 Actor / component / controller 契约损坏时返回 `HostInvalid`，绝不误报为空闲；
- GameMode `Tick` 在 `Super::Tick` 后、M01 extraction 的既有早退前泵送 Orbit，因此未激活 extraction 的地图也能推进已附着飞剑；
- `HostInvalid` 或结构失真写 Error；非法 delta 或移动拒绝写 Warning；日志携带 exact RunId 与 bound/orbiting/attempted/advanced 计数；成功和 no-op 不逐帧刷日志；
- 既有手工 `AdvanceControlledWeaponOrbit` 保留兼容；新增 `AdvanceControlledWeaponOrbitFrame` 暴露同一结构化 owner 结果。

## 3. 完整性

新增 1 个产品级自动化：

`ControlledWeaponRunHost.FrameOwnerResult` 覆盖：

1. 非法 delta 与空 Host no-op 的区分；
2. 两把逆序附着 Orbit item 的 exact Run、绑定数、稳定 batch 顺序与成功状态；
3. 超步长 frame 的结构化拒绝及零 transform mutation；
4. 两把 item 经 canonical Launch 后不再参与 Orbit cadence；
5. 有效但无 Orbit item 的 no-op 保留 Run identity；
6. 破坏非空 Host 的 Actor root 契约后返回 `HostInvalid` 而非 idle。

全量 `Shanmen.0_0_10` 从 P6.8 的 `155` 个增加至 `156` 个，最终 `156/156` Success。

## 4. 兼容性

- 未建立第二个 Tick、Run、Host、movement、command、impact、inventory 或 vitality authority；
- GameMode 仍是唯一真实 Run owner，帧泵只调用既有 P6.8 Host；
- 原 M01 extraction Tick 的条件、Authority advance、完成与 terminal 语义未改，只在其早退前加入独立 Orbit 泵；
- Directed movement、sweep、接触窗口、WorldDelivery、Impact、Launch / Redirect / Recall 路径未改；
- 新增路径没有 Spawn、伤害、物品事务、RNG、输入绑定、`FHitResult`、sweep、resolve 或 delivery；
- 未修改 Profile schema、存档、item definition、GameplayTags、input mapping 或资产；
- 未冻结正式帧频、catch-up 上限、substep 数、飞剑数量、轨道数值或 presentation。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`；
- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 5 | 0 | 0 | `8C457CAABADC3576A70802939E27B35BC2034FEEB62609F7ACDEAA0FFF1BC815` |
| `Shanmen.0_0_10` | 156 | 0 | 0 | `645424E8F05812CA30823D60A3436D0D8AB11A2E2EBD1B9F5F0E69513CC61087` |

- 两份最终日志均有唯一实际 `Cmd: Automation RunTests`、queue-empty、Fail `0`、Fatal / unhandled / ensure `0` 与原生退出 `0`；
- changed-file gate：`PASS Changed=5 Rules=2 Required=12 Logs=2`；完整套件覆盖所有 12 个路径映射组，RunHost 另有直接 focused 证据；
- 加入 Report / Log 后最终 staged gate：`PASS Changed=7 Rules=2 Required=12 Logs=2`；
- regression coverage self-test：`14/14 PASS`；
- 新增生产差异的 spawn / damage / inventory transaction / RNG / input binding / sweep / hit / resolve / delivery 扫描命中 `0`；
- 最终源文件时间早于已测试 Editor DLL 与 Game executable；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `28/28` actions、`Result: Succeeded`、native exit `0`、`126.30s`；首次 Game build 为 `27/27`、native exit `0`、`105.47s`。

首次 focused / full 自动化分别为 `5/5` 与 `156/156` Success，SHA-256 分别为 `4BF611CA7EF865605DFF8F02D77D6000EE166D149168164453AB9E260712845E`、`3AD291EC62053D2FFF6889A8F24CC3FC99B988E774C354593441D3A8C793BE73`。

契约复审发现拒绝/no-op 结果只带计数、缺少 exact Run identity。实现补入 `RunId + BoundCount` 并收紧状态自校验；GameMode 故障日志同步携带 RunId。复审后 Editor `28/28`、Game `27/27`，focused `5/5`、full `156/156` 再次成功。没有源码、自动化或编译失败。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次：`28/28`，`Result: Succeeded`，native exit `0`，`126.30s`；
- 最终：`28/28`，`Result: Succeeded`，native exit `0`，`95.54s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次：`27/27`，`Result: Succeeded`，native exit `0`，`105.47s`；
- 最终：`27/27`，`Result: Succeeded`，native exit `0`，`94.42s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner/Docs/Report/Dev.D.UE.0.0.10.P6.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-9-controlled-weapon-orbit-frame-owner/Docs/Log/Dev.D.UE.0.0.10.P6.9.r0_log.md>
