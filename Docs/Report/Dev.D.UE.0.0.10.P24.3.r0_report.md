# Dev.D.UE.0.0.10.P24.3.r0 Report

## 1. 结论

P24.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮修复了一条实际可复现的剑气产品生命周期缺口：投射物进入 `Impact`、`BlockingMiss`、`RangeExpired` 或 `Interrupted` 终态后，生产代码没有调用终态退役；下一次剑气命令因 Session 仍非空而持续得到 `HostBusy`。现在 GameMode 的既有帧所有者会在终态出现时自动、仅一次退役权威回执，并把结果交给 PlayerController/HUD。即使 HUD 暂时不可用，产品会话也会先解除，不让显示层阻塞下一次剑气。

```text
Sword Qi physical input:                 5 Success / 0 Fail
Sword Qi complete product family:       38 Success / 0 Fail
Complete Shanmen.0_0_10 regression:  1,277 Success / 0 Fail
Complete demo_map regression:        1,330 Success / 0 Fail
Changed-file regression coverage:       PASS (6 paths / 4 rules / 96 groups)
Regression gate self-test:               PASS 442/442
Game + Editor Development:               PASS (both native 0)
git diff --check:                         PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实键盘、截图、Smoke、Cook 或 Package。本轮证明最终编译代码上的无头帧推进、终态退役、HUD 文本选择和下一次命令解锁，不声明实际画面位置、字体、颜色、动画、音效或手感已经人工验收。

## 2. 根因审计

P24.2 已证明物理 `B` 可以发射真实产品投射物并把接触伤害提交到 M01 生命值权威，但测试在命中后直接调用 `TryRetireTerminal()`。生产代码中没有对应调用者。

`Fdemo_mapShanmenSwordQiProductSession::TryRoute()` 对任意非空 Host 均失败关闭为 `HostBusy`，Terminal 也属于非空。因此首枚投射物完成后，若没有测试辅助或 Run teardown，后续物理 `B` 无法开启新的 Issue。问题不是显示缺文案，而是产品生命周期没有在正式帧所有者处闭合。

本轮没有新增 Manager、Subsystem、Actor 或重复产品 Host。修复落在已经拥有 Sword Qi ProductController 的 `Ademo_mapGameMode`，并复用现有 PlayerController 与 HUD 表现入口。

## 3. 自动终态退役

`Ademo_mapGameMode::Tick()` 在既有 weapon-guard reconciliation 后调用 `RetireSwordQiTerminalForPlayerFeedback()`：

1. 仅在 Sword Qi Controller active、valid 且 Session 为 Terminal 时继续；
2. 通过现有 `TryRetireTerminal()` 取得权威 `Fdemo_mapShanmenSwordQiTerminalReceipt`；
3. 优先把回执发布给本地 PlayerController；
4. 发布成功记录 `TerminalPresented`；发布失败记录 `TerminalRetiredWithoutHUD`；
5. 无论 HUD 是否存在，退役已经完成，下一次命令不会被旧终态永久阻塞。

在飞状态不会被该路径触碰。聚焦测试在投射物飞行中显式推进一次真实 `GameMode::Tick(0.0f)`，确认 Session 仍为 InFlight 且没有伪造终态反馈。

## 4. 玩家可观察反馈

PlayerController 只接受有效终态回执。`Impact` 还必须包含已提交、有效的生命值回执和有限非负伤害；不满足时失败关闭。可见文本为：

| Terminal | HUD 文本 |
|---|---|
| Impact，伤害大于 0 | `SWORD QI · HIT · -<damage>` |
| Impact，目标生命值归零 | `SWORD QI · TARGET DOWN · -<damage>` |
| Impact，实际伤害为 0 | `SWORD QI · NO DAMAGE` |
| BlockingMiss | `SWORD QI · BLOCKED` |
| RangeExpired | `SWORD QI · OUT OF RANGE` |
| Interrupted | `SWORD QI · INTERRUPTED` |

终态反馈沿用既有剑气输入反馈的 2.25 秒可见窗口，并优先于 `RELEASED`、busy 或 blocked 等输入反馈。新的剑气输入会清除旧终态反馈，避免旧结果覆盖新动作。HUD 对命中使用绿色、中断使用灰色、阻挡或超距使用琥珀色；没有引入新的 Widget、资产或计时器。

## 5. 产品闭环证明

升级后的 `IssueRetryAndLock` 用例通过正式 `GameMode::Tick()` 证明：

- 在飞剑气经一帧推进后仍保持 InFlight；
- 首枚真实投射物接触 M01 目标后，下一帧自动退役为 `Impact`；
- HUD 收到同一权威回执并显示 `SWORD QI · HIT · -0.58`；
- 输入反馈被终态反馈取代；
- 目标生命值仍精确减少 `0.58`，权威修订增加 1；
- 冻结 Retry 的新输入清除旧命中文案，不改写冻结起点、方向或 AttackPower；
- Retry 中断后下一帧显示 `SWORD QI · INTERRUPTED` 并清空 Session；
- UI 结算锁不消耗命令身份；解除锁后，新 `B` 生成新的 Issue，且不是复用旧 Intent；
- 新 Issue 再次中断并自动退役，Run teardown 最终统计 3 个 committed events、3 个 captured intents、3 个 processed commands。

测试不直接调用私有退役辅助函数，也不再用测试侧 `TryRetireTerminal()` 完成产品流程。

## 6. 聚焦验证

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 269,746 | `85B7FC28D71DF2158ABF2FEA64F66F3C5460AC9E5D5099AD2D95F122300719BE` |
| `Shanmen.0_0_10.Product.SwordQi` | 38 | 0 | 306,454 | `9793153405D05676B72B0A12746F9CFD110CC5B13DD7162330BF289AC0C48195` |

首轮实现后的物理输入测试即为 5/0，原始日志 271,219 bytes，SHA-256 `36F3808D56F4E3E1EC8FF3FD1EEA2A31DAB4DC9F868727EF1A5738653FA4987F`。随后代码审查增加“飞行中帧不可误退役”断言，重新编译并得到表中的最终 5/0。没有源码构建失败或目标自动化失败需要隐藏或覆盖。

最终日志记录三次真实 `TerminalPresented`：一次 `Impact`、伤害 `0.580`，两次 `Interrupted`、伤害 `0.000`。两组均有 0 Fail、成功终止和原生退出码 0。

## 7. 完整回归与改动映射

| Evidence | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 63m08s | 1,942,538 | `CA125ED22B8C88264CE3FF5E9C67B3ABCA2BA32E94A9A81555FF9776DF5D969B` |
| `demo_map` | 1,330 | 0 | 约 37s | 1,655,350 | `5E5E264927FF47274DB8A8DA1B389F7681B4BF1229DF094CA1CE77063998B80A` |

两棵完整测试树合计 2,607/0。每棵均由单一 UnrealEditor-Cmd 实例自然清空；没有重启、拼接或删改失败行。两份日志均为 native 0，Fatal、Unhandled、Assertion 与 Ensure 为 0。

按改动文件推导的覆盖结果为 `PASS Changed=6 Rules=4 Required=96 Logs=2`。覆盖日志 11,990 bytes，SHA-256 `82D22A740D14C46ACDF177C36A8AE12C895B0F8DC6872A91AD005DC3CD0185C8`；覆盖器自测为 442/442，日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

| Evidence | Result | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P24.3_EditorBuild_initial.log` | PASS / native 0 | 47 | 5,192 | `70C43FE980C23FE5A91DC44C1E41B6982424D30760FCE5CD9C0715D1D10F273F` |
| `P24.3_EditorBuild_inflight.log` | PASS / native 0 | 4 | 2,393 | `E5AFA0E0F2B4D6A1CBA7B5FA336C8BFD8C3756EA27FB744D1FAC831C9B284015` |
| `P24.3_EditorBuild_final.log` | PASS / native 0 | 0 / up to date | 1,040 | `771222B28FE15E0AA1765860CFF613FFCB795B8FDC3A56B2CCDC529D6C60A00A` |
| `P24.3_GameBuild_final.log` | PASS / native 0 | 46 | 5,451 | `DB9ADB532805528A17B570D525832DF507B9BE246019ED984D56986A20801D4F` |

