# Dev.D.UE.0.0.10.P8.27.r0 Report

## 1. 结论

P8.27 已把 P8.26 registry 接受的 deterministic Apply／Remove 命令接入既有唯一属性权威 `Udemo_mapAttributeComponent`。新 adapter 无状态、无第二张 modifier 表，只在一个受控边界把 consumer projection 的 exact rational magnitude 转为旧组件所需的 `float`，并使用 projection 的确定性 handle 收敛 native modifier 状态。

本阶段结论为 **PASS**：adapter 专项 `4/4`、legacy attributes `4/4`、V3 items `5/5`、ItemUseAndArmor `46/46`、consumer projection `3/3`、consumer registry `4/4`、完整 influence 链 `67/67`、`Shanmen.0_0_10` 全量 `316/316`、changed-file regression gate、自检 `86/86`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 既有属性权威的 exact-handle 收敛 seam

- `AddModifier`／`RemoveModifier` 的 legacy 行为保持不变；legacy Apply 仍生成随机 handle，legacy Remove 仍按 handle 删除并返回是否实际删除；
- 新增 `EnsureModifierApplied(Spec, Handle)`：不存在时按调用方提供的确定性 handle 应用；同 handle、同 spec 返回 `ApplyReplayed`；同 handle、不同 spec 返回 `HandleConflict` 且不改状态；
- 新增 `EnsureModifierRemoved(Spec, Handle)`：精确匹配时删除；已不存在时返回 `RemoveReplayed`；同 handle、不同 spec 返回 `HandleConflict` 且不误删；
- exact seam 复用组件自己的 `ActiveModifiers` 与 `Recalculate()`，没有新增镜像权威。

### 2.2 单一 fixed-point 到 float 边界

- `TryBuildModifierSpec` 只从有效 consumer projection 构造旧 `Fdemo_mapModifierSpec`；
- source、attribute、operation 与 priority 直接保留 projection 证据；
- magnitude 只在该函数内执行 `int64 / int64 -> double -> float`；拒绝非有限值、超出 float 范围和非零值下溢为零；
- signed magnitude 被保留，专项验证 `-75 / 100 -> -0.75f`。

### 2.3 无状态同步与 acknowledgement

- `Synchronize` 只接受 registry 的 success result，并交叉核验 immutable receipt 的 command operation 与 application status；
- Apply／Remove 分别调用 exact native seam，不自行保存 active modifier；
- acknowledgement 封存 application receipt、native status 与 native modifier spec；ID 由 receipt/application/handle/spec evidence 确定性派生；
- lost acknowledgement 后重放返回同一语义 acknowledgement ID；`Applied` 与 `ApplyReplayed`、`Removed` 与 `RemoveReplayed` 仍分别记录本次 native outcome。

### 2.4 补偿与冲突安全

- registry 已接受 Apply、但 native 从未收到时，后续 accepted Remove 在空组件上返回 `RemoveReplayed`，最终状态安全收敛；
- 同 deterministic handle 已被不同 spec 占用时，Apply 和 Remove 都返回 native rejection；
- 冲突 Remove 必须携带期望 spec，不能仅凭 handle 删除，因此不会破坏非本 command 所拥有的 modifier；
- null component、rejected registry result、篡改的 status/operation correlation 均 fail closed。

## 3. 完整性与安全边界

adapter 不拥有 Actor、World、GAS、计时器、异步任务、RNG、持久化、inventory 或 modifier collection。它不查询 profile，也不创建第二套属性运算；native 数值重算仍只发生在 `Udemo_mapAttributeComponent`。

新 adapter 生产文件静态扫描：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`Timer/Async/RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`TMap = 0`、`TArray<Fdemo_mapActiveModifier> = 0`。唯一数值边界位于 `TryBuildModifierSpec`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.cpp`。

更新：

- `Source/demo_map/demo_mapAttributeTypes.h`；
- `Source/demo_map/demo_mapAttributeComponent.h`；
- `Source/demo_map/demo_mapAttributeComponent.cpp`；
- `Source/demo_map/demo_mapAttributeTests.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

adapter header／implementation 分别为 `79`／`283` 行。文档加入前，既有 tracked 文件净改动为 `483` 行新增、`11` 行删除；长期未跟踪的用户与 0.0.9B 工件未被修改或纳入提交。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.27-FormationInfluenceConsumerAttributeAdapter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | observed | `DC706A357A346A19729CD564C93238A9EB187755C5E01280511E62F5CF102FFE` |
| `P8.27-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | observed | `044FDE0F0B9D3DAD28502E8D8A66E1E1101CAAE7863B810CE0FEBEE13DAEE23E` |
| `P8.27-V3Items-final.log` | `demo_map.V3.Items` | 5 | 0 | 0 | observed | `21983E9FCB2DA37A6E4053F03C00E1350BACD70EFAB6892220232A903C7A569C` |
| `P8.27-ItemUseAndArmor-final.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | observed | `5944116507DB7ECCAC1EB3C014FA8070EE43748B0873BB0AA6A9E243BBEFA858` |
| `P8.27-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | observed | `085E488AE248CD2509EB4558B03B4447EC1B26F06002403AD3B14C6ECAF2471E` |
| `P8.27-FormationInfluenceConsumerRegistry-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | observed | `22CF46C10AD1F0C6BC853D487B36D9C3457206ABEC58C5490DD17576B57F5E1F` |
| `P8.27-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 67 | 0 | 0 | observed | `34A35F2AC090FEF6A6D293EBB079CED8F7ECB45516AB8125B5EF2CE727CA9D82` |
| `P8.27-Shanmen-full-final.log` | `Shanmen.0_0_10` | 316 | 0 | 0 | observed | `8869C4B73651866C1AAF96F83B793C965226E533D5BFA5F0FD0C749D92EBF436` |

