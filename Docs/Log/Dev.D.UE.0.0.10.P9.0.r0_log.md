# Dev.D.UE.0.0.10.P9.0.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P9.0.r0`；
- 基线：`c3af7ad4c1ddeb489692f5e3b7eb5c39f3456a34`（P8.41）；
- 分支：`agent/0.0.10-p9-0-spirit-shield-contract`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

依据人工确认的 0.0.10 战斗规划，在不伪造 formation 产品调用点的前提下，启动术法体系的首个纯 Runtime 增量：短时主动灵力护盾。建立不可变内容、Action commit 后激活、revisioned 容量投影、显式解除及可重放 receipt，不接真实时间、输入、能量、Actor 或 UI。

## P8.42 审计与停止决定

1. 从 P8.41 的 LifecycleCommandHost owner API 向上追踪 FormationProductHost、Session、World delivery 与 GameMode。
2. `FormationProductHost::TryStart` 和 `LifecycleCommandHost::TryOpen` 在生产代码中没有调用者，所有调用均来自 Automation tests。
3. 没有任何现有 owner 同时持有 CombatRun、ProductHost、LifecycleCommandHost 与目标 capability。
4. 继续接线只能新增 wrapper、GameMode glue、扫描或隐式发现，违反既有 authority 边界。
5. 冻结 P8.42，不产生 formation 代码变更；转入规划中的术法体系 P9.0。

## 设计记录

### Definition

新增 Capture/Definition 二段结构。Capture 可 author；Definition 私有字段、只读 getter。固定 canonical Action ID，并捕获 Rule、最大容量与六组 required/blocked tag filters。过滤器若要求某 tag 同时被相同或父级 blocked tag 排除，则 capture 失败。

### Activation

Runtime 状态为 `Uninitialized -> Prepared -> Active -> Deactivated`。Prepare 只冻结 Action/Definition；首次 Activate 必须由匹配的 ActionOrchestrator 在 Active commit 后触发。Receipt 自验证 shield identity、action content 与 definition。

### Projection

每个 projection 接受显式 `AuthorityRevision` 与 `AvailableCapacity`，输出一个 `AbsorbPoints` shield layer。Layer ID 在同一 shield instance 内稳定；projection ID 随 revision/capacity 变化。Runtime 保存最近 projection 以执行：exact replay、same-revision conflict、revision rollback、maximum 与 monotonic-decrease 检查。

所有投影层固定 `bRequiresCommitOnTrigger=true`。P9.0 不拥有容量扣除，后续 adapter 必须以 source instance 和 expected revision 提交 resolver 触发结果。

### Deactivation

解除是显式命令，原因由外部 owner 传入。Runtime 没有 duration 数值、Timer 或 wall-clock。终止 receipt 可 exact replay，不能改写原因，也不能复活或继续投影。

### Identity

使用 `FShanmenDeterministicId::FromCanonicalParts`。tag container 先按 tag name 排序；float 使用 canonical bits；instance identity 包含 Run/Owner/Activation/Source、ContentStamp、Definition 与全部 filters。

## 测试实现

新增五个 Automation leaf：

1. `DefinitionContract`：canonical capture、私有快照、错误 ID、零容量、冲突过滤器；
2. `CommitBoundActivation`：Startup 拒绝、commit 后激活、exact replay、foreign content 拒绝；
3. `RevisionedDefenseProjection`：revision/capacity 单调规则、固定 DefenseLayer 语义、`100 -> 70` resolver 守恒与 commit 标记；
4. `ExplicitDeactivation`：foreign ID、duration 解除、exact replay、reason rewrite、终止后投影与复活拒绝；
5. `DeterministicIdentity`：等价输入复现 instance/receipt/projection/layer ID，不同 content 隔离。

## Changed-file mapping

新增 `SpiritShieldRuntime` 规则。三个新 Runtime 文件除通用 `CombatRuntime` 外，还强制：

- `Shanmen.0_0_10.CombatRuntime.SpiritShield`；
- `Shanmen.0_0_10.CombatCore`。

self-test 新增一条通过 fixture 与一条缺失 CombatCore 的失败 fixture，计数由 `100/100` 提升为 `102/102`。

## 执行序列

1. 审查 P8 formation 生产调用链，确认无安全 caller 并冻结 P8.42。
2. 读取人工 0.0.10 规划，选择短时主动灵力护盾作为 P9.0。
3. 审查 ActionOrchestrator、DefenseLayer/Resolver、Combat tags、DeterministicId 与既有 immutable definition 模式。
4. 实现 Definition、activation/projection/deactivation receipts 与纯值 Runtime。
5. 增加 5 个 focused tests、回归映射规则与两条 self-test fixture。
6. mapping JSON 通过；self-test `102/102`。
7. Editor candidate 单并发构建成功。
8. 初次 focused tests `5/5`，但显式 `Quit` 使日志缺 queue-empty；日志留存但拒绝作为证据。
9. 改用 TestExit-only，focused `5/5`、CombatCore `9/9`、CombatRuntime `39/39`、full `342/342`。
10. 代码复审补充独立的容量回升拒绝断言。
11. Editor final 构建后重跑四组最终 Automation，共 `395/395`。
12. changed-file gate、静态扫描、diff check 与 Game final 构建通过。
13. 生成 Report/Log，执行 exact-stage gate、commit、push 与远端 SHA 核验。

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `SpiritShield-final.log` | 5 | 0 | yes | 0 | `A99751B563C8306B12D936965B7CAF57F1FDD4396C1574A1B9CD41E140D66764` |
| `CombatCore-final.log` | 9 | 0 | yes | 0 | `7FB1F177EF1791829DAB69A49EC2FF5D1AE741136E55A20F4D341FECCE6742D3` |
| `CombatRuntime-final.log` | 39 | 0 | yes | 0 | `8C1B3C4B398955C7FFB1A1E7709A0BA28764B9AC7F817F1C6CE9E16A290CCCE2` |
| `Shanmen-0_0_10-final.log` | 342 | 0 | yes | 0 | `98DE39ED48BA937D63DA9728834A40A1312D84DDDA0AD6E9F4F7DA4A9CBD98CC` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=70
SELF_TEST: PASS 102/102
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=3 Logs=4
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=2 Required=3 Logs=4
EXACT_STAGE: PASS Files=7
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`115C9B554100A72F1EFDF26EE8917E8957F7F5344E80EBD1F21B32F979E4AC5C`；
- self-test SHA-256：`E2F10B6D09AB83646674A989E84C2BC75F503A9B065FA0420008181F726116DE`；
- 世界对象、discovery、Timer/Tick、while、TMap 与 RNG 扫描命中均为 `0`；
- 代码与脚本净变更：`5 files / 1182 insertions`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 8 | 34.68s | 0 | `8A340943425BDBEA2591A774FF1840B0FC9422C2F934D44A0A51F3C56FF92DD6` |
| Editor final | 6 | 11.15s | 0 | `72E1595E42E3598A2DFD39792D407CA16832C42D6FDB93EE94920A67F4242881` |
| Game final | 5 | 29.44s | 0 | `C50077546B883DC5262665F59969E67A81FA7C8A640E60336C30C50860F379C5` |

## 真实异常

无源码、测试、门禁或构建失败。一次测试启动参数产生“测试成功但缺正式 queue-empty”的不合格证据；保留原日志 SHA-256 `449B5ACBA58597D3928FF5287462383BC06AFDD99955F976CDA5311B2598E5AB`，未把它计入正式结果。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
