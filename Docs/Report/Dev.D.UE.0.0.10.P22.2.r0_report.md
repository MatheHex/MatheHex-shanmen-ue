# Dev.D.UE.0.0.10.P22.2.r0 Report

## 1. 结论

P22.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮把标准投掷物既有 Run Host 的权威终态回执投影到现有主 HUD 战斗提示栈。玩家现在可以区分普通命中、击破、命中但零伤害、命中阻挡、超出射程、飞行时间耗尽和中断；呈现层不重算伤害、不推进动作、不拥有计时，也不建立第二套战斗状态。

```text
Thrown-weapon Run Host terminal matrix:   4 Success / 0 Fail
Changed-file mapped regression:         527 Success / 0 Fail (27 healthy logs)
Changed-file regression coverage:       PASS (12 files / 7 rules / 27 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 90 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明权威终态到 HUD 数据链、文字分类、布局容量和回归边界闭合，不宣称最终字体、动效、停留时长或玩家体验已经人工验收。

## 2. 玩家可见结果

| 权威终态 | HUD 文本 | 语义 |
|---|---|---|
| committed impact，伤害大于 0 | `飞刀 · 命中 · -N.N` | 实际扣除生命值 |
| committed impact，目标生命值归零 | `飞刀 · 击破 · -N.N` | 本次命中造成击破 |
| committed impact，伤害为 0 | `飞刀 · 未造成伤害` | 接触成立但未扣血 |
| blocking miss | `飞刀 · 命中阻挡` | 世界阻挡且没有伪造伤害 |
| range expired | `飞刀 · 超出射程` | 直线投掷物超过最大距离 |
| flight-time expired | `飞刀 · 落空` | 抛物线飞行时间自然结束 |
| interrupted | `飞刀 · 已中断` | 动作被权威生命周期中断 |

普通命中与击破显示 `FShanmenVitalityCommitReceipt` 的实际 Applied Damage，而不是请求伤害或重新计算值。零伤害仍属于 Impact，不会被误报成落空。

## 3. 单一事实链

```text
existing projectile callback / lifetime boundary
  -> existing ThrownWeapon Run Host terminal receipt
  -> Product Lifecycle read-only getter
  -> immutable TerminalFeedbackPresentation
  -> existing MainHUD combat hint stack
  -> existing Canvas renderer
