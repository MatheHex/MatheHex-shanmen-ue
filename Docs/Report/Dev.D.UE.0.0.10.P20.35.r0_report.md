# Dev.D.UE.0.0.10.P20.35.r0 Report

## 1. 结论

P20.35 已建立 consumer-owned Arc preview presentation session。一个消费者显式绑定一个 Combat Run，在内存中独占保存 P20.33 presentation state，并通过 P20.34 bounded update coordinator 完成同步更新。可见 Arc 更新会先验证产品 lifecycle、Combat Run coordinator 与 session 的 Run identity，再恰好调用一次 P20.34；任何拒绝都不会污染已提交状态。

session 的 TryEnd 使用精确 Run fence，Reset 则无条件丢弃 Run 与全部 presentation state。非 preview/clear 路径不依赖仍存活的产品来源，因此产品 shutdown 后仍可清除旧预览；新 Run 必须重新 Begin，不能继承旧 geometry 或 hidden tombstone。

新增 8 项自动化，完整 0.0.10 由 1029 增至 1037/0。按 changed-file 映射执行 10 份正式自动化日志，累计执行记录 1148/0；该累计数包含定向日志与全量日志的有意重叠。Editor 与 Game Development 构建均成功。

本轮没有连接 HUD/UI、World renderer、Actor/组件、真实设备输入、trace、collision、projectile、launch、库存 reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：2e62553d23e20c1abab7e06e84755224b5ed7867（P20.34）；
- 分支：agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Consumer-owned session contract

新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession：

1. TryBegin 只接受有效 Run identity；
2. 同一 Run 重复 Begin 幂等成功；
3. session 活跃时拒绝静默切换到第二个 Run；
4. session 内仅保存一个 RunId 与一个消费者 presentation state；
5. TryEnd 只接受当前精确 Run，错误 Run 不得清理别人的状态；
6. Reset 丢弃 RunId 与全部 state，恢复为空；
7. 新 Run 从 empty state 开始，即使 choice revision 与旧 Run 相同也不能复用旧 state identity。

session 不拥有 product lifecycle、Combat Run coordinator、库存、World、输入、HUD 或 renderer。它只消费只读引用并拥有表现状态。

## 4. Atomic update contract

TryUpdate 每次最多调用一次 P20.34 Update()。调用前保存 previous state；只有嵌套 update 的 IsCompleted() 为真、且候选 state 仍属于 session Run 时，才原子提交候选 session。

拒绝时返回值保留 previous/current state，session 本体不变。返回结果记录：

- session Run identity；
- previous state 与最终 state；
- 完整 P20.34 typed update result；
- coordinator 调用次数 0 或 1；
- typed status 与诊断。

status 区分 Invalid、SessionInactive、SessionInvalid、ProductUnavailable、RunMismatch、UpdateRejected、StateRejected、Applied 与 NoChange。IsAccepted、DidChange 与 IsNoChange 只在结果自身交叉验证通过后成立。

## 5. Visible、clear 与 Run fences

带 target 的 BallisticArc choice 属于可见更新。session 在调用 P20.34 前要求：

1. lifecycle active 且 valid；
2. Combat Run coordinator ready；
3. lifecycle RunId、coordinator RunId 与 session RunId 全部精确一致。

产品不可用或 foreign Run 会在调用前拒绝，coordinator call count 为 0，消费者状态保持不变。

非 preview choice 仍交给 P20.34 的 teardown 路径：empty state 返回 NoPresentationRequired；已有可见 state 可在 lifecycle/coordinator shutdown 后 Clear；重复 clear 为 Duplicate。该路径不伪造产品存活，不读取产品来决定是否允许清除旧视觉状态。

## 6. Determinism 与只读边界

session 完整复用 P20.33/P20.34 的 deterministic identity、strict revision、duplicate、stale/conflict 与 scope 规则。相同输入重放为 NoChange；严格较新 choice revision 才能替换；拒绝后恢复请求仍可正常提交。

生产实现对 lifecycle/coordinator 只持 const reference。boundary scan 确认没有产品 mutation、World/Actor/UI/renderer 调用，没有随机 GUID，没有 Draw/Spawn/GetWorld，也没有 placeholder。唯一外部编排调用是 P20.34 Update() 的一处静态调用。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession 新增 8 项：

1. RunBinding：有效 Begin、同 Run 幂等、foreign Run 拒绝与精确 End；
2. VisibleCommit：一次 bounded update 原子安装 live state，产品 authority 不变；
3. DuplicateAndRevision：重复输入 NoChange，严格新 revision 替换；
4. NoPreviewWithoutProduct：空消费者无需产品来源即可完成 no-op；
5. ClearAfterShutdown：产品 shutdown 后可 Clear，并对重复 clear 去重；
6. RejectionAtomicity：choice/capture 拒绝不污染 state，后续有效更新可恢复；
7. RunSourceFences：foreign 或 unavailable 产品来源在调用 P20.34 前拒绝；
8. EndResetRotation：错误 teardown 不清理、精确 End 清空、新 Run 隔离、Reset 后禁止未绑定更新。

专属测试首轮即为 8/0，没有生产代码或 fixture 修正轮。完整 0.0.10 为 1037/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewPresentationSession 规则，覆盖 session、update coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 21 个 required groups。

