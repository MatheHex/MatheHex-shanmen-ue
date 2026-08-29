# Dev.D.UE.0.0.10.P8.0.r0 Report

## 1. 结论

P8.0 已建立阵法体系的第一份纯 Runtime 基础契约，结论为 **PASS**。

新增 `FShanmenFormationDiagramDefinition` 与 `FShanmenFormationDeployment`：阵图以显式有序数组冻结阵眼、相对几何和材料需求；部署绑定既有 `FShanmenCombatActionSnapshot` 与 `FShanmenActionOrchestrator`，生成可重放的 DeploymentId、AnchorInstanceId 和事件 receipts。

本轮不修改库存，也不生成阵眼 Actor、范围、特效或 UI。外部系统只能提交一份精确的 committed material evidence；P8.0 负责验证 evidence 与阵图需求完全一致，但不伪装成 ShanmenItems authority。

## 2. 不可变阵图契约

阵图 authoring capture 包含：

- canonical action：`Combat.Action.Formation.Deploy`；
- 一个 `DiagramDefinitionId`；
- 显式有序的 `TArray<FShanmenFormationAnchorCapture>`；
- 每个阵眼的 `Order`、稳定 ID、relative offset；
- 每个阵眼内部显式有序的材料需求数组。

capture 成功后，所有 frozen 字段均为 private + BlueprintReadOnly。阵眼和材料行必须使用从 `0` 开始的连续 order；重复阵眼 ID、重复材料 definition、非有限坐标、空需求或非正数量均失败关闭。结构不使用固定字段，因此增加阵眼或材料类型无需改写 USTRUCT schema。

本阶段测试中的木材、灵核、灵石名称和数量只是夹具，不是正式阵法配方。

## 3. 部署身份与空间

创建 Deployment 必须提供有效 action、匹配 diagram、有限 origin 和可规范化的平面 forward。forward 忽略 pitch 后单位化；relative offset 通过 forward/right/up 三轴投影得到阵眼世界位置。

DeploymentId 的 canonical parts 包含：

- Run、owner、activation、source entity；
- action definition 与 content stamp；
- diagram ID；
- origin 与 canonical forward 的精确位；
- 完整有序阵眼结构、offset、材料 definition 与数量。

因此相同输入可重放；方向等比例和不同 pitch 归一到同一平面部署；即使调用方错误地复用 diagram ID/content stamp，结构变化仍产生不同 DeploymentId。每个 AnchorInstanceId 再由 DeploymentId、order 与 anchor ID 派生。

## 4. 材料 evidence 与守恒

`FShanmenFormationAnchorFulfillmentEvidence` 是以后 ShanmenItems adapter 的输入信封，包含 exact Run、owner、deployment、anchor、content、authority revision、fulfillment identity 与物理 item lines。

P8.0 只接受：

- 每条 line 有唯一有效 `ItemInstanceId`；
- material definition 与数量有效；
- 同 definition 的多 stack 数量可合并；
- 聚合后 definition 集合与阵眼需求完全一致；
- 每项数量精确相等，不允许不足、超量或额外材料；
- Run、owner、content、deployment 与 anchor 全部匹配。

commit receipt identity 使用排序后的独立 canonical parts，不依赖输入数组顺序，也不使用可产生拼接歧义的分隔字符串。外部 adapter 仍须证明 evidence 来自真实 durable item transaction；P8.0 不自行授予库存权威。

## 5. 部署状态机与 receipts

状态机为：

```text
Planned --Begin@ActionActive--> Deploying
Deploying --CommitAnchor--> Deploying
Deploying --FinalCommitAnchor--> Active
Planned/Deploying --Cancel--> Cancelled
Active --End--> Ended
```

非最终阵眼 commit 是合法的 `Deploying -> Deploying` 进度事件。相同 begin、anchor commit、cancel 或 end 返回原 receipt，不增加 sequence；冲突 evidence、已提交阵眼替换、foreign ActionRuntime 或损坏 deployment 均拒绝。每次 replay 也必须重新通过 Deployment 自校验和 ActionRuntime 绑定。

`IsValid()` 会重算 deployment/anchor/receipt identities，重放所有状态转换，核对连续 receipt sequence、已提交阵眼数量、fulfillment 与最终状态，避免不可变性只存在于注释。

## 6. 自动化证据

