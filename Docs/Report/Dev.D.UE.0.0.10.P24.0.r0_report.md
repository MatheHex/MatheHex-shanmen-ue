# Dev.D.UE.0.0.10.P24.0.r0 Report

## 1. 结论

P24.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.10 已有但没有玩家入口的剑气命令链接入正式主循环：玩家现在可通过可重映射的 `B` 键发射剑气；若已有冻结的繁忙请求，同一按键优先重试该请求。主 HUD 显示当前绑定键，并在 2.25 秒内反馈释放、繁忙重试、界面锁定、剑类装备无效、攻击力不可用或一般不可用状态。实现复用现有 Run、物品、属性、输入、命令、投射物与伤害权威，没有增加第二套状态所有者。

```text
Sword Qi physical input:                 5 Success / 0 Fail
Complete Shanmen.0_0_10 regression:  1,277 Success / 0 Fail
Complete demo_map regression:        1,330 Success / 0 Fail
Changed-file regression coverage:       PASS (20 paths / 9 rules / 98 groups)
Regression gate self-test:               PASS 442/442
Game + Editor Development:               PASS (both native 0)
git diff --check:                         PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明最终编译二进制上的无头输入路由、迁移、冻结重试、界面锁、HUD 状态和全量回归，不声明玩家手感或视觉验收已经完成。

## 2. 玩家侧变化

- 默认按 `B` 发射剑气，设置中的“剑气发射 / 重试”可与其它正式动作一样实时重映射；
- 可发射时，输入只在 Issue 路径采样角色当前位置上方 50 UU 和当前有效瞄准方向；
- 产品繁忙并保存冻结请求时，再按同一键走 Retry，不重新采样位置、方向或事件身份；
- 活跃 UI 锁会阻止产品调用和身份消耗，但仍给出可读 HUD 反馈；
- HUD 顶部输入提示显示当前剑气按键，短时反馈明确区分成功、繁忙、UI 阻塞、装备和攻击力问题。

## 3. 单一权威与路由

`Ademo_mapPlayerController` 只负责把物理脉冲转换成现有命令。它从 `Ademo_mapGameMode` 读取 P18.10 的 availability projection：`CanRetry()` 时选择 Retry，否则仅在 `CanIssue()` 时捕获 Issue。命令继续由既有 `Fdemo_mapShanmenSwordQiAvailabilityCommandRouter`、`Fdemo_mapShanmenSwordQiCommandEventOwner` 和产品输入链处理。

Retry 的空间样本来自命令所有者保存的不可变请求；控制器传入的采样闭包不会在 Retry 中执行。Issue 才读取 Pawn 与既有最后有效瞄准方向。没有复制或旁路以下权威：Combat Run、剑类装备授权、攻击力、资源事务、投射物生命周期、命中与伤害结算。

HUD 只读取控制器保存的最后一次路由结果与短时显示期限。该结果不是新的玩法权威，不参与重试判断、产品决策或持久化。

## 4. 输入注册与迁移

正式输入注册表从 31 项增加到 32 项，新增 `SwordQi`，默认键为 `B`，仅绑定 Press。输入配置版本从 9 升至 10。

版本 9 配置迁移时：若 `B` 未占用，剑气获得默认 `B`；若用户已经把其它动作改到 `B`，既有覆盖保持不变，剑气通过既有冲突规避获得可用键。自动化夹具验证了 `Interact=B` 时剑气稳定迁移到 `G`，并保持 32 项完整、无重复键。

所有依赖精确动作数量或输入配置版本的既有测试夹具同步更新。提交前扫描确认生产与测试源码中不再残留旧的 31 项精确断言。

## 5. HUD 与失败关闭

主 HUD 使用输入设置的实时值显示剑气键，不硬编码 `B`。反馈颜色和文案由现有命令结果确定：

- 接受：`SWORD QI · RELEASED`；
- 已保存或仍可重试：`SWORD QI BUSY · <Key> RETRY`；
- Gameplay 输入锁：`SWORD QI · BLOCKED BY ACTIVE UI`；
- 剑类授权失败：`SWORD QI · EQUIP A VALID SWORD`；
- 攻击力快照不可用：`SWORD QI · ATTACK POWER UNAVAILABLE`；
- 其它带诊断的失败：`SWORD QI · UNAVAILABLE`。

缺少产品 GameMode、投影失败或当前不在可发射 Combat Run 时均失败关闭，并保留诊断；不会伪造成功、消费事件身份或创建投射物。

## 6. 聚焦自动化与修复记录

新增 `Shanmen.0_0_10.Product.SwordQiPhysicalInput` 五项：

- `RegistryDefault`：32 项注册表、默认 `B`、Press-only 与标签；
- `VersionNineMigration`：空闲键和冲突键两种 v9→v10 迁移；
- `PressOnlyBinding`：一次 Press 只调用一次，Release 不重复；
- `LiveRemapAndLock`：重映射后旧键失效、新键即时生效；
- `IssueRetryAndLock`：Ready Issue、HostBusy 冻结 Retry、取消回 Ready、UI 锁不消费身份。

最终聚焦日志为 262,715 bytes，5/0，含成功终止标记，SHA-256 `C2F4FFB1EE301E0173BE5BD86C9EC86D0D8D89A833D1E9775F999BF302AE350E`。

开发中保留了三类首轮失败证据：

1. 初始世界夹具没有 GameInstance，`SetGameMode` 触发访问异常，原生退出码 3；日志 259,341 bytes，SHA-256 `9F015D203879176C92AC4A434DF08FB9C2E0A0997C12A813363D142849411315`。夹具改为由 rooted transient GameInstance、WorldContext 和 World 共同拥有生命周期。
2. 修复夹具后为 4/1，因为命令所有者与 CombatRunCoordinator 没有绑定同一 RunId；日志 263,822 bytes，SHA-256 `FBB54D434FAB4ED74A59F28E1029553D9710C411B208D5E7BF6FCB0936F85A7E`。同步同一 Run 与 Health 后，冻结 Retry 契约通过。
3. 首次完整 `demo_map` 回归为 1,329/1，唯一失败是 P7 集成测试仍断言 31 项；日志 1,649,333 bytes，SHA-256 `BCC3E2967FD2D8F7720EB80F743B73FFA315C1C027BDD969ABF1BCF0F9DE178D`。修正陈旧断言后为 1,330/0。

## 7. 全量回归与改动映射

| Evidence | Success | Fail | Duration | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 75m05s | 1,894,822 | `B9920B91DC184C6A21EA0AFB5AAB5C9F5346E0896637C912075965DD76E6FEA4` |
| `demo_map` | 1,330 | 0 | 约 1m07s | 1,648,966 | `DB74F66D03E7629D97C1080DC7B8E521452C57274B89B012A2AAFDD4DF85B2A6` |

两份日志均有唯一 RunTests 命令、0 Fail、0 fatal/unhandled/ensure、成功终止标记和原生退出码 0。

当前 0.0.10 全树已经包含大量持久化、恢复与多代轮换测试；本机完整 1,277 项实测约 75 分钟。因此早期小规模 106 项约一秒的成本数据不能代表当前全树。日常开发应继续先跑聚焦组，再由改动映射决定是否需要完整父树；不能把当前全树描述为零成本。

最终 20 个改动路径命中 9 条规则并要求 98 个测试组。两份完整日志覆盖全部要求，结果为 `PASS Changed=20 Rules=9 Required=98 Logs=2`。覆盖日志 12,177 bytes，SHA-256 `C2752B6137BDFF35DB82B300C7B7E47B20451641EB2005DF630E30C1544E844E`；覆盖器自测补齐剑气 HUD 依赖后为 442/442，日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

| Evidence | Result | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P24.0_EditorBuild_final.log` | PASS / up-to-date / native 0 | 0 | 1,030 | `1BE4DC899F9E01E39B0B550124C9CB16CF592FE648C890EDB198C1D898D4460E` |
| `P24.0_GameBuild_final.log` | PASS / native 0 | 55 | 22,559 | `9323A1A4AB7C9A2FE935E0A2B96928D4FA98E6662B989631F92691A1A871956A` |

