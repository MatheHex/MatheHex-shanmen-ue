# Dev.D.UE.0.0.10.P20.13.r0 Report

## 1. 结论

P20.13 已新增投掷武器 Arc choice 的 consumer-owned 惰性组合层，并把 P20.11 的冻结 choice state、P20.12 的纯值 projector 与 P20.8/P20.9 已有的 GameMode Arc InputAdapter route 接成一条单向路径。

组合层始终只调用既有 route 一次。只有该 route 完成 hotbar/item/Run/source/lifecycle 等前置分类并实际请求 target 时，才调用外部 basis sampler 一次并执行 projector 一次；apex 直接读取同一份缓存 projection。非暗器槽等提前结束路径不会采样 basis，也不会做几何投射。

GameMode 新入口只读取既有 `ThrownWeaponInputChoiceSession` 的当前不可变 state，并委托既有 `RouteThrownWeaponArcHotbarInput`。它没有复制 item、Run、source、Action gate、selection ordinal、lifecycle、session、controller、host 或 world delivery 权威。

最终验证为：新组合 exact `5/0`、projector `5/0`、choice `9/0`、InputAdapter `9/0`、0.0.10 全量 `882/0`，以及四组 GameMode 旧回归 `4/0 + 44/0 + 22/0 + 46/0`；九份日志合计 `1026/0`。Editor 与 Game Development 构建均成功。changed-file regression gate 为 `PASS Changed=7 Rules=2 Required=57 Logs=9`。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`b7a7cee0483a9bb4c7db518041cdbb4ed3c41e8b`（P20.12）；
- 分支：`agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 单向组合与权威边界

新增 `Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition`。它是无状态同步桥，只接收：冻结 choice state、冻结 projection policy、调用方 basis sampler，以及既有 Arc route 的同步回调。

调用顺序固定为：

1. 调用既有 Arc route 一次；
2. route 自行完成既有产品分类；
3. route 若请求 target，组合层采样 basis 一次并调用 projector 一次；
4. route 若继续请求 apex，组合层返回同一缓存 projection 的 apex；
5. 保留既有 InputAdapter result，同时增加组合层采样审计。

组合层不知道 hotbar item、active Run、source actor、Action gate、selection ordinal、产品 lifecycle/session/controller/host 或物品事务，因此不能成为第二套产品路由权威。

## 4. 惰性采样与一次性协议

结果显式记录 route invocation、basis sample、target request 与 apex request 次数。合法路径要求 route 恰好调用一次、basis 最多一次；target 与 apex 均不得重复请求，apex 不得先于 target。

target callback 在首次请求时才执行 basis capture 与 projection，并缓存完整 P20.12 result。apex callback 只读缓存，不重新采样 basis，也不重新投射。canonical 测试把冻结 choice `(0.5, 0.5)`、apex adjustment `0.25` 与既有 policy/basis 组合成 target `(1100, 350, 50)`、apex `350`，并证明重放产生相同 projection identity。

当 InputAdapter 在几何前完成 PassThrough 等结果时，状态为 `InputCompletedBeforeProjection`，basis/target/apex/projection 工作全部为零。这保持了 P20.8 的“先分类，只有被认领后才采样几何”契约。

## 5. 失败关闭与结果审计

组合结果区分：

- `InputCompletedBeforeProjection`：既有 route 在几何前完成；
- `ProjectionRejected`：route 请求 target，但 choice/basis/policy/projection 无效；
- `Delegated`：有效 projection 已原样委托，最终产品结果仍由既有 InputAdapter 决定；
- `RouteProtocolRejected`：route 重复或乱序请求 sampler，或其审计字段与实际 callback 使用不一致。

projection 失败时，target callback 返回非有限 sentinel，使既有 InputAdapter 以 `TargetUnavailable` 失败关闭，并保证不会继续请求 apex。组合结果保留 projector 的精确状态与诊断。重复 target 不会再次采样 basis；apex-before-target 不会采样 basis。`IsAccepted()` 只在组合状态为 `Delegated` 且既有 InputAdapter result 本身接受时为真。

## 6. GameMode 接入

`Ademo_mapGameMode::RouteThrownWeaponArcChoiceHotbarInput` 从唯一 `ThrownWeaponInputChoiceSession` 读取 state，把调用方 policy/basis sampler 交给组合层，并将投射出的 target/apex callbacks 委托给现有 `RouteThrownWeaponArcHotbarInput`。

旧的直接 Arc route 保留为“已投射几何”的低层 seam；本轮没有复制或替换它。transient GameMode 测试在无 GameInstance、空 source 的未认领场景中证明：既有 route 返回 PassThrough，basis 与 projection 均未执行，冻结 choice state identity/revision 仍保持 revision 3。

## 7. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition` 5 项：

- `LazyClassification`：route 一次，提前结束时零几何工作；
- `ProjectAndDelegate`：canonical target/apex 原样委托、basis/target/apex 各一次、identity 可重放；
- `ProjectionFailure`：缺 target 与无效 basis 保留 projector 原因，不请求 apex；
- `ProtocolFailure`：apex-before-target 与重复 target 失败关闭；
- `GameModeBoundary`：GameMode 委托既有 route，未认领路径不采样并保持 choice state。

