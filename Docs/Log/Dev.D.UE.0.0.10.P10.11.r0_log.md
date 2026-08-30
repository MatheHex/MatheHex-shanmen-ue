# Dev.D.UE.0.0.10.P10.11.r0 Development Log

## 目标

把 P10.10 Spirit Evasion 输入入口接入统一 InputAction registry 与真实 Controller `BindKey` 栈：选择无冲突、可重绑定默认键，只绑定 Press，并补齐旧配置迁移、冲突、live remap 与 input-lock 自动化；不创建临时 SpiritEnergy 或绕过既有产品 authority。

## 审计结论

1. P10.10 已提供唯一 PlayerController 输入适配入口，本阶段无需新建 route；
2. 现有 registry 有 21 个精确动作、唯一默认键与统一持久化；
3. `SpaceBar` 在 registry、Config 与产品 `BindKey` 中未被占用；
4. 旧配置前向合并原本直接写入新动作默认键，无法处理该默认键已被用户覆盖占用的情况；
5. `BindProductInputActions` 集中持有产品键位，应在这里加入唯一 Press binding；
6. gameplay lock、方向采样与产品 route 已由 P10.10 统一拥有，physical handler 不应复制；
7. 重映射测试必须验证旧键真正失效，不能只检查配置文本；
8. UE 5.8 `ClearBindingsForObject` 的实际实现只清 cache，不清 raw `KeyBindings`；
9. 输入 registry 变动会触及多个旧测试文件，changed-file map 必须明确要求相应 legacy evidence；
10. SpiritEnergy authority 仍不存在，资源成本不属于本阶段。

## 设计决策

1. registry 增加 `SpiritEvasion=SpaceBar`，动作总数变为 22；
2. metadata 标记为无需 Released event；
3. 配置序列化版本提升到 3；
4. 迁移优先保留旧用户覆盖，不为新动作强行抢键；
5. 冲突时按 registry 固定顺序选择第一个未占用默认键；
6. 无合法 fallback key 时 fail closed；
7. Controller 只绑定 `IE_Pressed -> StartSpiritEvasion`；
8. handler 只调用 P10.10 route；
9. live remap 记录并验证 Controller 自有连续 binding 区间；
10. ownership/range 异常时拒绝重建，不删除不确定 binding；
11. 自动化通过 transient GamePreview World 使用真实 PlayerInput/InputComponent；
12. map 同时要求 focused physical、P10.10 adapter、0.0.10 full 和受影响 legacy input contracts。

## 执行序列

1. 审计 registry、binding settings、PlayerController 与既有输入测试。
2. 注册 Spirit Evasion，选择无冲突 SpaceBar，并把动作数更新为 22。
3. 实现 Version 3 forward migration 与 occupied-default fallback。
4. 在 `BindProductInputActions` 增加唯一 Press binding。
5. 新增六个 physical input 自动化测试。
6. 更新 legacy registry count assertions 与 regression map/self-test。
7. Editor candidate：26 actions、123.75s、退出 0。
8. 首次 focused：5/6；发现旧 SpaceBar 在 live remap 后仍活跃。
9. 核验 UE 5.8 引擎源码，定位 `ClearBindingsForObject` 不移除 raw KeyBindings。
10. 实现 exact owned-range removal；Editor post-fix：15 actions、54.85s、退出 0。
11. focused post-fix：6/6。
12. exploratory 运行 legacy 整组，记录范围外旧断言失败，不改无关代码。
13. 将 mapping 收敛为受影响的精确 legacy cases，同时保留 broad/full evidence。
14. 代码审查增加范围与 delegate ownership fail-closed 验证。
15. Editor post-review：15 actions、52.96s、退出 0。
16. 串行生成 9 组正式 Automation：502 Success、0 Fail。
17. changed-file gate：Changed=13、Rules=2、Required=10、Logs=9，PASS。
18. JSON 88 rules、136/136 self-test、静态扫描与 `git diff --check` 全部通过。
19. Editor final：0 actions、0.93s、退出 0；Game final：25 actions、110.34s、退出 0。
20. 生成同名 Report/Log，执行 exact-stage gate，commit 并 push。

## 数据流

```text
registry SpiritEvasion = SpaceBar (rebindable)
  -> BindProductInputActions / IE_Pressed only
      -> StartSpiritEvasion
          -> P10.10 RouteSpiritEvasionStartInput
              -> existing gameplay surface gate
              -> authoritative GameMode route availability
              -> one direction sample
              -> P10.9 product route
                  -> P10.8 canonical config + Run reservation
                  -> P10.7 Router -> Component/Host/Lifecycle
```

