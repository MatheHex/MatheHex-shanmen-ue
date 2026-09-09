# Dev.D.UE.0.0.10.P24.0.r0 Development Log

## 1. 目标

- 审计 P18.10 剑气 availability command router 与现有产品链；
- 把剑气接入正式可重映射玩家输入和主 HUD；
- 保证同一按键自动选择新发射或冻结重试，并遵守 UI 输入锁；
- 不复制 Run、物品、属性、资源、投射物、命中或伤害权威；
- 按改动文件映射完成回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`1ddb5cd5ec3688159ff62f0e85dbb88ddcd6b899`；
- 工作分支：`agent/0.0.10-p24-0-sword-qi-player-loop`；
- 基线阶段：P23.4 神识多目标标记避让，P23 已关闭；
- 开始时 tracked tree clean，保留用户 103 个未跟踪文件；
- P18.10 已提供 availability projection、Issue/Retry 命令与命令事件所有者，但没有物理按键或主 HUD 消费者。

## 3. 设计决策

控制器不建立新的 Sword Qi Manager。每次物理脉冲先向 GameMode 请求现有 projection；pending retry 优先于 issue。Issue 的空间采样由现有输入适配器在需要时调用，Retry 只消费 owner 已保存的冻结请求。

HUD 反馈采用控制器上的短时结果快照，只服务显示，不参与产品决策。所有装备、攻击力、Run、资源、投射物和伤害判断继续由 P18.10 之前的权威链处理。

## 4. 实现

### 4.1 输入注册

- 新增 `Fdemo_mapInputActionIds::SwordQi`；
- 默认 `B`，Press-only，显示名“剑气发射 / 重试”，分类“战斗”；
- 精确动作数 31→32；输入配置版本 9→10；
- 更新所有精确数量与版本迁移夹具。

### 4.2 玩家路由

`Ademo_mapPlayerController::RouteSwordQiInput` 执行：

1. 获取产品 GameMode；
2. 投影当前 command availability；
3. `CanRetry` 时捕获 Retry，否则仅在 `CanIssue` 时捕获 Issue；
4. 通过既有 GameMode route 发送命令；
5. Issue 闭包采样 Pawn 位置 `+Z 50` 和 `GetLastValidAimDirection()`；
6. 保存结果供 2.25 秒 HUD 反馈。

物理处理器绑定 Press，不绑定 Release。测试计数仅在非 Shipping 构建存在。

### 4.3 HUD

主 HUD 的输入帮助行加入实时剑气按键。新增小型居中反馈板，按既有命令结果显示 released、busy/retry、active UI blocked、invalid sword、attack power unavailable 或 generic unavailable。

### 4.4 回归映射

把新测试文件纳入 `UnifiedInputRegistryAndBindings`，并把 `SwordQiPhysicalInput` 加入统一输入、GameMode、PlayerController 与 MainHUD 的必跑组。覆盖器自测增加对应 Sword Qi fixture，保持正例映射可执行。

## 5. 测试增量

新增 5 个无头自动化：

- 注册表默认值；
- 版本 9 两种迁移；
- Press-only 物理绑定；
- 实时重映射；
- Ready Issue、冻结 Retry、取消和 UI 锁。

最终聚焦结果：5 Success / 0 Fail，原生退出码 0，日志 262,715 bytes，SHA-256 `C2F4FFB1EE301E0173BE5BD86C9EC86D0D8D89A833D1E9775F999BF302AE350E`。

## 6. 首次失败保留与修复

| 阶段 | 结果 | 原因 | 修复 | SHA-256 |
|---|---|---|---|---|
| 初始聚焦 | native 3 | transient World 无 GameInstance，`SetGameMode` 访问异常 | 增加 rooted GameInstance、WorldContext、World 绑定和清理 | `9F015D203879176C92AC4A434DF08FB9C2E0A0997C12A813363D142849411315` |
| 夹具修复后 | 4/1, native 255 | command owner 与 CombatRunCoordinator 的 RunId 不一致 | 同步 Run 与 Health authority | `FBB54D434FAB4ED74A59F28E1029553D9710C411B208D5E7BF6FCB0936F85A7E` |
| 初始 demo_map 全树 | 1,329/1, native 255 | P7 集成测试残留 31 项断言 | 更新为 32 并扫描其它残留 | `BCC3E2967FD2D8F7720EB80F743B73FFA315C1C027BDD969ABF1BCF0F9DE178D` |