正反 mapping self-test 由 363 增至 365/365。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=21 Logs=10，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_presentation_session_final.log | Product.ThrownWeaponArcPreviewPresentationSession | 8/0 | 7E9D563F9188BCF805E9C97C53768D69AB4562743C1728074D5CB3E38B781CB1 |
| arc_preview_update_coordinator_final.log | Product.ThrownWeaponArcPreviewUpdateCoordinator | 8/0 | 199E69CE9979CECE3FC6A4C070321DC42185FDB9BCB9CF2BE3D76DFE05BA6E8C |
| arc_preview_presentation_final.log | Product.ThrownWeaponArcPreviewPresentation | 16/0 | 725D7E82B77A690584A942061FE8DD726994BE3A9185C44BBB9918873FE1FC0C |
| arc_preview_product_bridge_final.log | Product.ThrownWeaponArcPreviewProductBridge | 7/0 | A8C265F34274875F13A9ACE81E5C3732F867536C841A7F719140B7A8A1E3695B |
| arc_preview_capture_final.log | Product.ThrownWeaponArcPreviewCapture | 8/0 | 0DDDB0426D2B91BCA49B4B500FA1AF927DBD4FF6F80B3E3BF33F2CB0A72E597D |
| arc_preview_composition_final.log | Product.ThrownWeaponArcPreviewComposition | 7/0 | 0B4670B8E24819FFEB120ACF76C012BF8947B4C011BF5FDF823583205124180A |
| thrown_product_lifecycle_final.log | Product.ThrownWeaponProductLifecycle | 5/0 | F4128A6039B4D0D402B0D8857CE53778FEBC3914B326B824EC02DAEE63DC8978 |
| thrown_product_session_final.log | Product.ThrownWeaponProductSession | 6/0 | D7418A097BFC4B24F1C0538F0258B82B3CC1DED903B821C58A6CDC482FBA3A44 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 52798154694CEF85AF7DE320717E704E885BA6CAC908F1064438968950232D38 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1037/0 | 8C073C2BD79B9AA335DA51461A8CEB53BCF5453AEACF29F5335F77766A4A5E95 |

`ArcPreviewPresentation` 是 UE 的名称过滤前缀，因此该定向命令同时命中原有 8 项 presentation 与新增 8 项 PresentationSession，日志精确为 16/0。正式日志累计 1148/0 是执行记录总数，不表示存在 1148 个互不重复用例；独立完整套件为 1037/0。

- automation audit：PASS，10 logs / 1148 success / 0 fail-or-not-run，SHA-256 3C21EE13471C114038922F661DC79ECACF6052A5A5C43115C47FFCEE69D1B6AD；
- regression self-test：365/365，SHA-256 A39BAE64E4C1B42AF6873EEC1D352CE598B2060593FEBD3F2C90114152646C35；
- boundary scan：PASS，files 2、lines 467、forbidden mutation/World call 0、P20.34 Update call 1、mutable authority parameter 0、World/UI include 0、placeholder 0、consumer Run/state pair 1，SHA-256 E2AB26C4B9B19BC06BC10579CBE3EC0DF60FF8E5B25C5C66E8A9EFE5CC3049F8；
- changed-file gate：PASS Changed=5 Rules=2 Required=21 Logs=10，SHA-256 E0F090AE36ECF3DC632F5A4C37B30EBA9E97DB2078AEEAA00E2EB5D20AFF801F；
- git diff check：PASS，SHA-256 6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9；
- staged diff check：PASS，7 个精确暂存文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 ECE56D3FABEC3E7EF165397F82E4306AA6D8D2615C57C038F229BFB396D17B77；
- final Editor：up to date / native 0 / 1.18 秒，SHA-256 0BE8C41AE8EC1843D0F3F36C15801CD6411F6272C17AF3DDA4CD532C8A3CD8B8；
- final Game：4 actions / native 0 / 28.37 秒，SHA-256 F1FC9219123C085C52852B003795D58DD2980A9586F9777467C9692A1D9B7F15。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,717,824 bytes，SHA-256 5D1CA176E1689AF9270956BFE44D916D7099555CB7455EB150BB7068E8163170；
- Binaries/Win64/demo_map.exe：357,921,792 bytes，SHA-256 AD4299F5A17118388C45941812935ECBC2393DF876BCDE8D9187F37E8BA0064F。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：消费者显式 Run binding、同 Run 幂等、foreign Run 防线、一次 bounded update、completed-only atomic commit、rejection state preservation、product-independent teardown clear、shutdown 后清理、strict revision/duplicate、精确 End/Reset/new-Run isolation、产品 authority 不变，以及完整 0.0.10 与 changed-file 回归。

未验证：HUD/UI presentation、真实 renderer 或可见轨迹、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.36 在 session result 之上建立 renderer-neutral presentation delta/command（Show、Replace、Hide、NoOp），供未来 HUD renderer port 消费；仍不接真实 UI、World 或 renderer。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session/Docs/Report/Dev.D.UE.0.0.10.P20.35.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session/Docs/Log/Dev.D.UE.0.0.10.P20.35.r0_log.md>