```text
Version 2 config missing SpiritEvasion
  -> preserve every existing action/key
  -> SpaceBar free ? SpiritEvasion=SpaceBar
                   : first unused registry default
  -> ValidateBindings
  -> persist Version 3
```

## Automation 证据

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| SpiritEvasionPhysicalInput | 6 | 0 | `F03F1A921A464276D8E285ECB3F0B44D0F19D1DD3A0A4695F3B4085DCD6832F4` |
| SpiritEvasionInputAdapter | 6 | 0 | `2A7335F25F22D994CD09C9B08903EFC929EDE21C66FEA4A2AC6535F69E97454F` |
| Shanmen.0_0_10 | 455 | 0 | `048566AD67BB4B5621ACE9A30BB9DF8799FF73D34091E0650F7458A7AB132183` |
| FullSystemLoop.41 | 1 | 0 | `3C0D6AF45AE9A63AF2E74B55C962E1F513ADA2CAC68AA05F7B4A1321DBD9C602` |
| FullSystemLoop.47 | 1 | 0 | `C32EE2E36BAFDAEE1D8DB62310C3AB01C000901DA150075BD889B309FB1AFCAE` |
| P7Integration | 9 | 0 | `8147907D45103E681F9C0289BBD2E3E53A7BB9F02B36E644C10EE8E9993EBBA8` |
| P5RuntimeInterface.06 | 1 | 0 | `BB7D8A3CFEC77DCB930CDA1A418820EC2AB0FE2EC17ED812F731A7564CE433A0` |
| InputRestore.32 | 1 | 0 | `19904A033862E3014FDD3FFAE2660B4ADFDFDDC84133BF9E97012F27F1BD7261` |
| V2RangedCompatibility | 22 | 0 | `77CC4B7D08C107498C74852E4A815683F1F47AF13D34A0490D40A23EC8374548` |

所有正式日志均为 native exit `0`、terminal success `1`、selected Fail `0`、fatal/unhandled/ensure `0`。

## 门禁与构建

```text
REGRESSION_MAP_JSON: PASS Rules=88
SELF_TEST: PASS 136/136
REGRESSION_COVERAGE: PASS Changed=13 Rules=2 Required=10 Logs=9
git diff --check: PASS
SPIRIT_EVASION_PRESS_BINDINGS=1
SPIRIT_EVASION_RELEASE_BINDINGS=0
SPIRIT_HANDLER_FORBIDDEN_HITS=0
Editor candidate: 26 actions / 123.75s / exit 0
Editor post-fix: 15 actions / 54.85s / exit 0
Editor post-review: 15 actions / 52.96s / exit 0
Editor final: 0 actions / 0.93s / exit 0
Game final: 25 actions / 110.34s / exit 0
```

mapping SHA-256：`A5BC2267C83C9E685F869F25FA03352E5FDB150986DF36797C57218C1F340AAA`；self-test SHA-256：`7DC759F5274B81C0A4F1673DB93B04E7D57141309BF900EAD8953ABAB735345A`。

最终 DLL SHA-256：`F41B20029BAFA0C71F9CB84CD403A12725C181D95D89A3AED00628CE96DF9C76`；Game EXE SHA-256：`C20F4928B19C133694F977B2827D300243E570D013BCA197CACA6BB8AC1AE5B9`。

## 真实异常

首次 physical focused 为 `5/6`，`LiveRemap` 证明旧 SpaceBar binding 未被 UE 5.8 `ClearBindingsForObject` 删除。失败日志 SHA-256：`2DC0BB790BE74BCB9063A8642E17CEC7788C9D8EE8A7BBC5C3FF3517D04BF4CD`。修正为 exact owned-range removal 后 post-fix `6/6`，SHA-256：`0C6D0BEF01C5309AED3C94C1E762548E4DBBBF2449EC19900954866C35D97342`。

探索性 legacy 整组还原了范围外旧债：FullSystemLoop `39/50`、P5RuntimeInterface `8/9`、InputRestore `98/101`。本轮没有篡改这些失败，也没有把它们写成成功；最终门禁以实际受影响的精确 legacy cases 加 broad 0.0.10/P7/V2 evidence 通过。

九份正式 UE 日志均保留 selected suites 之前的 `UE::UnifiedErrorTest` 13 行初始化噪声；selected results、terminal marker 与 native exit 均成功。

没有源码构建失败、内存环境错误、非零 UBT 退出或外层超时。

## P/F 边界

仅执行 P 阶段实现、transient World 无头 Automation、静态/路径门禁和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、操作系统级真实输入、截图、Smoke、Cook 或 Package。

## 下一步

优先进行 F 阶段真实输入与重绑定验收；SpiritEnergy 成本必须等待唯一资源 authority。若继续 P 阶段，应先定义资源 authority/事务契约，并把成本接入产品层，不接入 physical handler。