上述失败日志均保留在 `Saved/Codex/P24.0`，没有覆盖或美化。最终对应测试全部通过。

## 7. 最终自动化与覆盖

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 262,715 | `C2F4FFB1EE301E0173BE5BD86C9EC86D0D8D89A833D1E9775F999BF302AE350E` |
| `Shanmen.0_0_10` | 1,277 | 0 | 1,894,822 | `B9920B91DC184C6A21EA0AFB5AAB5C9F5346E0896637C912075965DD76E6FEA4` |
| `demo_map` | 1,330 | 0 | 1,648,966 | `DB74F66D03E7629D97C1080DC7B8E521452C57274B89B012A2AAFDD4DF85B2A6` |

`Shanmen.0_0_10` 从 19:51:35 至 21:06:40，约 75 分钟；`demo_map` 从 21:10:57 至 21:12:04，约 67 秒。两棵树合计 2,607/0。

改动映射最终结果：`PASS Changed=20 Rules=9 Required=98 Logs=2`。覆盖日志 SHA-256 `C2752B6137BDFF35DB82B300C7B7E47B20451641EB2005DF630E30C1544E844E`；覆盖器自测 442/442，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded / up-to-date | 0 | 0 | `1BE4DC899F9E01E39B0B550124C9CB16CF592FE648C890EDB198C1D898D4460E` |
| `demo_map Win64 Development` | Succeeded | 0 | 55 | `9323A1A4AB7C9A2FE935E0A2B96928D4FA98E6662B989631F92691A1A871956A` |

Game 构建中 UBA 因主机提交内存接近上限自动回收并重试编译进程，所有动作最终成功，没有源码或链接错误。

最终二进制：

- `demo_map.exe`：359,676,928 bytes，SHA-256 `6D242E35FC10C22B328F0119066C1451D789B936CA4C67A1483FBD5034D02CF8`；
- `UnrealEditor-demo_map.dll`：18,939,392 bytes，SHA-256 `4475186483EFF11580647034C2A35774716EAB345FC833D0332BB167B0612206`。

## 9. 静态与 P/F 边界

- 非文档增量：20 files、`+690/-26`；生产 7 files `+243/-4`，测试 11 files `+438/-21`，流程 2 files `+9/-1`；
- `ShanmenRegressionMap.json`：可解析；
- 旧 31 项动作断言扫描：0；
- 0.0.10 报告旧技术信封扫描：0；
- `git diff --check`：PASS；
- 新增资产、模块、UCLASS/USTRUCT、Actor、Subsystem 或持久玩法状态：0。

P 阶段证明物理绑定、迁移、Issue/Retry 选择、冻结请求、UI 锁、HUD 结果、完整回归和最终二进制。F 阶段保留真实键盘、装备/属性产品配置、投射物视觉与命中、动画音效、HUD 布局和手感验收。

## 10. 后续建议与 GitHub

P24.1 应先审计真实 M01 产品配置是否能在无头世界中准备有效剑类装备和攻击力，并证明一次物理输入沿现有 P18.10 链生成正式投射物/结果、一次 HostBusy 输入重试同一冻结请求。若已有等价证明，则转向玩家可见的剑气冷却/失败提示收口；不增加新的命令包装层。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-0-sword-qi-player-loop>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-0-sword-qi-player-loop/Docs/Report/Dev.D.UE.0.0.10.P24.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-0-sword-qi-player-loop/Docs/Log/Dev.D.UE.0.0.10.P24.0.r0_log.md>
