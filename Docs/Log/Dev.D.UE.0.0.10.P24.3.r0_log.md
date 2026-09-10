# Dev.D.UE.0.0.10.P24.3.r0 Development Log

## 1. 目标

- 从 P24.2 后的可验证事实审计剑气命中终态是否在生产流程中自动闭合；
- 修复首枚剑气结束后 Session 停留 Terminal、后续命令永久 `HostBusy` 的生命周期缺口；
- 让玩家通过既有 HUD 看见权威命中、击倒、零伤、阻挡、超距与中断结果；
- 保证显示失败不阻塞玩法进度，不新增第二套产品运行时；
- 按改动文件映射完成聚焦测试、两棵完整回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`f73b45afbe2c19a5cbe88ccfa0c823ff550acdfd`；
- 工作分支：`agent/0.0.10-p24-3-sword-qi-terminal-feedback`；
- 基线阶段：P24.2 已贯通物理 `B`、真实产品投射物、M01 Impact 与生命值提交；
- 开始时 tracked tree clean，保留用户 103 个未跟踪文件；
- P24.2 测试仍手动调用 `TryRetireTerminal()`，因此先审计生产调用者，而非继续增加表现封装。

## 3. 根因与决策

全仓检索确认 `RetireSwordQiTerminal()` 只有公开转发和测试调用，没有生产帧调用者。ProductSession 在 Host 非空时拒绝新命令；Terminal Host 也非空。因此真实产品在首个 Impact、BlockingMiss、RangeExpired 或 Interrupted 后会保持终态，直到 Run teardown，期间后续输入持续 `HostBusy`。

责任归属选择现有 `Ademo_mapGameMode::Tick()`：GameMode 已拥有 SwordQiProductController，也已经按帧协调其它战斗权威。没有创建新的 terminal manager、ticker、timer、Actor 或 Subsystem。

## 4. 生产实现

### 4.1 GameMode 生命周期闭合

新增私有 `RetireSwordQiTerminalForPlayerFeedback()`：仅接受 active、valid、Terminal Session；调用现有 `TryRetireTerminal()` 取得权威回执；随后尝试发布到本地 PlayerController。发布成功记录 `TerminalPresented`，失败记录 `TerminalRetiredWithoutHUD`。

关键顺序是“先退役、后显示”。因此 HUD 缺失、Controller 缺失或回执无法表现时，旧终态仍已释放，玩法不会被 UI 反向阻塞。该函数在 GameMode 的既有 Tick 中、weapon-guard reconciliation 后执行一次。

### 4.2 PlayerController 回执表现

PlayerController 新增短生命周期的终态回执、文本、到期时间和 active 标志。`TryPresentSwordQiTerminalFeedback()` 先验证终态回执；Impact 额外要求生命值提交状态为 Committed、回执有效、伤害有限且非负。

文本分支为 HIT、TARGET DOWN、NO DAMAGE、BLOCKED、OUT OF RANGE 与 INTERRUPTED。终态沿用既有 2.25 秒输入反馈时长，成功发布时关闭旧输入反馈；任何新的剑气输入也会清除旧终态，避免跨动作残留。

### 4.3 HUD 复用

既有 `DrawSwordQiInputFeedback()` 扩展并重命名为 `DrawSwordQiFeedback()`。绘制顺序先检查终态，再回退到原输入反馈。Impact 使用绿色，中断使用灰色，阻挡/超距使用琥珀色。未新增 Widget、资产、布局系统或第二条 HUD 通道。

## 5. 测试升级

`IssueRetryAndLock` 不再在测试侧手动退役终态，而是调用正式 `GameMode::Tick(0.0f)`：

- 投射物仍在飞行时先推进一帧，确认 InFlight 保持且没有终态反馈；
- 命中后推进一帧，确认 Session 自动清空、Impact 回执进入 HUD、文本为 `SWORD QI · HIT · -0.58`；
- 保留 0.58 生命值差与 revision +1；
- Retry 输入确认清除旧终态，且继续使用冻结身份、起点、方向和 AttackPower；
- Retry 中断后推进一帧，确认 `SWORD QI · INTERRUPTED`；
- UI 结算锁仍不消耗命令身份；解除后再按 `B`，确认是新的 Issue、不是复用旧 Intent；
- 新 Issue 再次中断并自动退役；最终 teardown 计数调整为 3/3/3。

这一组合既证明实际缺口已关闭，也保护“帧所有者不能误收飞行中投射物”的反向边界。

## 6. 首次运行与审查修正

首轮生产实现编译为 47 actions、Result Succeeded；首轮物理输入自动化即为 5/0：

| Evidence | Result | Bytes | SHA-256 |
|---|---:|---:|---|
| `P24.3_EditorBuild_initial.log` | PASS / native 0 | 5,192 | `70C43FE980C23FE5A91DC44C1E41B6982424D30760FCE5CD9C0715D1D10F273F` |
| `P24.3_SwordQiPhysicalInput_initial.log` | 5/0 / native 0 | 271,219 | `36F3808D56F4E3E1EC8FF3FD1EEA2A31DAB4DC9F868727EF1A5738653FA4987F` |

