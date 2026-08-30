# Dev.D.UE.0.0.10.P10.9.r0 Report

## 1. 结论

P10.9 **PASS**。本阶段把 P10.8 的 canonical Spirit Evasion 产品身份链与 P10.7 的 typed command Router 合并为一个无状态产品路由，并把它设为 GameMode 唯一公开的 Spirit Evasion 启动入口。上游现在只能提交 device-independent 候选方向，不能再向 GameMode 注入自选 command、content、config 或 activation identity。

路由在 activation reservation 前检查 player Character、World、已注册组件、组件归属、组件空闲状态、active Combat Run 与 Run 内玩家实体映射；全部通过后才调用 P10.8 ProductAuthority 取得 frozen command，再仅交给 P10.7 Router。返回结果同时保留产品 reservation 与组件 start receipt，并验证三处 ActivationId 完全一致。

最终验证为 focused `6/6`、0.0.10 全量 `443/443`、EnemySkillFramework `44/44`、V2RangedCompatibility `22/22`、V3.Attributes `4/4`、FormationInfluenceConsumerWorldResolution `1/1`，六份正式日志合计 `520` 条 Success、`0` Fail。changed-file gate、132/132 mapping self-test、静态边界扫描、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 单一产品路由

`Fdemo_mapShanmenSpiritEvasionProductRoute` 是无状态 composition seam。它接收：

- 已安装的 Spirit Evasion component；
- GameMode 私有 Combat Run Coordinator；
- 当前 player Character；
- 一次 device-independent 候选方向。

它不接收 action snapshot、definition、config、sequence 或 ActivationId，因此调用方无法绕过 P10.8 canonical product authority。

### 2.2 Reservation 前置栅栏

`ValidateEntry` 在任何 sequence mutation 前依次拒绝：

1. owner 不可用；
2. owner 没有 World；
3. component 不可用或未注册；
4. component 不属于该 owner；
5. component 已持有非终态 action；
6. Combat Run Coordinator 未 ready；
7. owner 不能从 active Run registry 精确解析为当前玩家 entity。

方向有限性、XY 平面有效性与 canonical config 检查继续由 P10.8 ProductAuthority 在 reservation 前完成。以上拒绝均不执行 preflight，也不消耗下一 activation sequence。

### 2.3 完整 typed proof

entry validation 通过后，路由严格按以下顺序组合：

1. `ProductAuthority::PrepareStart` 生成 canonical config、Run-owned reservation 与 frozen command；
2. `CommandRouter::TryRoute` 把该 command 交给既有 component/host/lifecycle 链；
3. 返回 `ProductStart + CommandRoute + typed status + diagnostic`。

`IsAccepted()` 只有在 route status 为 Applied、产品 proof ready、Router receipt accepted、command kind 为 Start，且 reservation、command、receipt 的 ActivationId 三者完全相同时才成立。

若 entry/intent 在 reservation 前失败，不消耗 identity；若 reservation 已发布而 preflight 或执行路由失败，该 sequence 保持已消费，后续重试必须取得新的 identity，避免一次已观察尝试被重放或复用。

### 2.4 GameMode 产品入口

`Ademo_mapGameMode::RouteSpiritEvasionCommand(const Command&)` 已从公开接口移除，替换为：

```cpp
Fdemo_mapShanmenSpiritEvasionProductRouteResult
RouteSpiritEvasionStartIntent(const FVector& CandidateDirection);
```

GameMode 只解析当前玩家、复用既有组件安装 seam，并把方向交给产品路由。Cancel/Release 等内部生命周期操作仍使用既有 typed Router，不形成第二个公开启动入口。

## 3. 完整性与兼容性

- 复用 P10.8 ProductAuthority，不复制 config、direction validation 或 reservation；
- 复用 P10.7 CommandRouter，不直接调用 component start 或 host mutation；
- 复用 GameMode 私有 Combat Run Coordinator 与 active Run entity registry；
- 不创建 GUID，不重算或伪造 ActivationId；
- 不绑定 PlayerController 输入，不占用按键，不改变现有 input consume 顺序；
- 不读取、预留或写入 SpiritEnergy，资源 authority 继续后置；
- 产品路由不直接执行 Actor/World 位移；
- 不修改 item/profile/schema/CodeB；
- 不修改 P10.0—P10.8 的 resolver、scheduler、preflight 或 swept movement 契约；
- Enemy/V2 displacement、legacy attributes 与 0.0.10 全量回归均通过。

本阶段已完成从 GameMode 方向意图到组件 start receipt 的产品链，但尚未建立真实 PlayerController 输入 adapter 或物理按键绑定，不能描述为玩家已经能通过实际输入触发。

## 4. 关键不变量

