# Dev.D.UE.0.0.10.P21.8.r0 Development Log

## 1. 基线与目标

- base：`bf6215e431315cf966a2a37039fe30cb71162eaf`（P21.7 flying-sword input command）；
- branch：`agent/0.0.10-p21-8-flying-sword-directed-runtime`；
- 目标：让 P21.7 的 canonical Launch 命令通过现有 P6 swept movement 在固定 Combat Run tick 上产生真实 World 位移，并让 blocking contact 沿既有伤害与 lifecycle 权威收敛；
- 边界：不新增第二时钟、movement/damage authority、输入动作或库存路径，不启动可视产品，不修改 Content、Engine 或 Windows。

## 2. 所有权审计

审计确认 P6 已有完整但未被生产消费的能力：

- `Fdemo_mapShanmenControlledWeaponProductController::TryAdvanceDirected` 拥有单 Actor swept movement；
- Run Host 的 `TryAdvanceDirectedInOrder` 拥有稳定 GUID 顺序的多飞剑批处理；
- Host 已拥有 contact window、world delivery、Recall/Complete 与 Interrupt；
- GameMode 已拥有唯一 30 Hz `CombatRunFixedTimeline`，但此前只给 combat condition 消费；
- World lifecycle 已拥有实际 `TrainingFlyingSword` Actor、碰撞体与 Run teardown。

因此本轮没有创建新 service/wrapper 链，只在 Host 增加一个固定 tick 批处理操作，并从 GameMode 唯一调用。

## 3. 实现

### 3.1 Directed timeline result

新增紧凑 result，记录 Run/timeline、起止 tick、请求 tick、实际 movement tick、移动数、阻挡数、伤害交付数、终态数、fallback interrupt 与精确失败 item。`IsSuccess` 验证计数守恒、终态守恒与诊断完整性。

每轮最多消费 300 个 30 Hz tick。无完整 tick、无 Host 或无 directed item 返回成功 no-op；非法 sample、外部 timeline、Host/Coordinator 不一致、超预算或 movement 拒绝均在有限边界内返回错误。

### 3.2 Blocking path

每个 blocking movement entry 只走已有 contact → coordinator vitality delivery → contact close → Recall/Complete。正常路径失败时最多尝试一次已有 Interrupt；仍未终态化则整批返回拒绝。未注册几何阻挡不会被伪装成 delivered Impact。

### 3.3 GameMode wiring

GameMode 每次成功推进 fixed timeline 后捕获一次 sample，并先交给 ControlledWeapon Host，再把同一 sample 交给 combat-condition component。拒绝与 blocking terminal 都只写结构化日志；没有 frame retry、Timer 或第二 accumulator。

## 4. 首次执行与修复

初始 Editor 非 Unity 编译执行 60 个动作并以 native 0 成功，新增 GameMode、Host 和测试均编译通过。

第一次启动 focused 测试时，命令中的项目路径误写为 `Dev.D.UE.0.B`；UE 在加载项目之前返回 native 1。自动备份原始日志：`P21.8_focused_controlled_weapon_run_host_initial-backup-2026.09.08-05.25.25.log`，7,891 bytes，SHA-256 `D5279A4EA481297C391360FB3C8B5157E636DA4E47027B11CA260CD986A12D9C`。

修正为精确项目路径后未修改代码，首轮有效 focused 即为 12/0。后续没有产品测试、回归、覆盖或构建失败。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.8_focused_controlled_weapon_run_host_initial.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 12/0 | 276,593 | `18B49B5B5A8356A862F7A07C58C0AA0337D67297B6C7BC05D5A99ABCEBF1176D` |
| `P21.8_full_0_0_10.log` | `Shanmen.0_0_10` | 1241/0 | 1,900,154 | `5D3EC2035448C2941F9B372DADCCA1FE6EEF02864E50069F100AC45E0CE970A6` |
| `P21.8_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 264,965 | `47FAAE560B65002888B25896BDB37DD0D2F55B1924A9F65609BE82B0CB482CA2` |
| `P21.8_legacy_enemy_skill_framework.log` | `demo_map.EnemySkillFramework` | 44/0 | 304,852 | `2903F3D369CEB7229ACB72C6F64DA44AAB8DC4133EBF86EE6ACEF4156C41772C` |
| `P21.8_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,491 | `4FD59F036EEE1314E549C1B21A24E9D54D12BD78D15B5F30214F9EA696DAD495` |
| `P21.8_legacy_item_use_and_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,768 | `AA0CF9D0E886A58681C246DD5BDA62F88029B1B0DEB48ED9E62FBAFB3CDBC4EA` |

full 从 `05:27:50.305` 至 `06:42:12.746` 自然清空 1241 项，原生 `TEST COMPLETE. EXIT CODE: 0`。六份最终日志均无 Fail、Fatal、Unhandled Exception 或 Ensure condition failed。

## 6. Regression coverage

- self-test：`PASS 435/435`；42,923 bytes；SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- gate：`PASS Changed=4 Rules=2 Required=67 Logs=6`；8,272 bytes；SHA-256 `6D1CEAF5F23CA1C0CC41A8C74B7D499099537A95669F786B0C3C8C51F48E1050`。

GameMode 规则要求 full、V3.Attributes、EnemySkillFramework、V2RangedCompatibility 与 ItemUseAndArmor；Run Host 规则要求 focused 及其完整依赖链。父组证据覆盖全部 67 个 required group，没有复用 P21.7 日志或用主题相近测试替代改动文件覆盖。

## 7. 静态与构建

- implementation/test diff：4 files，`+708 / -27`；production `+387 / -27`，tests `+321`；
- production pump 调用点 1；新增 Timer、独立时钟、RNG、World/Spawn 与直接 ApplyDamage API 0；
- `git diff --check` 通过；raw logs 不进入 Git；无残留 UnrealEditor-Cmd。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| session output | Editor initial | Succeeded / native 0 | 60 / 280.03s UBA, 283.72s total | n/a | n/a |
| `P21.8_game_build_final.log` | Game | Succeeded / native 0 | 59 / 218.47s UBA, 221.03s total | 6,452 | `BF68CFDD14D44D67CE37AC997F56412D5D8A058637EBB2B93F755A65B77AAD4F` |
| `P21.8_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 0.14s UBA, 1.06s total | 1,019 | `240D5A736BF7990269390487359DE749F4D7B762378F1477D26F8DBFEFD6CF51` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,453,696 bytes / SHA-256 `99505AC229127509CFAD26B018787E74865CBC35ECC0299EF7DC1131A670645D`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,630,144 bytes / SHA-256 `B472EBC23A574E72DA8C18D30637477E4EF1DFA291CD2D5154244C5673CE3570`。

## 8. 提交边界

计划提交 4 个实现/测试文件、本 Report 与本 Development Log，共 6 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.8` raw logs 不入 Git。

未修改 Content、地图、Engine、Windows、save schema、输入注册表、库存权威、Combat Run identity、Impact resolver 或 P6 movement math。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

下一轮 P21.9 建议实现可见返航/复位和再次部署闭环；当前 P21.8 的 blocking/Recall 仍使用既有即时逻辑终态，Actor 在 Run teardown 前保留最后物理位置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-8-flying-sword-directed-runtime>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-8-flying-sword-directed-runtime/Docs/Report/Dev.D.UE.0.0.10.P21.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-8-flying-sword-directed-runtime/Docs/Log/Dev.D.UE.0.0.10.P21.8.r0_log.md>