最终 `demo_map.exe` 为 359,687,680 bytes，SHA-256 `44688C33FB960399AFD745786450DE4CBCB39501280E9EA11F890EAFF4C19882`；`UnrealEditor-demo_map.dll` 为 18,950,656 bytes，SHA-256 `740FE1B6DDB8E52EA8E54B030AB4A0B9FE05A160B02913A8B86876A6FB577694`。

非文档增量为 6 个文件、`+238/-25`；`git diff --check` 原生退出码 0。新增生产行对 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor` 与 `Destroy` 的命中均为 0。没有新增模块、资产、UCLASS/USTRUCT、Actor、Subsystem、输入动作或存档字段。验证结束后项目 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：正式帧所有者自动退役；InFlight 不误回收；Impact/Interrupted 权威回执进入 HUD；0.58 伤害、生命值差与修订保持一致；终态文案优先；新输入清除旧反馈；终态后新 Issue 可立即开启；冻结 Retry、UI 锁和三次命令清理保持；聚焦 43/0；完整回归 2,607/0；改动映射、自测、双目标构建与静态检查通过。

未声明：真实键盘设备、真实场景碰撞或 sweep、渲染布局、字体、颜色、动画、特效、音效、不同分辨率或手感已经验收。后续若继续剑气，应优先补可见产品行为或正式 F 阶段验收，不复制 ProductController、Session、RunHost、WorldDelivery 或 HUD 通道。

## 10. 提交边界与 GitHub

基线提交为 `f73b45afbe2c19a5cbe88ccfa0c823ff550acdfd`，工作分支为 `agent/0.0.10-p24-3-sword-qi-terminal-feedback`。本阶段精确提交 5 个生产文件、1 个测试文件、本 Report 与本 Development Log，共 8 个文件。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.3` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-3-sword-qi-terminal-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-3-sword-qi-terminal-feedback/Docs/Report/Dev.D.UE.0.0.10.P24.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-3-sword-qi-terminal-feedback/Docs/Log/Dev.D.UE.0.0.10.P24.3.r0_log.md>
