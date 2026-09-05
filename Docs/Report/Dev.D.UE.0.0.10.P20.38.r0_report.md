# Dev.D.UE.0.0.10.P20.38.r0 Report

## 1. 结论

P20.38 已在 P20.36 presentation command 与 P20.37 acknowledgement/cursor ledger 之上建立 renderer-neutral presentation port 和 bounded delivery coordinator。新 command 只有通过 command、ledger、Run、consumer 与 cursor 五层预检后才会调用 port；同一 CommandId 一旦存在 Applied 或 Rejected receipt，后续交付只重放 ledger 证据，不再调用 port。

port 每次返回一个自校验 response。有效 Applied/Rejected response 会被 coordinator 封存为同 outcome receipt 并原子写入候选 ledger；空回复、无效回复或 stale CommandId 回复统一封存为稳定 Rejected receipt。因此所有正常响应路径都在一次 port 调用后形成终止证据，避免自动重试制造重复视觉副作用。

新增 8 项 fake-port 自动化，完整 0.0.10 由 1053 增至 1061/0。按 changed-file 映射采用 3 份正式日志，累计执行记录 1115/0；累计数包含专属、legacy 与全量日志的有意重叠。映射 self-test 为 371/371，Editor 与 Game Development 构建均成功。

本轮仍未连接 HUD/UI、真实 renderer、widget/component、World 或 Actor。response 与 receipt 都是调用方声明，不证明可见帧已实际出现。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行真实设备输入、trace、collision、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：46269e88031edc989aaf08790083a26270caced9（P20.37）；
- 分支：agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Renderer-neutral port response

新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse：

1. 必须绑定一个有效 CommandId；
2. outcome 只允许 Applied 或 Rejected；
3. 必须携带稳定、非空 OutcomeCode；
4. ResponseId 由 CommandId、outcome 与 outcome code 确定性派生；
5. IsValid 重新派生 ResponseId；
6. Matches 比较全部证据，不只比较 GUID；
7. response 不声明动画、widget、renderer 或可见帧已经真实发生。

新增 Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort，接口仅暴露稳定 ConsumerDefinitionId 与 Apply(command)。它不拥有 ledger、retry、World、Actor、widget、component、输入或产品 authority；未来具体 adapter 可以实现该窄能力，而 coordinator 保持 renderer-neutral。

## 4. 有界交付与预检

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator::Deliver 是无持久状态的单次交付入口。调用 Apply 前严格检查：

1. command 自身有效；
2. ledger 自校验有效且 active；
3. command Run 与 ledger Run 精确相同；
4. port 提供非空稳定 consumer identity；
5. port consumer 与 ledger consumer 精确相同；
6. 若无历史 receipt，command previous state 必须匹配当前 ledger cursor。

任何预检失败都返回 typed result，PortCallCount=0，ledger 不变。未来 Replace 不能跳过尚未应用的 Show；跨 Run、跨 consumer 或 inactive ledger 也不会触发外部副作用。

## 5. Exactly-once 去重与终止证据

coordinator 在 cursor 预检前先查询已有 receipt：

- 已 Applied：以 ApplicationReplayed 返回原 receipt，port 不调用，cursor 不重复推进；
- 已 Rejected：以 RejectionReplayed 返回原 receipt，port 不调用，不自动重试；
- 同 command 同时具有 P20.37 显式恢复后的 rejected/applied 证据：优先重放 Applied；
- 无历史证据且预检通过：Apply 恰好调用一次。

有效 response 的 CommandId 必须匹配输入 command。Applied response 生成 Applied receipt 并推进 cursor；Rejected response 生成 Rejected receipt且不推进。无效或 stale response 使用 Renderer.ArcPreview.InvalidPortResponse 封存终止性 rejection，后续调用只重放该 rejection。

ledger 在 port 调用前复制为 candidate，receipt 只先写 candidate；成功自校验后整体提交，避免局部 ledger mutation。理论上的 ledger commit 内部失配会再尝试记录 Renderer.ArcPreview.LedgerCommitRejected；若连终止证据也无法形成，则返回显式 InvariantViolation，不把内部不一致伪装成成功。

## 6. Show、Replace、Hide 与 NoOp

四种 P20.36 command 都通过同一 port 契约。Show、Replace、Hide 要求 renderer mutation；NoOp 不要求绘制变更，但仍调用 port 一次，让 consumer 观察 accepted revision，再以 Applied receipt 推进 audit cursor。

这样 hidden tombstone 等 NoOp 不会丢失顺序位置：Show → Hide → hidden NoOp 形成三条连续 receipt，下一 command 仍可从精确 current state 开始。coordinator 不重新解释 geometry，也不读取 product lifecycle；视觉差量语义继续由 command 自身承载。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator 新增 8 项：

