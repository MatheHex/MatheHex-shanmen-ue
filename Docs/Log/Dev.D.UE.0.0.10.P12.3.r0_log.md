# Dev.D.UE.0.0.10.P12.3.r0 Development Log

## 1. 目标

把 P12.2 的 immutable SwordRhythm receipt 投影为可供 UI／动画／状态机读取的正式表现态，同时保持 Run timeline、Host、receipt 与 damage 权威不变。

## 2. 实现

- 新增 private-field、`BlueprintReadOnly` 的 `Fdemo_mapShanmenSwordRhythmPresentationState`；
- 状态冻结 config/content/receipt/action/timeline/window/tick/revision/count/band；
- `PresentationStateId` 由完整 canonical parts 确定性派生；
- 无状态 projector 提供 7 种 typed projection status，并拒绝 invalid/cross-Run identity；
- Started、PreciseLinked、RestartedEarly、RestartedLate 的窗口、count 与 overflow 不变量由状态自校验；
- ProductSession 在 Host 副本观察后，再原子提交 Host、receipt 与 presentation；
- exact replay 不增加 revision，teardown 清空全部 Run-local 状态；
- GameMode 提供 `BlueprintPure` 只读查询，并在成功日志中关联 presentation identity；
- 没有动画命令、第二时间源、输入、damage 或 balance mutation。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 2 | 0 | `9BD1B173B6E5FAA702E59D783EAE44BB023F5355ACB4AA8ED8A2CF67EACF5229` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | `CE23BFFEDE9062FC4467C6CB88E784FFC9DCB7223CCE7F3086528B3B29CD1D10` |
| `Shanmen.0_0_10` | 553 | 0 | `76917A03FE8748AFA44D163841672A595C5FF9DFDF4457444E780D8C13B7953A` |
| `demo_map.V3.Attributes` | 4 | 0 | `77A77EC385ADB6303903DFC5F9479AE1D46E8D9B76F37C360C023AB10FCDF0D3` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `FD9C2CE9D19FFEEBD9B5DA2147CC34CC024BDB8BDB0D23662E2FC1F53CC6E730` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `6F03D211D294679D7BE811820154A113B36A9870B5C5EC6F8A444F4AF046D1B7` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `0578B823DF2E0C8A3C0E173938ECED9D12A7BB25C524A3CFAAE4AEBFBA62AADE` |

原始日志 `673 Success / 0 Fail`；focused 4 条已包含于 full 553，按 test identity 去重为 `669`。7 个进程原生退出均为 `0`，选定测试阶段 Error/Fatal/Unhandled/Assertion/Ensure 均为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=44 Logs=7
SELF_TEST: PASS 174/174
BOUNDARY_SCAN: PASS CorePaths=4 CoreLines=791 CoreForbiddenHits=0
BOUNDARY_SCAN: PASS GameModeAddedLines=24 GameModeAddedForbiddenHits=0
REGRESSION_MAP_JSON: PASS
git diff --check: PASS (native exit 0)
```

Coverage 日志 SHA-256：`0838C9A28B363C062078C2727C7EEA4DC4D9A04D5D5592C658ABAB82D908A031`。Self-test 日志 SHA-256：`CACA74D299C199EBED1614BD627F7F68AC3530CEB890B6C117A6F63EA25EB711`。

## 5. 构建

- Editor first：26 actions / 120.77s / exit `0` / log SHA `00C94738592CA8FD9A026129B85EEFEAC846CC9CCB7093991DFE27B419CCA39B`；
- Editor final：5 actions / 7.39s / exit `0` / log SHA `0AA9AB33FE1C1AF334103E60A7C12700B261E3033BE74B9973525ED5B458378B`；
- Game final：25 actions / 101.50s / exit `0` / log SHA `335244AFFBCB924E0C351C39AFF15EC780678EEFB4838441515E4F5D12987BAD`；
- `UnrealEditor-demo_map.dll`：13,015,552 bytes / SHA `039E9B34C370957B03A0E298FDD0D7271F5F807F2CF668A824865376CE768800`；
- `demo_map.exe`：354,553,856 bytes / SHA `8D05361CCB42CCA4FBCEEAB0934EBF1508390707AC6D6945622A5F46355BF691`；
- 构建无源码或环境失败。

## 6. 修改与兼容性

不含文档共 9 个路径，`779 additions / 8 deletions`。表现态和 Session 不依赖 World/Actor/timer/RNG/damage API；GameMode 新增代码只记录和复制 read model。没有修改 Impact、Vitality、inventory、schema、GAS、动画资产或输入。长期未跟踪的 0.0.9B 资料保持未暂存。

## 7. 流程与边界

动态循环启动器在创建进程前被本机策略拒绝一次；改为固定参数逐组运行后全部成功。UE 5.8 的 UnifiedError/`Condition failed` 固定启动自测发生在选定 RunTests 命令之前，未掩盖，选定测试阶段无错误标记。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。下一阶段可建立窄的 animation/state consumer adapter，仍不得把 rhythm count/band 转为伤害。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-3-sword-rhythm-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-3-sword-rhythm-presentation/Docs/Report/Dev.D.UE.0.0.10.P12.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-3-sword-rhythm-presentation/Docs/Log/Dev.D.UE.0.0.10.P12.3.r0_log.md>
