# Dev.D.UE.0.0.10.P10.10.r0 Development Log

## 目标

在 P10.9 direction-intent product route 之上建立可无头验证的 PlayerController 输入适配边界：先检查 gameplay surface 与 route availability，再恰好采样一次方向并恰好委托一次；不绑定物理键、不创建 SpiritEnergy、不重试或回退旧技能。

## 审计结论

1. GameMode 已有唯一公开的 Spirit Evasion direction-intent start route；
2. PlayerController 已集中持有 gameplay/UI lock 与 last-valid combat aim；
3. 现有 thrown-weapon adapter 证明惰性采样应发生在产品分类之后；
4. 当前 InputAction registry 有严格 21-action 契约，本阶段不应顺带选键并扩大迁移范围；
5. adapter 若自行归一化方向，会与 P10.8 产品 authority 形成第二套方向政策；
6. adapter 若在 downstream rejection 后自动重试，会额外消费 activation identity；
7. PlayerController 应只依赖 GameMode 产品入口，不能直接接触 component/host；
8. 修改 PlayerController 与输入 adapter 需要 0.0.10、V2/input compatibility 和完整 Spirit Evasion product-chain 回归。

## 设计决策

1. adapter 为 stateless struct，不持有 input ordinal、sequence 或 cache；
2. gameplay blocked 与 route unavailable 都在 sampler 前拒绝；
3. eligible input 只调用 sampler 一次并保存原始向量；
4. 同一原始向量只委托 P10.9 route 一次；
5. adapter 不做有限性、XY 投影或 normalization；
6. product route 的完整 typed result 原样嵌入 input receipt；
7. accepted 必须要求完整 downstream proof，而不是仅看 status；
8. PlayerController 复用 `IsGameplayInputAllowed` 与 `GetLastValidAimDirection`；
9. GameMode 缺失时不采样，且 route callback 自身也 fail closed；
10. 不修改 `BindProductInputActions`、InputAction registry 或默认配置；
11. 不读取资源、不创建 identity、不直接移动 Actor；
12. regression map 对 adapter 与 PlayerController 建立路径到测试组的自动映射。

## 执行序列

1. 审计 PlayerController、input registry、thrown input adapter 与 P10.9 GameMode route。
2. 新建 typed input status、audit result 与 stateless adapter。
3. 接入 PlayerController 预绑定入口，复用 gameplay gate 与 last-valid aim。
4. 新增六个 focused tests，覆盖零次、一次、accepted 与 rejection 语义。
5. regression map 增至 87 rules，self-test 增至 134 cases。
6. Editor candidate 16 actions、88.31s、原生退出 0。
7. focused adapter `6/6` 成功。
8. 首轮完整回归通过后，代码审查补充空 GameMode callback fail-closed 防护。
9. Editor post-review 4 actions、14.84s、原生退出 0。
10. 重新串行生成六组正式 Automation，共 `526` success、`0` fail。
11. changed-file gate 以 `Changed=7 / Rules=2 / Required=20 / Logs=6` 通过。
12. 静态扫描、mapping JSON、134/134 self-test 与 `git diff --check` 通过。
13. Editor final 0 actions、0.89s；Game final 3 actions、23.71s；原生退出均为 0。
14. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
future dedicated physical key press
  -> PlayerController::RouteSpiritEvasionStartInput
      -> existing IsGameplayInputAllowed gate
      -> authoritative GameMode availability gate
      -> sample GetLastValidAimDirection exactly once
      -> SpiritEvasionInputAdapter receipt
          -> GameMode::RouteSpiritEvasionStartIntent exactly once
              -> P10.9 ProductRoute
                  -> P10.8 canonical config + Run reservation
                  -> P10.7 Router -> Component/Host/Lifecycle
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| SpiritEvasionInputAdapter | 6 | 0 | `0BE352E725F4707ED9086EAA328075620FE48968B9EB194B8387EE672609EF63` |
| Shanmen.0_0_10 | 449 | 0 | `A9AF65122CE3312BB379E39C8934ED9280BB9D3AA8FB6F10F9A0D8E04974882B` |
| EnemySkillFramework | 44 | 0 | `BEAE91FB19756889ADDC3DF70193BF19985D7B946DA8D647E2FAE225B0C6A145` |
| V2RangedCompatibility | 22 | 0 | `828B61C758C9005FA16B47452A026D7351D461DEE2569306728FC850DD12137A` |
| V3.Attributes | 4 | 0 | `08732B2C87614F43523BCF9DAC1283F55EDF62A3637C4EBD946C05E923B38B16` |
| FormationInfluenceConsumerWorldResolution | 1 | 0 | `FB9E14C8132E8C50F56277BBB3D24FD0AA206BAF7D18DC7C9F4F4C5DBA8AC906` |

所有正式日志均为 native exit `0`、terminal success `1`、selected Fail `0`、fatal/unhandled/ensure `0`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=87
SELF_TEST: PASS 134/134
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=20 Logs=6
git diff --check: PASS
PHYSICAL_BINDING_HITS=0
RESOURCE_HITS=0
MUTATION_HITS=0
IDENTITY_FACTORY_HITS=0
PLAYER_CONTROLLER_BINDING_HITS=0
PLAYER_CONTROLLER_ROUTE_CALLS=1
Editor candidate: 16 actions / 88.31s / exit 0
Editor post-review: 4 actions / 14.84s / exit 0
Editor final: 0 actions / 0.89s / exit 0
Game final: 3 actions / 23.71s / exit 0
```

mapping SHA-256：`56F5192AD15A3BEFCA78C73048DF9977B3CA04DDE2ED570CA9FCBA608A9E6D70`；self-test SHA-256：`834DB80DACC4BD5332B0D2097564B29C479990FB5B1F45F694D185A7C44FC34C`。

最终 DLL SHA-256：`A8F223415172289D072B920B8768FC9756A2427604641D849710AE4681F380D6`；Game EXE SHA-256：`2F42C3723C47301FBD54A84FFD5F606FC7E0CA8B702FBE384727F3F163CAC3F3`。

## 真实异常

没有源码、selected Automation、门禁或构建失败；没有内存环境错误、非零原生退出、外层超时或失败重试。

正式证据前的最后代码审查增加空 GameMode callback fail-closed 防护；随后重新编译、重跑全部正式测试和双目标构建。UnrealEditor-Cmd 的 `UE::UnifiedErrorTest` 初始化噪声仍完整保留，但不属于 selected suites；所有 selected tests 和 native terminal 均成功。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P10.11 将 Spirit Evasion 加入统一 InputAction registry，选择无冲突、可重绑定的默认键，并把 Press 只绑定到 P10.10 入口；补齐配置迁移、冲突/交换、input lock 与 synthetic key 自动化。SpiritEnergy 继续等待唯一资源 authority。
