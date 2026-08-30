# Dev.D.UE.0.0.10.P10.8.r0 Report

## 1. 结论

P10.8 **PASS**。本阶段建立了 Spirit Evasion 的产品配置唯一来源与 Combat Run 自有 activation reservation：调用方只提供候选方向，产品 authority 选择 canonical content、defense rule、movement policy 与 trajectory；Combat Run Coordinator 使用自己的单调序列生成 deterministic activation identity，随后冻结 P10.7 typed start command。

方向在身份分配前先验证为有限且具有非零 XY 分量，并统一投影为平面单位向量。无效、纯垂直或非有限方向不会消耗序列；一旦 reservation 成功，该序列即永久消费，即使后续 command capture fail closed 也不允许复用。

最终验证为 focused `7/7`、0.0.10 全量 `437/437`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、V3.Attributes `4/4`、FormationInfluenceConsumerWorldResolution `1/1`，六份正式日志合计 `515` 条 Success、`0` Fail。changed-file gate、130/130 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 Canonical product config

`Fdemo_mapShanmenSpiritEvasionProductConfig` 是只读值对象，私有保存并公开只读访问：

- content version：`0.0.10.P10.8`；
- content digest：`Shanmen.SpiritEvasion.ProductConfig.r1`；
- action definition：既有 canonical Spirit Evasion action；
- defense rule：`Defense.Spell.SpiritEvasion01`；
- required target tag：`TargetLiving`；
- movement policy：`Movement.Spell.SpiritEvasion.GroundStep`；
- requested/minimum distance：`400 / 100`；
- WorldStatic clearance：`2`；
- duration / segments：`0.2s / 2`。

ConfigId 由以上 canonical parts 确定性派生；任意调用均得到相同身份。配置不要求 `SourcePlayer` 作为被攻击来源，避免把玩家主动闪避错误限制为只能规避玩家伤害。

### 2.2 Run-owned reservation

`Fdemo_mapCombatRunCoordinator::TryReservePlayerSpiritEvasionAction` 只在 ready Run 与 canonical config 下工作：

- RunId 来自当前 Coordinator；
- OwnerId 与 SourceEntityId 都是当前注册玩家实体；
- action definition/content 来自 canonical config；
- source tag 精确为 `SourcePlayer`；
- 不伪造 source item identity；
- ActivationId 由 RunId、source、action definition 与 Run-local sequence 派生；
- 输出 reservation 自校验所有字段与派生身份；
- 仅成功 reservation 后递增序列；
- EndRun/Reset 清回序列 1，但新 RunId 保证 identity 不碰撞。

### 2.3 Product composition

`Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart` 按固定顺序执行：

1. 验证三轴有限，投影 XY，并拒绝零平面方向；
2. 创建 canonical product config；
3. 向 active Combat Run 申请一次 reservation；
4. 用 reservation + config + planar direction 捕获 P10.7 typed Start command；
5. 返回 typed status、诊断、config、reservation 与 command 组成的完整 proof。

authority 不保存 sequence、组件、Actor、资源、时钟、移动或生命周期状态。

## 3. 完整性与兼容性

- 复用 P10.7 command router，不建立第二套 dispatch；
- 复用 Combat Run Coordinator，不建立第二套 Run 或 entity authority；
- 复用既有 deterministic ID factory 与 immutable action snapshot；
- 不修改 P10.0—P10.7 的结算、scheduler、碰撞 preflight 或 swept mutation；
- 不绑定 PlayerController 输入，不占用按键，不改变现有输入消费顺序；
- 不读取或写入 SpiritEnergy；资源 authority 仍明确后置；
- 不修改 item/profile/schema/CodeB；
- 不启动真实施法，也不把配置存在 Actor/component 中；
- 旧 Enemy/V2 displacement、Attributes 与 0.0.10 全量回归均通过。

本阶段已完成“输入意图之前”的产品身份链，但尚未把它接入 GameMode 产品入口或真实 PlayerController，因此不能把 P10.8 描述为玩家已能按键施放。

## 4. 关键不变量

1. canonical Spirit Evasion 配置只有一个确定性 ConfigId；
2. 输入、GameMode 与 Router 均不能选择或覆写 canonical 配置；
3. activation sequence 只由 active Combat Run 持有和递增；
4. 非 ready Run、非 canonical config 与无效方向不得消耗 sequence；
5. 纯垂直方向没有产品平面位移，必须在 reservation 前拒绝；
6. 每个成功 reservation 恰好消费一个 sequence；
7. 已消费 sequence 永不回滚或复用；
8. sequence-one 在不同 Run 中仍由 RunId 保持全局身份区分；
9. action owner/source 必须等于当前注册玩家实体；
10. Spirit Evasion action 不携带伪造 item identity；
11. command 必须逐字段等于 reservation 与 canonical config；
12. authority 不拥有输入、资源、World mutation 或 action lifecycle 状态。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionProductAuthority` 七个测试：

- `CanonicalConfig`：配置值、稳定 ConfigId、target/source/damage tag 语义；
- `ReservationFences`：unready Run 与非 canonical config fail closed 且不推进 sequence；
- `SequentialReservation`：1、2 单调序列、确定性 ActivationId 与玩家 ownership；
- `RunReset`：EndRun 重置局部序列，新 Run identity 保持不同；
- `InvalidDirection`：零、NaN 与纯垂直方向均在 reservation 前拒绝；
- `StartComposition`：reservation/config/command 精确组合及 XY 归一化；
- `DistinctAttempts`：共享 canonical config、不同 activation identity 与请求方向。

0.0.10 全量由 P10.7 的 `430` 增至 `437`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthority.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthority.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthorityTests.cpp`。

