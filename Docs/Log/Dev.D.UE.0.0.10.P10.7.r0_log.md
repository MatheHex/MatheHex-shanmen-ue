# Dev.D.UE.0.0.10.P10.7.r0 Development Log

## 目标

把 P10.6 Spirit Evasion Component 安装到玩家运行时，并建立只消费外部冻结 payload 的 typed command route；不在 GameMode/输入层制造 action/content 身份，不复制 Host 状态，不建立临时 SpiritEnergy。

## 审计结论

1. P10.6 组件已经是唯一 UE 生命周期桥，但尚未自动安装；
2. `Fdemo_mapCombatRunCoordinator` 是当前 Run 与玩家 World Entity 的产品权威；
3. Coordinator 尚无通用 Spirit Evasion activation reservation，因此 P10.7 只能验证外部 snapshot，不能擅自生成 sequence/GUID；
4. 现有 GameMode 已集中安装 Health/Skill/Attribute，适合作为唯一组件安装点；
5. combat Run release 会收口 controlled/thrown weapon，但原先没有 Spirit Evasion；
6. PlayerController 输入体量大且 content/resource authority 未完成，本阶段不应绑定按键；
7. regression map 已按 GameMode 变更要求完整 0.0.10 产品链和两个 legacy 根组。

## 设计决策

1. 新增 payload-free control kinds：Cancel、Interrupt、FinishRecovery；
2. Start command 捕获 frozen action/definition/policy/trajectory/direction；
3. Router 不提供任意 Kind setter，控制信号只能由 typed factory 创建；
4. Start capture 要求 canonical action-definition binding 与 normalized finite direction；
5. Start route 要求 Coordinator ready；
6. action RunId 必须等于 active Run；
7. action SourceEntityId 必须等于 Coordinator player entity；
8. owner object 必须在 registry 中解析为该 source；
9. component owner 必须等于 routed Character；
10. control signal 不依赖 active Coordinator，以便 teardown；
11. Router 不保存 ledger、active bit、action 或 motion state；
12. installation 仅允许 0→1 或 1→1，多于一个 fail closed；
13. GameMode mission readiness 要求安装成功；
14. Run release 在 Coordinator 释放前 typed-interrupt 非终态 Host；
15. 新 Run 激活拒绝遗留非终态 Host；
16. automation-only explicit-time seam 不进入 Shipping；
17. 不绑定 input、不调用 displacement mutation、不读写 SpiritEnergy。

## 执行序列

1. 审计 CombatRunCoordinator action construction、现有 command routers、GameMode 安装和 release 路径。
2. 创建 `Fdemo_mapShanmenSpiritEvasionCommand`、installation/result/status contracts。
3. 实现无状态 Router 的 owner/Run/source fences 与 typed dispatch。
4. 实现 idempotent unique installation。
5. 在 GameMode 初始化与公开 route seam 接线。
6. 新增七个 focused tests。
7. regression map 增至 84 rules；self-test 增至 128 cases。
8. 首次 Editor candidate 24 actions、122.96s、原生退出 0。
9. focused candidate `7/7`、原生退出 0。
10. 代码审查识别 Run release 生命周期缺口，增加 interrupt-before-release 与 stale-host activation fence。
11. 增量 Editor candidate 22 actions、76.68s、原生退出 0。
12. 识别 UE 5.8 native completion marker 变化，门禁兼容 old queue-empty/new TEST COMPLETE 并补一条自测。
13. mapping self-test `128/128`。
14. 串行执行四组正式 Automation，共 `503` success、`0` fail。
15. changed-file gate 以 `Changed=8 / Rules=2 / Required=24 / Logs=4` 通过。
16. 静态扫描、`git diff --check` 和精确八文件暂存区通过，`+1246/-6`。
17. Editor final 0 actions、0.89s；Game final 23 actions、106.48s；原生退出均为 0。
18. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
future action/content authority
  -> frozen action + definition + policy + trajectory + direction
  -> GameMode.RouteSpiritEvasionCommand
      -> EnsureInstalled(Character): 0->1 / 1->1 / duplicate reject
      -> Router owner + active Run + registry source fences
      -> P10.6 Component
      -> P10.5 Host -> P10.4 -> P10.3 swept authority

cancel / interrupt / finish-recovery
  -> payload-free typed signal
  -> same owner component
  -> exact Host operation

combat Run release
  -> typed interrupt if Host is nonterminal
  -> existing controlled/thrown teardown
  -> Coordinator end
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| SpiritEvasionCommandRouter | 7 | 0 | `2FDDA039E9957E9241606CA9FD0D9B7C5219F3EB28E0BF58D358F7E9B7AC0E12` |
| Shanmen.0_0_10 | 430 | 0 | `465E8030F8EF94F63FFE9AF1A0FACE0CCBA1C691AD9187FBADC3E3452017A528` |
| EnemySkillFramework | 44 | 0 | `A061DF8A7283AF74BCA97916E7A3096F1F359E0C4C0E7B8E0C999DECC2F960C8` |
| V2RangedCompatibility | 22 | 0 | `054B7CC55545A44D8CC49CDCE111C4D61E3D68A530EDFDED41DD416E27EF4996` |

所有正式日志均为 native exit `0`、TEST COMPLETE `1`、Fail `0`、fatal/unhandled/ensure `0`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=84
SELF_TEST: PASS 128/128
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=24 Logs=4
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
ROUTER_STATE_FIELDS=0
GAME_MODE_INSTALL_CALLS=3
RUN_RELEASE_ROUTES=3
Editor candidate initial: 24 actions / 122.96s / exit 0
Editor candidate after lifecycle review: 22 actions / 76.68s / exit 0
Editor final: 0 actions / 0.89s / exit 0
Game final: 23 actions / 106.48s / exit 0
```

mapping SHA-256：`B113565B9C5DF6AFC5E8CDCB15B3FC0C9E494EF5870EB6155485F04B0F1BF8E9`；self-test SHA-256：`124357A32F145D04B4497C7318C87B46863CF6A67321585EDB26F7C121C4C8DF`。

最终 DLL SHA-256：`62FB6D404C1368BBAFF5FEA140EDE7251A627A8570AE3594093F67EA3F4E33B0`；Game EXE SHA-256：`D5D1352CFE4400863C69F7628EC96C878D2AD399D7161BF374B76971734D675C`。

## 真实异常

没有源码、Automation、门禁或构建失败；没有内存环境错误、非零原生退出、外层超时或失败重试。

候选测试使用 `;Quit` 后缀时，UE 原生测试成功，但旧 parser 把后缀计入 group name，因此该日志未作为正式门禁证据。正式运行只使用 `-TestExit`，并以 UE 5.8 native `TEST COMPLETE / EXIT CODE 0` 收口。验证器兼容两代 native completion marker，且新增自测，不接受无终止标记日志。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

测试 World 只验证安装/owner/Run/typed dispatch；真实地图初始化、真实输入和连续画面未执行。

## 下一步

P10.8 建立 Run-owned Spirit Evasion activation reservation 与 canonical product config authority。只有 sequence/content/policy/trajectory 的唯一来源完成后，才把真实 PlayerController input 转换为本阶段 typed command；SpiritEnergy 继续等待统一资源 authority。
