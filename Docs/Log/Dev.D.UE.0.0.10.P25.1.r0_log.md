# Dev.D.UE.0.0.10.P25.1.r0 Development Log

## 1. 目标

- 把共享 Run `SpiritEnergy` 接成可由玩家启动的短时灵力护盾；
- 复用既有 Spirit Shield Session，不新增第二资源余额、第二时间线或平行防御系统；
- 激活必须原子扣费，活动期重复输入不得二次扣费；
- 固定 30 容量、20 灵力成本、90 tick / 3 秒期限；
- 接入统一可重映射物理输入并完成 N-1 输入配置迁移；
- 在本轮只完成启动与生命周期，不夹带敌方 Impact、HUD 或产品运行验收；
- 按改动文件映射执行回归、构建并交付 Report/Log。

## 2. 基线与范围

- 基线：`1ea933ef3ab237e05e03df2c65edacf40036d6ca`（P25.0）；
- 分支：`agent/0.0.10-p25-1-spirit-shield-activation`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件保持原样；
- 本轮不修改敌方伤害公式、存档、物品权威、装备、关卡或 UI 内容。

## 3. 产品实现

新增：

- `demo_mapShanmenSpiritShieldProductSession.h/.cpp`；
- `demo_mapShanmenSpiritShieldProductSessionTests.cpp`；
- `demo_mapShanmenSpiritShieldPhysicalInputTests.cpp`。

产品 Session 捕获规范 Action、Definition、Cost 与 Schedule，并在 Divine Sense Product Controller 的共享灵力权威副本中执行底层 Shield `Begin + Commit`。资源 Receipt、Controller 与 Shield Session 全部验证通过后才原子发布。

Action、Transaction、Command 均由 Run/Activation 身份确定性派生。活动期重复激活在动作仲裁、Run 序号和资源支付前拒绝；余额不足时 Controller 与 Session 都不发布。

## 4. Run 与 GameMode 接线

Combat Run Coordinator 新增护盾激活序号和 `TryReservePlayerSpiritShieldAction()`。Player Action Arbitration 接受 `SpiritShield` 瞬时动作，但不把持续护盾错误建模成互斥动作占用。

GameMode 新增唯一 `SpiritShieldProductSession`：

- `RouteSpiritShieldInput()` 捕获固定 Run 时间并激活；
- Tick 只在 Due 时调用 `ObserveTimeline()`；
- `ReleaseCombatProductRun()` 先释放护盾，再关闭共享 Divine Sense 灵力 Host；
- 日志记录 Status、Error、ActivationId、容量、Deadline 与剩余灵力。

## 5. 输入与迁移

统一注册表新增 `SpiritShield`。首次默认键为 `Left Shift`，但既有 `ValidateKey()` 明确保留所有修饰键，完整回归因此暴露 12 个旧迁移/重映射失败。

修复选择 `H`，注册数 32 -> 33，序列化版本 10 -> 11。新增版本 10 迁移测试分别证明：

- `H` 空闲时护盾取得 `H`；
- 既有用户覆盖占用 `H` 时保持覆盖不动，护盾取得空闲的 `G`；
- 合并后全部 33 项合法且无重复。

PlayerController 将 `H / IE_Pressed` 绑定到 `UseSpiritShield()`，自动化计数器证明一次物理 Press 只路由一次。

## 6. 首错与修复历史

### Editor 编译首错

`P25.1_EditorBuild_first.log`：两处 `UE_LOG` 使用条件表达式选择 Verbosity，宏展开要求编译期常量，UBT 返回 `OtherCompilationError / 6`。改为固定 `Log` 后重新构建成功；首错日志未覆盖。

### 广覆盖首错

`P25.1_Shanmen_0_0_10_initial.log`：1286 项被发现，运行至 743 项时共有 12 个失败，全部集中在旧物理输入迁移/实时重映射：Controlled Weapon 5、Divine Sense 2、Spirit Evasion 3、Sword Qi 2。根因为新增默认 `Left Shift` 无法通过既有保留键校验。

该过时二进制队列随后停止；修复为 `H`、配置版本 11 并补迁移断言后，五个直接受影响组共 26 项全部通过，再从头运行正式全量。

### 旧 FullSystemLoop 夹具首错

正式全量通过后，改动文件门禁要求的 `demo_map.FullSystemLoop` 精确复测出现 47 Success / 3 Fail。三个失败都来自旧夹具把 `H` 当成空闲重映射键；P25.1 已将 `H` 正式分配给 `SpiritShield`，所以注册表正确拒绝重复键，而旧期望已经失效。

修复仅将夹具中的任意空闲重映射键换为 `K`，没有修改生产代码、默认绑定、唯一性校验或断言强度。`P25.1_demo_map_FullSystemLoop_initial.log` 原样保留，重新编译后精确组 50/50 通过。

## 7. 自动化与覆盖

正式结果：

