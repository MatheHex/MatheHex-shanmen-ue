# Dev.D.UE.0.0.10.P10.9.r0 Development Log

## 目标

把 P10.8 的 canonical product composition 接到 P10.7 typed Router，并建立 GameMode 唯一公开的 Spirit Evasion 启动入口；上游只提交 device-independent 方向，不允许注入完整 command、config 或 activation identity。仍不绑定真实输入、不创建临时 SpiritEnergy。

## 审计结论

1. P10.8 已能从方向生成 canonical config、Run-owned reservation 与 frozen start command；
2. P10.7 已能把 frozen command 安全路由到 component/host/lifecycle；
3. GameMode 仍公开接收完整 command，调用方理论上可绕过 P10.8 产品唯一来源；
4. GameMode 私有 Coordinator、玩家解析与组件安装 seam 已具备，不需要第二套 authority；
5. owner/component/Run 的可观察失败若晚于 reservation，会无意义消费 activation sequence；
6. 已发布 reservation 即使下游 preflight 拒绝也不能回滚，否则重试会复用身份；
7. PlayerController 尚无适合 Spirit Evasion 的 device-independent adapter，真实输入边界应后置；
8. GameMode 与新产品路由会触发完整 Spirit Evasion 链、World gameplay、legacy attributes 与 Enemy/V2 回归映射。

## 设计决策

1. 新 route 为 stateless composition seam，不持有 component、Actor、Run、资源或时钟；
2. route 输入仅为 component、Coordinator、owner 与 candidate direction；
3. owner、World、component registration/ownership/busy、Run ready 与 registry identity 全部先于 reservation；
4. 方向与 canonical config 继续只由 P10.8 ProductAuthority 校验和选择；
5. frozen command 继续只由 P10.7 CommandRouter dispatch；
6. composite result 同时保留 ProductStart 与 CommandRoute proof；
7. accepted proof 必须逐项验证 Start kind 和三处 ActivationId 一致；
8. pre-reservation failure 不消费 sequence；post-reservation failure 保持 sequence 已消费；
9. GameMode 删除公开完整-command start route，改为公开 direction-intent route；
10. GameMode 内部 Cancel/Release 不被误当作第二个公开 start authority；
11. route 不直接做 input binding、resource access、identity creation 或 World mutation；
12. changed-file mapping 为新 route 与 GameMode 加入完整依赖链证据。

## 执行序列

1. 审计 GameMode、P10.8 ProductAuthority、P10.7 CommandRouter、component install 与 Run registry 边界。
2. 新建 typed route status、composite result 与 stateless product route。
3. 实现 reservation 前 owner/component/Run fences。
4. 组合 ProductAuthority 与 CommandRouter，并校验完整 ActivationId proof。
5. 用 direction-only GameMode start entry 替换公开完整-command route。
6. 新增六个 focused tests，覆盖 entry、intent、busy、success、execution rejection 与 terminal reuse。
7. regression map 增至 86 rules，self-test 增至 132 cases。
8. Editor candidate 24 actions、124.48s、原生退出 0。
9. focused route `6/6` 成功。
10. 串行执行六组正式 Automation，共 `520` success、`0` fail。
11. changed-file gate 以 `Changed=7 / Rules=2 / Required=28 / Logs=6` 通过。
12. 静态扫描、mapping JSON、132/132 self-test 与 `git diff --check` 通过。
13. Editor final 0 actions、1.24s；Game final 23 actions、103.02s；原生退出均为 0。
14. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
future device-independent input adapter
  -> GameMode::RouteSpiritEvasionStartIntent(direction)
      -> resolve current player + existing component install seam
      -> ProductRoute pre-reservation fences
          -> P10.8 ProductAuthority
              -> canonical config
              -> Run-owned reservation / deterministic ActivationId
              -> frozen typed Start command
          -> P10.7 CommandRouter
              -> Component -> ProductHost -> ActionCoordinator
              -> preflight -> swept movement/lifecycle authorities
      -> composite ProductStart + CommandRoute receipt
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| SpiritEvasionProductRoute | 6 | 0 | `D068DF4B153F24A921F77448A23B004A4F5578D08B669659B5FB393512248171` |
| Shanmen.0_0_10 | 443 | 0 | `C6475FF9CD1BB2E14B85ED53411372B0D4CAB30F26E1DA63F7895AAE8A98EE7F` |
| EnemySkillFramework | 44 | 0 | `4EE720A83E31783942BED46D936268919D3A45FE0377C97DF953483AC6A7AA88` |
| V2RangedCompatibility | 22 | 0 | `68F0534D6F0283D413DD86A0FA90269090EDDC486906FAF0B94D33CBC756CDC4` |
| V3.Attributes | 4 | 0 | `E375C89AE6C3F4392BAD56A6AB77FEE8377A86442940BC0C67C325DE1621E53D` |
| FormationInfluenceConsumerWorldResolution | 1 | 0 | `3AF749328124BDAF4A02A86FA6F5778CA9356AEBF0A0E0130624C4AB56BEDD36` |

所有正式日志均为 native exit `0`、terminal success `1`、selected Fail `0`、fatal/unhandled/ensure `0`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=86
SELF_TEST: PASS 132/132
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=28 Logs=6
git diff --check: PASS
LEGACY_PUBLIC_START_ROUTE_HITS=0
INPUT_HITS=0
RESOURCE_HITS=0
MUTATION_HITS=0
IDENTITY_FACTORY_HITS=0
Editor candidate: 24 actions / 124.48s / exit 0
Editor final: 0 actions / 1.24s / exit 0
Game final: 23 actions / 103.02s / exit 0
```

mapping SHA-256：`9CA58C03CD1364257BDF89E5DBC4543033F963CDCAA94BB95CA024B8020A5FA7`；self-test SHA-256：`C5F9079A62CED0717FEB07C57336CD2046737134BE61D2F47B59525B3E3F6752`。

最终 DLL SHA-256：`2818909E0D0B8DCDC246D1AE24757C6BDB87428F2D69A46E326E3E3499D87718`；Game EXE SHA-256：`203AAE1B7EFF19551C0A004794BB1149F678B50F125B961608CA64DC39BB1382`。

## 真实异常

没有源码、selected Automation、门禁或构建失败；没有内存环境错误、非零原生退出、外层超时或失败重试。

所有 UnrealEditor-Cmd 进程在 selected suite 启动前均记录了引擎自带 `UE::UnifiedErrorTest` 初始化输出和 13 行通用 `Condition failed`。它们发生在 Engine 初始化阶段，不属于本轮测试；随后 selected tests 全部 Success、native terminal exit 为 0，且没有 ensure/fatal/unhandled。原始噪声完整保留在日志中。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.10 建立 device-independent PlayerController input adapter：一次输入只分类一次、采样一次方向，并仅调用 GameMode direction-intent route。先用无头 fixture 固定消费、采样与 fail-closed 契约；不临时创建 SpiritEnergy，真实按键绑定与 F 阶段产品输入验证继续后置。
