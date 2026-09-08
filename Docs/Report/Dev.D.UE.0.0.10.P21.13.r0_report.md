# Dev.D.UE.0.0.10.P21.13.r0 Report

## 1. 结论

P21.13 在 P 阶段边界内完成，结论为 **PASS**。

本轮把飞剑真实命中结果接入既有飞剑 HUD 状态行。只有现有权威伤害链完成一次新的 `Committed` 生命值提交后，返航中的飞剑才会显示命中伤害、击破或未造成伤害；没有创建第二套伤害结算、目标权威、HUD 面板或计时系统。

```text
Controlled-weapon focused:                    62 Success / 0 Fail
Shanmen.0_0_10 full:                        1247 Success / 0 Fail
Required 0.0.9B compatibility groups:        217 Success / 0 Fail
Regression coverage:                          PASS (Changed=10 / Rules=6 / Required=83 / Logs=6)
Regression gate self-test:                    PASS 437/437
Game + Editor Development:                    PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实键鼠、截图、Smoke、Cook 或 Package。因此证明的是权威回执路由、Actor 投影、HUD 文案、幂等/失败关闭、回归兼容与编译闭合；不宣称实际屏幕上的视觉可读性、动画、音效或玩家手感已经人工验收。

## 2. 玩家可见结果

既有飞剑状态行在 `Returning` 阶段可追加一次当前激活的命中结果：

- 普通命中：`飞剑 · 返航 · 命中 -12.5`；
- 目标生命值归零：`飞剑 · 返航 · 击破 -7.0`；
- 命中已提交但实际伤害为零：`飞剑 · 返航 · 未造成伤害`。

既有 `屏外`、`近身目标 N`、阶段状态与输入提示仍由原 planner 组合。没有新增并行面板；HUD 只读取当前飞剑 Actor 已接受的表现快照。

## 3. 单一权威链

`Fdemo_mapShanmenControlledWeaponDirectedTimelineResult` 现在保留本次定向时间线产生的精确 `DeliveredImpacts`，而不再只保留计数。结果自检要求：

- 数组数量与 `DeliveredImpactCount` 一致；
- 每项均为成功投递，Run、物品、目标、Impact 与生命值回执身份一致；
- 同一结果中不允许重复 Impact ID。

`Ademo_mapGameMode` 只把当前 lifecycle 物品的全新 `Committed` 结果交给当前飞剑 Actor。`AlreadyCommitted`、其它物品、错误身份或无 Actor 的结果都不会伪装成玩家反馈。

`Ademo_mapShanmenControlledWeaponActor` 仅保存表现所需的 Impact ID、Activation ID、目标 ID、实际伤害和提交后生命值。它不写目标生命值，不重新计算伤害，也不成为 gameplay 权威。

## 4. 生命周期与失败关闭

- 精确相同的已提交回执可幂等重放，内容冲突则拒绝；
- Actor 必须与同一 Run、物品和来源 Actor 绑定；
- 若已有飞行 read model，Activation ID 必须完全一致；
- 新激活 read model 会清除上一激活的命中反馈；停用产品碰撞也会清除；
- HUD 反馈只允许出现在 `Returning`，其它阶段携带反馈会失败关闭并清空复用输出；
- 隐藏反馈不得携带残留伤害或击破位；NaN、负伤害和非法生命值被拒绝；
- 零伤害提交仍被诚实表达为“未造成伤害”；只有正伤害且提交后生命值为零才显示“击破”。

## 5. 自动化与首次失败

专项组 `Shanmen.0_0_10.Product.ControlledWeapon` 最终执行 62 项，全部成功。真实阻挡接触 fixture 已由通用 `AActor` 升级为实际 `Ademo_mapShanmenControlledWeaponActor`，覆盖：

- RunHost 保留一份身份完整的权威投递；
- 同一个实际飞剑 Actor 接受并幂等重放该回执；
- Returning HUD 生成精确命中文案；
- 新 Activation 清除旧反馈；
- terminal 后续 tick 不再产生投递；
- 纯 planner 覆盖普通命中、击破、零伤害、阶段边界、隐藏残留与 NaN。

首轮专项为 `61 Success / 1 Fail`。失败来自新增测试把多项身份检查和伤害浮点比较合并在一个断言中；拆分后所有身份/状态检查均通过，唯一失败是默认近似比较容差小于生命值差值的浮点舍入误差。测试改为逐字段断言，并为伤害比较显式使用 `KINDA_SMALL_NUMBER`；没有因此放宽产品身份或状态边界。隔离用例随后 `1/0`，最终专项 `62/0`。首次失败日志完整保留。

全量 `Shanmen.0_0_10` 从 P21.12 的 1246 项增加到 1247 项，日志从 `2026.09.08-15.28.12:092` 至 `2026.09.08-16.43.40:580` 自然完成；有一个实际 RunTests 命令、一个成功终止标记，原生退出码为 0。

## 6. 测试与门禁证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused final | 62 | 0 | 331,930 | `F6F0342EE3AFF582C7E9DED93DD75169C5D17F85D17F990725730404C3EC0444` |
| `Shanmen.0_0_10` full | 1247 | 0 | 1,907,406 | `A6EAB86B5BB792238EAF2B2D27C199909AAB08FC8A099BA9C35BE3969F34E5A9` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 305,825 | `0902BB7976EDBD476270954A377DE57E8D93FBA967ED2DEA5E8B5A44B6068DA7` |
| `demo_map.InputRestore` | 101 | 0 | 395,101 | `AE81FC7FF8AA9A5E461E4E55C258F388094869EF87628381A1AAFDB63B08184F` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 309,576 | `32518FFA1D97C795F036CB7A5FDD02F21484FF3E6D46B0AE7D50A37999981F88` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,501 | `840A61F6A8C6534385EED3E2349FC828B8B1DCB01E5DA879B66A320254D26E3F` |
| `demo_map.V3.Attributes` | 4 | 0 | 264,159 | `650069CB2379C0C0F61EFAA0F94001EF2020C257F60F913CBE1132526FCC2FE1` |

七份最终自动化日志各有一个实际 RunTests 命令和一个成功终止标记；Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

- regression gate：`PASS Changed=10 Rules=6 Required=83 Logs=6`，10,143 bytes，SHA-256 `4B9EA69A1A3FA430100F0D13EA0C75E6862FDC06DB95800D877BDDA79528ACAA`；
- gate self-test：`PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

