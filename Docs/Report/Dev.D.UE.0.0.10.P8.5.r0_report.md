# Dev.D.UE.0.0.10.P8.5.r0 Report

## 1. 结论

P8.5 已建立纯 Formation 区域规则 provider，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationAreaProvider`：只消费同一 Run／Owner／Deployment／Content 下已经完成 P8.3 World placement 的有效 receipts，生成确定性水平凸包、单实体 membership receipt 与批量 coverage receipt。该层不读取 Actor、World、Registry、库存或 Action 状态，也不施加伤害、Buff、节拍或正式内容数值。

最终 focused `4/4`、0.0.10 full `231/231`、changed-file gate、43/43 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 区域定义与“不猜数值”边界

区域使用已投放 anchor 的 **XY 水平凸包**：

- 所有 source anchors 保留在 snapshot 中，内部 anchor 不从审计证据消失；
- boundary 只保存 canonical counter-clockwise hull indices；
- Z 参与 AreaId 与 evidence identity，但 membership 明确只在 XY 地面平面求值；
- 少于三个非共线点、水平重合点或零面积集合都失败关闭；
- 不擅自发明二点连线半径、垂直高度、区域伤害、覆盖倍率或凹多边形 authored order。

因此 P8.5 提供的是可重放的几何真值，不是正式阵法内容。未来若策划需要圆形、胶囊、凹多边形或高度层，必须通过明确的 content contract 扩展，而不是由 provider 猜测。

## 3. Immutable snapshot 与确定性身份

`Fdemo_mapShanmenFormationAreaSnapshot` 同时保存：

- RunId、OwnerId、DeploymentId 与 Content stamp；
- 每个 placement receipt／placement／anchor definition／anchor instance identity；
- 每个 anchor 的 exact XYZ bit pattern；
- canonical anchor order 与 counter-clockwise hull indices。

`AreaId` 使用上述完整 evidence 生成。输入 receipt 顺序变化不改变 identity；任一 source identity 或坐标变化都会改变 identity。`IsValid()` 会重新检查唯一性、canonical 排序、凸包与 AreaId，因此 snapshot 创建后的字段漂移不能继续冒充原区域。

## 4. Membership 与 coverage

单实体查询只携带 stable `SubjectEntityId` 与有限坐标，输出三态关系：

- `Inside`；
- `Boundary`；
- `Outside`。

Boundary 保持独立，不被悄悄折叠成 Inside；`IsCovered()` 明确把 Inside 与 Boundary 视为 covered，调用方仍可读取原始关系。membership receipt identity 绑定 AreaId、实体、exact XYZ 与关系。

批量 coverage 按 SubjectEntityId canonical 排序，每个实体只接受一个位置，分别统计 Inside／Boundary／Outside。query 输入顺序不会改变 coverage receipt；重复实体、空集合、无效 Area 或非有限坐标全部失败关闭。

## 5. 权威、完整性与兼容性

P8.5 不拥有 placement、deployment 或 entity location 权威：

- World placement 真值仍由 P8.3 receipt 提供；
- material／deployment／host 真值仍在 P8.0—P8.4；
- entity identity 与实时位置采样仍后置给 WorldGameplay／产品 adapter；
- provider 只复制 receipt evidence 并执行纯几何。

没有修改 P8.0—P8.4、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。没有第二套库存、Actor registry 或效果系统。

## 6. 自动化证据

最终日志均为 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`，进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationAreaProvider` / `FormationAreaProvider.log` | 4 | 0 | `5BE98469BF66698DEA374CFB7EF2C27E261CC06552E407C217E2F8AC17F690F9` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 231 | 0 | `0125A95EAA9C21ACEFF721C6C8EC26CC04465482DCCC697E8D7E5CF6C6AD2AB2` |

四项 focused 测试覆盖：

1. shuffled receipts、内部点保留、canonical CCW hull 与 replay-stable AreaId；
2. Inside／Boundary／Outside、不同 Z 的水平语义、batch canonical order 与 coverage replay；
3. 空集、少于三点、共线、重合点、重复 identity、混 deployment、无效 receipt／query 与重复 subject；
4. exact XYZ geometry seal、snapshot mutation rejection 与区域 identity 分离。

完整 suite 从 P8.4 的 `227` 增至 `231`，此前测试全部继续通过。

## 7. 失败证据与修正

首次 Editor build 原生退出码 `1`，UBT 为 `OtherCompilationError`：四个新测试使用了本工程 UE 5.8 不提供的 `EAutomationTestFlags::ApplicationContextMask`。按现有 P8 suites 统一改为 `EditorContext | EngineFilter` 后，Editor source build、focused、full 与最终两目标构建全部通过。

该失败发生在测试注册标志，production provider translation unit 已在同次构建中完成编译；没有把它描述为内存或环境故障，也没有隐藏首次失败。

## 8. 改动—回归与静态门禁

新增 `FormationAreaProvider` mapping rule，要求六组证据：focused AreaProvider、FormationProductHost、FormationWorldDelivery、FormationSession、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`41` rules；
- mapping self-test：`43/43 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=2`；
- map SHA-256：`6F4F47EB5C273F0337281D8E7D4EF10743BC6C7D8C6B48531815FA085EBDD1CE`；
- self-test SHA-256：`26CE9DF25521E8BBA9E0DD5FA088CD8BDB6499450C555B1DE9A95F01B3A7C8F5`；
- production boundary scan：无 UWorld、AActor、damage/effect、RNG、Tick/timer、input、Widget 或 legacy item subsystem；
- `git diff --check` 与最终 staged `git diff --cached --check`：PASS。

## 9. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor final | Succeeded / up to date | 0 | 1.06s | `4CC0898B8C90A7E79624DF0AC7C96B6EE49C0086E14980647D2A0956160D9465` |
| Game final | Succeeded | 0 | 23.02s | `2886A1B94A74FE2C512276FB5506F68AB13F5DDCAB54E94A3B2247854AD05A6E` |

最终源码 Editor 实际重编译也成功，原生退出码 `0`、总耗时 `10.42s`；随后 final up-to-date check 再次返回 `0`。

- Editor module：`11049472` bytes，UTC `2026-08-29T17:31:10.2297632Z`；
- Game executable：`352265216` bytes，UTC `2026-08-29T17:33:47.8910642Z`。

## 10. 修改范围、P/F 边界与下一步

新增：

- `demo_mapShanmenFormationAreaProvider.h/.cpp`；
- `demo_mapShanmenFormationAreaProviderTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

P8.6 建议建立窄 World coverage sampler：从既有 WorldEntityRegistry 取得 stable EntityId 与瞬时位置，形成一批 query 后只调用 P8.5；不得把 Actor pointer、效果应用或定时节拍塞回纯 provider。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-5-formation-area-provider/Docs/Report/Dev.D.UE.0.0.10.P8.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-5-formation-area-provider/Docs/Log/Dev.D.UE.0.0.10.P8.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-5-formation-area-provider>
