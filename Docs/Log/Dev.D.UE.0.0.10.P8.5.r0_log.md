# Dev.D.UE.0.0.10.P8.5.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.5.r0`；
- 基线：`07f32df1d684d311b9c803f2d808d931c2963d7f`（P8.4）；
- 分支：`agent/0.0.10-p8-5-formation-area-provider`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.5 在 P8.3 placement receipts 之上建立纯区域规则，产出可重放的 area、membership 与 coverage evidence。

明确后置：World entity 位置采样、Actor/Registry adapter、区域进入离开事件、效果策略、伤害/Buff、节拍、输入/UI、正式阵图 content 与数值。

## 设计决策

### 水平凸包

未从现有 receipt 猜测 radius、height 或 authored polygon order。三个以上非共线 placement 在 XY 形成唯一、输入顺序无关的 convex hull；少于三点、水平重合或共线集合失败关闭。所有 source anchors 都保留在 snapshot，内部点只是不进入 boundary indices。

### Identity seal

AreaId 绑定 scope、content、每个 placement/receipt/anchor identity 与 exact XYZ bits。snapshot self-validation 重算排序、hull 与 identity。membership 绑定 area/entity/location/relation；coverage 绑定 canonical subject order 与每个 membership receipt。

### 纯边界

Provider 不接 UWorld、AActor、Registry、Items、Action runtime、RNG、Tick/timer、damage/effect、input 或 UI。Boundary 与 Inside 分开保存；consumer 可通过 `IsCovered()` 显式采用二者。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationAreaProvider.h`；
- `Source/demo_map/demo_mapShanmenFormationAreaProvider.cpp`；
- `Source/demo_map/demo_mapShanmenFormationAreaProviderTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

没有修改 P8.0—P8.4、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。

## 诊断与修正记录

1. 完整审查 P8.0 deployment geometry、P8.3 receipt 与 P8.4 host 边界后，选择 order-independent horizontal convex hull。
2. 首次 Editor build：production provider 编译完成；四个测试注册在 UE 5.8 因 `ApplicationContextMask` 不存在而失败，UBT native exit `1`、`OtherCompilationError`。
3. 测试标志按现有 P8 suites 改为 `EditorContext | EngineFilter`，Editor source build exit `0`。
4. focused 首轮 `4/4`、full 首轮 `231/231`。
5. 语义复审增加 unknown relation fail-closed，并把 geometry-seal fixture 改为重新构造合法 receipt，而不是依赖字段原地改写；最终重新编译并重跑 focused/full。
6. 最终 Editor/Game、mapping、boundary 与 diff gates 全部通过。

首次失败属于测试源码 API 兼容错误，不是内存、页面文件或环境故障；按原生退出码如实保留。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationAreaProvider.log` | `Shanmen.0_0_10.Product.FormationAreaProvider` | 4 | 0 | 0 | `5BE98469BF66698DEA374CFB7EF2C27E261CC06552E407C217E2F8AC17F690F9` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 231 | 0 | 0 | `0125A95EAA9C21ACEFF721C6C8EC26CC04465482DCCC697E8D7E5CF6C6AD2AB2` |

两份最终日志均 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`。引擎启动时的 13 条 UnifiedError self-test `Condition failed` 与 P8.4 日志相同，不属于项目 Automation case；项目结果只按 `Test Completed Result`、queue-empty 与 fatal/assert/ensure 判定。

## 测试内容

1. canonical source ordering、CCW hull、内部点证据与 shuffled replay；
2. Inside／Boundary／Outside 与 coverage counts／replay；
3. invalid receipt、mixed scope、duplicate identities/points、insufficient/collinear geometry；
4. invalid/duplicate queries、exact XYZ identity seal、post-capture mutation rejection、horizontal Z semantics。

## Changed-file regression gate

`FormationAreaProvider` rule 要求 AreaProvider、ProductHost、WorldDelivery、Session、WorldGameplay 与 FormationDeployment 六组证据。focused-only fixture 必须失败，broad full 可覆盖 parent seams。

```text
REGRESSION_MAP_JSON: PASS Rules=41
SELF_TEST: PASS 43/43
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=2
```

- map SHA-256：`6F4F47EB5C273F0337281D8E7D4EF10743BC6C7D8C6B48531815FA085EBDD1CE`；
- self-test SHA-256：`26CE9DF25521E8BBA9E0DD5FA088CD8BDB6499450C555B1DE9A95F01B3A7C8F5`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | OtherCompilationError | 1 | 11.87s | 未作为成功证据 |
| Editor final source compile | Succeeded | 0 | 10.42s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 1.06s | `4CC0898B8C90A7E79624DF0AC7C96B6EE49C0086E14980647D2A0956160D9465` |
| Game final | Succeeded | 0 | 23.02s | `2886A1B94A74FE2C512276FB5506F68AB13F5DDCAB54E94A3B2247854AD05A6E` |

- Editor module：`11049472` bytes，UTC `2026-08-29T17:31:10.2297632Z`；
- Game executable：`352265216` bytes，UTC `2026-08-29T17:33:47.8910642Z`。

## 静态、兼容性与 P/F 边界

- map JSON、43/43 mapping self-test、changed-file gate、production boundary scan 与 `git diff --check`：PASS；
- staged `git diff --cached --check` 在提交前执行；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- 没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.6 建议建立窄 World coverage sampler，只把既有 Registry 的 stable EntityId 与当前坐标投影为 P8.5 queries；Actor lifecycle 与效果应用继续留在 adapter 外侧。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-5-formation-area-provider/Docs/Report/Dev.D.UE.0.0.10.P8.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-5-formation-area-provider/Docs/Log/Dev.D.UE.0.0.10.P8.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-5-formation-area-provider>