Game 构建期间主机提交内存接近上限，UBA 自动回收并重试若干编译进程；55 个动作最终全部完成，结果为 Succeeded，不是源码或链接失败。

最终 `demo_map.exe` 为 359,676,928 bytes，SHA-256 `6D242E35FC10C22B328F0119066C1451D789B936CA4C67A1483FBD5034D02CF8`；`UnrealEditor-demo_map.dll` 为 18,939,392 bytes，SHA-256 `4475186483EFF11580647034C2A35774716EAB345FC833D0332BB167B0612206`。

非文档增量为 20 个文件、`+690/-26`：生产代码 7 文件 `+243/-4`，测试 11 文件 `+438/-21`，回归流程 2 文件 `+9/-1`。映射 JSON 可解析；旧动作数量扫描、0.0.10 报告技术信封扫描和 `git diff --check` 均通过。没有新增资产、模块、UCLASS/USTRUCT、Actor、Subsystem 或持久玩法状态。

## 9. P/F 边界

PASS：默认键与实时重映射；Press-only；v9→v10 冲突迁移；Ready Issue；冻结 HostBusy Retry；取消后恢复 Issue；UI 锁不消费身份；HUD 反馈；聚焦 5/0；两棵完整测试树 2,607/0；映射覆盖、自测和双构建通过。

未声明：真实键盘输入、实际装备剑、属性快照、投射物视觉、命中目标、动画、音效、HUD 字体与布局、帧时序或玩家手感已经人工验收。Game 目标只完成构建，没有启动产品。下一轮若继续剑气，应优先建立真实产品配置下从物理按键到投射物生成/命中回执的无头端到端证明，而不是增加新包装层。

## 10. 提交边界与 GitHub

基线提交为 `1ddb5cd5ec3688159ff62f0e85dbb88ddcd6b899`。本阶段只提交 7 个生产文件、11 个测试文件、2 个回归流程文件、本 Report 与本 Development Log，共 22 个文件。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.0` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-0-sword-qi-player-loop>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-0-sword-qi-player-loop/Docs/Report/Dev.D.UE.0.0.10.P24.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-0-sword-qi-player-loop/Docs/Log/Dev.D.UE.0.0.10.P24.0.r0_log.md>