1. PortResponseDeterminism：response identity、Applied/Rejected 隔离与输入拒绝；
2. PreflightNoPortCall：invalid/inactive/Run/consumer/cursor 全部零 port 调用；
3. AppliedExactlyOnce：首次 Applied 推进，重放不再调用；
4. RejectedExactlyOnce：首次 Rejected 留证，重放不自动重试；
5. InvalidResponseFailsClosed：空回复与 stale CommandId 均终止性拒绝；
6. OrderedShowReplace：越序 Replace 零调用，Show 后 Replace 各调用一次；
7. HideAndNoOp：Show/Hide/NoOp 全链连续，NoOp 被观察一次；
8. ExternalRecoveryReplay：兼容 P20.37 显式外部恢复，恢复后只重放 Applied。

候选 Editor build 与专属自动化首轮均成功。静态复核后补充 InvariantViolation，并把文件级 FName 常量改为按需构造，避免模块初始化顺序依赖；最终专属仍为 8/0。完整 0.0.10 为 1061/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewPresentationDeliveryCoordinator 规则，覆盖 delivery coordinator、command ledger、presentation command、session、update coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 24 个 required groups。

正反 mapping self-test 由 369 增至 371/371。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=24 Logs=3，全部具备健康证据。

证据继续保持 3 份：delivery coordinator 专属、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10。完整日志覆盖其余上游映射组，不为同一二进制重复启动大量子组；测试选择仍由实际改动路径推导。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_delivery_coordinator_final.log | Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator | 8/0 | F178A40079D0F6EC4B76BE9208ADA825752D587B63DD5C6795591045B35CC7B5 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 34C2900EF73F762A435EB88CD26006751397EA7DC7CDE42372BFEC31BE743E60 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1061/0 | 4D32D50347D05323AB60F4A75B276F59C2F7ED7580C233AE20427BFB9AAA4689 |

正式日志累计 1115/0，包含定向、legacy 与全量重叠；独立完整套件为 1061/0。完整套件在既有 SwordRhythm retry/journal/checkpoint 大型快照区段出现长 tick，UE 将其记录为不惩罚 unresponsive test，进程随后原生完成；没有外层 timeout 或人工中止替代结果。

- automation audit：PASS，3 logs / 1115 success / 0 fail-or-not-run，SHA-256 ED326027D8A65A6A9115B2376A7196C96CF03CD8FAD1F0FBBD8EE7A4FD8043FE；
- regression self-test：371/371，SHA-256 A3216731630BCBC7EF635478B0558F04EA6122717F079A68B777861101F4E1C5；
- boundary scan：PASS，files 2、lines 737、forbidden 0、deterministic ID factories 1、World/UI include 0、placeholder 0、port entrypoints 2、map rule 1/24 groups，SHA-256 3FDA92178A2698A349547AAF903B72D20168FBFDA1A134B4142B6A9DDE48B4A2；
- changed-file gate：PASS Changed=5 Rules=2 Required=24 Logs=3，SHA-256 69AE1D57AF00927D1A4BC9D08BEC0A9F9D8D0994A57C9D5DF9FF1AFBCD9B27C5；
- git diff check：PASS，7 个本轮文件、0 个空白错误，SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS，7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD；
- candidate Editor：5 actions / native 0 / 8.17 秒，SHA-256 3F1BB2B37C9DB2AF910B4E2996FA279685EC9E64FFDBFCD36196C81AC1C51823；
- candidate focused coordinator：8/0，SHA-256 217A4E5A0BB402C24E851C3FC73387487DB5F842B30D8A482B73D1C97D64C909；
- final Editor：4 actions / native 0 / 7.52 秒，SHA-256 39D4695BB70C7EC87A9B4C05463A7D15939E975C8A71491E2DDE6F5E0143E1B8；
- final Game：4 actions / native 0 / 34.84 秒，SHA-256 AD16BC4A45750FD886EC3F66C0CA2D655CA156CB6FEB523F6951AEE3C3010C36。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16895488 bytes，SHA-256 9AACAA9A4BD70887D18BF735C09BF2A43FFBF3EFB348CB9F8D44E8EB16FA4D0C；
- Binaries/Win64/demo_map.exe：358070272 bytes，SHA-256 B6F497C56C341F7200F93E375447C503291C9FC5DED00A910A741739F3A69748。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：port response deterministic identity、自校验、command/Run/consumer/cursor 预检、零调用拒绝、Applied/Rejected terminal receipt、invalid/stale response fail-closed、同 CommandId 去重、external recovery replay、Show/Replace/Hide/NoOp 顺序交付、candidate-ledger 原子提交，以及完整 0.0.10 与 changed-file 回归。

未验证：真实 HUD/renderer/port adapter、widget/component/visible frame、跨线程调用、进程崩溃后的持久 exactly-once、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。无头 fake-port 通过不能描述为可见产品验收。

建议 P20.39 建立 consumer-owned delivery session：由一个 Run-scoped owner 创建/关闭 ledger，串行接受 command stream，委托 P20.38 coordinator，并要求关闭前 cursor 已隐藏或为空；仍先使用 fake port，不连接真实 HUD/World。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator/Docs/Report/Dev.D.UE.0.0.10.P20.38.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator/Docs/Log/Dev.D.UE.0.0.10.P20.38.r0_log.md>
