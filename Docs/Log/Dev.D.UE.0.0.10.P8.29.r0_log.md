# Dev.D.UE.0.0.10.P8.29.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.29.r0`；
- 基线：`cdad0770403b04814be9526dc1de6a8d0e58c91f`（P8.28）；
- 分支：`agent/0.0.10-p8-29-formation-influence-consumer-command-host`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

在 P8.28 single-subject coordinator 之上增加窄 run-scoped consumer command host：显式注册 exact subject/component binding，按 command 冻结的 subject/run/content 路由，保留 coordinator 原始 transaction/replay 语义，并用 append-only binding fence 防止 drain 后换 component 重新接纳历史 Apply。不得复制 registry、native modifier 或 coordinator completed history。

## 设计记录

### append-only resolver

host 使用 `TArray` 保存 subject binding record；每条 record 只含 binding receipt 与一个 P8.28 coordinator。subject 与 live component 各自一对一。binding 在 active/drained 阶段都不能替换，整个 host 仅在所有 coordinator drained 时允许 teardown。

### single transaction authority

route 不创建新 candidate 或 ledger。P8.28 transaction 成功即映射为 Routed；P8.28 immutable replay 映射为 TransactionReplayed；native/registry rejection 原样嵌套返回。host 只聚合 count，不维护 active application mirror。

### receipt correlation correction

提交前审查把 coordinator ID 加入 binding receipt 派生和 route result 自验证，确保公开结果的 binding 与 transaction 来自 exact coordinator；同时 bind 只接受有效 live UObject。

## 执行序列

1. 审查 ProductHost、LifecycleCommandHost、P8.28 coordinator 与多 subject 路由模式。
2. 确定 run-scoped、append-only、无 unregister 的 resolver 边界。
3. 新增 binding receipt/result、route result 与 consumer command host。
4. 实现 exact bind replay、双向 conflict、scope check、direct coordinator delegation、aggregate drain/count。
5. 增加 MultiSubjectRoutingAndDrain、BindingAndScopeFence、RetryAndHistoricalReplay、AppendOnlyLifecycleFence 四个 case。
6. regression map 增至 `65` 条，自检由 `88/88` 增至 `90/90`。
7. 首次 Editor `5 actions / 31.59s / exit 0`；首轮八组 Automation 全绿。
8. 审查修正 receipt/coordinator 关联与 live UObject gate；review Editor `5 actions / 8.77s / exit 0`。
9. 最终八组 Automation 全绿，gate 为 `Changed=5 / Rules=2 / Required=11 / Logs=8`。
10. 最终 Editor up-to-date，Game `4 actions` 成功；完成静态检查、文档与 exact-stage。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.29-FormationInfluenceConsumerCommandHost-final.log` | 4 | 0 | 0 | `E795948D98DA206E2A6075DE6F1A5D3AACF8BE28AC8A3F90CB1B83B750035CB2` |
| `P8.29-FormationInfluenceConsumerApplicationCoordinator-final.log` | 4 | 0 | 0 | `7A3746AC0CBA093FD52B7B39759089360643245C52FF5EA6686B53CD4E10198E` |
| `P8.29-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `A3BC45A5801DD1E718BD2D3082FB6F43D0E94C786DBD8DCAB8212363B70AB507` |
| `P8.29-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `B43EBDE79907CC6C0B7D191464845B239C9A96D28BAE0A3E00CE848334B23348` |
| `P8.29-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `322E1CECD8A26AAE4D859D5F7C46209034B88AFA921CD2FB4736D9F4AE00CB42` |
| `P8.29-Attributes-final.log` | 4 | 0 | 0 | `3B43DE0DFCDA374BFFA3015D1B450D4267114A7A885E17F2E58391B040478CCC` |
| `P8.29-FormationInfluence-final.log` | 75 | 0 | 0 | `4720485DE8EE8E1A197CAD04AF33C130E21208057F85A918394590C54662D503` |
| `P8.29-Shanmen-full-final.log` | 324 | 0 | 0 | `027BB2EBB25716489A7248565DC926476B9991481EFD86F69E672367746B32D8` |

全部日志各有唯一 RunTests、正式 queue-empty、Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=65
SELF_TEST: PASS 90/90
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=8
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=11 Logs=8
```

- mapping SHA-256：`8DAD0E10FF9FDF9E32E902EB6E9D240834E1BEA59A18BA644E504644785F3E7B`；
- self-test SHA-256：`13AE7D0E96EE5B63FD0BD6BEA19E56A182918848E1641FE358597FB34DBF8846`。

## 静态边界

```text
UWorld/AActor = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
RNG = 0
SaveGame/ProfileRepository = 0
Tick/while = 0
TMap = 0
ActiveModifiers/PendingCommands/RetryQueue = 0
```

## 构建证据

- Editor initial：`5 actions / 31.59s / exit 0`；
- Editor review correction：`5 actions / 8.77s / exit 0`，SHA-256 `8789E43551A6ED0B2481F7018AF7CD50A22F71133C91D12073A02F5FC6E0BBCA`；
- Editor final：`0 actions / 0.90s / exit 0`，SHA-256 `798ECDC256ED5A45D9B6ECA3610D3573608ACBAEDA5409ACACBB75F87001D1C4`；
- Game final：`4 actions / 23.90s / exit 0`，SHA-256 `497C7BC3CB178AD041B46B57551735CBB92C1A5C017AFEDA3221761D42A84EC0`；
- `UnrealEditor-demo_map.dll`：`11962880` bytes，SHA-256 `EC15B479907872D8C7A1460EE3FE610C10AE8AAEF9122311AF193F43D31E7185`；
- `demo_map.exe`：`353036800` bytes，SHA-256 `26F383D76A12E9E7C5CEADDE93059503E1A3AD407D7BCE67301E619E0D0F409C`。

## 真实异常记录

产品源码、Automation 与构建没有失败。第一次 regression gate 以 `pwsh -File` 传数组时发生 PowerShell 参数绑定错误；改为 `pwsh -Command` 显式数组后通过。文档加入后的第一次复查使用过宽的 `*-final.log`，把 Editor/Game build log 当作 Automation evidence，gate 正确拒绝；仅传八份 Automation log 后通过。UE 非 Win64 SDK 枚举警告未影响 Win64 VALID 状态或退出码。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.30 可由既有 product host/session 显式提供 subject component 并调用 P8.29 bind/route；不得自动发现 component、自动 retry、替换 drained binding，或复制 host/coordinator history。