最终日志：

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_choice_composition_final.log` | `Product.ThrownWeaponArcChoiceInputComposition` | `5/0` | `95278410FFA6EF4A2A42085AB1BB3A808FC2CA9581061A56E6666E84B922EA81` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `D436F822473E629C1DDA2E6ACD580DA8264CA0923074E14C3685116D0A5A9884` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `3ECD16514CF4C682FF8F19D3E96F5F3BFB1CB35E586A67AFB1ADE82B711BDB6A` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | `9/0` | `71C20E69F270AC0686F9904A827006718ACC54E3D8B1F7459C5915DFDC917CF7` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `882/0` | `9966AC41B787E4427C2A62E34BD9FCEAD4118604DED4018C402CF81848562423` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | `4/0` | `B20570F06E13C9B20B6225F3E690FD1BB6F94F6A600EA169140D4380393F125D` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | `44/0` | `DEC6804929E1004A36ACAA7A667DCEF7EF6955A59A9CA97E6DE070811F3B2224` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `DE886FCB56E49F1AA16E74F79A2AA7E871A57F2C1F41914ECCF43E329859D475` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | `46/0` | `1F83A47133F0E1ABBA6FCB2BAE28381B8218F73F92429CE7597A6A1461779783` |

九份日志均只有一个 canonical RunTests、一个 queue-empty、一个 TestExit，合计 1026 个 success marker、0 fail、0 fatal/unhandled/ensure。审计 SHA-256：`CA4956E5C3D405AA69DD477DC901F7632AE353A50D6BF1CD3F6A07C042678229`。

0.0.10 全量从 P20.12 的 877 增至 `882`，恰好增加本轮 5 项。

## 8. 首次验证、门禁与静态边界

首次 Editor 构建直接通过：28 actions / native 0，SHA-256 `4AC237C56A6C26B90B456C3BD14E4E665E2635D25ED439F9A1460A27B3404184`。首次 composition exact 直接为 `5/0`、无 fatal/unhandled/ensure，SHA-256 `AC4B11E2F7144587552F6114CB6380B18BDB1B70E8CEFFE566CFCCEA4663074E`；没有源码、测试断言或环境重试。

新增 composition 映射，要求 composition exact、projection、choice、InputAdapter 与 0.0.10 full；M01 GameMode 映射增加 composition 组。流程自测新增 broad-evidence 正例与无关 item evidence 必须失败的反例，最终 `319/319`，SHA-256 `030739C20C101D34BD70A5562CF960A114976D84F1AC1F01EEFCD9665FDA27E8`。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=57 Logs=9
```

gate SHA-256：`F92C01632E0CDD5E56EDB441CDA87114AC0DFD424327BED3A65013753F5B4096`。

对 composition 两个 production 文件的 267 行扫描 World/Actor、spawn、trace/sweep、controller/device input、RNG、库存/物品权威与产品开始/提交 API：0 matches；对 GameMode 的 33 条新增行扫描直接 world gameplay、spawn、trace/sweep、RNG、库存与新 authority 创建：0 matches。结果 `PASS`，SHA-256 `3D3BDE67CFEC6081A64B9D64DD7069701595FBA1B06D3B1E817AE91089BE6C82`。

`git diff --cached --check`：精确暂存本轮 9 个文件后 `PASS / native 0`；证据 SHA-256：`CACB0431BAA40216E8F3F6A3D6D207BBF1ACAE121CB47671E3E1C2C9B7F90A5C`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：up to date / 0 actions / native 0，SHA-256 `3D8D1E3D64CBCB340B48ACC2DCA1002EEF7FF0415C12B07E7218487E875CB46C`；
- final Game：27 actions / native 0，SHA-256 `B0C479BD0DCC17DB2A6D6B63B678C279B9ED94912291C47BA872E371E53C36EE`。

产物：

- `Binaries/Win64/demo_map.exe`：`357290496` bytes，SHA-256 `A5C3C4CDC71B4E2E1046433099EA63AE7DC50E4587BCE901397DC439945C0243`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`15971840` bytes，SHA-256 `88E4A03DCF0FD822D9CA8E42E07C8922E900103F73D4C1E61B239C6D71A54AF8`。

headless 日志中的非 Win64 SDK 不可用信息与 `generate_204` 网络探测警告属于既有环境噪声；Win64 构建、测试 terminal 与原生退出码均成功。

## 10. P/F 边界与下一步

P20.13 证明的是：“一个冻结 Arc choice 能在既有 InputAdapter 真正认领输入后，惰性采样调用方 basis 一次、投射一次，并把缓存 target/apex 委托给唯一现有产品 route；提前结束和非法采样协议均可审计且失败关闭。”

它没有证明真实 source actor basis 的世界采样、物理设备命令、焦点/输入模式、UI、轨迹预览、trace、碰撞、命中、库存扣减或伤害。`882/0` 不能描述为产品运行验收。

建议 P20.14 增加一个唯一的 world-basis adapter：在本轮惰性 callback 内从 canonical source actor 读取 location/forward/right 恰好一次并调用 GameMode choice route；继续让 InputAdapter 先完成分类，不接按键、UI 或预览，也不创建第二套 source/world authority。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition/Docs/Report/Dev.D.UE.0.0.10.P20.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-13-thrown-weapon-arc-choice-input-composition/Docs/Log/Dev.D.UE.0.0.10.P20.13.r0_log.md>