首次失败证据：

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Focused initial | 61 | 1 | 332,932 | `ED5E156F62A0C953FB6EAA9D6DFD5921733D7A563FF9F1F0724DC6926DD7A3BE` |
| Isolated diagnostic | 0 | 1 | 262,407 | `5BE42AED1F3CB274A70D0033FED9C2FADCCB8C2D1CEDD514180290BCF822C7A1` |
| Isolated after explicit tolerance | 1 | 0 | 262,030 | `83D816D0B3BFEF159B9EF89F8A7C39C5E8D81A2A8DD4AEC458132288B0CF569A` |

## 7. 构建、产物与静态边界

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 62 / 54.99s | 6,555 | `A37829AD8381BAE49053903A5053EA4C2279DC95414B91D6729270456E20E516` |
| Game Development final | Succeeded / native 0 | 61 / 55.05s | 6,340 | `0A170AF742E84B6F4DA1ED86E6042624B4451EE7DF53DB4B4F24528A61D54EA4` |
| Editor Development final | Succeeded / native 0 | 0 / 0.98s | 1,021 | `E3AD98DC71094A7B84CCE49E49AFE77D6D75CD3DEE6CA66D1132F5817F49B5FF` |

最终产物：

- `demo_map.exe`：359,531,520 bytes，SHA-256 `28FF761387F01C945E8E755F7E659BFFC07E6443F3F5E928775B9B5F246BE821`；
- `UnrealEditor-demo_map.dll`：18,722,816 bytes，SHA-256 `4FA50090721467C8B733E17C0F220984F4358E273703FDEC532633DF1734BEFD`。

静态结果：实现/测试 diff 为 10 files、`+540 / -24`；8 个生产文件共有 331 条新增行，Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；`git diff --check` 原生退出码 0，仅有工作树 LF→CRLF 提示；验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0；103 个用户原有 untracked 文件保持未暂存。

## 8. P/F 边界与后续方向

PASS：全新权威命中可以沿既有 RunHost → GameMode → 飞剑 Actor → HUD 单向链显示；重复/冲突/跨 Run、物品、目标与 Activation 数据失败关闭；零伤害与击破语义准确；下一激活不会继承旧命中；focused、full、旧版兼容、覆盖门、门禁自测和双目标构建全部通过。

未声明：真实 UI 中的排版、停留时长、动画/VFX/音效、命中特效强度、玩家感知、PIE/Standalone 或打包体验已验收。

下一轮宜继续加强同一条飞剑玩家闭环，优先增加不引入新权威的目标选择确认或视觉/音频命中反馈；若要证明实际画面体验，需要另行执行被本轮 P 边界排除的真实 UI 验收。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-13-flying-sword-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-13-flying-sword-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P21.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-13-flying-sword-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P21.13.r0_log.md>