八份最终日志 fatal／unhandled／ensure 均为 `0`。

专项覆盖：

1. exact Apply 与 lost-ack replay 只保留一个 native modifier；
2. exact Remove 与 lost-ack replay 恢复 baseline 且不重复变更；
3. accepted Apply 未抵达 native 时，Remove 在空状态安全补偿；
4. Apply／Remove 的 same-handle different-spec 冲突均不改权威状态；
5. signed rational 转换、null component、rejected result 与 evidence tamper fail closed；
6. legacy attribute、V3 item 与 ItemUseAndArmor 链继续通过。

## 6. 首轮与提交前审查

首次 Editor 构建成功：`186 actions / 520.11s / exit 0`；首轮八组 Automation 全绿。提交前手工审查仍发现一项真实风险：若 exact Remove 仅按 handle 收敛，极端 deterministic handle 冲突会删除同 handle、不同 spec 的既有 modifier。

最终修正把 exact Remove 改为同时携带并校验期望 spec；legacy `RemoveModifier(handle)` 保持原语义。专项测试增加冲突 Apply 后的冲突 Remove，并断言错误 modifier 仍然存在。修正后重新执行 Editor 构建、全部八组 Automation、regression gate 与 Game 构建，均通过。没有源码编译失败或 Automation case 失败。

一次本地 self-test 误用了 Windows PowerShell 5.1，因脚本采用 PowerShell 7 管道换行语法而产生 parser error；改用项目既有 `pwsh` 后 `86/86` 通过。该错误属于命令解释器选择，不是产品、测试或脚本逻辑失败。

## 7. Changed-file regression gate

新增 adapter 与 exact attribute mutation 两条 path rule；共享 projection tests 同时命中 projection rule。`git diff --name-only` 中每个本阶段产品路径都由映射推导所需证据。

```text
REGRESSION_MAP_JSON: PASS Rules=63
SELF_TEST: PASS 86/86
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=11 Logs=8
REGRESSION_COVERAGE: PASS Changed=11 Rules=3 Required=11 Logs=8
```

- mapping SHA-256：`5608BC29C79651B499BFE84C3CB4212CE115D6EDB79421530334BA82A59228ED`；
- self-test SHA-256：`ACA5C684EAFA9AC3AFE56176C06DAF25039BAB78B5FA2E6E0050F84EEE3579B1`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 186 actions / 520.11s | 0 | `B764E6A59A945A20826931BC7A94E6898CD059D09697EBDB5011CF8B92151FA7` |
| Editor final | Succeeded / 25 actions / 73.06s | 0 | `52BC4494E317DA559B1C7FF606B5C41A524ABBB6D3FC04E2CD39601F133251E2` |
| Game final | Succeeded / 185 actions / 457.83s | 0 | `AA8DD37709C04E9B3209533DD4BD9355485DB4B23E79A324EB37F422E96AFAD1` |

- `UnrealEditor-demo_map.dll`：`11897856` bytes，SHA-256 `8893BC77D0A9D0BAB83C0E79E11B12733CD9AF6016A8401A3B97BF271C8CE88F`；
- `demo_map.exe`：`352979456` bytes，SHA-256 `7DF619AC4A7EBC4B39B07AF615FA9DD0E24463AA10A8D080CD79128D6802AED6`。

## 9. 兼容性与工作区保护

- P8.25 projection wire shape 与 P8.26 registry/receipt 未修改；
- legacy attribute caller 不需要提供 deterministic handle，既有随机 handle 与 bool Remove API 继续工作；
- adapter 只依赖 registry evidence 和既有 component，不持有 UObject 生命周期；
- native rejection 不伪造 acknowledgement，也不自动回滚 registry；后续 coordinator 必须显式处理跨权威顺序与补偿；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；本阶段只 exact-stage 上述十一份文件。

`git diff --check` 与最终 `git diff --cached --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.28 增加窄 consumer application coordinator：绑定正确 target attribute component，规定 registry 与 native adapter 的调用顺序，持久化/转发 acknowledgement，并对 native rejection、teardown 与 retry 做显式补偿；不得把 adapter 扩成第二套 registry 或 attribute authority。
