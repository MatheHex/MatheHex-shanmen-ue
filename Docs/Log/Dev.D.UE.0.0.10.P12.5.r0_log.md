# Dev.D.UE.0.0.10.P12.5.r0 Development Log

## 1. 目标

把真实 precise-link、perfect-guard 与 spirit-evasion projection receipt 统一为不可变 SwordRhythm contribution evidence，为后续“前一次行为转化为下一次攻击力量”建立来源契约；本轮不定义数值、不消费贡献、不改伤害。

## 2. 实现

- 新增 `EShanmenSwordRhythmContributionKind` 三类来源；
- 新增 private-field、`BlueprintReadOnly` 的 `FShanmenSwordRhythmContribution`；
- contribution 冻结完整 action snapshot、source receipt、timeline 和 tick；
- deterministic ID 纳入完整 action/content/tag provenance；
- precise-link adapter 仅接受 `PreciseLinked`；
- perfect-guard adapter 仅接受 `Perfect`；
- spirit-evasion adapter 仅接受真实 defense projection receipt，并要求显式有效时间样本；
- 不建立 ledger、buff、属性修改、damage multiplier、World/Actor/timer 或输入路径。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `SwordRhythmContribution` | 3 | 0 | `62989B2F939A95E339886EF50E637316CB803806E66048841B949CF1CBA62C1E` |
| `SwordRhythm` | 9 | 0 | `D5302C9783FFE7056B6CD24FA0F9D8C226AF72D0DD6127B88B8C0EB1F99DF5CB` |
| `WeaponPerfectGuard` | 6 | 0 | `7551691FE754FD041E6612E64E338AB1B813910C3D86AD0D3A45715B2A804498` |
| `SpiritEvasion` | 11 | 0 | `DD14589429B65B0E3563D4421384E13C509FDF7D768F4F955AAF79F38434678F` |
| `ActionLifecycle` | 1 | 0 | `79A5AFCB85778E4744F863C858315E9B0889B7A3E6229132F5BD870B5FD7E302` |
| `CombatRuntime` | 114 | 0 | `6CD6487EA671866053573D37DD23374731A32206418A09B3D27FA2F76C3B9A8D` |
| `Shanmen.0_0_10` | 558 | 0 | `091BA49DF257892DF0AC0E71FABD1C6AB07FC78DB162A5FF63CA7C414004DDEE` |

原始日志 `702 Success / 0 Fail`；focused/parent 组均包含于 full 558。所有最终进程原生退出 `0`，选定测试阶段错误标记为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=2 Required=6 Logs=7
SELF_TEST: PASS 178/178
REGRESSION_MAP_JSON: PASS
BOUNDARY_SCAN: PASS ForbiddenHits=0
DAMAGE_SCAN: PASS ProductionDamageHits=0
git diff --check: PASS (native exit 0)
```

Coverage SHA：`1648916B1D7000FA05576A4DBA1E44FFBCF02B8A8773E4AD25AA26C6C79BF4A6`；Self-test SHA：`235BE53E94C12BE5B68D92ECD71754F9EF1EF63C4DECC5BB77DEBC5C3EC53A55`。

## 5. 构建

- Editor：4 actions / 5.01s / exit `0` / log SHA `790CCE7627D9C0D8ABB721F598558E22179B6F2D8DC557556F48859434A94840`；
- Game：5 actions / 43.45s / exit `0` / log SHA `ADDCA9AB0A5710E31BB10073CA6CF72148B40DF89B3CB8D1219D1F41E242467A`；
- Runtime DLL：1,589,248 bytes / SHA `93EED19056C78FD91CF31BF85130F0B3DBC55446CD5C4EBD4606DFBCA34912BD`；
- Game EXE：354,603,008 bytes / SHA `D367AC670DC65D05A7043F442536B59C838B13F2BCFDAAF47DA3533F8B997C2E`；
- 无源码或环境构建失败。

## 6. 修改与兼容性

新增 556 行 contribution 生产/测试代码，并扩展 changed-file regression map 与 self-test。既有 SwordRhythm、BasicSword、WeaponGuard、SpiritEvasion、Impact、Vitality、inventory、schema、GAS、输入和表现路径均未改动。长期未跟踪资料保持未暂存。

## 7. 流程与边界

静态复审补齐了 action content/item/tag identity 后，Editor 与全部 Automation 均覆盖重跑。changed-file gate 曾因调用集合误含 build log 而 fail closed，改为显式 7 份 Automation 后通过；没有产品或测试 case 失败。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。下一阶段可建立不含数值的一次性 next-action binding ledger。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-5-sword-rhythm-contributions>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-5-sword-rhythm-contributions/Docs/Report/Dev.D.UE.0.0.10.P12.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-5-sword-rhythm-contributions/Docs/Log/Dev.D.UE.0.0.10.P12.5.r0_log.md>
