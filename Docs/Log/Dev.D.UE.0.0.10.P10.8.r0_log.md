# Dev.D.UE.0.0.10.P10.8.r0 Development Log

## 目标

把 P10.7 明确后置的身份缺口补齐：建立 canonical Spirit Evasion product config、Combat Run 自有 activation sequence 与只消费方向意图的 start composition authority；不绑定输入、不创建临时 SpiritEnergy，也不把身份选择塞回 Router。

## 审计结论

1. P10.7 Router 已正确拒绝生成 GUID 或选择 content；
2. Combat Run Coordinator 已持有 RunId、玩家 entity 与其他玩家 action sequence，是 Spirit Evasion sequence 的唯一合理归属；
3. definition、movement policy 与 trajectory 仍由测试调用方自由拼装，没有产品唯一来源；
4. GameMode 已能 route typed command，但缺少 reservation/config composition；
5. 真实 PlayerController 输入与资源 authority 都尚未具备稳定上游边界；
6. 方向必须在 sequence mutation 前验证，否则无效输入会烧掉可审计 identity；
7. 一旦 reservation 发布，sequence 不得因下游失败回滚或复用；
8. Coordinator 变更会触发 formation World resolution 与 legacy attributes 回归映射。

## 设计决策

1. config 使用私有字段与只读 getter，调用方不能覆写；
2. ConfigId 由全部 canonical product values 确定性派生；
3. canonical defense 只要求 living target，不错误要求 player attacker；
4. Coordinator 构造 action owner/source、SourcePlayer tag 与 deterministic ActivationId；
5. reservation 自校验 canonical config、action、sequence 与派生 identity；
6. sequence 只在成功 reservation 后递增，0/MAX fail closed；
7. EndRun 与 Reset 都把 Run-local sequence 重置为 1；
8. PrepareStart 先验证有限 XY 方向，再创建 config、reserve identity、capture command；
9. 方向统一投影至 XY 并在 command capture 时归一化；
10. CommandRejected 不返还已消费 sequence；
11. authority 不拥有 Actor/component/input/resource/clock/movement/lifecycle 状态；
12. SpiritEnergy 继续等待唯一资源 authority。

## 执行序列

1. 审计 P10.7 Router、Combat Run action construction 与既有 sequence 归属。
2. 新建 immutable product config、action reservation、typed start result 与 authority。
3. 在 Combat Run Coordinator 增加 Spirit Evasion Run-local monotonic sequence。
4. 建立 canonical action capture 与 deterministic identity 校验。
5. 新增七个 focused tests。
6. 首次 Editor candidate 56 actions、195.86s、原生退出 0。
7. regression map 增至 85 rules，self-test 增至 130 cases。
8. 代码审查发现纯 Z 输入可能先消耗 sequence，改为 reservation 前 XY 投影验证并补断言。
9. 增量 Editor candidate 5 actions、7.15s、原生退出 0。
10. focused authority `7/7` 成功。
11. 串行执行六组正式 Automation，共 `515` success、`0` fail。
12. changed-file gate 以 `Changed=7 / Rules=2 / Required=17 / Logs=6` 通过。
13. 静态扫描、mapping JSON、130/130 self-test 与 `git diff --check` 通过。
14. Editor final 0 actions、0.90s；Game final 55 actions、171.81s；原生退出均为 0。
15. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
device-independent candidate direction
  -> ProductAuthority
      -> finite XY validation (no sequence mutation on rejection)
      -> canonical immutable ProductConfig
      -> CombatRunCoordinator reservation
          -> Run-local monotonic sequence
          -> deterministic ActivationId
          -> frozen player action snapshot
      -> P10.7 typed Start command capture
  -> future GameMode product route
  -> existing P10.7 Router -> Component -> Host -> swept authority
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| SpiritEvasionProductAuthority | 7 | 0 | `FD8E0630DF96B9E9962CAF275617E1294741B04A8FF18C6D8B535423117A33AA` |
| Shanmen.0_0_10 | 437 | 0 | `CA1192EA47953A1A41EA04F0B30F3FD2F328E020DE4BB04E31FB182BE51AB845` |
| EnemySkillFramework | 44 | 0 | `DDFCD117193B724800B44B56BF5BCFB403EBCF668796F13D2AC28C6F6F35628F` |
| V2RangedCompatibility | 22 | 0 | `242DB1E0D05E234FEB6A92C4F7053C38ED4D6D68A5387373774BD452B5F0EAD3` |
| V3.Attributes | 4 | 0 | `063994AE10905E3771FF488DF870658B7E60CFAE769FEAAED42B90271D43A855` |
| FormationInfluenceConsumerWorldResolution | 1 | 0 | `98E0942E90E4D8E541F33FC414A36BBE3ED728189A0B6F8AFB9A54EE89C6CC60` |

所有正式日志均为 native exit `0`、terminal success `1`、Fail `0`、fatal/unhandled/ensure `0`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=85
SELF_TEST: PASS 130/130
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=17 Logs=6
git diff --check: PASS
DIRECT_MUTATION_HITS=0
RNG_HITS=0
SPIRIT_ENERGY_HITS=0
INPUT_BINDING_HITS=0
SEQUENCE_INCREMENT_HITS=1
Editor candidate: 56 actions / 195.86s / exit 0
Editor candidate after direction review: 5 actions / 7.15s / exit 0
Editor final: 0 actions / 0.90s / exit 0
Game final: 55 actions / 171.81s / exit 0
```

mapping SHA-256：`C48A560F0A09AF30B3ABF1386FA33A63BFA9961FB709D4CE1AC1A1FA169BF23A`；self-test SHA-256：`32CB9832CB66D3CEF900C93351C5AA620888AD87B4C36534E803ABD1760ED88A`。

最终 DLL SHA-256：`5F317399D5E8938622A6BEAC302AD16BE05D9F6B92CF48FF8ADE688EA99820DE`；Game EXE SHA-256：`F07ECC484590F26A26C4DE069FAA003C94C93E90D87F16990E23584D3E573D53`。

## 真实异常

没有源码、Automation、门禁或构建失败；没有内存环境错误、非零原生退出、外层超时或失败重试。

代码审查在正式 Automation 前发现纯 Z 输入的 pre-reservation 验证缺口。修正为 XY 投影后验证，并补充垂直方向不推进 sequence 的测试；随后必要的增量 Editor candidate 原生成功。该过程作为审查修正如实保留，不描述为环境或测试失败。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.9 建立 GameMode 单一产品入口，把 device-independent direction 交给 P10.8 ProductAuthority，再把 typed command 交给 P10.7 Router，并返回完整 composition/route result。仍不绑定真实按键；真实 PlayerController adapter 与 SpiritEnergy resource authorization 分别等待稳定产品入口和统一资源 authority。
