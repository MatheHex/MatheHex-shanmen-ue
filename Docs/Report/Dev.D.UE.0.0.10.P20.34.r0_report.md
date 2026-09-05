# Dev.D.UE.0.0.10.P20.34.r0 Report

## 1. 结论

P20.34 已把 P20.32 的 live product capture 与 P20.33 的 renderer-neutral presentation 串成一个同步、有界、无状态的 Arc preview update coordinator。单次可见更新严格执行一次 Capture、一次 Project、一次 Replace；非 preview choice 则不读取产品、不计算几何，只在已有消费者状态时执行一次 Clear。

返回值完整记录 choice、previous state、每个阶段的 typed result、输出状态和精确调用次数。调用方仍是 presentation state 的唯一持有者；coordinator 不持有产品会话、库存、Run、World、Actor、组件、计时器、输入、HUD 或 renderer 权威。

新增 8 项自动化，完整 0.0.10 由 1021 增至 1029/0。按 changed-file 映射执行 9 份最终自动化证据，合计 1124/0；Editor 与 Game Development 构建均成功。

本轮没有连接 HUD/UI、World renderer、真实设备输入、trace、collision、projectile、launch、库存 reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：21c02bcfa71dbb62266cfa0486cc51f0f6141372（P20.33）；
- 分支：agent/0.0.10-p20-34-thrown-weapon-arc-preview-update-coordinator；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Bounded update contract

Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update() 接收：

1. 当前 hotbar slot、choice、choice policy、segment count 与 source basis；
2. 消费者持有的 previous presentation state；
3. 只读 product lifecycle 与 Combat Run coordinator。

当 choice 是带 target 的 BallisticArc 时，更新链固定为：

1. P20.32 ProductBridge::Capture() 一次；
2. P20.33 PresentationProjector::Project() 一次；
3. P20.33 PresentationReducer::Replace() 一次。

任一阶段拒绝后立即停止，不调用后续阶段。结果中的 CaptureCount、ProjectCount 与 ReduceCount 只能为 0 或 1，并与 typed status、嵌套结果和输出状态互相验证。

## 4. Clear without product access

非 preview choice 走独立清理路径：

- previous state 为空时返回 NoPresentationRequired，调用计数为 0/0/0；
- previous state 有效时只调用一次 Clear，调用计数为 0/0/1；
- clear 不读取 hotbar、不访问 lifecycle/coordinator、不执行 Arc 几何投影。

因此，即使产品 lifecycle 与 Combat Run 已经 shutdown，调用方仍能清除旧预览。无效 slot、空 policy、空 basis 不会阻碍 teardown；clear 后保留 P20.33 hidden tombstone 的 scope/revision 防线。

## 5. Revision、scope 与幂等

coordinator 不重写 P20.33 reducer 的规则：

- 首个有效 visible snapshot 安装为 Replaced；
- 相同输入重放为 Duplicate，不产生第二状态；
- 严格更高 choice revision 才能 Replace 或 Clear；
- 较旧 revision 返回 StaleRevision；
- 相同 revision 但几何漂移返回 RevisionConflict；
- foreign Run/player/item previous state 返回 IdentityMismatch；
- clear 后更高修订的 Arc target 可以重新显示。

所有拒绝结果都不输出伪造 presentation state；调用方只有在 IsCompleted() 后才可采用 GetState()。

## 6. Typed 状态与只读边界

Update status 区分 Invalid、ChoiceRejected、PreviousStateRejected、NoPresentationRequired、CaptureRejected、ProjectRejected、ReduceRejected、Replaced、Cleared 与 Duplicate。

IsCompleted() 只接受 NoPresentationRequired、Replaced、Cleared 与 Duplicate。DidChange() 只接受 Replaced/Cleared；IsNoChange() 只接受 NoPresentationRequired/Duplicate。

生产实现对 lifecycle/coordinator 只持 const reference。boundary scan 确认没有产品 mutation、World/Actor/UI/renderer 调用，没有随机 GUID，没有 Draw/Spawn/GetWorld，也没有 placeholder。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator 新增 8 项：

1. VisibleInstall：一次 1/1/1 更新安装 live renderer-neutral snapshot，产品 authority 不变；
2. DeterministicDuplicate：相同 choice/basis/state 重放为幂等 duplicate；
3. NewerRevisionReplace：严格较新修订替换当前 snapshot；
4. ClearWithoutProduct：空状态 0/0/0，已有状态在 shutdown 后 0/0/1 清除；
5. RetargetAfterClear：hidden tombstone 后更高修订 target 可恢复 visible；
6. StageFences：choice/capture/project rejection 阻断后续调用；
7. RevisionFences：equal-revision geometry drift 与 stale work fail-closed；
8. ScopeFence：previous state 不得跨 Run/player/item scope。

专属测试首轮即为 8/0，没有生产代码或 fixture 修正轮。完整 0.0.10 为 1029/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewUpdateCoordinator 规则，覆盖 coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 20 个 required groups。