更新：

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

产品、测试和流程本体共 `7` 个文件、`972` insertions、`0` deletions；加入本 Report 与同名 Log 后 exact stage 为 `9` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.8-SpiritEvasionProductAuthority-final.log` | focused authority | 7 | 0 | `FD8E0630DF96B9E9962CAF275617E1294741B04A8FF18C6D8B535423117A33AA` |
| `P10.8-Shanmen-0_0_10-final.log` | 0.0.10 full | 437 | 0 | `CA1192EA47953A1A41EA04F0B30F3FD2F328E020DE4BB04E31FB182BE51AB845` |
| `P10.8-EnemySkillFramework-final.log` | enemy skills | 44 | 0 | `DDFCD117193B724800B44B56BF5BCFB403EBCF668796F13D2AC28C6F6F35628F` |
| `P10.8-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `242DB1E0D05E234FEB6A92C4F7053C38ED4D6D68A5387373774BD452B5F0EAD3` |
| `P10.8-V3-Attributes-final.log` | legacy attributes | 4 | 0 | `063994AE10905E3771FF488DF870658B7E60CFAE769FEAAED42B90271D43A855` |
| `P10.8-FormationInfluenceConsumerWorldResolution-final.log` | focused coordinator dependency | 1 | 0 | `98E0942E90E4D8E541F33FC414A36BBE3ED728189A0B6F8AFB9A54EE89C6CC60` |

所有正式进程原生退出码均为 `0`，每份日志均有 native terminal-success 标记，且 selected fail、fatal、unhandled 与 ensure 均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=85
SELF_TEST: PASS 130/130
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=17 Logs=6
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
SEQUENCE_INCREMENT_HITS=1
```

mapping SHA-256：`C48A560F0A09AF30B3ABF1386FA33A63BFA9961FB709D4CE1AC1A1FA169BF23A`；self-test SHA-256：`32CB9832CB66D3CEF900C93351C5AA620888AD87B4C36534E803ABD1760ED88A`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 56 / 195.86s | 0 | `9D86B4114B566F2E467E2617981C691F61381CAC17B63C6EF977E4BD5DA4CF60` |
| Editor candidate after direction review | Succeeded | 5 / 7.15s | 0 | `77EE13028313B1DF5751DA49DC8DB574C8399F78F73E97E323744FA7FED18004` |
| Editor final | Succeeded, up to date | 0 / 0.90s | 0 | `81FE2A1AD01BEF80F5B74C7BEA9E3163013B02D99E63070F9B680D731502B483` |
| Game final | Succeeded | 55 / 171.81s | 0 | `888F3C34DA69687C6137841897FEAC2EC958A681E1E8A99A53A374B887F79FC1` |

最终 `UnrealEditor-demo_map.dll`：`12428288` bytes / SHA-256 `5F317399D5E8938622A6BEAC302AD16BE05D9F6B92CF48FF8ADE688EA99820DE`；`demo_map.exe`：`353885184` bytes / SHA-256 `F07ECC484590F26A26C4DE069FAA003C94C93E90D87F16990E23584D3E573D53`。

## 9. 真实异常

没有源码编译失败、Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

候选编译后代码审查发现初稿以三维 `IsNearlyZero` 做前置验证：纯 Z 输入会先取得 reservation，再在平面产品语义下失效。正式 Automation 前已改为先构造 XY 平面方向并拒绝零平面输入，补充纯垂直不消耗 sequence 的断言，并完成一次必要的 5-action 增量 Editor 构建。该问题未进入正式测试或最终构建证据。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 使用 transient Pawn/Health/Run fixture 验证配置、identity 与 typed command composition；真实地图 GameMode、真实 PlayerController 输入与连续画面尚未执行。

下一阶段建议 P10.9 建立 GameMode 的单一产品入口：接收 device-independent 方向意图，调用本阶段 ProductAuthority，再把生成的 typed command 交给 P10.7 Router，并返回 reservation/route 的组合结果。仍不绑定真实按键、不临时创建 SpiritEnergy；真实 PlayerController adapter 留在该入口稳定后的下一阶段。