最终无头自动化全部通过；每份日志只有一个 `RunTests`、queue-empty、Fail `0`、fatal/assert/ensure `0`，原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` / `FormationDeployment-Targeted.log` | 4 | 0 | `DF89D012FDD02D0A5B01FCFD9E90161B957D2829C86F4AA0E7F9EBF6399BE60F` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 211 | 0 | `0995B59ECADB6CBAAE316019F7E8EC5E626B8E0BE964EEE3FD467CCF006A5380` |

四项新增测试覆盖：

1. diagram capture、显式 order、非法结构、平面几何与完整结构 identity；
2. action commit fence、begin/anchor replay、多 stack 守恒和 final activation；
3. wrong Run/owner/content、缺量、额外材料、重复 item 与未知阵眼的 mutation-free rejection；
4. planned cancel、active end、terminal replay 与 foreign Run rejection。

完整 suite 从 P7.9 的 `207` 增至 `211`。

## 7. 首次失败与修复

首次 Editor integration 原生退出码 `6`，结果 `OtherCompilationError`，总时长 `28.39s`。唯一错误是测试使用 `FShanmenCombatIdFactory` 却未显式包含 `ShanmenCombatResolver.h`；生产实现和 UHT 已通过。补测试 include 后编译成功。失败 UBT SHA-256：`3716853B722D3A2BAE893411BCB21074A0BE533C2A55DEEB9877F892F10FA058`。

首次 targeted 产生 `1 Success / 2 Fail`，随后第四项夹具 `check` 因前置 commit 失败终止，进程原生退出码 `3`。失败日志 SHA-256：`38737C76DBF5191327E66DC64A0DA29B16BED4F7112ABC20F2AEAB2C1DFC85BC`。

根因有两项：

- 测试把原本已经是木材的行再次改成木材，未真实构造 duplicate definition；
- receipt 通用校验错误禁止 `Deploying -> Deploying`，导致合法非最终阵眼进度被拒绝。

修复测试输入，并仅允许 `CommitAnchor` 使用同态状态；其他事件仍要求状态变化。最终又加入完整 diagram identity 和 replay self-validation 后重新编译、重跑全部证据。未通过放宽材料守恒或跳过测试获得成功。

## 8. 改动—回归、静态门禁与构建

- regression map JSON：PASS；
- mapping self-test：`32/32 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=2 Logs=2`；
- 新 rule 防止 unrelated thrown-runtime 子组冒充 formation evidence；
- boundary scan：无 World、Actor、Tick、timer、spawn、damage、legacy/Shanmen item subsystem 或 RNG；
- `git diff --check`：native exit `0`。

统一构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | `OtherCompilationError` | 6 | 28.39s | `3716853B722D3A2BAE893411BCB21074A0BE533C2A55DEEB9877F892F10FA058` |
| Editor final | Succeeded | 0 | 9.90s | `AC642983764D499C8CABE63AD4FB2A4800762E55A37D737D1286A76903B103D4` |
| Game final | Succeeded | 0 | 11.41s | `BD643EEE510758A4B33CFB449B56710D10199FA0754D4B176C7001B2D01006B3` |

- Editor CombatRuntime DLL：`660480` bytes，UTC `2026-08-29T14:57:40Z`；
- Game executable：`352009728` bytes，UTC `2026-08-29T14:58:57Z`。

## 9. 修改范围、兼容性与 P/F 边界

修改范围：

- `ShanmenFormationDeployment.h/.cpp`；
- `ShanmenFormationDeploymentTests.cpp`；
- regression map 与 self-test；
- 本 Report 与同名 Development Log。

未修改 Build.cs、GameplayTags、save schema、Content、ShanmenItems、GameMode、输入或旧产品链；P0–P7 全部 207 项既有测试继续通过。长期未跟踪的用户和 0.0.9B 文件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P8.1 建议建立 ShanmenItems 单向材料事务 adapter：按阵眼 requirements 预留 exact active-Run item quantities，durable commit 后才生成本轮 fulfillment evidence，cancel/失败则释放；仍不在同一阶段创建阵眼 Actor、交互 UI、阵法效果或正式配方。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-0-formation-deployment-core/Docs/Report/Dev.D.UE.0.0.10.P8.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-0-formation-deployment-core/Docs/Log/Dev.D.UE.0.0.10.P8.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-0-formation-deployment-core>
