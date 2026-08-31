# Dev.D.UE.0.0.10.P11.4.r0 Report

## 1. 结论

P11.4 已完成并通过 P 阶段门禁。

本阶段新增唯一的武器格挡防御快照协调边界：调用方提供既有基础 `FShanmenDefenseSnapshot`、P11.0 active window、P11.1 caller-owned timeline observation、P11.2 arc policy、P11.3 World/registry/Actor 输入与 incoming candidate；协调器只组合这些已有权威。正面威胁时追加 P11.2 唯一 selected layer，背面威胁时原样保留基础快照，任何身份、生命周期、World 或 layer 冲突均 fail closed。

最终结果：

- 生产协调器与 5 个 focused tests 编译成功；
- 9 组最终 Automation 日志共记录 537 次 Success、0 次 Fail，其中 0.0.10 全量为 483/483；
- final pre-document regression gate：`PASS Changed=5 Rules=1 Required=9 Logs=9`；
- mapping 93 rules，自检 146/146；
- `git diff --check` 通过；
- Editor Development 与 Game Development 单并发最终构建均为原生退出码 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 Append-only 组合

`Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose` 按以下顺序工作：

1. 验证 P11.0–P11.3 frozen inputs 与 incoming candidate；
2. 验证调用方已经捕获的基础 defense snapshot；
3. 调用 P11.1 `TryProject`，不复制格挡窗口或时序状态机；
4. 调用 P11.3 World adapter，复用既有 World、registry 与 Actor 身份围栏；
5. `Qualified` 时检查 selected layer 身份不与基础层冲突，并追加到数组尾部；
6. `OutsideArc` 时保持基础 layers、顺序与 target tags 完全不变；
7. 对基础与最终快照生成确定性 digest，并签发组合 receipt；
8. 最终通过 `IsSuccess` 重新验证整条证据链。

### 2.2 可观测结果

- Perfect 正面：100 原始伤害由 Perfect Guard 全部阻止，最终伤害 0；
- Ordinary 正面：25% Guard 后再由 10 点 Armor 吸收，阻止 35，最终伤害 65；
- 背面：不追加 guard layer，只保留 10 点 Armor，最终伤害 90；
- 同一 frozen input 重放产生同一 receipt；
- 基础快照变化或威胁 Actor 位置变化产生不同 receipt；
- 已存在相同 selected layer ID 时拒绝二次追加。

## 3. 完整性

协调器建立的是进入既有 Impact resolver 之前的窄组合 seam：

```text
caller-owned base defense snapshot
  + P11.0 window / action lifecycle
  + P11.1 timing projection
  + P11.2 arc policy and selected layer
  + P11.3 World identity/transform evidence
  + incoming hit candidate
  -> validate exact authority chain
  -> qualified: append one exact selected layer
  -> outside arc: preserve base snapshot
  -> deterministic UObject-free receipt
```

没有新增：

- Tick、timer、异步任务或重试循环；
- 输入读取、窗口状态机、夹角数学或 Actor discovery；
- Impact 解析、生命值写入、伤害公式或 ledger；
- 资源 prepare/commit/cancel、库存或耐久写入；
- UObject/Actor 指针持久化；
- 第二套 World、实体、transform、目标或防御层权威。

## 4. 兼容性

- 未修改 `ShanmenCombatCore`、`ShanmenCombatRuntime`、P11.0、P11.1、P11.2、P11.3 或现有 Combat Run coordinator；
- 基础快照按值复制，调用方对象不被修改；
- 既有 layer 顺序、精确 float bits 与所有 tag container 都进入 snapshot digest；
- P11.2 仍是 selected layer 唯一权威；
- P11.3 仍是 live Actor 到 stable entity 与方向采样唯一产品适配边界；
- 本阶段不把新能力接入大型 Run coordinator，避免在一个阶段同时改变组合契约与产品调用路径。

## 5. 修改范围

实现与门禁共 5 个文件、1109 行新增、0 行删除：

