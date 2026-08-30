# Dev.D.UE.0.0.10.P8.29.r0 Report

## 1. 结论

P8.29 已增加 run-scoped formation influence consumer command host/resolver。上层显式把 subject entity 绑定到 exact `Udemo_mapAttributeComponent`，host 按 command 内冻结的 subject、run 与 content 路由到 P8.28 coordinator。绑定在整个 host 生命周期内只增不换；即使 coordinator 已 drain，旧 subject 也不能换绑新 component，因此历史 Apply command 不会绕过 completed transaction fence 重新生效。

本阶段结论为 **PASS**：host 专项 `4/4`、coordinator `4/4`、adapter `4/4`、registry `4/4`、projection `3/3`、legacy attributes `4/4`、完整 influence 链 `75/75`、`Shanmen.0_0_10` 全量 `324/324`、changed-file regression gate、自检 `90/90`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 run/content 与多 subject 绑定

- `TryOpen` 冻结有效 run/content，并确定性派生 host ID；
- `TryBindSubject` 为每个 subject 创建且仅创建一个 P8.28 coordinator；
- 同一 subject/component 再次绑定返回同一 receipt 的 `BindingReplayed`；
- 同一 subject 换 component 返回 `SubjectBindingConflict`；同一 live component 绑定另一 subject 返回 `ComponentBindingConflict`；
- host 支持多个 subject 并行路由，各 component 的 modifier 与属性值彼此隔离；
- binding receipt 交叉包含 host、subject 与 coordinator identity，route result 必须与嵌套 transaction 的 coordinator/subject 同时吻合才有效。

### 2.2 窄路由，不建立第二套事务

- `TryRoute` 先校验 command、run/content scope、subject binding 与 component 生命周期；
- 合法命令直接委托对应 P8.28 coordinator；host 不生成第二个 application candidate、native acknowledgement 或 completed command history；
- coordinator 的 `Applied`／`Removed` 映射为 `Routed`，且 `bHostStateChanged=true`；
- coordinator 的 immutable replay 映射为 `TransactionReplayed`，且 `bHostStateChanged=false`；
- native rejection 保留完整嵌套 transaction diagnostic 与 handle-conflict evidence，host 自身不吞错、不自动重试。

### 2.3 append-only lifecycle fence

- binding 在 coordinator active 时不可替换；
- exact Remove 后 coordinator drain，但 binding identity 和 completed transaction history继续保留；
- drain 后仍拒绝 replacement component，只允许原 exact binding replay；
- 整个空 host 或所有 coordinator 均 drain 时，`IsDrained()` 才允许上层销毁 host；
- host 聚合 active application 与 completed transaction count，仅作为查询，不镜像每条 application/native modifier。

### 2.4 retry 与历史 replay

- same-handle different-spec native conflict 经 host 返回 `TransactionRejected`，coordinator candidate 未提交；
- 冲突清理后同一 deterministic command 可直接重试；
- native desired state 已预先满足时，P8.27 `ApplyReplayed` 仍可完成第一次 coordinator transaction；
- Apply、Remove 完成后重放旧 Apply，host 返回原 transaction receipt，不复活 modifier，也不增加 completed history。

## 3. 完整性与安全边界

host 只保存 binding receipt 与一个 P8.28 coordinator/subject。active application 权威仍属于 P8.26 registry，native modifier 权威仍属于 `Udemo_mapAttributeComponent`，completed command history仍只属于 coordinator。没有 queue、scheduler、timer、自动 retry、World/Actor discovery、GAS、RNG、存档、inventory 或第二套 modifier collection。

