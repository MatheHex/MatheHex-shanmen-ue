# Dev.D.UE.0.0.10.P20.39.r0 Report

## 1. 结论

P20.39 已建立 consumer-owned、Run-scoped 的 thrown-weapon Arc preview presentation delivery Session。Session 私有创建和持有 P20.37 ledger，串行接受 renderer-neutral command，单次委托 P20.38 coordinator，并只在 cursor 为 Hidden 或 Empty 时允许 graceful end。

本阶段仍是 P 阶段无头契约：fake port 证明 command stream、receipt、cursor 与关闭规则，不证明真实 HUD、renderer、widget、component 或可见帧已经存在。

## 2. 基线、分支与范围

- 基线：`ede8c39ed9ccf6cd5d39d956d9dd524867f30570`；
- 分支：`agent/0.0.10-p20-39-thrown-weapon-arc-preview-delivery-session`；
- 新增：`demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h/.cpp`；
- 测试承载：`demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 流程：`ShanmenRegressionMap.json` 与 mapping self-test；
- 未连接真实 HUD、World、Actor、widget、component、设备输入、projectile、库存或产品 mutation。

## 3. Session ownership

Session 固定一组 exact `(RunId, ConsumerDefinitionId)`：

1. `TryBegin` 要求有效 Run 与非空 consumer；
2. 首次 begin 创建私有 command ledger；
3. exact begin replay 幂等返回成功；
4. active Session 拒绝 Run 或 consumer 轮换；
5. ledger 只通过 const getter暴露，调用方不能绕过 coordinator 直接修改；
6. Session、ledger、cursor 必须始终共享同一 Run/consumer scope；
7. graceful end 原子清空 ledger 与 Session scope，下一 Run 获得独立 LedgerId。

没有公开无条件 Reset；因此调用方不能用“丢弃 C++ 状态”伪装 renderer 已隐藏。

## 4. 串行 delivery

`TryDeliver` 在 Session 层先检查 active、command 与 Run；通过后：

1. 复制 candidate ledger；
2. 将 delivery-in-progress guard 置位；
3. 恰好一次调用 P20.38 coordinator；
4. coordinator 对 consumer、cursor、receipt、port response 与去重负责；
5. 仅当 delivery result 和 candidate ledger 均自校验成功时整体提交；
6. post-commit result 异常时恢复 exact previous ledger 并返回 InvariantViolation。

新的 Applied/Rejected、ApplicationReplayed/RejectionReplayed 都有独立 Session status。preflight failure 的 coordinator count 为 0；delegated failure 的 coordinator count 为 1；result 交叉校验 command、before/after cursor、applied/rejected count 与 nested ledger result。

## 5. 重入防护

port `Apply` 是外部虚接口，理论上可以同步回调 Session。P20.39 使用 `TGuardValue<bool>` 封住重入窗口：

- 回调内再次 `TryDeliver` 返回 `DeliveryInProgress`；
- 第二次 coordinator call 为 0；
- 第二次 port call 为 0；
- 回调内 `TryEnd` 失败；
- 外层 delivery 仍只提交一次 candidate ledger。

这保证单线程同步对象的串行语义；不宣称跨线程安全或进程崩溃后的 exactly-once。

## 6. Hidden/Empty 关闭不变量

`CanEnd` 与 `TryEnd` 只接受：

- Empty cursor：尚未成功 Show，或首次 Show 已终止拒绝；
- Hidden cursor：Hide 已 Applied，后续 Hidden NoOp 也可安全关闭。

Visible cursor 一律阻止关闭。尤其当 Show 已 Applied、Hide 被 port 拒绝时：

- rejection 被 ledger 保留；
- cursor 继续 Visible；
- 同 Hide replay 不重新调用 port；
- graceful end 继续失败，不能把 rejection 当成隐藏证明。

## 7. 自动化覆盖

新增 7 项：

1. `Lifecycle`：begin、exact replay、scope rotation、empty end、next-Run isolation；
2. `Preflight`：inactive、invalid command、foreign Run、consumer mismatch、cursor mismatch；
3. `OrderedReplay`：Show、exact replay、Replace 与 visible-end fence；
4. `HiddenEnd`：Show、Hide、hidden NoOp 与原子清空；
5. `RejectedHideBlocksEnd`：rejection receipt、无自动重试、visible teardown fence；
6. `EmptyCursorRejectionEnd`：无效首次 Show response 保持 Empty，可安全结束；
7. `ReentrantPortBlocked`：delivery/end 回调重入均在第二次副作用前被拒绝。

完整 `Shanmen.0_0_10` 从 1061 增至 1068 项。

## 8. Changed-file 回归映射

新增 `ThrownWeaponArcPreviewPresentationDeliverySession` 规则，覆盖 delivery Session、delivery coordinator、command ledger、presentation command、presentation state Session、update coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad `Shanmen.0_0_10` 与 legacy `ItemUseAndArmor`，共 25 个 required groups。

mapping self-test 从 371 增至 373/373。最终 gate 对 5 个实现、测试和流程路径求并集：Changed=5 Rules=2 Required=25 Logs=3，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| delivery_session_final.log | Product.ThrownWeaponArcPreviewPresentationDeliverySession | 7/0 | 9B0761C138253489A71A8EBDB5912DFD18A8B162F9C8522FDEFC833E29D50B43 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 469FC6A3FF1748A2C60AC3C615C947DD3F65BD19462042D0D524B6EF17244D68 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1068/0 | F06D40CE08B5692F48C13DD44B503C2C0C44CCF17B679A62E1FF354C39C5C8A4 |

正式日志累计 1121/0，包含定向、legacy 与完整套件重叠；独立完整套件为 1068/0。完整套件在既有 SwordRhythm retry/journal/checkpoint 大型快照区段出现长 tick，UE 明确记录为不惩罚 unresponsive test，随后原生完成；没有外层 timeout 或人工中止替代结果。

- automation audit：PASS，3 logs / 1121 success / 0 fail-or-not-run，SHA-256 DED1EE5A17CDA4C1F6901365D35467A0AE80703D47B71244ACC616BDD5A679E1；
- regression self-test：373/373，SHA-256 5B007DFB655FF35AC8A644555BCA03E2A362250611F3BDA49E52A5E1ED1F4F77；
- boundary scan：PASS，files 2、lines 650、forbidden 0、includes 3、coordinator entrypoints 1、reentrancy guards 1、mutable ledger access 0、map rule 1/25 groups，SHA-256 AE1636BA1B4284F7CF401E7E79DCEDE3587352EA889FA789B01B4BD83B087A1F；
- changed-file gate：PASS Changed=5 Rules=2 Required=25 Logs=3，SHA-256 01D0E917F817569B02F6C8F23A94BED4E3EC893815D2215D66FCE5BC983CC30F；
- git diff check：PASS，7 个本轮文件、0 个空白错误，SHA-256 `13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6`；
- staged diff check：PASS，7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 `B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD`；
- candidate Editor：target up to date / 0 actions / native 0 / 1.03 秒，SHA-256 170B6F0A2A6CC2826D9BE31022C4BA07B4E78007E56F75E0EF113DE02369A10B；
- candidate focused Session：7/0，SHA-256 F1CEDC6EE214BAB105514EB46D49CEE49532938F5B221C8DB79700AC23563EFD；
- final Editor：target up to date / 0 actions / native 0 / 1.02 秒，SHA-256 DC0A8C580B4EC410C4FBF1CB56ED6A8D1A935CD46605720EC7E792281B5077A3；
- final Game：4 actions / native 0 / 31.47 秒，SHA-256 58D819616DC004F868B45F7CA177F45C840497B3A972F88568839A9E10840621。

首次候选 Editor 日志目标目录尚未创建，日志包装器未留存该次输出；创建证据目录后的 Editor 构建确认目标已是最新，专属 Editor 自动化加载新模块并通过 7/0。最终 Game 构建日志明确记录新 Session 与测试源的 compile/link 4 actions，避免把未捕获输出当成编译证据。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16949760 bytes，SHA-256 C33A9ADC446CAE5D596BBB40387BF175D220CE5B57F998AE756EC9A53C409312；
- `Binaries/Win64/demo_map.exe`：358116352 bytes，SHA-256 65992B8AC4EA5C941EA172ECF5FE0A853E1C1A628847E8321B59731A1526BDF7。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：Run/consumer Session ownership、private ledger begin/end、exact begin replay、scope isolation、single coordinator delegation、candidate-ledger atomic commit、typed result、application/rejection replay、reentrancy guard、Visible end fence、Hidden/Empty safe end，以及完整 0.0.10 与 changed-file 回归。

未验证：真实 HUD/renderer/port adapter、widget/component/visible frame、跨线程调用、进程崩溃后的持久 exactly-once、被拒 Hide 的外部 Applied recovery API、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。fake-port 通过不能描述为可见产品验收。

建议 P20.40 建立窄的 externally-attested recovery seam：只允许 Session 为已存在 Rejected receipt 的 exact CommandId 接收后续 Applied receipt，继续保持 ledger 私有，并验证 recovered Hide 后才可 graceful end；仍不连接真实 HUD/World。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-39-thrown-weapon-arc-preview-delivery-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-39-thrown-weapon-arc-preview-delivery-session/Docs/Report/Dev.D.UE.0.0.10.P20.39.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-39-thrown-weapon-arc-preview-delivery-session/Docs/Log/Dev.D.UE.0.0.10.P20.39.r0_log.md>
