# Dev.D.UE.0.0.10.P21.7.r0 Development Log

## 1. 基线与目标

- base：`5d803868e663ea454474ab444869d35c4c386acd`（P21.6 flying-sword threat layout policy）；
- branch：`agent/0.0.10-p21-7-flying-sword-input-command`；
- 目标：把 P6 canonical `TrainingFlyingSword` 的 Launch/Recall 接到一项统一、可重绑、按下触发的玩家输入；
- 边界：不建立第二个 command/inventory/movement/damage authority，不启动可视产品，不修改 Content、Engine 或 Windows。

## 2. 设计选择

P6 已具备 exact-item command intent、Run command router、Host、顺序号、幂等账本和 controller 状态机，缺少的是玩家输入边界。本轮没有复制 P6 服务，只新增无状态适配器：先冻结一次 canonical read model，再按状态映射一条既有命令。

`Orbiting -> Launch`，并且只调用一次瞄准采样；`Directed -> Recall`，不采样方向。`Recalled`、Run 不一致、Actor 不一致、无 GameMode、无有效 aim 或 gameplay lock 均在产品路由前失败关闭。

## 3. 实现

### 3.1 输入适配器

新增 `demo_mapShanmenControlledWeaponInputAdapter.h/.cpp`：

- read model 只接受有效 `RunId`、`ItemInstanceId` 与 Orbiting/Directed 状态；
- canonical reader 核验 lifecycle、Host、exact controller 与 exact Actor 指针；
- result 记录 canonical read、intent id、direction sample、intent capture 和 product route 是否发生；
- 每个依赖最多调用一次，不做 fallback、循环重试、Actor movement 或 damage resolution。

### 3.2 PlayerController 与输入配置

- 注册 `ControlledWeaponLaunchRecall`，默认 `X`、press-only；
- exact registry count 28→29，binding serialization Version 6→7；
- `Ademo_mapPlayerController` 将物理输入路由到 GameMode 的 sole World lifecycle / Run Host / command router；
- 生产 intent id 使用 `FGuid::NewGuid()`；Launch 使用已有 `GetLastValidAimDirection()`；
- 所有受 exact count / Version 影响的旧测试断言同步迁移。

### 3.3 自动化与覆盖映射

新增 4 个逻辑测试：gameplay fence、route fence、identity/aim fences、exact-item Launch→Recall；新增 5 个物理测试：registry default、Version 6 migration、occupied-X conflict、press-only binding、live remap + lock。

回归映射新增 `ControlledWeaponInputAdapter` 规则，并把新物理组加入 unified input 与 PlayerController 规则。self-test 增加正向映射、focused-only 反向缺证和统一输入覆盖 fixture，总数 433→435。

## 4. 首轮失败与修复

首轮 Editor 编译已完成新增测试及所有受 registry 影响的旧测试编译，随后在适配器 `.cpp` 报 C2446：编译单元只看见 `Ademo_mapShanmenControlledWeaponActor` 前置声明，无法把其指针与 lifecycle 暴露的 `AActor*` 做派生转换。

