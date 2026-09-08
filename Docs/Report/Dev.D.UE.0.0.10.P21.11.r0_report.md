# Dev.D.UE.0.0.10.P21.11.r0 Report

## 1. 结论

P21.11 在 P 阶段边界内完成，结论为 **PASS**。

本轮把既有权威安全的飞剑 `Redirect` 命令接成了真实玩家输入：默认按键为可重绑定的 `C`。玩家在飞剑已经处于 `Directed` 状态时按下该键，系统只采样一次当前瞄准方向、创建一个 intent identity，并通过现有 Run/GameMode 权威路由让同一把 canonical 飞剑改向。原有 `X` 语义保持不变：环绕时发射、定向飞行时召回。

```text
Controlled-weapon focused final:             60 Success / 0 Fail
Shanmen.0_0_10 full:                       1245 Success / 0 Fail
Required 0.0.9B compatibility groups:       191 Success / 0 Fail
Regression coverage:                         PASS (Changed=17 / Rules=8 / Required=43 / Logs=6)
Regression gate self-test:                   PASS 437/437
Game + Editor Development:                   PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实键鼠、截图、Smoke、Cook 或 Package。因此证明的是完整输入注册、物理输入分发、权威路由、既有定向飞行消费、迁移与无头自动化；不宣称玩家实际手感、镜头瞄准体验或视觉反馈已经人工验收。

## 2. 玩家可用的改向语义

统一输入注册表新增 `ControlledWeaponRedirect`：

- 默认键 `C`，只响应 Press，不响应 Release；
- 文案为“飞剑改向”，归类在既有“战斗”组；
- 可经现有输入设置实时重绑定；
- 与 `ControlledWeaponLaunchRecall` 的默认 `X` 完全分离。

玩家控制器只在按下改向键时调用一次新入口。入口继续使用现有 gameplay surface lock；菜单、结算或其它输入锁生效时，在读取 canonical 飞剑、创建 identity、采样瞄准和调用产品路由之前失败关闭。

## 3. 单一权威与状态边界

`Fdemo_mapShanmenControlledWeaponInputAdapter::RouteRedirectInput` 复用 P6 已有的 canonical read、Run command intent 与 GameMode route：

- 先读取唯一 Run/item/state 快照；
- 只有 `Directed` 状态允许 Redirect，Orbiting/Returning/终态均拒绝；
- 只为合法 Directed 请求创建一次 intent ID、采样一次当前瞄准方向；
- 方向无效时在产品 mutation 前拒绝；
- intent 固定携带当前 canonical item instance，路由至既有 Controller/Host；
- 路由最多调用一次，不重试、不选择另一件物品。

改向入口本身不移动 Actor、不推进时间、不做碰撞或伤害、不修改库存。通过既有路由接受后，原有 fixed timeline 在后续帧读取 Controller 已更新的方向并继续移动，因此没有第二个飞剑状态机、移动器、伤害路径或物品权威。

## 4. 输入配置迁移

统一输入动作由 29 个增加为 30 个，持久化输入配置版本由 7 升至 8。加载旧配置时继续保留已有按键：

- version 6 配置同时补入后续新增的发射/召回与改向动作；
- version 7 配置只补入新改向动作；
- `C` 空闲时采用默认 `C`；
- `C` 已被旧配置占用时保留用户旧绑定，并由现有分配器选择无冲突备用键；
- 实测冲突夹具保持 `Interact=C`，为 Redirect 分配 `G`。

迁移测试同时验证注册表仍无重复 ActionId、无默认键冲突，恢复默认值后精确得到 30 个动作。没有修改玩家存档 schema、物品 schema 或战斗 schema。

## 5. 产品级自动化与首次失败

focused 测试扩展为 60 项，覆盖：独立 C Press/Release、实时 C→Z 重绑定、gameplay lock、version 6/7 迁移与冲突回退、Orbiting 改向拒绝、零方向拒绝，以及 Launch → Redirect → Recall 始终命中同一 item instance。

首轮 focused 真实结果为 59 Success / 1 Fail。唯一失败发生在测试夹具：测试在 Redirect 前缓存了 Host 内 Controller 的指针，而既有原子路由以 host copy/commit 替换内部存储，使该测试指针失效。产品 Redirect 已被接受；修复只是在 commit 后重新按 item ID 获取 Controller，没有改产品路由或放宽断言。增量 Editor 编译 4/4 后，最终 focused 为 60/0。

全量 `Shanmen.0_0_10` 从 P21.10 的 1243 项增加到 1245 项，两项增量分别验证 version 7 Redirect 迁移和 Redirect 实时重绑定/输入锁。全量在日志时钟 `11:16:51.576` 至 `12:31:29.833` 自然完成 1245/0，并收到原生退出码 0。

## 6. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused initial | 59 | 1 | 331,013 | `37FD3EFFC5860E74A2D15812E075D49FF9A82BB7B7900BCB6D8F8F78C7AAC3CA` |
| Controlled-weapon focused final | 60 | 0 | 330,313 | `C0A3B4F3EE26C100E2DC84EB1F9EA97209D8CD7BFF4C2D374C1D1EB74A9E8BE6` |
| `Shanmen.0_0_10` full | 1245 | 0 | 1,908,962 | `9B4D788CE1B63876F9B7B3CAF52F20DF0CAA2D6672FC928543C650D3678F649D` |
| `demo_map.FullSystemLoop` | 50 | 0 | 312,966 | `095CB9B200820BB60C83DC93A5A861DE8E15B9037E8C168B0A2FB4E98D4986AD` |
| `demo_map.P7Integration` | 9 | 0 | 271,331 | `824FF3BBDF3CE3C70FFAD36CF94D0E3D63E29578B0D6938CFD429C52FAD7E96E` |
| `demo_map.P5RuntimeInterface` | 9 | 0 | 270,093 | `FB58AC07491228F0D70DC96B34B338D198479ADFD341A0C89D8465EDCF8ED1BE` |
| `demo_map.InputRestore` | 101 | 0 | 394,910 | `ACF050BDCCF29C848B735CE106B722073FC3A6D0DE5F8C6C2A6C610D20297946` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,503 | `75C56A18FD2D21DDBA82DF256C590E77B3CFFD415C34D5EE2D163D48D78B9B41` |

最终七份健康自动化日志（focused、full 与五组兼容）各有且仅有一个实际 RunTests 命令和一个成功终止标记；最终日志中的 Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。旧版兼容五组合计 191/0。

## 7. 覆盖门、静态审计与构建

只提供 full 日志时，首轮 changed-file gate 真实失败，精确指出缺少 `FullSystemLoop`、`P7Integration`、`P5RuntimeInterface`、`InputRestore` 与 `V2RangedCompatibility`。补跑五组后，最终 gate 为 `PASS Changed=17 Rules=8 Required=43 Logs=6`。

- 首轮 gate failure：395 bytes，SHA-256 `0D03D0F4322F27A7633AF398D7345FB038C22E34824F7DB98E294DCCB372AEBA`；
- 最终 gate：5,760 bytes，SHA-256 `33135AE2B87BCFE407C26F7E4E4EE835F251F8817B7C7C648AF0D1FE5EB8F1FA`；
- regression self-test：`PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`；
- implementation/test diff：17 files，`+496 / -49`；
- 193 条新增生产行中的 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；
- Redirect 在注册表中唯一、物理 BindKey 调用点 1、adapter 路由定义 1；
- `git diff --check` 通过，仅有工作树 LF→CRLF 提示；
- 验证结束后无 UnrealEditor、UnrealEditor-Cmd 或产品进程残留。

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 40 / 42.50s | 5,275 | `68E22AD35FD5835D95BA07A613E482843631010FCC27B5000D37CA874A4EDE70` |
| Editor after fixture fix | Succeeded / native 0 | 4 / 6.16s | 2,815 | `E9EA9DC66DCDA529CB9A4B9A3632E3565705631303AC29B48C84623333972BB6` |
| Game Development final | Succeeded / native 0 | 39 / 114.28s | 5,070 | `86038F1C7C56DCDEAD30B7D9B1663FC996BA4331B056BA68905BF3594211B3AE` |
| Editor Development final | Succeeded / native 0 | 0 / 1.17s | 1,021 | `5F85D218B99802036346DFD6EAF1421636B8857BDDA3FB0C66FDDB0DD2EC2965` |

最终产物：

- `demo_map.exe`：359,507,968 bytes，SHA-256 `208D752AB695828121EED75BB1023A7B6A5570ED462A0E411EA785A14734A1E3`；
- `UnrealEditor-demo_map.dll`：18,690,560 bytes，SHA-256 `B4D2E5DB1559F96131A1CCBE816156424225BE9A9CB43FF94391EC162152CA01`。

## 8. P/F 边界与 GitHub

PASS：玩家拥有独立、可重绑定、Press-only 的飞剑改向输入；只有 Directed 飞剑可接受；当前方向只采样一次；同一 Run/item 通过既有权威路由更新；原 X 发射/召回语义不变；旧输入配置无损迁移；focused、full、兼容、覆盖门与双目标构建全部通过。

未声明：真实键鼠手感、玩家镜头下瞄准方向品质、网络联机同步、动画/音效/VFX、PIE/Standalone 或打包体验。默认 `C` 只是当前无冲突基线，玩家仍可通过既有设置重绑定。

下一轮应在这条可操作飞剑闭环上增加可观察的战斗决策价值，例如把改向与已有目标/威胁信息组合成明确的玩家反馈；仍应复用唯一 Controller、Run Host、fixed timeline、Impact 与 Actor，不再增加只读 wrapper 链。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-11-flying-sword-player-redirect>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-11-flying-sword-player-redirect/Docs/Report/Dev.D.UE.0.0.10.P21.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-11-flying-sword-player-redirect/Docs/Log/Dev.D.UE.0.0.10.P21.11.r0_log.md>
