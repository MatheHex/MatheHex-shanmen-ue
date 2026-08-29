# Dev.D.UE.0.0.10.P8.6.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.6.r0`；
- 基线：`b421a468d39b1eda142e365ca0852b46fc12fd84`（P8.5）；
- 分支：`agent/0.0.10-p8-6-formation-world-coverage`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.6 把 P8.5 纯区域规则接到既有 World entity registry：从调用方明确给出的 live Actor subset 读取 stable EntityId 与当前位置，产出 P8.5 canonical coverage evidence。

明确后置：World discovery、cadence／Tick、enter／leave transition、effect policy、damage／Buff、AI、输入/UI、正式阵法 content 与数值。

## 设计决策

### 显式 source subset

Sampler 不枚举 UWorld，也不拥有目标筛选规则。调用方必须提交明确非空 Actor 列表；这避免新建第二套 Actor registry、隐式全图扫描或把 faction／targeting policy 塞入几何层。

### 既有 registry 是唯一 identity seam

每个 Actor 只通过 `FShanmenWorldEntityRegistry::TryResolveObject` 的 exact actor-wide binding 取得 stable EntityId。Registry 必须 active，且 RunId 与 area 一致；duplicate stable entity 失败关闭，sampler 不修改 Registry。

### 瞬时 pointer、持久值证据

World 与 Actor pointer 只活在同步调用栈。结果复制 stable identity 与 exact live location，再交给 P8.5；返回结构不保存 UObject pointer。输入 Actor 顺序在调用 P8.5 前 canonical 化。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationWorldCoverageSampler.h`；
- `Source/demo_map/demo_mapShanmenFormationWorldCoverageSampler.cpp`；
- `Source/demo_map/demo_mapShanmenFormationWorldCoverageSamplerTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

没有修改 P8.0—P8.5、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。

## 诊断与修正记录

1. 审查 P8.5 area query、P8.3 World delivery 与 `FShanmenWorldEntityRegistry` 后，选择无状态同步 adapter；不引入 subsystem 或 component owner。
2. regression map 增加 WorldCoverage rule，映射自测扩展为 45 项并通过。
3. 首次 Editor source build 成功，原生退出码 `0`；首次 focused 的进程也退出 `0`，但 case 结果为 `2 Success / 2 Fail`。
4. 两个几何测试均显示所有 Actor 位于 `(0,0,0)`。根因是测试使用无 root component 的裸 `AActor`，spawn transform 与移动没有进入 `GetActorLocation()`；Identity／World fences 同轮已通过。
5. 首次失败日志原样保留。测试夹具增加最小 transient scene root 并验证 location，不修改 production sampler。
6. 修正后 Editor source rebuild 原生退出码 `0`，focused `4/4`、full `235/235`，所有门禁和最终双目标构建通过。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationWorldCoverage-first.log` | `Shanmen.0_0_10.Product.FormationWorldCoverage` | 2 | 2 | 0 | `98F3F62681166045CF0B6BCAE7383596E9B80EA1C9C12F36D97A1CF4C554F1CC` |
| `FormationWorldCoverage.log` | `Shanmen.0_0_10.Product.FormationWorldCoverage` | 4 | 0 | 0 | `BDF0BAE0B8181F70FBED20B7AC1EE46E5E4FEDFAD4334445E2C4B380C0F4FD89` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 235 | 0 | 0 | `7CD8C924B925684B5F1B80962801CF1B3A4CF88794263D6398D120A1A1EBAD1C` |

两份最终日志均 one command、one queue-empty、Fail `0`、fatal／unhandled／ensure `0`。引擎启动时的 13 条 UnifiedError self-test `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue-empty 与 fatal／unhandled／ensure 判定。

## 测试内容

1. canonical subset 与 P8.5 Inside／Boundary／Outside 投影；
2. live location 更新、receipt identity 变化与精确重放；
3. Registry inactive／Run mismatch／missing binding／duplicate stable entity；
4. invalid area／World、cross-World 与 destroyed Actor；
5. sampler 不写 Registry，不发布失败的部分结果。

## Changed-file regression gate

`FormationWorldCoverage` rule 要求 WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment 六组证据。focused-only fixture 必须失败；broad full evidence 可覆盖父级 seam。

```text
REGRESSION_MAP_JSON: PASS Rules=42
SELF_TEST: PASS 45/45
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=2
```

- map SHA-256：`90678CB6309F4CD42A22924E803A270597879EE543625451DE5FAB9F6B67D6EA`；
- self-test SHA-256：`C7ECB6929254910A1F1CC4EC981FBCFF15D59BF48DC2F7656D7AD6B535391EF4`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor initial source build | Succeeded | 0 | 13.55s | 后续重编译覆盖 |
| Editor rebuild after fixture correction | Succeeded | 0 | 6.65s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.92s | `A0DEEA350D86123775D1B866EDF040E17130221473044FC4B2A6211492927A60` |
| Game final | Succeeded | 0 | 19.37s | `8CE7A012620FE11167B15F307B8F9AAAE1BF99340C7E19BADD38489AD24D1300` |

- Editor module：`11085312` bytes，UTC `2026-08-29T17:57:55.7152016Z`；
- Game executable：`352295424` bytes，UTC `2026-08-29T18:00:29.6848910Z`。

## 静态、兼容性与 P/F 边界

- map JSON、45/45 mapping self-test、changed-file gate、pointer-ownership scan 与 `git diff --check`：PASS；
- production result 没有 UObject ownership field；sampling 之外不保存 pointer；
- 无 discovery、Tick／timer、overlap、RNG、damage／effect、Registry mutation、input/UI 或 item subsystem；
- staged `git diff --cached --check` 在提交前执行；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- 没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.7 建议用纯 reducer 比较前后 membership receipts，产出 Enter／Stay／Leave facts；不要在 reducer 中隐藏 cadence 或 effect application。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-6-formation-world-coverage/Docs/Report/Dev.D.UE.0.0.10.P8.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-6-formation-world-coverage/Docs/Log/Dev.D.UE.0.0.10.P8.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-6-formation-world-coverage>