代码审查随后发现需要显式保护 InFlight 反向边界，于是只增加一条真实 Tick 断言。增量 Editor 构建 4 actions、native 0，最终物理组仍为 5/0。没有源码构建失败或目标测试失败。

覆盖自检入口曾被误当作 UE Automation group 调用；该调用选择 0 项，因此立即拒绝作为证据。核验后运行实际 PowerShell 自检脚本，得到正式 442/442。这个过程没有被计作产品或测试 PASS，也没有修改断言来迁就结果。

## 7. 最终验证

### 7.1 聚焦组

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 269,746 | `85B7FC28D71DF2158ABF2FEA64F66F3C5460AC9E5D5099AD2D95F122300719BE` |
| `Shanmen.0_0_10.Product.SwordQi` | 38 | 0 | 306,454 | `9793153405D05676B72B0A12746F9CFD110CC5B13DD7162330BF289AC0C48195` |

### 7.2 完整回归

| Group | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 02:30:52.878–03:34:00.469 UTC | 1,942,538 | `CA125ED22B8C88264CE3FF5E9C67B3ABCA2BA32E94A9A81555FF9776DF5D969B` |
| `demo_map` | 1,330 | 0 | 03:34:41.567–03:35:18.906 UTC | 1,655,350 | `5E5E264927FF47274DB8A8DA1B389F7681B4BF1229DF094CA1CE77063998B80A` |

合计 2,607/0。两份完整日志均有 native 0 和 queue-empty 终止；Fatal、Unhandled、Assertion、Ensure 均为 0。

### 7.3 改动文件覆盖

六个精确改动路径触发 4 条映射规则和 96 个必跑组。两棵完整日志覆盖全部要求：`PASS Changed=6 Rules=4 Required=96 Logs=2`。覆盖日志 SHA-256 `82D22A740D14C46ACDF177C36A8AE12C895B0F8DC6872A91AD005DC3CD0185C8`；门禁自检 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_mapEditor Win64 Development`（initial） | Succeeded | 0 | 47 | `70C43FE980C23FE5A91DC44C1E41B6982424D30760FCE5CD9C0715D1D10F273F` |
| `demo_mapEditor Win64 Development`（in-flight assertion） | Succeeded | 0 | 4 | `E5AFA0E0F2B4D6A1CBA7B5FA336C8BFD8C3756EA27FB744D1FAC831C9B284015` |
| `demo_mapEditor Win64 Development`（final） | Succeeded / up to date | 0 | 0 | `771222B28FE15E0AA1765860CFF613FFCB795B8FDC3A56B2CCDC529D6C60A00A` |
| `demo_map Win64 Development`（final） | Succeeded | 0 | 46 | `DB9ADB532805528A17B570D525832DF507B9BE246019ED984D56986A20801D4F` |

最终二进制：

- `demo_map.exe`：359,687,680 bytes，SHA-256 `44688C33FB960399AFD745786450DE4CBCB39501280E9EA11F890EAFF4C19882`；
- `UnrealEditor-demo_map.dll`：18,950,656 bytes，SHA-256 `740FE1B6DDB8E52EA8E54B030AB4A0B9FE05A160B02913A8B86876A6FB577694`。

## 9. 静态与 P/F 边界

- 非文档增量：6 files、`+238/-25`；
- `git diff --check`：native 0；
- 新增生产行的 Timer、SetTimer、RNG、ApplyDamage、SpawnActor、Destroy 命中：全部 0；
- 新增模块、资产、UCLASS/USTRUCT、Actor、Subsystem、输入动作、存档字段：0；
- 验证结束后项目 UnrealEditor、UnrealEditor-Cmd、demo_map 进程：0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P 阶段证明无头正式帧会把 Sword Qi Terminal 退役成短时 HUD 反馈并解除下一次命令。F 阶段仍保留实际布局、可读性、颜色、真实碰撞、帧节奏、动画、音效和手感验收。

## 10. 提交与后续

精确提交 8 个文件：

- `Source/demo_map/demo_mapGameMode.cpp`；
- `Source/demo_map/demo_mapGameMode.h`；
- `Source/demo_map/demo_mapHUD.cpp`；
- `Source/demo_map/demo_mapPlayerController.cpp`；
- `Source/demo_map/demo_mapPlayerController.h`；
- `Source/demo_map/demo_mapShanmenSwordQiPhysicalInputTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.3.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.3.r0_log.md`。

用户 103 个未跟踪文件保持未暂存，原始验证日志保持本地忽略。后续阶段若继续剑气，优先安排真实可见验收或已有产品通道上的新行为，不增加重复 Manager、Session、RunHost、WorldDelivery 或 HUD 通道。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-3-sword-qi-terminal-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-3-sword-qi-terminal-feedback/Docs/Report/Dev.D.UE.0.0.10.P24.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-3-sword-qi-terminal-feedback/Docs/Log/Dev.D.UE.0.0.10.P24.3.r0_log.md>