| Evidence | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P25.1_Shanmen_0_0_10.log` | `Shanmen.0_0_10` | 1287 | 0 | `6FE9BFDCBEE56A1D540CABC4D3D140D658E23208AC4F6AFEBABA96278510ACEE` |
| `P25.1_demo_map_FullSystemLoop.log` | `demo_map.FullSystemLoop` | 50 | 0 | `D4106E6AB58378B8325A3326BA63B769EF2C5A42C8BCA8CA070335319E2EAC0A` |
| `P25.1_demo_map_P7Integration.log` | `demo_map.P7Integration` | 9 | 0 | `5C7B50C8A866C1D6B79090AFC9123E01DAFBE87E9A59031FCA5FAE823E72AC2E` |
| `P25.1_demo_map_P5RuntimeInterface.log` | `demo_map.P5RuntimeInterface` | 9 | 0 | `1183ECB408148E9AA515A762E9F026324BB66D6D68E9E432D611A8C15FC5A98F` |
| `P25.1_demo_map_InputRestore.log` | `demo_map.InputRestore` | 101 | 0 | `4113C751996DDA02BF1364E1BC1D8DF41CAE0EA57B401654937F202A32A9070E` |
| `P25.1_demo_map_V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | `C7D8B1BD3636FE64028A96896FFB9D6A80AC7DDCD77724606E086698F04579BB` |
| `P25.1_demo_map_V3_Attributes.log` | `demo_map.V3.Attributes` | 4 | 0 | `3785419DA7E1279B1B02C5853A9C5AE3080C327697288ECD14FEE3410E9F6CCF` |
| `P25.1_demo_map_EnemySkillFramework.log` | `demo_map.EnemySkillFramework` | 44 | 0 | `58619AD92324ABCD38DC1B382684BBCDF635E41CDAD29CC0641CCF4D72A2DA3A` |
| `P25.1_demo_map_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `94A4722026941856ADBA4144360FC9814772EF1E929AB565C8B08A542C6DFA45` |

覆盖门禁：`PASS Changed=31 Rules=11 Required=95 Logs=9`，SHA-256 `AD35DF646F168F4AAB12D74DFAE246C77681EFDD13CD364B911D6D0331BDE569`。

覆盖器自检：446/446 PASS，SHA-256 `2064B121A357201879B2A267DA01042BF0787CA797EB95F2C0F10AB42BAED09F`。

UE 5.8 Home Panel 在无界面测试中仍会访问 `https://www.google.com/generate_204`。正式测试命令使用 `-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0`；5 项探针测试证明队列正常退出且网络请求为 0。没有修改引擎或 Windows。

## 8. 构建与静态检查

| Target | Result | Native exit | SHA-256 |
|---|---|---:|---|
| Editor first | `OtherCompilationError` | 6 | `2BF0753D43A083DC64A7C65426F9FE4A78016A9DA82A7C522585F6D1085FC5AF` |
| Editor final | `Succeeded` | 0 | `722BC6DC574197EF88DA9A74C001815CAD6AFF17187CFF9827C407C40BCBABA8` |
| Game final | `Succeeded` | 0 | `21E629AF785EDC919377DBBB5BE2C971A0143538948B198BB11C4F00A52E7179` |

- `git diff --check`：PASS；
- 新生产文件对 `UWorld`、`AActor`、Timer、RNG、`ApplyDamage`、存档与物品 Subsystem：0 命中；
- 回归结束后项目相关进程：0；
- 最终 `UnrealEditor-demo_map.dll`：19072000 bytes，SHA-256 `B3E613DFA26CB078A81065D8CBE7A30BB439EBB688A017C1138C2F548D8F886B`；
- 最终 `demo_map.exe`：359785472 bytes，SHA-256 `39EC7F0FA74F6F7A7CC3FAA9AC1B9DEE4085B8AE04EB4B15D34434EAE93A99E6`。

旧 Sword Rhythm 与 Thrown Weapon 深层检查点测试存在递归自校验成本，个别测试达到分钟级；这是既有包装链性能债。本轮未用放宽断言、跳过组或修改系统权限规避，完整原生结果仍作为最终证据。

## 9. P/F 边界

P 阶段已证明：H 输入、动作授权、确定性身份、共享灵力支付、原子失败、活动期幂等、期限关闭、再激活和 Run 释放。

F 阶段未执行：没有把护盾容量接入敌方 Impact，没有 HUD 呈现，也没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P25.2 应通过 ProductSession 的类型化受击提交 API 将活动 Shield 投影到所有敌方 Impact 路径；禁止调用方取得可写底层 Session 并直接改容量。

## 10. 精确提交与 GitHub

只暂存本阶段明确修改的 25 个 tracked 文件、4 个新增源码/测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件不暂存；`Saved/Codex/P25.1` 原始日志留在本地忽略目录。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-1-spirit-shield-activation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-1-spirit-shield-activation/Docs/Report/Dev.D.UE.0.0.10.P25.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-1-spirit-shield-activation/Docs/Log/Dev.D.UE.0.0.10.P25.1.r0_log.md>
