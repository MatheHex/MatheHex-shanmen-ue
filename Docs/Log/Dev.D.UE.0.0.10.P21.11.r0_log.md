# Dev.D.UE.0.0.10.P21.11.r0 Development Log

## 1. 基线与目标

- base：`84aab96a915c17eb3944dab142a29ca8aee162e5`（P21.10 flight-phase presentation）；
- branch：`agent/0.0.10-p21-11-flying-sword-player-redirect`；
- 目标：把现有 authority-safe `Redirect` 接成独立玩家输入，同时保持 X 的 Launch/Recall 行为；
- 边界：复用唯一 input settings、PlayerController、GameMode route、Run Host、controlled-weapon controller、fixed timeline 与 canonical Actor；输入层不移动、不伤害、不改库存。

## 2. 路径审计与决策

P21.10 已能从表现层观察 Orbiting/Directed/Returning/Redeployed，但玩家只有 X：Orbiting 时 Launch，Directed 时 Recall。底层 Controller 和 Run command router 已支持 Redirect，后续 fixed timeline 也已消费被更新的定向方向；缺口只在真实玩家输入。

若把 Redirect 复用到 X，就会覆盖 Directed 状态下的 Recall，使玩家无法主动召回。因此选择新增独立动作 `ControlledWeaponRedirect`，默认 C；X 完全不改。adapter 只负责冻结一次输入并调用现有 route，不引入新的 gameplay owner。

## 3. 实现

### 3.1 注册、绑定与控制器入口

输入注册表由 29 项增至 30 项，新增 `ControlledWeaponRedirect` / C / Press-only / “飞剑改向”。PlayerController 在现有统一重建流程中绑定一次 C，并公开自动化观测计数与最后结果。

控制器入口先执行既有 `IsGameplayInputAllowed()`，再获取 authority GameMode。合法时传入 canonical reader、单次 GUID factory、`GetLastValidAimDirection()` 与 `RouteControlledWeaponIntent()`；没有直接访问 Actor、Host 内部或库存。

### 3.2 Redirect adapter

输入结果增加 `CommandUnavailable`，区分“存在 canonical 飞剑但当前状态不允许改向”与“没有 canonical 飞剑”。`IsAccepted()` 现在显式校验 Launch、Redirect、Recall 三种行为的状态和方向采样纪律。

`RouteRedirectInput` 顺序固定为：surface lock → product route availability → canonical read → Directed fence → intent ID → aim sample → intent capture → one route。任一步失败都停止，后续回调不执行；zero/non-finite direction 由既有 intent capture 拒绝。

### 3.3 配置迁移

输入配置序列化版本升至 8。沿用既有“保留已知旧键，再为缺失动作寻找无冲突键”的加载策略：v7 只增加 Redirect，v6 同时补入 Launch/Recall 与 Redirect。没有建立第二份迁移器或特殊配置文件。

## 4. 测试开发

adapter 测试扩展 Launch → Redirect → Recall：三次 intent 均命中同一 item，Redirect receipt 的 command kind 与归一化方向正确，Host 仍处于 Directed，随后 Recall 正常关闭。另覆盖 Orbiting 在 identity/aim/route 前拒绝、zero aim 在 route 前拒绝。

physical-input 测试新增：

- registry 精确 30 项，X 与 C 无冲突且均 Press-only；
- v6 free/conflict migration 同时分配两个飞剑动作；
- v7 free-C 与 occupied-C→G migration；
- C Press 调用一次、Release 不重复；
- C→Z 实时重绑定后旧 C 失效、新 Z 生效；
- settlement gameplay lock 在 read/identity/aim/route 前拒绝。

所有引用固定注册数量或持久化版本的旧夹具同步更新为 30 / version 8；其中 `MakeVersionSevenConfig` 有意保留 version 7，作为本轮迁移输入。

## 5. 首次失败与修复

初始 Editor 编译为 40/40、42.50s、native 0。首轮 focused 共执行 60 项，结果 59/1；失败项为 `ControlledWeaponInputAdapter.LaunchRecallExactItem` 的新增组合断言。

根因不是产品拒绝 Redirect，而是测试在 Launch 后缓存了 `Host.FindController()` 返回的内部指针。Redirect 走既有原子 host copy/commit 后，原 Host 存储被替换，缓存指针不再是有效观测点。修复是在 commit 后按同一 item ID 重新获取 Controller，再验证 Directed 与新方向。未修改生产语义、未放宽 receipt 或 identity 断言。

- 首次失败：`P21.11_focused_controlled_weapon_initial.log`，59/1，331,013 bytes，SHA-256 `37FD3EFFC5860E74A2D15812E075D49FF9A82BB7B7900BCB6D8F8F78C7AAC3CA`；
- fixture 修复后 Editor 编译：4/4、6.16s、native 0；
- 最终 focused：60/0，330,313 bytes，SHA-256 `C0A3B4F3EE26C100E2DC84EB1F9EA97209D8CD7BFF4C2D374C1D1EB74A9E8BE6`。