正反 mapping self-test 由 361 增至 363/363。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=20 Logs=9，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_update_coordinator_final.log | Product.ThrownWeaponArcPreviewUpdateCoordinator | 8/0 | 150F6CF66A4B91803B1265DF21D25C031D1F80991E29F39C9A76932F7D2E6C26 |
| arc_preview_presentation_final.log | Product.ThrownWeaponArcPreviewPresentation | 8/0 | 46E2DE71D9BDF82B34E7E48E12A717F1D4B422CFE7F5F83D3EE7D02886C2F122 |
| arc_preview_product_bridge_final.log | Product.ThrownWeaponArcPreviewProductBridge | 7/0 | C582999C5DCC30DF4315B16AF20C271A2122FDB4044E7A8AB25D49AA312BAC5A |
| arc_preview_capture_final.log | Product.ThrownWeaponArcPreviewCapture | 8/0 | C43404B41DB554F8D7CA32C5E2D30DEF6F808C530E800BB1DD0F5EB4BA5F8AD2 |
| arc_preview_composition_final.log | Product.ThrownWeaponArcPreviewComposition | 7/0 | 6AE73F056FB7A2F33E0B7454A2E9FCA761626FD1206CC7FAF1809A567DB7683F |
| thrown_product_lifecycle_final.log | Product.ThrownWeaponProductLifecycle | 5/0 | 3222D5DA16BA371D6F3D18B2DA967F96B909B408D5F6A55101152775B45D3847 |
| thrown_product_session_final.log | Product.ThrownWeaponProductSession | 6/0 | 18BB6162D99427559CACCCB8FBBA515FC31A29B0AA7ACBE6C9F78D2A29098DE9 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 52E93AC48ECCB621B26B67740B94EB5190F93CAFD5536C562F33AF0F0808FCE5 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1029/0 | 03C35AFA06D77EEC3126AA4B2737BD886A576B82FC98BA09EB583983C4828C75 |

最终自动化证据合计 1124/0。每份日志均有唯一 RunTests、精确成功数、零 fail/not-run、唯一 UE 5.8 原生成功终止标记与零 fatal/unhandled/ensure/no-match。

- automation audit：PASS，SHA-256 72C6D455C9521824AFF3CC92B8EE1F6BFC610EB06080A423DCB3CB9A444F88BC；
- regression self-test：363/363，SHA-256 3FEF4985D317410E350CC978060415E04EC962F65AC2CC9D30AE5F33F4D1F408；
- boundary scan：PASS，update files 2、lines 422、forbidden mutation/World call 0、Capture/Project/Replace/Clear 均恰好 1、mutable authority parameter 0、World/UI include 0、placeholder 0，SHA-256 40E556B6AE0639B35A256EFE02177B366AD92FAC75219A0D52567A368EDF9C8F；
- changed-file gate：PASS Changed=5 Rules=2 Required=20 Logs=9，SHA-256 527472D4F3E57914EDF0F529492AEDB0C0B95988E5230B5B29643C2CA9EDDF24；
- staged diff check：PASS，7 个精确暂存文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 2564CE5CD59AE9A8AF828919E8003D17BC9F38A4363F726B82244014BCA663C6；
- final Editor：up to date / native 0 / 2.21 秒，SHA-256 7BBA2B24B1647395D8D14909047776E1B49764FB6E1379CF5886F97457425EAD；
- final Game：4 actions / native 0 / 36.30 秒，SHA-256 D6C5C30EA3C1C364961FFFC968209165BAA906F9D314DF219D634846EBB72292。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,675,840 bytes，SHA-256 2E73C9411DD4862276DD4FE149A45D2EE6EA6C9740B89F9678106120D12A4169；
- Binaries/Win64/demo_map.exe：357,886,464 bytes，SHA-256 B1B8097A26F8D26459CB89EFFF3E551AC40FCBA5B517EF8A08519EE49E05D173。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：live product Arc preview 的单次 capture/project/replace、无需产品访问的 teardown clear、精确阶段调用预算、typed stop-on-rejection、确定性 duplicate、strict revision replace/clear、stale/conflict/foreign scope fencing、产品 authority/sequence/Host/selection 不变，以及完整 0.0.10 与 changed-file 回归。

未验证：HUD/UI presentation、真实 renderer 或可见轨迹、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.35 建立 consumer-owned in-memory Arc preview presentation session：由一个明确 Run scope 的消费者保存 state、调用 P20.34 bounded update，并提供显式 Run rotation/reset；仍不连接真实 HUD、World 或 renderer。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-34-thrown-weapon-arc-preview-update-coordinator>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-34-thrown-weapon-arc-preview-update-coordinator/Docs/Report/Dev.D.UE.0.0.10.P20.34.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-34-thrown-weapon-arc-preview-update-coordinator/Docs/Log/Dev.D.UE.0.0.10.P20.34.r0_log.md>
