# Dev.D.UE.0.0.10.P12.4.r0 Development Log

## 1. 目标

把 P12.3 的 immutable SwordRhythm presentation state 适配为可供 UI／动画／状态机重复轮询的确定性表现事件，同时保持 Session、receipt、timeline 与 damage 权威不变。

## 2. 实现

- 新增 private-field、`BlueprintReadOnly` 的 `Fdemo_mapShanmenSwordRhythmPresentationEvent`；
- 事件冻结 presentation/Run/receipt/activation/timeline identity、cue、revision、ticks、window 与 tick rate；
- `EventId` 由完整 canonical parts 确定性派生，`IsValid()` 会重算 identity；
- 无状态 adapter 提供 4 种 typed status，并把 P12.3 的四种 band 映射到四种 presentation cue；
- 相同 state 重复适配保持同一 EventId，各消费者独立去重，不建立共享 cursor；
- GameMode 提供 `BlueprintPure` 事件查询，首个动作前 fail closed；
- 没有 dispatcher、delegate、动画命令、第二时间源、输入、damage 或 balance mutation。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmPresentationEvent` | 2 | 0 | `3952FAE31551E0BDCF3BCA1D0BC1D59DEC2018EEDD47C6D2AF9B371DB2D57AC5` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | `D971DC1086528382CCF44C7D8EE8E4B8D7D037602F00AD89702A6D8A2ED96F1A` |
| `Shanmen.0_0_10` | 555 | 0 | `32388AF2DE3A746F30EE12DF02961E7646E3B9B25ED7EBB0CEAD4BA9DFB3AD49` |
| `demo_map.V3.Attributes` | 4 | 0 | `2990711E253A7398313E39945C5842C362108B6DBE16002BC3EE4C8B83FC2ADB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `5EF6BB9E6B544DF95E97F6D41F333BF4958408F09ED75E7F31711178AC8A32A4` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `781E8577F6CE1FEBF1AC20AD13EFA8683FF7C8C253242FE5F4E804F364D6BA15` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `98D7124CC8AB6E57393493E09587A94EDB8CB098A662BC20D4352B793765F5F6` |

原始日志 `677 Success / 0 Fail`；focused 6 条已包含于 full 555，按 test identity 去重为 `671`。7 个最终进程原生退出均为 `0`，选定测试阶段 Error/Fatal/Unhandled/Assertion/Ensure 均为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=45 Logs=7
SELF_TEST: PASS 176/176
BOUNDARY_SCAN: PASS CorePaths=2 CoreLines=342 CoreForbiddenHits=0
BOUNDARY_SCAN: PASS GameModeAddedLines=24 GameModeAddedForbiddenHits=0
DAMAGE_SCAN: PASS CoreDamageHits=0
REGRESSION_MAP_JSON: PASS
git diff --check: PASS (native exit 0)
```

Coverage 日志 SHA-256：`2F066C2D39D11E44E5108BA5D0B54D55909EE8AB559BF28CD94C31039D481198`。Self-test 日志 SHA-256：`DFF268C304A19751D44840CCC89907DD5C564A632931254707A69EABDF5D4B6A`。

## 5. 构建

- Editor first：25 actions / 121.13s / exit `0` / log SHA `EC25678129BD8852B45C165D888A16A39A02CEFF53224267315D4464609C001B`；
- Editor final：0 actions / 0.91s / exit `0` / log SHA `BD448C22AD7ECB2C4AB744CEE78B7E9B8F28B7259D799B80A04BCA257E44A901`；
- Game final：24 actions / 106.85s / exit `0` / log SHA `C715188D8D104C2F18067F36E91B5509F1A73D5387ABB1654EFE2F1C2132ED4B`；
- `UnrealEditor-demo_map.dll`：13,044,736 bytes / SHA `9D2C983A017CE30CD323F6AA2EC3529F659316879DB1170941F6D12DE01CF55E`；
- `demo_map.exe`：354,576,384 bytes / SHA `07F01424C4082B710EAE0259A14365436A61214B25036C294FFEC72E941A0682`；
- 构建无源码或环境失败。

## 6. 修改与兼容性

不含文档共 7 个路径，`683 additions / 2 deletions`。事件生产代码 342 行不依赖 World/Actor/timer/RNG/delegate/damage API；GameMode 新增代码只查询 state、适配并复制事件。没有修改 Impact、Vitality、inventory、schema、GAS、动画资产或输入。长期未跟踪的 0.0.9B 资料保持未暂存。

## 7. 流程与边界

本轮出现三次证据生成方式修正：Windows PowerShell 5.1 不支持既有脚本的 PowerShell 7 语法；相对 `-Log` 没有生成可审计文件；带 `;Quit` 的测试组名被 changed-file gate 正确拒绝。三处均在最终结论前修正并覆盖重跑，最终证据只采用 PowerShell 7、绝对日志与原生 `TestExit` 结果。没有产品源码、测试 case 或构建失败。

UE 5.8 的固定启动自测噪声发生在选定 `RunTests` 命令之前，未掩盖；选定测试阶段错误标记为 `0`。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。SwordRhythm 的 P 阶段 presentation handoff 已闭合；真实动画与手感验证留给正式资产和 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-4-sword-rhythm-presentation-event>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-4-sword-rhythm-presentation-event/Docs/Report/Dev.D.UE.0.0.10.P12.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-4-sword-rhythm-presentation-event/Docs/Log/Dev.D.UE.0.0.10.P12.4.r0_log.md>