```

Impact 只有在 World Delivery 为 Delivered、生命值提交状态为 `Committed` 且回执有效时才可投影。无效回执、非权威提交结果和不一致数据全部失败关闭。呈现对象只携带 Kind、LaunchId、Applied Damage、Defeated 与规范文本。

## 4. HUD 集成

终态行复用现有提示栈，没有新增面板或 Widget。规范顺序仍为轨迹模式、弧线高度、目标、输入提示、预发手势，终态反馈固定在最后一行：

- 直线模式由原 1 行扩展为有终态时 2 行；
- 抛物线模式最大由 5 行扩展为 6 行；
- 标准和紧凑布局均完整保留全部行，不截断终态；
- 旧四参数 `TryCompose` 继续可用，并转发一个无效终态，既有调用行为不变；
- 命中、击破、零伤害、阻挡、过期和中断分别使用现有 Canvas 渲染器中的明确色调。

终态回执由生命周期保留到 Host reset 或下一次动作接管；没有为了 HUD 新建 Tick、Timer 或延迟任务。

## 5. 终态矩阵证明

Run Host 专项 `4/0` 在生产路径上证明：

1. 中断生成唯一且可读的中断反馈；
2. 普通命中读取精确 `0.7` Applied Damage，重复投影确定一致；
3. 同一终态可作为直线提示栈第二行，Kind、Tone 和文本一致；
4. 临时 GamePreview World 中的致死命中走真实敌人生命值/死亡路径并显示击破；
5. 已提交的零伤害仍显示“未造成伤害”；
6. 世界阻挡显示“命中阻挡”，Applied Damage 保持 0；
7. 射程耗尽显示“超出射程”。

Run Command 专项额外证明抛物线 Flight Time Expired 显示“落空”。布局与提示栈专项分别为 `3/0` 和 `3/0`，验证六行上限、紧凑边界、规范排序与旧调用兼容性。

## 6. 首次失败与根因修正

首个致死测试使用无 World 的 transient 敌人。真实死亡路径会在 `demo_mapEnemyCharacter.cpp:650` 请求 World Timer，因夹具缺少运行世界触发访问冲突；日志在 `1 Success / 0 Fail` 后异常终止。修正没有绕开死亡逻辑，而是建立最小 GamePreview World 并注册真实敌人。

第一次 World 修正后，敌人有 World，但投掷物 Carrier 仍来自 transient package，导致致死飞行无法启动，测试以普通断言失败 `3/1`。随后让该夹具通过既有 `SpawnStagedCarrier` 在同一 World 生成 Carrier。一次接线编辑误把 World 参数传给普通夹具重载，Editor 编译按 C2039 失败；修正精确重载后重新构建和测试。

一轮广域诊断达到 `727 Success / 0 Fail` 后主动停止，没有自然终止标记，因此不计为通过。最终按改动文件映射运行 27 个精确组。门禁自测的第一份输出只到 142 个用例且缺最终汇总；完整重跑发现 HUD fixture 少了一份 Input Choice 日志，补齐后自然完成 `439/439`。所有失败与不完整日志均保留，没有覆盖成成功证据。

## 7. 改动文件回归

最终 27 份日志都有且只有一个 `Automation RunTests` 命令、至少一个成功结果、零失败、零 Fatal/Unhandled/Ensure，并包含 UE 5.8 原生成功终止标记。合计 `527 Success / 0 Fail`。

覆盖门结果：

```text
REGRESSION_COVERAGE: PASS Changed=12 Rules=7 Required=27 Logs=27
```

门禁日志为 10,007 bytes，SHA-256 `280909942CC0DA373F5607D49EF8AFFC9D46E7D9C6B1DB049A445EBDEF0E2E70`。完整门禁自测为 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

关键终态矩阵日志为 262,201 bytes，`4/0`，SHA-256 `24C89135AC656837EE01F02B517E1A37EC966BBCF334DD3DFFA77EE32E9D08F9`。27 份映射日志总计 7,563,241 bytes。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 90 / 165.31s | `22BF077EDEDC088DE3F06D80BF589457BBED842BB343491CDCE66FA956E3A330` |
| Editor Development final | Succeeded / native 0 | 0 / 1.26s | `6B450DC28DA63038AD4E6339A189D3DD4DA7A30AA994E4A9CD86C5D4782BDD50` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,555,584 bytes；SHA-256 `A2EE2AA0FE9ED9CAC337DE1A707CC21A16BB533F601A893EFA95DCC4121D0892`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,752,512 bytes；SHA-256 `2BE5DD08B4F12084303233449630DD805DCE544E01896D40F40E97A9C8BC84F2`。

实现/测试/门禁映射 diff（不含本 Report/Log）为 12 files / 703 insertions / 45 deletions。317 条新增生产行中 Timer、SetTimer、Tick、RNG、ApplyDamage、TakeDamage、SpawnActor 与 Destroy 均为 0；`git diff --check` 为 0；验证后相关运行进程数为 0。

## 9. P/F 边界

PASS：七类玩家可见终态均由既有权威回执投影；Applied Damage 来自 committed vitality receipt；致死判断来自提交后生命值；零伤害与落空分离；既有 HUD 顺序、紧凑布局和无终态调用保持兼容；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：最终 UI 美术、动画、声音、文本本地化、消息淡出策略、多人网络同步或真实地图可读性已经验收。本轮没有改变投掷物伤害公式、碰撞、库存、目标选择、生命值、死亡、Run、输入或计时权威。

## 10. 提交边界与 GitHub

本阶段只提交 12 个实现/测试/流程文件、本 Report 与本 Development Log，共 14 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.2` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback/Docs/Report/Dev.D.UE.0.0.10.P22.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-2-thrown-weapon-terminal-feedback/Docs/Log/Dev.D.UE.0.0.10.P22.2.r0_log.md>