- `Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinator.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinator.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinatorTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 7 个文件。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `PerfectImpact`：Perfect layer append-only 组合，并交给既有 resolver 验证 100 → 0；
2. `OrdinaryAndOutside`：正面 100 → 65，背面 100 → 90，基础快照不变；
3. `Conflict`：重复 selected layer 与重复基础 layer identity 都 fail closed；
4. `AuthorityFailure`：错误 timeline、null World 与错误 threat identity 不发布组合 receipt；
5. `ReplayLifecycle`：相同输入确定性重放，快照/位置变化改变 receipt，Recovery 关闭组合。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator` | 5 | 0 | 1 | `F68DE604FA1D140126E90A1F2B25DE98FD7367C38EDB5E08E61A95D422CBB1F3` |
| `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter` | 5 | 0 | 1 | `A63260C6A1445030CDB28FEDABFE19014E50D01189601F531FA938BC49484C24` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc` | 6 | 0 | 1 | `B70CD85F664B58AE2D802EEDE9AB602AEDB7738370B5AD2ED1516CA297060960` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | 1 | `2CAE4178621128D044A35D41C81781E6C68D451C9F748820DC3026DBD737F4C6` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuard` | 12 | 0 | 1 | `310B4A79054A5590B916AA3A17C967BF3D98A3BB72375EC43B3CC166D27DDAFC` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `26839636CA02D0D668CB796BCF723FB4F7053DD3A9BBFB174373E491E940BEF7` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | `A98D03AC6FF9316FD756553D7309380F6D36AFC63596ABDAD2589E7657B75EA0` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 1 | `DE916835552894E463E281FBDD957789FBBF0740149BD01F848805C4E73DE6A4` |
| `Shanmen.0_0_10` | 483 | 0 | 1 | `E5FC481C9CC13C24F5654D8BD6A3F24BAADDEDE6F2FC9F5956650D897BF56EA2` |

九组日志合计 537 次 Success / 0 次 Fail；该合计包含 focused 与全量之间的重复覆盖，不表示 537 个唯一用例。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=93
SELF_TEST: PASS 146/146
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=9 Logs=9
git diff --check: PASS
BOUNDARY_SCAN: PASS production cpp hits=0
```

- 新 `WeaponGuardDefenseCoordinator` rule 强制要求 full、coordinator、World adapter、arc、perfect/ordinary timing、action lifecycle、CombatCore 与 WorldGameplay 共 9 组证据；
- mapping SHA-256：`B4C75ED8FAC6B6200D9882F48EB4D49FAD33E2F37153BF5E401981FACB2CE179`；
- self-test SHA-256：`341E6CCF031D946FB599019C61780E0008C45AB459E71A74D15AE8346CD8FA22`；
- production cpp 边界扫描不含 trace/sweep、Actor discovery、pointer retention、timer、input、Impact resolver、资源事务或耐久写入。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development（首次） | Failed / invalid test tag accessor | 5 scheduled / 21.00s | 6 | `0E2057BA7AF1A266DEBE31A5445BB2F6C16A50B5AB6CC40204271454DAE114AF` |
| Editor Development（最终） | Succeeded | 4 / 6.30s | 0 | `EA581B998231854F7C929669446096709BAED4ED4CC912030627D659C6B33590` |
| Game Development（最终） | Succeeded | 4 / 23.62s | 0 | `A55E3ED1AC8CAC236F174169455E15059447676DDABA80671EF3339E7025D424` |

- `UnrealEditor-demo_map.dll`：12,602,880 bytes，SHA-256 `A04C50D59431EAE1AB99129C7E4B07DFC004195C9C6B89AE5860C0C5BBDE755E`；
- `demo_map.exe`：354,163,712 bytes，SHA-256 `1F76B0A158B8067EEA79E659AF218E5D06EC594C9EF20ADD94DEEC9B29814A26`。

## 9. 真实异常与修正

1. 首次 mapping self-test 误用 Windows PowerShell 5.1；既有脚本采用 PowerShell 7 行首管道语法，解析退出 1。改用项目 bundled `pwsh` 后 146/146，通过；未修改 validator 来放宽规则。
2. 首次 Editor 构建原生退出码 6，UBT 分类 `OtherCompilationError`。新增测试夹具调用不存在的 `FShanmenCombatNativeTags::SourceEnemy()`；该 threat action 无 source-tag 约束，删除错误调用后构建成功。生产协调器在该轮已编译通过。
3. 首次 focused Automation 原生退出码 3。`Conflict` 夹具用 `Invalid.Layers.Add(Invalid.Layers[0])` 从正在扩容的同一 `TArray` 引用追加元素，触发 UE 容器别名断言；改为先值拷贝再追加。失败日志 SHA-256 为 `6E4313971E3D93C0049C4370403363737086A3516937DFFD8CD8E50F6B34262A`，最终 focused 5/5。
4. 没有产品协调逻辑测试失败、内存/页面文件错误或 Win64 SDK 失败。UE 启动时打印的非 Win64 SDK metadata 不影响 Win64 构建和 selected tests。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及一次必要的最终 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或大规模 F 阶段回归。

建议 P11.5 把本协调器接入现有 `demo_mapCombatRunCoordinator` 的基础 defense capture 与资源适配之后、`FShanmenImpactRequest` 构造之前；接线阶段必须继续使用 incoming candidate 已有 attacker/target 身份，不得复制 Impact resolver、资源事务或防御快照权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-4-weapon-guard-defense-coordinator>