新增 host 生产文件静态扫描：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`TMap = 0`、`ActiveModifiers/PendingCommands/RetryQueue = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerCommandHost.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerCommandHost.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

host header/implementation 分别为 `138/453` 行；新增四个 Automation case。文档加入前，本阶段产品与流程文件新增 `924` 行、删除 `0` 行。长期未跟踪的用户与 0.0.9B 工件未修改、未 stage。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.29-FormationInfluenceConsumerCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost` | 4 | 0 | 0 | observed | `E795948D98DA206E2A6075DE6F1A5D3AACF8BE28AC8A3F90CB1B83B750035CB2` |
| `P8.29-FormationInfluenceConsumerApplicationCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator` | 4 | 0 | 0 | observed | `7A3746AC0CBA093FD52B7B39759089360643245C52FF5EA6686B53CD4E10198E` |
| `P8.29-FormationInfluenceConsumerAttributeAdapter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | observed | `A3BC45A5801DD1E718BD2D3082FB6F43D0E94C786DBD8DCAB8212363B70AB507` |
| `P8.29-FormationInfluenceConsumerRegistry-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | observed | `B43EBDE79907CC6C0B7D191464845B239C9A96D28BAE0A3E00CE848334B23348` |
| `P8.29-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | observed | `322E1CECD8A26AAE4D859D5F7C46209034B88AFA921CD2FB4736D9F4AE00CB42` |
| `P8.29-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | observed | `3B43DE0DFCDA374BFFA3015D1B450D4267114A7A885E17F2E58391B040478CCC` |
| `P8.29-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 75 | 0 | 0 | observed | `4720485DE8EE8E1A197CAD04AF33C130E21208057F85A918394590C54662D503` |
| `P8.29-Shanmen-full-final.log` | `Shanmen.0_0_10` | 324 | 0 | 0 | observed | `027BB2EBB25716489A7248565DC926476B9991481EFD86F69E672367746B32D8` |

八份最终日志各有一个 RunTests command、一个正式 queue-empty，fatal/unhandled/ensure 均为 `0`。

## 6. 专项覆盖与提交前审查

四个 host case 覆盖：

1. 两个 subject/component 独立 Apply/Remove，并聚合 drain/count；
2. exact binding replay、subject replacement conflict 与 component reuse conflict；
3. malformed command、invalid subject、null component、unbound subject 与 content mismatch；
4. native handle conflict 不提交、清理后直接重试；
5. Apply/Remove 后旧 Apply replay 不复活 modifier；
6. active 与 drained 两个阶段均禁止 component replacement，binding receipt 可查询。

首轮 Editor 与首轮八组 Automation 全部成功。提交前审查发现 binding receipt 只关联 host/subject，独立 route result 尚不能证明 receipt 与嵌套 transaction 来自 exact coordinator。最终把 coordinator ID 纳入 binding receipt 派生、consistency 与 `RouteResult::IsSuccess()`，并把 bind 输入收紧为有效 UObject。修正后重新执行 Editor、全部八组 Automation、gate 与双目标构建，全部通过。

## 7. Changed-file regression gate

新增 command-host path rule，并把 host 专项纳入 projection、attribute adapter、application coordinator 与 exact attribute mutation 的反向覆盖。共享 tests 与新增 host 源码共同推导十一组必跑证据。

```text
REGRESSION_MAP_JSON: PASS Rules=65
SELF_TEST: PASS 90/90
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=8
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=11 Logs=8
```

- mapping SHA-256：`8DAD0E10FF9FDF9E32E902EB6E9D240834E1BEA59A18BA644E504644785F3E7B`；
- self-test SHA-256：`13AE7D0E96EE5B63FD0BD6BEA19E56A182918848E1641FE358597FB34DBF8846`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 5 actions / 31.59s | 0 | terminal evidence |
| Editor review correction | Succeeded / 5 actions / 8.77s | 0 | `8789E43551A6ED0B2481F7018AF7CD50A22F71133C91D12073A02F5FC6E0BBCA` |
| Editor final | Succeeded / 0 actions / 0.90s | 0 | `798ECDC256ED5A45D9B6ECA3610D3573608ACBAEDA5409ACACBB75F87001D1C4` |
| Game final | Succeeded / 4 actions / 23.90s | 0 | `497C7BC3CB178AD041B46B57551735CBB92C1A5C017AFEDA3221761D42A84EC0` |

- `UnrealEditor-demo_map.dll`：`11962880` bytes，SHA-256 `EC15B479907872D8C7A1460EE3FE610C10AE8AAEF9122311AF193F43D31E7185`；
- `demo_map.exe`：`353036800` bytes，SHA-256 `26F383D76A12E9E7C5CEADDE93059503E1A3AD407D7BCE67301E619E0D0F409C`。

## 9. 真实异常与兼容性

产品源码、Automation case 与 Editor/Game 构建没有失败。第一次 regression gate 调用使用 `pwsh -File` 直接传数组，PowerShell 把后续 changed path 误判为位置参数；改用 `pwsh -Command` 显式构造数组后 gate 通过。文档加入后的第一次 gate 复查又因 `*-final.log` 同时选中 Editor/Game build log 而按设计 fail closed；把输入收窄为八份 Automation log 后通过。这两项都是验证命令的参数/证据选择错误，不是产品或映射失败。UE 启动时枚举未安装的非 Win64 SDK，Win64 明确为 VALID，不影响原生退出码。

P8.25 projection/command、P8.26 registry、P8.27 adapter 与 P8.28 coordinator wire shape 均未修改。host 通过组合既有接口增加多 subject 路由，不绕过 attribute component，不删除 drain 后 history。`git diff --check` 与最终 `git diff --cached --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.30 增加产品生命周期桥：由现有 product host/session 在明确 subject component 可用时调用 P8.29 bind/route，并把 route result 返回上游；桥不得自动发现 component、自动 retry、替换 drained binding，或复制 command host/coordinator history。
