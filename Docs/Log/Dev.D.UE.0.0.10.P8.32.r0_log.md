# Dev.D.UE.0.0.10.P8.32.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.32.r0`；
- 基线：`71cdbbe8c2f20c466c4e71fb81697a5604c437d3`（P8.31）；
- 分支：`agent/0.0.10-p8-32-formation-influence-consumer-product-runtime`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.24–31 已完成的 projection、registry、adapter、coordinator、command host 与 product bridge 组合为一个显式 product consumer lifecycle facade。runtime 只接收 caller-provided ProductHost/component/frozen command，负责 Apply、Remove、immutable replay 与 teardown readiness，不增加发现、调度、重试、持久化或第二套权威。

## 设计记录

### 独立于旧 influence execution runtime

仓库已有 P8.16–22 的 formation influence execution/lifecycle runtime，职责是 lease 与 Host 调度。P8.32 没有复用或扩张该类型来管理 native attribute consumer，而是建立名字明确的 `ConsumerProductRuntime`；两者分别位于 lease execution 与 product attribute consumption 两条既有链上，避免形成含混的大型 runtime。

### caller-owned dependencies

runtime 内部只拥有一个值类型 product bridge。ProductHost、subject component 与 command 均由调用者逐次提供并验证，不保存 raw/UObject pointer，也不从 World 或 subsystem 发现依赖。

### ordered fail-closed activation

activation 在 append-only binding 前完成 command validity、Apply operation、exact product identity 与 exact subject 检查。binding 只在全部 preflight 通过后发生；native mutation 仍由现有 bridge/command host/coordinator/adapter/registry 链执行。

### explicit teardown

deactivation 必须提供 Remove command。runtime 不缓存 Apply 来合成 Remove；teardown readiness 只读取 bridge drained 状态，不自动清理。ProductHost terminal 状态也不由 runtime 改写。

## 执行序列

1. 复审 P8.16–22 execution/lifecycle runtime 与 P8.24–31 consumer product 链，确认职责不可合并。
2. 新增 deterministic `ConsumerProductRuntime`、typed status/result 与嵌套证据。
3. 实现 ProductHost/product identity/operation/subject preflight、append-only bind、Apply/Remove route 与 teardown readiness。
4. 增加四个 Automation case，并通过真实 ProductHost→area→intent→evaluator→lease→projector 生成命令。
5. 扩展 regression map 为 67 条，新增 15 组 runtime rule，并补齐 bridge/host/session/consumer/attribute 反向依赖。
6. 自测增加 runtime 正例与 focused-evidence 负例，由 `92/92` 增至 `94/94`。
7. 首次 Editor 编译失败：fixture 调用了不存在的 modifier factory；改为现有 `TryCreate` 后编译通过。
8. 首次专项日志为 `2/4`：两项测试在 Remove 后读取激活态；改为 Remove 前冻结快照，产品源码未变。
9. runtime 重跑 `4/4`，随后十一组映射回归全部通过；influence `84/84`、全量 `333/333`。
10. regression gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建全部通过。
11. 生成 Report/Log，准备 exact-stage、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.32-FormationInfluenceConsumerProductRuntime-final.log` | 4 | 0 | 0 | `3637252E13C518C8490335F0F70ADFF4830DE4EAC24AE8B4B755CCB866CE4C2F` |
| `P8.32-FormationInfluenceConsumerProductBridge-final.log` | 5 | 0 | 0 | `CBD6CDC405337C09B07BA18336A8D7CAE165BFD6969EE9B3CF147A8328659330` |
| `P8.32-FormationProductHost-final.log` | 6 | 0 | 0 | `80386DA39D9ECD162300C5A7274BA1C627123E2C1A5E5D18122E51ED6D28399C` |
| `P8.32-FormationSession-final.log` | 4 | 0 | 0 | `9660FB2655525A81E30E8E089017FFACB101C554FF9DBE63B5B998CFDD883103` |
| `P8.32-FormationInfluenceConsumerCommandHost-final.log` | 4 | 0 | 0 | `1D3A93CACE25F7A192F94E6A48EB10D6AAC39F77E1CF2797FEDA6CB5903B1BEC` |
| `P8.32-FormationInfluenceConsumerApplicationCoordinator-final.log` | 4 | 0 | 0 | `56D3350FE592CBFFBEFF7B053BBE9BDFF5D2FC8410A75878BB6937B7BFE433CC` |
| `P8.32-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `C706615D076D5AFDED372DBAEA69F26074C154B82127CCCD7F50196F60AE8E8B` |
| `P8.32-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `AC95CBF757B8711BE4E0F0498DB3F3879C7CAD419C87D0CE474FAED869EF3855` |
| `P8.32-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `C4C19248B411207809513073AB923EDD60535D5E07637D72CFC30869C3415E2B` |
| `P8.32-Attributes-final.log` | 4 | 0 | 0 | `FE036CB4B977AAA640EC3C3108EB196B728827651AF442F3015C1D583625EADD` |
| `P8.32-FormationInfluence-final.log` | 84 | 0 | 0 | `D1DF559CD783B475EA30D58A4A7E97F8B9CA68271E466DEDB0BB48417D13674C` |
| `P8.32-Shanmen-full-final.log` | 333 | 0 | 0 | `03A023C32DFE0E845B4FA383AA9E189341473BF13C29685F6AD9D4FE7A1AA12B` |