1. GameMode 公开的 Spirit Evasion 启动入口只接收候选方向；
2. 外部调用方不能向 GameMode 注入完整 command 或自选产品身份；
3. owner/component/Run 的所有可观察失败必须发生在 reservation 前；
4. owner 必须精确等于 active Run registry 中的玩家实体；
5. 非终态 component 必须在 reservation 前拒绝第二次启动；
6. 无效、纯垂直或非有限方向不得消耗 sequence；
7. 一个 accepted route 恰好对应一个 canonical reservation 和一个 component receipt；
8. reservation、command 与 receipt 必须共享同一 ActivationId；
9. reservation 发布后的执行失败不得返还或复用 sequence；
10. 终态 component 可再次启动，但必须取得不同 activation identity；
11. 产品路由不拥有输入、资源、时钟、移动或 lifecycle 状态；
12. 真实 start mutation 仍只经 P10.7 Router/Component/Host 链发生。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.Product.SpiritEvasionProductRoute` 六个测试：

- `EntryFences`：缺 owner、缺 component、组件 owner 不匹配、Run 外 owner 与 unready Run 均在 reservation 前拒绝；
- `IntentFences`：零方向、纯垂直与 NaN 方向均不消费 sequence、不执行 preflight；
- `AppliedProof`：XY 归一化、完整 proof、组件 active、精确一次 preflight 与 identity 一致；
- `BusyFence`：非终态 component 拒绝第二次启动且不消费下一 sequence；
- `RejectedExecution`：preflight 拒绝后 reservation 保持已消费，重试取得 sequence two 与不同 identity；
- `TerminalReuse`：typed cancel 后可使用同一 canonical config 再启动，但 activation identity 必须不同。

0.0.10 全量由 P10.8 的 `437` 增至 `443`。

## 6. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.h`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.cpp`；
- `Source/demo_map/demo_mapShanmenSpiritEvasionProductRouteTests.cpp`。

更新：

- `Source/demo_map/demo_mapGameMode.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

产品、测试和流程本体共 `7` 个文件、`767` insertions、`10` deletions；加入本 Report 与同名 Log 后 exact stage 为 `9` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P10.9-SpiritEvasionProductRoute-final.log` | focused product route | 6 | 0 | `D068DF4B153F24A921F77448A23B004A4F5578D08B669659B5FB393512248171` |
| `P10.9-Shanmen-0_0_10-final.log` | 0.0.10 full | 443 | 0 | `C6475FF9CD1BB2E14B85ED53411372B0D4CAB30F26E1DA63F7895AAE8A98EE7F` |
| `P10.9-EnemySkillFramework-final.log` | enemy skills | 44 | 0 | `4EE720A83E31783942BED46D936268919D3A45FE0377C97DF953483AC6A7AA88` |
| `P10.9-V2RangedCompatibility-final.log` | V2 displacement | 22 | 0 | `68F0534D6F0283D413DD86A0FA90269090EDDC486906FAF0B94D33CBC756CDC4` |
| `P10.9-V3-Attributes-final.log` | legacy attributes | 4 | 0 | `E375C89AE6C3F4392BAD56A6AB77FEE8377A86442940BC0C67C325DE1621E53D` |
| `P10.9-FormationInfluenceConsumerWorldResolution-final.log` | focused coordinator dependency | 1 | 0 | `3AF749328124BDAF4A02A86FA6F5778CA9356AEBF0A0E0130624C4AB56BEDD36` |

所有正式进程原生退出码均为 `0`，每份日志均有 native terminal-success 标记；selected test Fail、fatal、unhandled 与 ensure 均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=86
SELF_TEST: PASS 132/132
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=28 Logs=6
git diff --check: PASS
LEGACY_PUBLIC_START_ROUTE_HITS=0
INPUT_HITS=0
RESOURCE_HITS=0
MUTATION_HITS=0
IDENTITY_FACTORY_HITS=0
```

mapping SHA-256：`9CA58C03CD1364257BDF89E5DBC4543033F963CDCAA94BB95CA024B8020A5FA7`；self-test SHA-256：`C5F9079A62CED0717FEB07C57336CD2046737134BE61D2F47B59525B3E3F6752`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 24 / 124.48s | 0 | `2005A2577D793665B4E698101498D4B69FB05CD0F303167DC45B588CF683C924` |
| Editor final | Succeeded, up to date | 0 / 1.24s | 0 | `F20172C12F70819771D651DF4BA6C82F58B3964BEE91D3D9EB5032214CEE051E` |
| Game final | Succeeded | 23 / 103.02s | 0 | `E6686F0916548C0CE424D3B575AD0F5BC291262BD1A8FE4EA430A5E2A2FBD4B6` |

最终 `UnrealEditor-demo_map.dll`：`12458496` bytes / SHA-256 `2818909E0D0B8DCDC246D1AE24757C6BDB87428F2D69A46E326E3E3499D87718`；`demo_map.exe`：`353908736` bytes / SHA-256 `203AAE1B7EFF19551C0A004794BB1149F678B50F125B961608CA64DC39BB1382`。

## 9. 真实异常

没有源码编译失败、selected Automation failure、changed-file gate failure、C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

六次 UnrealEditor-Cmd 启动阶段均出现引擎自带 `UE::UnifiedErrorTest` 初始化噪声，包括 13 行 `LogAutomationTest: Error: Condition failed`。这些行发生在 `LogEngine: Initializing Engine` 之前，不属于本轮 selected group；随后每组均完整运行、所有 selected tests 为 Success、Fail 为 0、native terminal exit 为 0，且没有 ensure/fatal/unhandled。该启动噪声如实保留在原始日志中，未删除或改写。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段代码、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Automation 使用 transient GamePreview World、Character、Health、component、Combat Run 与可控 preflight port 验证完整产品路由；它不是 F 阶段真实地图或输入验证。

下一阶段建议 P10.10 建立 device-independent PlayerController input adapter：把一次输入分类为 Spirit Evasion start intent、只采样一次方向，并仅调用 GameMode 的 `RouteSpiritEvasionStartIntent`。该阶段先固定 adapter 的单次采样、消费与 fail-closed 契约，不临时创建 SpiritEnergy；真实物理按键绑定与 F 阶段产品输入验证在 adapter 稳定后推进。
