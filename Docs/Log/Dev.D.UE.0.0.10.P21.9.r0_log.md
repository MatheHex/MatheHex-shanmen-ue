# Dev.D.UE.0.0.10.P21.9.r0 Development Log

## 1. 基线与目标

- base：`9db4803440783485dff1d4904020bbb1c8543eae`（P21.8 directed runtime）；
- branch：`agent/0.0.10-p21-9-flying-sword-return-redeploy`；
- 目标：让 P21.8 进入 Completed 的实体飞剑可见返回原始环绕锚点，原子换代为新 activation，并可再次发射；
- 边界：复用唯一 Combat Run 30 Hz timeline、既有 Actor/item/Host/ActiveRunRoute；不新增第二状态机、第二时钟、伤害路径、库存写入或 Actor 重建。

## 2. 所有权审计

P21.8 已具备出航与终态权威，但终态 Actor 停在碰撞点。审计确定：

- Controller 已持有 Actor、motion、initial orbit location 与 terminal session；
- Run Host 已持有稳定 GUID 顺序、Run identity、controller 集合与固定 tick 消费边界；
- World lifecycle 已持有 Actor ↔ item ↔ controller 的唯一绑定及 Active Run Route；
- GameMode 已持有唯一 canonical timeline sample；
- 物品 subsystem 是库存真值，本轮不应被返回或换代写入。

因此返回被定义为现有 Completed 状态上的派生行为，不增加生命周期枚举；再次部署通过既有 Route 产生新 controller，而不是复制初始化逻辑。

## 3. 实现

### 3.1 Controller 返回移动

新增 `Fdemo_mapShanmenControlledWeaponReturnMovementReceipt` 与完成/锚点只读查询。`TryAdvanceCompletedReturn` 只允许 terminal controller，使用既有 directed speed 和固定 delta 逐步逼近锚点；抵达时精确落点。返回采用非 swept Actor movement，保证不会打开 contact window 或重复提交 Impact。

### 3.2 Host 固定时间线批处理

新增 `Fdemo_mapShanmenControlledWeaponReturnTimelineResult`、精确 error 与 `AdvanceCompletedReturnsFixedTicks`。批处理验证 Run/timeline、tick 范围和 300-tick catch-up 上限，按稳定 GUID 顺序推进全部待返回 controller，并记录 movement、arrival 与精确失败 item。no-op 与失败边界均可审计。

### 3.3 World 原子换代

World lifecycle 保存下一 deterministic activation sequence，并新增 `TryRedeployReturned`。它先验证返回 receipt、managed Actor、item、Run、旧 terminal 与锚点，再在 candidate Host 中删除旧 activation、调用既有 Active Run Route 创建新 activation，最后验证 Orbiting/Actor/item/identity 后提交。

没有 `SpawnActor`、`Destroy` 或库存写入。旧 activation identity 不复用，sequence 0/溢出失败关闭。

### 3.4 GameMode 顺序

GameMode 对同一 timeline sample 先推进已存在的 terminal return，再推进 directed flight。刚碰撞完成的飞剑下一帧开始返回；刚 redeploy 的飞剑跳过本帧 orbit advance，避免重复消费同一 frame delta。两个新增生产入口均只有这一处调用。

## 4. 测试开发与首次结果

- Run Host 新增 `FixedTimelineVisibleReturn`，覆盖零 tick、非法 timeline、超预算、逐 tick 移动、抵达与抵达后静止；
- canonical World lifecycle 测试扩展为完整两次发射闭环，检查同一 Actor/item、新 ActivationId、单 Actor 数量、库存快照不变与第二次真实位移；
- 初始 Editor 非 Unity 编译 62 个动作，native 0；UBA 曾因本机内存阈值回收并重试个别 worker，但最终 62/62 完成，不构成一次失败构建；
- 同一代码的首轮有效 focused 测试为 57/0，没有产品测试失败或放宽断言后的重跑。

