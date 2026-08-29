# Dev.D.UE.0.0.10.P6.7.r0 Report

## 1. 结论

P6.7 已完成受控武器 Run Command Router，结论为 **PASS**。

未来输入层只需提交一个冻结的 `IntentId + RunId + exact item 集合 + Launch / Redirect / Recall` 意图；Router 从每个既有 Execution 内部派生下一命令序号，以稳定 item 顺序在 Host 副本上执行，并把 Host 状态与幂等记录一次性提交。输入层不再拥有每把飞剑的 sequence、批量顺序、回滚或重放状态。

`Ademo_mapGameMode` 仍是唯一真实 Run owner。本轮未绑定按键，未生成 Actor，未决定飞剑数量、选择策略、编队、速度、物品定义或内容资产。

## 2. 功能性

- `Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture` 校验有效 intent / Run identity、非空且无重复的 exact item ID，并按 GUID digits 规范排序；
- Launch / Redirect 方向必须有限且非零，捕获时归一化；Recall 必须使用零方向；
- Router 同时核验 Coordinator、Host 与 Intent 的 exact Run / source identity，旧 Run Router 不能服务新 Run；
- 每个目标的 `ExpectedSequence` 从其现有 Session Execution 读取，调用方不能伪造或维护序号；
- 整批命令在 Host 值副本上按稳定 item 顺序执行；任一后续目标拒绝时，前序目标的 staged mutation 全部丢弃；
- 首次成功同时提交 Host 与 Router ledger；相同 `IntentId` 的同 payload 重放返回原收据且不推进序号，不同 payload 复用同一 ID 时失败关闭；
- Recall 收据同时校验 command、Recovery 与 Completed 三段状态迁移；
- 空 Router 必须同时具有空 ledger 与无效 RunId，损坏的隐藏 Run identity 会被 `IsValid` 拒绝；
- GameMode 暴露输入无关的 `RouteControlledWeaponIntent`，新 Run 激活前拒绝残留 Host 或 Router；
- Preparation deactivation / EndPlay 在既有 P6.6 teardown 后重置 Router；异常清理分支也显式清除 ledger，防止跨 Run 重放。

## 3. 完整性

新增 4 个产品级自动化：

1. `StableAtomicBatch`：逆序输入被规范为稳定顺序，两把 exact item 的 Launch / Redirect / Recall 分别取得序号 0 / 1 / 2，Recall 后均 terminal；
2. `ReplayAndConflict`：等价归一化意图精确重放且不推进序号；同 ID 冲突 payload 被拒绝；下一新意图仍取得正确序号；
3. `AtomicRollback`：第二目标拒绝时第一目标的 staged Launch 不泄漏；未知目标在 mutation 前失败；重复目标无法捕获；
4. `RunIdentityFence`：Intent / Host / Coordinator 混合 Run 被拒绝，已绑定 Router 拒绝第二 Run，显式 Reset 后恢复空状态。

全量 `Shanmen.0_0_10` 从 P6.6 的 `149` 个增加到 `153` 个，最终 `153/153` Success。

## 4. 兼容性

