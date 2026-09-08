# Dev.D.UE.0.0.10.P21.8.r0 Report

## 1. 结论

P21.8 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P6 已存在但此前没有生产调用点的定向飞行步进接入唯一 Combat Run 固定时间线。P21.7 的 `X` 发射命令现在会由 `Ademo_mapGameMode` 按 canonical 30 Hz tick 推进 exact `TrainingFlyingSword`；每一步继续使用既有 swept Actor movement。发生阻挡时，飞剑沿既有 contact → Combat Run vitality delivery → lifecycle 路径最多交付一次 Impact，并收敛到终态。

```text
Controlled-weapon Run Host focused:        12 Success / 0 Fail
Shanmen.0_0_10 full:                     1241 Success / 0 Fail
Required 0.0.9B compatibility groups:     116 Success / 0 Fail
Regression coverage:                       PASS (Changed=4 / Rules=2 / Required=67 / Logs=6)
Regression gate self-test:                 PASS 435/435
Game + Editor Development:                 PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是固定 tick 接线、真实物理 sweep、一次伤害交付、阻挡终态和原生可构建性；不宣称玩家视角下的飞行动画、手感或画面验收已通过。

## 2. 固定时间线运行

`Fdemo_mapShanmenControlledWeaponRunHost::AdvanceDirectedFixedTicks` 消费 GameMode 已经推进并捕获的同一份 `Fdemo_mapShanmenCombatRunTimelineSample`：

- 不建立 Timer、独立 accumulator 或第二时钟；固定步长来自现有 30 Hz Combat Run timeline；
- 每个 canonical tick 最多调用一次既有 `TryAdvanceDirectedInOrder`；多把飞剑继续按稳定 GUID 顺序推进；
- 当前帧没有完整 tick、Host 为空或没有飞剑处于 `Directed` 时是可审计 no-op；
- timeline/Run 不匹配、Host 非法或一次 catch-up 超过 300 tick 时，在移动前失败关闭；
- 一把飞剑阻挡终止后，其余仍处于 `Directed` 的飞剑可以在后续 tick 继续推进。

GameMode 在一次成功的 timeline advance 后只捕获一次 sample；飞剑运动与既有 combat-condition expiry 都消费这同一时间读数，未产生帧内时钟分叉。

## 3. 阻挡接触与终态

每条 swept movement receipt 若报告 blocking hit，Host 会顺序调用既有权威：

1. `TryBeginContactWindow`；
2. `ResolveSweepContact`，由 Combat Run coordinator 决定是否形成 vitality delivery；
3. `TryEndContactWindow`；
4. `TryRecallAndComplete`，沿既有 command sequence 进入 Recovery → Completed。

若意外的 contact close / recall 路径不能完成，只允许一次既有 `TryInterrupt` 作为有界安全收敛；结果会显式记录 fallback 次数。仍不能终态化时返回 `ContactLifecycleRejected`，不伪造成功。

普通墙体等未注册目标可以阻挡并终止飞剑，但不会伪造伤害交付；注册 M01 敌人的真实 sweep 则在测试中形成且只形成一次 committed Impact。

## 4. 自动化覆盖

Run Host focused 由原 10 项增加到 12 项。新增两项为：

- `FixedTimelineDirectedMovement`：零 tick 不移动；3 tick 精确移动 40 units；外部 timeline 与 301-tick catch-up 在物理移动前拒绝；
- `BlockingContactTerminal`：在带 physics scene 的临时 GamePreview World 中生成 source、飞剑碰撞体和已注册 M01 敌人，验证真实 sweep 命中、一次 vitality 下降、一次 committed Impact、contact 关闭、终态收敛，以及后续 tick 不再移动或重复伤害。

全量测试由 P21.7 的 1239 项精确增加到 1241 项。

## 5. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon Run Host focused | 12 | 0 | 276,593 | `18B49B5B5A8356A862F7A07C58C0AA0337D67297B6C7BC05D5A99ABCEBF1176D` |
| `Shanmen.0_0_10` full | 1241 | 0 | 1,900,154 | `5D3EC2035448C2941F9B372DADCCA1FE6EEF02864E50069F100AC45E0CE970A6` |
| `demo_map.V3.Attributes` | 4 | 0 | 264,965 | `47FAAE560B65002888B25896BDB37DD0D2F55B1924A9F65609BE82B0CB482CA2` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 304,852 | `2903F3D369CEB7229ACB72C6F64DA44AAB8DC4133EBF86EE6ACEF4156C41772C` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,491 | `4FD59F036EEE1314E549C1B21A24E9D54D12BD78D15B5F30214F9EA696DAD495` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 309,768 | `AA0CF9D0E886A58681C246DD5BDA62F88029B1B0DEB48ED9E62FBAFB3CDBC4EA` |

六份最终自动化日志合计 1369/0（focused 与 full 有意重叠）。完整套件从 `2026-09-08 05:27:50.305` 至 `06:42:12.746`，同一 UnrealEditor-Cmd 实例自然清空 1241 项并收到 native exit 0；全部最终日志的 Fatal、Unhandled Exception 与 Ensure condition failed 命中均为 0。

## 6. 首次执行记录

首次 focused 命令把项目目录误写成 `Dev.D.UE.0.B`，因此在加载项目之前以 native 1 结束；这不是产品测试失败。该原始日志仍保留：7,891 bytes，SHA-256 `D5279A4EA481297C391360FB3C8B5157E636DA4E47027B11CA260CD986A12D9C`。

仅修正命令路径为 `Dev.D.UE.0.0.9B` 后，同一代码首轮有效 focused 测试即为 12/0；之后 full、四组旧版兼容、覆盖闸门及双目标构建均一次通过。没有为取得通过而改变产品行为或放宽断言。

## 7. 覆盖门、静态审计与构建

- regression self-test：`PASS 435/435`，42,923 bytes，SHA-256 `2C1394ACBD8CF8C02AC117ADC3B74507849F5D19D7F6298415E9D2328E303873`；
- changed-file gate：`PASS Changed=4 Rules=2 Required=67 Logs=6`，8,272 bytes，SHA-256 `6D1CEAF5F23CA1C0CC41A8C74B7D499099537A95669F786B0C3C8C51F48E1050`；
- 4 个实现/测试文件，`+708 / -27`，其中 321 行为真实 World 与 fixed-tick 测试；
- 新增生产行中的 Timer、第二时钟、RNG、World/Spawn 与直接 `ApplyDamage` API 命中 0；
- 生产 `AdvanceDirectedFixedTicks` 调用点严格为 1，位于 GameMode 固定时间线消费处；
- `git diff --check` 通过，仅有工作树 LF→CRLF 提示；验证结束后无 UnrealEditor / UnrealEditor-Cmd / 产品进程残留。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial compile | Succeeded / native 0 | 60 / 280.03s UBA, 283.72s total | session evidence |
| Game Development final | Succeeded / native 0 | 59 / 218.47s UBA, 221.03s total | `BF68CFDD14D44D67CE37AC997F56412D5D8A058637EBB2B93F755A65B77AAD4F` |
| Editor Development final | Succeeded / native 0 | 0 / 0.14s UBA, 1.06s total | `240D5A736BF7990269390487359DE749F4D7B762378F1477D26F8DBFEFD6CF51` |

最终产物：

- `demo_map.exe`：359,453,696 bytes，SHA-256 `99505AC229127509CFAD26B018787E74865CBC35ECC0299EF7DC1131A670645D`；
- `UnrealEditor-demo_map.dll`：18,630,144 bytes，SHA-256 `B472EBC23A574E72DA8C18D30637477E4EF1DFA291CD2D5154244C5673CE3570`。

## 8. P/F 边界与后续

PASS：P21.7 Launch 后由唯一固定 Run timeline 产生逐 tick swept movement；零 tick 不动；身份和 catch-up 边界失败关闭；注册敌人的真实 blocking sweep 只交付一次伤害；contact 关闭并进入既有终态；终态后不再移动或伤害；完整、旧版、覆盖门和双目标构建通过。

未声明：可见返航动画、碰撞后的物理回飞、飞剑 Actor 在终态时立即回到环绕位置、真实按键手感、玩家镜头观感、音画反馈或人工玩法验收。当前 blocking/Recall 采用既有即时逻辑终态，Actor 保持在最后物理位置并由正常 Run teardown 回收。

下一轮 P21.9 应优先完成可见返航/复位与再次部署闭环，并沿用同一固定时间线、Host、command sequence 和 World lifecycle；不在 GameMode 或 PlayerController 建立第二套飞剑状态机。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-8-flying-sword-directed-runtime>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-8-flying-sword-directed-runtime/Docs/Report/Dev.D.UE.0.0.10.P21.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-8-flying-sword-directed-runtime/Docs/Log/Dev.D.UE.0.0.10.P21.8.r0_log.md>