focused 日志在 RunTests 前含 UE 内建 Automation 自检的通用 `Condition failed` 启动文本；选定组自身 `Result={Fail}` 为 0，全部 57 项成功，并自然输出 native exit 0。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.9_focused_controlled_weapon_initial.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 57/0 | 326,272 | `D9E6C46F8EB7D13829DA1984C5543FD779AB2F83DA05C6DB753F0DACB3ECF77D` |
| `P21.9_full_0_0_10.log` | `Shanmen.0_0_10` | 1242/0 | 1,896,451 | `074EE581F2DC90D374FAFA5F5B4B811275B6066D9B307C85697D6520EAC9896B` |
| `P21.9_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 264,360 | `04781DE73D716C789BBC564A5F1221217BED831F9BAD8301DEAE60B933F53EA7` |
| `P21.9_legacy_enemy_skill_framework.log` | `demo_map.EnemySkillFramework` | 44/0 | 304,851 | `A6DFDF3F2C5B63C63FD9B17C2D406A36A93661F6CBF4CB95C82BE2C328A09242` |
| `P21.9_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,491 | `F89C3818B73BE51B6B88267A06425A784DDDE10020CCB8FB2242452808074C3B` |
| `P21.9_legacy_item_use_and_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,572 | `2DD33AF12382B49971366C9F9EF6C9476CD8E1525A29969EFE10363F435D60CB` |
| `P21.9_legacy_p4_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 267,282 | `3CE6F7190E98053DFB0FA7840B2CE60839A8EF43EEBD2B0FEA1099C2D549366D` |

full 由 `07:25:40.908` 至 `08:39:33.549` 自然清空 1242 项。五组旧版兼容合计 123/0。七份日志均有且仅有一个 RunTests 命令、终止成功标记与 native 0；Fail/Fatal/Unhandled/Ensure 均为 0。

## 6. Regression coverage

- self-test：`PASS 435/435`；42,923 bytes；SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- gate：`PASS Changed=9 Rules=5 Required=70 Logs=6`；8,473 bytes；SHA-256 `9582CB85A8EFDF7546E9E0B99194801D2D9914B533ED56AFFC3D27E490F8E10F`。

GameMode 改动要求 full、V3.Attributes、EnemySkillFramework、V2RangedCompatibility 与 ItemUseAndArmor。`demo_mapShanmenPreparationAdapterTests.cpp` 同时命中 ItemProductAdapters 规则，因此额外要求 `demo_map.P4.Hotbar`。六份 gate evidence 覆盖全部 70 个 required group，没有复用 P21.8 日志。

## 7. 静态与构建

- implementation/test diff：9 files，`+920 / -2`；
- 新增生产行中的 Timer/SetTimer、RNG、ApplyDamage、SpawnActor 与 Destroy 命中 0；
- 两个新增生产 pump 的 GameMode 调用点各 1；
- `git diff --check` 通过，仅有 LF→CRLF 工作树提示；
- 验证结束后无 UnrealEditor、UnrealEditor-Cmd 或 demo_map 进程残留。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| session evidence | Editor initial | Succeeded / native 0 | 62 / 43.87s | n/a | n/a |
| `P21.9_game_build_final.log` | Game | Succeeded / native 0 | 61 / 45.44s | 14,857 | `0FB615FF501DC4DC92F9217A01D662333104961C06FDF54DB2DCB36D0501AD4B` |
| `P21.9_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 0.95s | 6,274 | `13288F918663521FAF561920872D1B3810599A62B5B0C52BA7FDFBD4BCA747E2` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,481,344 bytes / SHA-256 `658E51A3003928A7A540B71F3D038446FA5FAFB2820E5AE60EB18E202B965C20`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,659,328 bytes / SHA-256 `8A3CF333313F2AA9156CDFC66E072D87995FD0C06953B18CE85BF394C1BB0446`。

## 8. 提交边界

提交 9 个实现/测试文件、本 Report 与本 Development Log，共 11 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.9` raw logs 不进入 Git。

未修改 Content、地图、Engine、Windows、save schema、输入注册表、物品权威、Impact resolver 或既有出航 damage math。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

下一轮建议建立只读 flight-phase/presentation cue，让表现层可观察 Returning/Redeployed，而不反向拥有 gameplay 状态或时间。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-9-flying-sword-return-redeploy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-9-flying-sword-return-redeploy/Docs/Report/Dev.D.UE.0.0.10.P21.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-9-flying-sword-return-redeploy/Docs/Log/Dev.D.UE.0.0.10.P21.9.r0_log.md>
