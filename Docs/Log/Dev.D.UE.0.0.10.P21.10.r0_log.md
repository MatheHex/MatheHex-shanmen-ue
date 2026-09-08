# Dev.D.UE.0.0.10.P21.10.r0 Development Log

## 1. 基线与目标

- base：`c89fba10b154dfe07975c542077b44cf355f6d19`（P21.9 return/redeploy loop）；
- branch：`agent/0.0.10-p21-10-flying-sword-phase-presentation`；
- 目标：让表现层只读观察 Orbiting、Directed、Returning、Redeployed，而不复制受控飞剑 gameplay 状态或时间；
- 边界：复用现有 Actor、Controller、Run Host、World lifecycle、GameMode frame owner、材质与 HUD readout；不新增 gameplay transition、timer、movement、contact、Impact 或 inventory authority。

## 2. 所有权审计

P21.9 已具备完整 gameplay 闭环，但表现层只知道近身 threat cue，无法区分飞剑当前处于环绕、出击、返航还是刚归位。审计确定：

- Controller 是当前 activation、状态、Actor 与返回锚点的现有事实源；
- World lifecycle 保证 Actor ↔ item ↔ controller 的 canonical 绑定；
- GameMode 已按唯一 fixed timeline 完成本帧运动、换代和威胁采样；
- Actor 材质与 HUD 已是现有呈现出口。

因此本轮只在 frame owner 完成 gameplay 后捕获不可变观察，不把 phase 写回 Controller，也不创建呈现侧时钟或状态迁移。

## 3. 实现

### 3.1 Flight read model

新增 `Fdemo_mapShanmenControlledWeaponFlightReadModel` 与四个可见阶段。模型保存 Run/item/activation、当前位置和返回锚点；验证 GUID、有限向量、合法阶段，并要求 Redeployed 精确位于锚点。默认对象为 Invalid，失败捕获会清空输出。

### 3.2 Controller 派生

`TryCaptureFlightReadModel` 从现有状态派生 phase。`bRedeployedThisFrame` 只是 GameMode 已知的本帧事务结果，不被持久化；它只在新 controller 已 Orbiting 且 Actor 在锚点时生成一帧 Redeployed。后续观察自然回到 Orbiting。

### 3.3 Actor 与 threat overlay

Actor 仅接受同一 Run/item 且位置等于自身当前 transform 的模型。阶段映射到程序化材质颜色；已有 threat presence cue 继续作为临时高优先级覆盖，清空后通过同一刷新函数恢复阶段色。lifecycle deactivation 同时清空 read model，Actor Tick 保持关闭。

### 3.4 GameMode 与 HUD

GameMode 在本帧 controlled-weapon gameplay 完成后执行唯一 capture/publish。HUD 从 canonical Actor 读取模型，并扩展既有 readout plan：零目标也可显示阶段文案，目标数大于零时追加近身计数，投影失败时沿用屏内 fallback。没有第二 HUD 面板。

### 3.5 Regression mapping

新增 `ControlledWeaponFlightReadModel` 路径规则，要求 Controller、WorldLifecycle、ThreatCue、ThreatReadoutPresentation、WorldGameplay 与 CombatRuntime 证据；self-test 增加一条预期通过和一条 readout-only 证据预期失败，共 437 项。

## 4. 测试开发与首次结果