全部最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=15 Logs=12
```

- mapping SHA-256：`A934B8BA4DB6BEDD55612973306F3FA234709E331BA9739842C0FC3CE2DCCB68`；
- self-test SHA-256：`1D058B83B572948C5AB0C223459F255DE6292D4EE0E37BD0444FC8596F3DBCA1`。

## 静态边界

```text
UWorld/AActor = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
RNG = 0
SaveGame/ProfileRepository = 0
Tick/while = 0
GetSubsystem/FindComponent/TActorIterator = 0
Retry/PendingCommands/ActiveModifiers = 0
TMap = 0
```

## 构建证据

- Editor initial：生产 runtime 编译通过，test fixture 因错误 factory 名称出现 `C2039/C3861`，原生退出码 `1`；
- Editor corrected：`4 actions / 6.22s / exit 0`；
- Editor after assertion correction：`4 actions / 6.30s / exit 0`；
- Editor final：`0 actions / 0.95s / exit 0`，log SHA-256 `FC4F550C9877A61EB32050267CBC865D889EA0ADBB37EF8D24CBB659E47BA02B`；
- Game final：`4 actions / 24.33s / exit 0`，log SHA-256 `8A5D626E0BCDEECA482A90644A95050253561E044AE4C7C65C7AC8BB8EEBC126`；
- `UnrealEditor-demo_map.dll`：`12048896` bytes，SHA-256 `5E5A2EC84E92CEF5BCADAB85AADEF159F0B5C8815CA4B91504B8235ACF31D835`；
- `demo_map.exe`：`353101312` bytes，SHA-256 `B282C33CAF057F5825FA4B91EC01D54A1CB7606A0DB016685875F4073A270E07`。

## 真实异常记录

### 首次编译

首次 Editor build 在 `demo_mapShanmenFormationInfluenceConsumerProductRuntimeTests.cpp` 报 `C2039/C3861`，原因是 fixture 使用了不存在的 `TryCreateOffensePowerAdditive`。现有正式 API 为 `TryCreate`；修正调用名后编译通过。该错误没有发生在生产 runtime，也没有被描述为环境或内存故障。

### 首次专项测试

首次专项进程退出码为 `0`，但只有 `2 Success / 2 Fail / queue-empty`。失败断言在所有操作完成后读取 runtime 当前计数，而此时 Remove 已正确清空 active application；改为在 Remove 前冻结激活态快照后 `4/4`。失败日志 SHA-256：`4E422AA40D2CB3C854DD48C6D60CB45F84AB7D13637C364AA848099D3088F283`。

这两次修正均只影响测试夹具/断言，不改变产品 runtime 语义。最终聚合与全量测试验证新测试被父组实际收录。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.33 可增加 caller-owned ProductHost lifecycle integration，在产品 teardown 前调用 runtime readiness，并显式传入已知 component 与 frozen Apply/Remove commands；继续禁止自动发现、调度、重试与第二套权威。
