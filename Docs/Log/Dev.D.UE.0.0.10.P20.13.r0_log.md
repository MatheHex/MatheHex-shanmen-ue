# Dev.D.UE.0.0.10.P20.13.r0 Development Log

## 1. 目标与基线

- 基线：`b7a7cee0483a9bb4c7db518041cdbb4ed3c41e8b`（P20.12）；
- 分支：`agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition`；
- 目标：把冻结 Arc choice 的 projector 惰性组合进既有 GameMode Arc InputAdapter route；
- 约束：不复制产品权威，不做设备绑定、UI/preview、world trace、库存、projectile 或伤害开发。

## 2. 接入前审计

P20.8 已定义“先 hotbar/item/Run/lifecycle 分类，再按轨迹一次性采样几何”的唯一 InputAdapter；P20.9 把 Arc target/apex callback 暴露到 GameMode；P20.11 让 GameMode 的唯一 choice session 持有冻结 state；P20.12 只做 choice + caller basis + policy 到 target/apex 的纯值投射。

缺口是没有一条安全组合缝能同时满足：未认领输入不做 projection、basis 只采样一次、target/apex 来自同一 projection、最终 item/Run/Action/world delivery 仍归已有 InputAdapter 管理。本轮只填该组合缝。

## 3. Composition result 契约

新增只读 `Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult`，记录状态、诊断、route/basis/target/apex 次数、P20.12 projection result 与既有 InputAdapter result。

状态为 `InputCompletedBeforeProjection`、`ProjectionRejected`、`Delegated` 或 `RouteProtocolRejected`。`IsValid()` 按状态检查次数、projection 与 InputAdapter 采样审计的一致性；`IsAccepted()` 只接受有效 `Delegated` 且下游 InputAdapter 已接受的结果。

## 4. 惰性投射实现

`Route` 先构造 target/apex 两个同步 callback，再把它们交给既有 route 一次。target 首次被请求时才采样 caller basis，并缓存 projector result。apex 只能在 target 成功投射后请求一次，并直接返回缓存 apex。

因此非暗器槽、无 active Run 等在既有 route 的分类阶段结束时，basis sampler 完全不会运行。有效 Arc 路径不会为 target/apex 各自读取一遍 source basis，也不会产生两个 projection identity。

## 5. 协议与投射失败关闭

重复 target、重复 apex、apex-before-target、InputAdapter 采样审计与 callback 次数不一致都会设置 protocol violation。第二次 target 请求返回非有限 sentinel，但绝不重新采样 basis。

choice/basis/policy/projection 无效时，首次 target 返回 NaN sentinel。既有 InputAdapter 因 target 非有限而返回 `TargetUnavailable`，且不会请求 apex；composition 保留 projector 的精确失败状态。若 route 对 rejected projection 给出接受或不一致结果，则升级为 `RouteProtocolRejected`。

## 6. GameMode 组合

GameMode 新增 `RouteThrownWeaponArcChoiceHotbarInput`。它读取现有 choice session state，把 policy 与 basis sampler 交给单一 composition，并把其投射 callback 委托到现有 `RouteThrownWeaponArcHotbarInput`。

旧 direct Arc route 继续存在。新方法没有直接读取 item、Run、Action、selection ordinal 或产品 session，也没有新增 controller/host/world delivery。composition 为无状态成员，不拥有 choice 或产品生命周期。

## 7. 新增自动化与首次验证

新增 exact 5 项：

- lazy classification；
- canonical projection/delegation 与 replay；
- missing target/invalid basis projection failure；
- apex-before-target/duplicate-target protocol failure；
- transient GameMode pass-through 与 frozen choice preservation。

首次 Editor build：28 actions / native 0，SHA-256 `4AC237C56A6C26B90B456C3BD14E4E665E2635D25ED439F9A1460A27B3404184`。首次 exact：`5/0`、fatal/unhandled/ensure 0，SHA-256 `AC4B11E2F7144587552F6114CB6380B18BDB1B70E8CEFFE566CFCCEA4663074E`。本轮没有源码、断言或环境重试。