- 新增 `ControlledWeaponController.FlightReadModel`，覆盖四阶段、Redeployed 单帧、锚点/身份/终态失败边界与输出清空；
- canonical World lifecycle 测试覆盖 Returning → Redeployed → Orbiting → second Directed，并验证新 activation、同一 Actor/item；
- threat cue integration 验证阶段色、威胁覆盖和恢复；
- threat readout tests 覆盖四阶段文案、零计数、屏外 fallback、非法 phase 与负计数；
- 初始 Editor 编译 65/65 动作，49.93s，native 0；
- 首轮 focused 为 58/0，没有产品测试失败或放宽断言后的重跑。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.10_focused_controlled_weapon_initial.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 58/0 | 327,105 | `F03FA53A7C3D04A4942D65B1906A9D3BF6582DDDC37950330D1698EBAC83CF17` |
| `P21.10_full_0_0_10.log` | `Shanmen.0_0_10` | 1243/0 | 1,898,790 | `369C3CFFD60302F4ABB20AFEB2E144D7A418EF869FDC063D34B2DF8712597760` |
| `P21.10_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 264,361 | `B3FCD98580883C4D06243BEC85BAEC383E0AA612914196A5E1D941A9B57433C6` |
| `P21.10_legacy_enemy_skill_framework.log` | `demo_map.EnemySkillFramework` | 44/0 | 304,855 | `335E2F119DCB4909B5FEAB7E0EDB5D715DCF7717BED121DF0E64842C215BB54A` |
| `P21.10_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 285,290 | `E6036AFFF4A254F50164F6AB103E9AAD897A037872C15A446ACB0C7F65A36A8B` |
| `P21.10_legacy_item_use_and_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | 310,380 | `21FD495021E89EDAE18A95123C9F6C115B13737102F4FB63E16F8A03D6F9CF48` |
| `P21.10_legacy_p4_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 267,479 | `7C0651BEB4CA61499D26F0AA036653541A6816A88EBD0EF11FDCE9BE3C39C531` |
| `P21.10_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 395,101 | `A1F48EFF3D0A32830E3ED5A9686D0E139FE9223B55DD14306AED60189ECF5154` |

full 由 `09:25:59.159` 至 `10:37:17.274` 自然完成 1243/0。八份日志均有一个实际 RunTests 命令、一个 native 成功终止标记；Fail/Fatal/Unhandled/Ensure 均为 0。

## 6. Regression coverage 与首次失败

首轮 changed-file gate 失败：`missing required groups: demo_map.InputRestore`。失败发生在所有已运行日志健康、但 GameMode/HUD 改动对应旧输入恢复证据缺失时；没有修改 mapping 来放宽要求。

- 首次失败日志：`P21.10_regression_gate_initial_fail.log`，345 bytes，SHA-256 `B1666433283C14CD233E84623DBD32617D7FCF678EF86402C618620403CFEEF3`；
- 修复：新增执行 `demo_map.InputRestore`，101/0，native 0；
- 最终 gate：`PASS Changed=15 Rules=8 Required=84 Logs=7`，10,410 bytes，SHA-256 `26ADFE651729F87113678303F74E37C0FFA911A24EBB32B58859B768FB46952D`；
- self-test：`PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

这次失败证明 changed-file gate 能实际阻止主题式裁剪遗漏；修复只补证据，没有改产品代码或降低门禁。

## 7. 静态与构建

- implementation/test/mapping：15 files，`+631 / -27`；
- 新增生产行中的 Timer/SetTimer、RNG、ApplyDamage、SpawnActor、Destroy 命中 0；
- GameMode 中 capture/publish 生产调用各 1；
- `git diff --check` 通过，仅有 LF→CRLF 工作树提示；
- 验证结束后无 UnrealEditor、UnrealEditor-Cmd 或 demo_map 进程残留。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.10_editor_build_initial.log` | Editor initial | Succeeded / native 0 | 65 / 49.93s | 6,884 | `78EEB66F5E28C0F550CCEBE71A34FC1B8BABEA341A934A28A44D7F9027D164A0` |
| `P21.10_game_build_final.log` | Game final | Succeeded / native 0 | 64 / 52.80s | 6,678 | `2315E7B7A59637ADF1DE28D73E04C74E01A5911E99136BF2213BEF38F6F3BB8D` |
| `P21.10_editor_build_final.log` | Editor final | Succeeded / native 0 | 0 / 0.97s | 1,030 | `BFF4C9306D8F9CC6A8F2023BC88E84D8CF7F82E8FF6484C806545B88347A6C0E` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,493,120 bytes / SHA-256 `F83D9EA4036E489D1445F4FE1215C7DFECF70818557043BABB6119C8380E9E0D`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,672,640 bytes / SHA-256 `5D3D22FC60821567CFFA09869B423F1F14BC572AC2AF9DEA2447503F9FD90E4F`。

## 8. 提交边界

提交 15 个实现/测试/mapping 文件、本 Report 与本 Development Log，共 17 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.10` raw logs 不进入 Git。

未修改 Content、地图、Engine、Windows、save schema、输入注册表、物品权威、Impact resolver、返回移动或定向伤害数学。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

下一轮优先推进真实玩法语义，并继续复用唯一 authority；玩家镜头下的视觉验收留给单独授权的受控 PIE 轮次。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-10-flying-sword-phase-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-10-flying-sword-phase-presentation/Docs/Report/Dev.D.UE.0.0.10.P21.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-10-flying-sword-phase-presentation/Docs/Log/Dev.D.UE.0.0.10.P21.10.r0_log.md>