修复为包含 `demo_mapShanmenControlledWeaponActor.h`。恢复编译只需 1 个编译、2 个链接和 1 个 metadata 动作，7.64 秒成功。没有绕开类型检查，也没有改行为。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.7_focused_controlled_weapon_input_adapter.log` | `Shanmen.0_0_10.Product.ControlledWeaponInputAdapter` | 4/0 | 266,173 | `7BBA4B4CBFE99DF484FA312353DF8204C304C90F9AAFD98FF5DE5F72E79188D0` |
| `P21.7_focused_controlled_weapon_physical_input.log` | `Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput` | 5/0 | 268,146 | `3C59A31AA9840E2FCF83A894E8B3ECC46821191BD31AFB9ECB8ACF80A400D715` |
| `P21.7_full_0_0_10.log` | `Shanmen.0_0_10` | 1239/0 | 1,897,201 | `5474B71075709876B3125985C853E576D8064BC51B8BEB63D082158543A4E3D1` |
| `P21.7_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 395,334 | `A9209653663F09126E61019FC07131BCF40BF1CFF7D7FCBF5D4D1A19B33D1322` |
| `P21.7_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 285,955 | `354FE99040D93B4FE9B0615B831ACC05C53D94688DBD26BFE0292564C5C9D23E` |
| `P21.7_legacy_full_system_loop.log` | `demo_map.FullSystemLoop` | 50/0 | 312,522 | `4BDC4AF3ED091708A457E7411B28BF089158CAA4DF45C56C991C5D0CCFA7FD81` |
| `P21.7_legacy_p5_runtime_interface.log` | `demo_map.P5RuntimeInterface` | 9/0 | 271,032 | `576E1D80E524F560FEA6AB027F6703873137706CD3ADE783665DEEC3C976A57B` |
| `P21.7_legacy_p7_integration.log` | `demo_map.P7Integration` | 9/0 | 270,613 | `85387CBD78B55DAC294809685873E12AADF0C38A8FF24B9F62F9CB004027D10C` |

最终自动化为 1439/0（focused 与 full 重叠）。全量 1239 项从 `02:57:01.445` 至 `04:11:43.917` 自然清空；慢段来自既有持久化/恢复等待窗口。未使用外层超时码替代 native 0。

## 6. Regression coverage

- self-test：`PASS 435/435`；42,923 bytes；SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- gate：`PASS Changed=19 Rules=8 Required=43 Logs=8`；6,240 bytes；SHA-256 `B5425BA3D6F866BF7BE7220D895611F9527F012F0E8024D754BF02329D8DEDBD`。

Gate 按本轮 19 个实现/测试/流程路径推导回归：除 0.0.10 full 与两个 focused 组外，实际运行 InputRestore、V2RangedCompatibility、FullSystemLoop、P5RuntimeInterface 和 P7Integration。父组真实覆盖其 32、41、47、06 子项，没有用主题相近但未覆盖改动文件的测试替代。

## 7. 静态与构建

- implementation/test/process diff：19 files，`+1109 / -22`；
- production adapter 261 行；新增两组测试 702 行；
- damage/Impact、Actor movement、directed pump、inventory transaction、Timer、RNG、World/Spawn API 扫描命中 0；
- JSON parse、diff check 与残留 UnrealEditor-Cmd 检查通过。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.7_editor_build_initial.log` | Editor | Failed / native 6, C2446 | 40 planned / 110.67s | 5,383 | `AA4A57806E6405A1A615E28C043CA96963BF2320B411C172E300222B3079238A` |
| `P21.7_editor_build_recovery.log` | Editor | Succeeded / native 0 | 4 / 7.64s | 2,747 | `D85B27AB4ABAEAC966CEAC167097E39E5AF0347D64F1E2C76D0F853A5A7CDE08` |
| `P21.7_game_build_final.log` | Game | Succeeded / native 0 | 39 / 114.46s | 4,981 | `4B6D650BC7674E95BE169C4DB01A2DC8E263B48DD49C438432F45F59F1DC0899` |
| `P21.7_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 1.23s | 958 | `3B0618B5250D514611E3711597485A6A63B05F280CFF6EDD75A21EC60CFD5312` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,432,704 bytes / SHA-256 `1F6983C45697B03E3E7405840E8EEF8774046B7F7E6CB500EA06EAAC953C3293`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,605,568 bytes / SHA-256 `90A36DF9C34F0EBEE93870B66555C2D3B2576DB3897613F2028E9CEEB4B9473F`。

## 8. 提交边界与下一步

计划提交 19 个实现/测试/流程文件、本 Report 与本 Development Log，共 21 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.7` raw logs 不入 Git。

未修改 Content、地图、Engine、Windows、save schema、inventory authority、damage/effect resolver 或 P6 movement math。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

下一轮 P21.8 应把现有 `TryAdvanceDirectedInOrder` 接到固定 Run timeline，并以 Host/lifecycle 处理 blocking contact 与返航终态；PlayerController 继续只提交命令。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-7-flying-sword-input-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-7-flying-sword-input-command/Docs/Report/Dev.D.UE.0.0.10.P21.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-7-flying-sword-input-command/Docs/Log/Dev.D.UE.0.0.10.P21.7.r0_log.md>