- 未建立第二个 Run、entity、inventory、deployed-item、vitality、impact 或 command authority；
- 未修改 P6.1 Adapter、P6.2 Session、P6.3 World Adapter、P6.4 Controller、P6.5 Host、P6.6 Lifecycle、CombatCore、CombatRuntime、WorldGameplay 或 Items 的生产语义；
- Router 只组合既有 Host / Coordinator / Controller 收据，不调用 `SpawnActor`、`ApplyDamage`、`TakeDamage`、物品 Reserve / Commit / Consume、RNG 或输入绑定；
- Intent 的重放比较使用捕获后方向的精确值相等，与底层 CommandId 的 canonical bit 语义一致，不以浮点容差吞并两个不同 payload；
- 未修改 Profile schema、存档格式、item definition、GameplayTags 或 input mapping；
- 未把 legacy `SkillProjectile` 解释为 controlled weapon，也未冻结正式 flying-sword presentation。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponRunCommandRouter.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunCommandRouterTests.cpp`；
- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter` | 4 | 0 | 0 | `322AB9133571D3B269532DDAB4DF4F552F2EE23AF09E35CB6E184DE7BD074F76` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `7FF21311D2B4E2F172EA476A16DCBC0D42E0A7660A7FA4835AA7D66CF02D3C58` |
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `DEAEB8CF1B70D57DB820386368023408E25201A46C221E57863A7862A04E507A` |
| `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `522A6C5FC15EF143343293BDA1160ACDEC7F70E26E820273A8A8FACD594E8F1B` |
| `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `A3003A0FF34E3AAEC9B3E4040AA17C8C0661A7EFDFF3421682AC5BB2036562EF` |
| `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `640A0492A70477134197B3B7F293D049D91CCD5922864C53F07BA5DD32C7F05E` |
| `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `666BBA4B92430198EA76EC7E03C880EA0AAC4FC5E7528674D0228248CEBFDA52` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `75F794E3BE1BE03EBB78DB3FF6587CB60B8132F71DD2B58AD7D9BBE5CE292BCF` |
| `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `33E13FBC827D4804C7618F9ECD8F5C7E90DAFFB4B16B306D24BE011809E369F1` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `3F2D0FBA3A2CFB68667C186DBE4A6E65B052F10F8FF948A1C385654C9CCC1924` |
| `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `560F77BF4B24A08EAC9BFB2C4DB7843D111DC2BB228FD76E99339D4186E968F9` |
| `Shanmen.0_0_10` | 153 | 0 | 0 | `736C5840867FABF45DC3F0B021552009B25EC7B4B0FFD47417E46B0A13BCCACA` |

- 十二条最终日志均有唯一实际 `Cmd: Automation RunTests`、唯一 `tests performed` queue-empty、Fail `0`、原生退出 `0`，Fatal / unhandled / ensure `0`；
- changed-file gate：生产与脚本路径为 `PASS Changed=7 Rules=2 Required=11 Logs=12`；加入 Report / Log 后最终 staged gate 为 `PASS Changed=9 Rules=2 Required=11 Logs=12`；
- regression coverage self-test：`14/14 PASS`；
- regression JSON parse：PASS；
- Router 生产文件的 spawn / damage / inventory transaction / RNG / input binding 边界扫描命中 `0`；
- 最终源文件时间早于已测试 Editor DLL 与 Game executable；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`。

## 7. 首次验证与契约复审

首次 Editor integration build 为 `24/24` actions、`Result: Succeeded`、native exit `0`、`112.88s`。

首次 focused 日志 `p67_command_router_first.log` 为 `4/4` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `84C745B8A779EE2D986D1FA6F837BD9E84FF16DEE6798D8CA4F1DC9B7EA50F39`。

随后契约复审发现：重放 identity 不应以浮点容差比较方向，否则两个 canonical bits 不同的 payload 可能被错误合并。实现改为捕获后 `FVector` 精确值比较，与底层 CommandId 规范一致。

修正后的 Editor 增量构建为 `4/4` actions、`Result: Succeeded`、native exit `0`、`5.71s`；focused 复跑 `4/4` Success、SHA-256 `CCD9D6D6BE14680AFC64809D35B733C577AF61E7C4ED14900BD77FEDDD99ABCC`。

首次 full 日志 `p67_full_first.log` 为 `153/153` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `BBB30562804DC86BD0B4D8975D5622F32DCD9036CF5B7C7BC203E34D3EF66F47`。没有源码、测试或编译失败。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次集成：`24/24` actions，`Result: Succeeded`，native exit `0`，`112.88s`；
- 契约修正后增量：`4/4` actions，`Result: Succeeded`，native exit `0`，`5.71s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `23/23` actions；
- `Result: Succeeded`；
- native exit `0`；
- `97.17s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-7-controlled-weapon-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-7-controlled-weapon-command-router/Docs/Report/Dev.D.UE.0.0.10.P6.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-7-controlled-weapon-command-router/Docs/Log/Dev.D.UE.0.0.10.P6.7.r0_log.md>
