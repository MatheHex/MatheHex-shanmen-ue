# Dev.D.UE.0.0.10.P21.7.r0 Report

## 1. 结论

P21.7 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P6 已存在的飞剑 `Launch` / `Recall` 命令接入统一、可重绑的玩家输入。默认按键为 `X`：canonical `TrainingFlyingSword` 处于 `Orbiting` 时，按下一次以当前有效瞄准方向发射；处于 `Directed` 时，再按一次召回同一 `ItemInstanceId`。输入层不选择替代物品、不重试、不移动 Actor，也不处理伤害。

```text
Controlled-weapon input focused:             4 Success / 0 Fail
Controlled-weapon physical input focused:    5 Success / 0 Fail
Shanmen.0_0_10 full:                      1239 Success / 0 Fail
Required 0.0.9B compatibility groups:      191 Success / 0 Fail
Regression coverage:                        PASS (Changed=19 / Rules=8 / Required=43 / Logs=8)
Regression gate self-test:                  PASS 435/435
Game + Editor Development:                  PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是物理按键绑定、输入迁移、canonical 身份、逻辑命令路由和原生可构建性；不宣称飞剑已有可见的逐帧出剑/返航表现或人工手感验收。

## 2. 玩家输入行为

统一输入注册表新增 `ControlledWeaponLaunchRecall`：

- 默认键为 `X`，只绑定 `Pressed`，释放不会重复触发；
- 运行时重绑继续走现有 `ApplyOverrideWithSwap` 与输入重建，不建立第二份配置；
- settlement 等既有 gameplay input lock 在读取飞剑状态前即失败关闭；
- 缺少权威 `GameMode` 路由时不读取 lifecycle、不创建 intent、不采样瞄准；
- 发射只采样一次 `GetLastValidAimDirection()`；无有效方向时不调用产品路由；
- 召回不采样方向，并只携带已读取的同一 `ItemInstanceId`。

`Ademo_mapPlayerController` 只负责物理键到输入适配器的接线。实际命令仍由 P6 `Fdemo_mapShanmenControlledWeaponRunCommandRouter` 处理，沿用其顺序号、幂等、事务提交和回放规则。

## 3. Canonical 身份与失败关闭

`Fdemo_mapShanmenControlledWeaponInputAdapter::TryReadCanonical` 同时验证：

- World lifecycle 已激活；
- lifecycle 与 Run Host 的 `RunId` 相同；
- lifecycle 的 exact item 在 Host 中存在 controller；
- controller 持有的 Actor 指针与 lifecycle 的 canonical Actor 完全一致；
- 当前状态严格为 `Orbiting` 或 `Directed`。

任何不一致都会清空复用 read model 并返回失败，不搜索其它飞剑。成功读取后只创建一个 intent id，只捕获一份 intent，产品 Router 最多调用一次。

## 4. 配置迁移

输入注册表由 28 项增加到 29 项，序列化版本由 6 升至 7：

- 旧 Version 6 配置未占用 `X` 时，新动作自动获得 `X`；
- 旧配置已把其它动作绑定到 `X` 时，保留用户旧覆盖，新动作使用现有确定性冲突回退得到空闲 `G`；
- 新动作可在运行中从 `X` 重绑到 `C`，输入组件重建后旧键立即失效、新键只触发一次；
- 所有旧 exact-count 与序列化版本断言同步到 29 / Version 7。

## 5. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon input focused | 4 | 0 | 266,173 | `7BBA4B4CBFE99DF484FA312353DF8204C304C90F9AAFD98FF5DE5F72E79188D0` |
| Controlled-weapon physical input focused | 5 | 0 | 268,146 | `3C59A31AA9840E2FCF83A894E8B3ECC46821191BD31AFB9ECB8ACF80A400D715` |
| `Shanmen.0_0_10` full | 1239 | 0 | 1,897,201 | `5474B71075709876B3125985C853E576D8064BC51B8BEB63D082158543A4E3D1` |
| `demo_map.InputRestore` | 101 | 0 | 395,334 | `A9209653663F09126E61019FC07131BCF40BF1CFF7D7FCBF5D4D1A19B33D1322` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 285,955 | `354FE99040D93B4FE9B0615B831ACC05C53D94688DBD26BFE0292564C5C9D23E` |
| `demo_map.FullSystemLoop` | 50 | 0 | 312,522 | `4BDC4AF3ED091708A457E7411B28BF089158CAA4DF45C56C991C5D0CCFA7FD81` |
| `demo_map.P5RuntimeInterface` | 9 | 0 | 271,032 | `576E1D80E524F560FEA6AB027F6703873137706CD3ADE783665DEEC3C976A57B` |
| `demo_map.P7Integration` | 9 | 0 | 270,613 | `85387CBD78B55DAC294809685873E12AADF0C38A8FF24B9F62F9CB004027D10C` |

八份最终自动化日志合计 1439/0（focused 与 full 有意重叠）。全量套件从 `2026-09-08 02:57:01.445` 至 `04:11:43.917`，同一 UnrealEditor-Cmd 实例自然清空 1239 项并原生退出 0；全部最终自动化日志的 Fatal、Unhandled Exception 与 Ensure condition failed 命中均为 0。

## 6. 首败与修复

首轮 Editor 构建在 40 个计划动作的第 37 项后以 `OtherCompilationError` / native 6 停止：输入适配器比较 canonical Actor 指针时，`Ademo_mapShanmenControlledWeaponActor` 在该编译单元仍是不完整类型，编译器无法完成派生类到 `AActor` 的静态转换（C2446）。

修复只是在适配器 `.cpp` 中包含 Actor 的完整类型定义；未改变路由行为。恢复构建执行 4 个动作并成功，随后 focused、full、旧版回归以及 Game/Editor 最终构建全部通过。首败日志保留：5,383 bytes，SHA-256 `AA4A57806E6405A1A615E28C043CA96963BF2320B411C172E300222B3079238A`。

## 7. 覆盖门、静态审计与构建

- regression self-test：`PASS 435/435`，42,923 bytes，SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- changed-file gate：`PASS Changed=19 Rules=8 Required=43 Logs=8`，6,240 bytes，SHA-256 `B5425BA3D6F866BF7BE7220D895611F9527F012F0E8024D754BF02329D8DEDBD`；
- 本轮实现、测试和映射共 19 个文件，`+1109 / -22`，其中新增 702 行为两组测试；
- 新输入适配器中的 damage、Impact、Actor movement、directed movement pump、inventory transaction、Timer、RNG 与 World/Spawn API 命中 0；
- regression map JSON parse 与 `git diff --check` 通过。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | Failed / native 6 | 40 planned / 110.67s total | `AA4A57806E6405A1A615E28C043CA96963BF2320B411C172E300222B3079238A` |
| Editor recovery | Succeeded / native 0 | 4 / 7.64s total | `D85B27AB4ABAEAC966CEAC167097E39E5AF0347D64F1E2C76D0F853A5A7CDE08` |
| Game Development final | Succeeded / native 0 | 39 / 109.36s UBA, 114.46s total | `4B6D650BC7674E95BE169C4DB01A2DC8E263B48DD49C438432F45F59F1DC0899` |
| Editor Development final | Succeeded / native 0 | 0 / 0.15s UBA, 1.23s total | `3B0618B5250D514611E3711597485A6A63B05F280CFF6EDD75A21EC60CFD5312` |

最终产物：

- `demo_map.exe`：359,432,704 bytes，SHA-256 `1F6983C45697B03E3E7405840E8EEF8774046B7F7E6CB500EA06EAAC953C3293`；
- `UnrealEditor-demo_map.dll`：18,605,568 bytes，SHA-256 `90A36DF9C34F0EBEE93870B66555C2D3B2576DB3897613F2028E9CEEB4B9473F`。

## 8. P/F 边界与后续

PASS：可重绑单键进入现有 P6 路由；Orbiting 发射、Directed 召回；发射只读一次有效瞄准；召回同一 item 且不读瞄准；身份、锁定、无 route、无方向全部失败关闭；Version 6 配置可无损迁移；完整与旧版回归、覆盖门及双目标构建通过。

未声明：真实按键手感、可见飞行、飞剑逐帧位移、碰撞命中、自动返航、伤害、音画反馈或人工玩法验收。生产代码中仍没有调用 P6 已存在的 `TryAdvanceDirectedInOrder`，所以本轮闭合的是“玩家输入到权威逻辑命令”，不是“命令到可见飞行全过程”。

下一轮应把 `TryAdvanceDirectedInOrder` 接入同一固定 Run 时间线，并把阻挡接触与返航终态交给既有 Host/lifecycle 处理；不得在 PlayerController 中直接移动 Actor 或结算伤害。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-7-flying-sword-input-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-7-flying-sword-input-command/Docs/Report/Dev.D.UE.0.0.10.P21.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-7-flying-sword-input-command/Docs/Log/Dev.D.UE.0.0.10.P21.7.r0_log.md>