## 8. 最终自动化与 changed-file gate

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_choice_composition_final.log` | `5/0` | `95278410FFA6EF4A2A42085AB1BB3A808FC2CA9581061A56E6666E84B922EA81` |
| `arc_choice_projection_final.log` | `5/0` | `D436F822473E629C1DDA2E6ACD580DA8264CA0923074E14C3685116D0A5A9884` |
| `input_choice_final.log` | `9/0` | `3ECD16514CF4C682FF8F19D3E96F5F3BFB1CB35E586A67AFB1ADE82B711BDB6A` |
| `input_adapter_final.log` | `9/0` | `71C20E69F270AC0686F9904A827006718ACC54E3D8B1F7459C5915DFDC917CF7` |
| `full_0_0_10_final.log` | `882/0` | `9966AC41B787E4427C2A62E34BD9FCEAD4118604DED4018C402CF81848562423` |
| `legacy_v3_attributes_final.log` | `4/0` | `B20570F06E13C9B20B6225F3E690FD1BB6F94F6A600EA169140D4380393F125D` |
| `legacy_enemy_skill_final.log` | `44/0` | `DEC6804929E1004A36ACAA7A667DCEF7EF6955A59A9CA97E6DE070811F3B2224` |
| `legacy_v2_ranged_final.log` | `22/0` | `DE886FCB56E49F1AA16E74F79A2AA7E871A57F2C1F41914ECCF43E329859D475` |
| `legacy_item_armor_final.log` | `46/0` | `1F83A47133F0E1ABBA6FCB2BAE28381B8218F73F92429CE7597A6A1461779783` |

日志审计：每份 1 个 canonical RunTests、1 个 queue-empty、1 个 TestExit，合计 success 1026、fail 0、fatal/unhandled/ensure 0；SHA-256 `CA4956E5C3D405AA69DD477DC901F7632AE353A50D6BF1CD3F6A07C042678229`。

映射新增 composition rule，并把 composition exact 加入 M01 GameMode required groups。流程自测为 `319/319`，SHA-256 `030739C20C101D34BD70A5562CF960A114976D84F1AC1F01EEFCD9665FDA27E8`。

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=57 Logs=9
```

- gate SHA-256：`F92C01632E0CDD5E56EDB441CDA87114AC0DFD424327BED3A65013753F5B4096`；
- production boundary：composition 267 行 + GameMode 33 条新增行，0 forbidden matches；
- boundary SHA-256：`3D3BDE67CFEC6081A64B9D64DD7069701595FBA1B06D3B1E817AE91089BE6C82`；
- placeholder scan：0 matches；JSON mapping parse：PASS / 184 rules；
- `git diff --cached --check`：精确暂存 9 个文件后 PASS / native 0；证据 SHA-256 `CACB0431BAA40216E8F3F6A3D6D207BBF1ACAE121CB47671E3E1C2C9B7F90A5C`；
- 长期未跟踪文件不纳入暂存、提交或推送。

## 9. 最终构建与产物

- final Editor：up to date / 0 actions / native 0，SHA-256 `3D8D1E3D64CBCB340B48ACC2DCA1002EEF7FF0415C12B07E7218487E875CB46C`；
- final Game：27 actions / native 0，SHA-256 `B0C479BD0DCC17DB2A6D6B63B678C279B9ED94912291C47BA872E371E53C36EE`；
- `demo_map.exe`：`357290496` bytes / `A5C3C4CDC71B4E2E1046433099EA63AE7DC50E4587BCE901397DC439945C0243`；
- `UnrealEditor-demo_map.dll`：`15971840` bytes / `88E4A03DCF0FD822D9CA8E42E07C8922E900103F73D4C1E61B239C6D71A54AF8`。

只执行编译与无头自动化，没有启动产品。

## 10. P/F 边界与后续判断

本轮证明冻结 choice、caller basis、projector 与既有 InputAdapter 可以按惰性、单次、可审计协议组合，且 GameMode 没有形成第二套产品权威。它不证明真实 actor basis、设备输入、UI/preview、trace、命中或产品运行。

P20.14 应增加一个唯一 world-basis adapter：只在本轮 target callback 真正被请求时，从 canonical source actor 采样 location/forward/right 一次并交给 GameMode choice route；继续不接设备绑定与 UI，也不替代既有 source/world authority。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition/Docs/Report/Dev.D.UE.0.0.10.P20.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition/Docs/Log/Dev.D.UE.0.0.10.P20.13.r0_log.md>