## 6. 自动化与覆盖门

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.11_focused_controlled_weapon_final.log` | `Shanmen.0_0_10.Product.ControlledWeapon` | 60/0 | 330,313 | `C0A3B4F3EE26C100E2DC84EB1F9EA97209D8CD7BFF4C2D374C1D1EB74A9E8BE6` |
| `P21.11_full_0_0_10.log` | `Shanmen.0_0_10` | 1245/0 | 1,908,962 | `9B4D788CE1B63876F9B7B3CAF52F20DF0CAA2D6672FC928543C650D3678F649D` |
| `P21.11_legacy_full_system_loop.log` | `demo_map.FullSystemLoop` | 50/0 | 312,966 | `095CB9B200820BB60C83DC93A5A861DE8E15B9037E8C168B0A2FB4E98D4986AD` |
| `P21.11_legacy_p7_integration.log` | `demo_map.P7Integration` | 9/0 | 271,331 | `824FF3BBDF3CE3C70FFAD36CF94D0E3D63E29578B0D6938CFD429C52FAD7E96E` |
| `P21.11_legacy_p5_runtime_interface.log` | `demo_map.P5RuntimeInterface` | 9/0 | 270,093 | `FB58AC07491228F0D70DC96B34B338D198479ADFD341A0C89D8465EDCF8ED1BE` |
| `P21.11_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 394,910 | `ACF050BDCCF29C848B735CE106B722073FC3A6D0DE5F8C6C2A6C610D20297946` |
| `P21.11_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,503 | `75C56A18FD2D21DDBA82DF256C590E77B3CFFD415C34D5EE2D163D48D78B9B41` |

full 从日志时钟 `11:16:51.576` 至 `12:31:29.833` 自然清空 1245 项。五组旧兼容合计 191/0。最终七份自动化日志各有一个实际 RunTests 命令和一个 native 成功终止标记，Fail/Fatal/Unhandled/Ensure 均为 0。

首轮 changed-file gate 只提供 full 证据，按预期失败并列出九个缺失要求；这些要求由五个父测试组完整覆盖。补跑后最终为 `PASS Changed=17 Rules=8 Required=43 Logs=6`。

- gate initial failure：395 bytes / `0D03D0F4322F27A7633AF398D7345FB038C22E34824F7DB98E294DCCB372AEBA`；
- gate final：5,760 bytes / `33135AE2B87BCFE407C26F7E4E4EE835F251F8817B7C7C648AF0D1FE5EB8F1FA`；
- gate self-test：`PASS 437/437`，43,073 bytes / `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

## 7. 静态与构建

- 实现/测试 diff：17 files，`+496 / -49`；
- 193 条新增生产行内 Timer/SetTimer/RNG/ApplyDamage/SpawnActor/Destroy 命中 0；
- Redirect 注册、物理绑定和 adapter 定义均为唯一既有路径扩展；
- `git diff --check` native 0，仅有 LF→CRLF 提示；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.11_editor_build_initial.log` | Editor initial | Succeeded / native 0 | 40 / 42.50s | 5,275 | `68E22AD35FD5835D95BA07A613E482843631010FCC27B5000D37CA874A4EDE70` |
| `P21.11_editor_build_after_fixture_fix.log` | Editor after fixture fix | Succeeded / native 0 | 4 / 6.16s | 2,815 | `E9EA9DC66DCDA529CB9A4B9A3632E3565705631303AC29B48C84623333972BB6` |
| `P21.11_game_build_final.log` | Game final | Succeeded / native 0 | 39 / 114.28s | 5,070 | `86038F1C7C56DCDEAD30B7D9B1663FC996BA4331B056BA68905BF3594211B3AE` |
| `P21.11_editor_build_final.log` | Editor final | Succeeded / native 0 | 0 / 1.17s | 1,021 | `5F85D218B99802036346DFD6EAF1421636B8857BDDA3FB0C66FDDB0DD2EC2965` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,507,968 bytes / SHA-256 `208D752AB695828121EED75BB1023A7B6A5570ED462A0E411EA785A14734A1E3`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,690,560 bytes / SHA-256 `B4D2E5DB1559F96131A1CCBE816156424225BE9A9CB43FF94391EC162152CA01`。

## 8. 提交边界

提交 17 个实现/测试文件、本 Report 与本 Development Log，共 19 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.11` raw logs 不进入 Git。

未修改 Content、地图、Engine、Windows、玩家存档 schema、物品权威、Impact resolver、飞剑 Actor、返回/环绕/定向移动数学或伤害数学。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-11-flying-sword-player-redirect>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-11-flying-sword-player-redirect/Docs/Report/Dev.D.UE.0.0.10.P21.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-11-flying-sword-player-redirect/Docs/Log/Dev.D.UE.0.0.10.P21.11.r0_log.md>
