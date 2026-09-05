# Dev.D.UE.0.0.10.P20.42.r0 Report

## 1. 结论

P20.42 已建立 renderer-facing 的 thrown-weapon Arc preview presentation consumer adapter contract。Adapter 实现既有 delivery port，把 `Show`、`Replace`、`Hide` 映射为单一 consumer surface 上严格对应的一次 typed mutation，把 `NoOp` 映射为零 surface call，并以 Applied/Rejected surface response 与物理 surface cursor 形成自验证证据。

物理 surface 只允许 Empty 或 Visible：delivery ledger 中的 Hidden audit state 在 renderer 边界统一归一化为空。由此，Show 固定为 Empty -> Visible，Replace 固定为 Visible -> different Visible，Hide 固定为 Visible -> Empty；Rejected 必须保持 cursor 不变。

本阶段仍是 P 阶段无头契约：所有行为由 fake surface 验证，没有连接真实 MainHUD、widget、component、World 或可见帧。

## 2. 基线、分支与改动范围

- 基线：`16912bb62683bcea3bd72425c09d1a0a37d44d0e`（P20.41）；
- 分支：`agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未修改 P20.41 Host、delivery Session、command、ledger 或 presentation state 的 public contract；
- 未连接真实 HUD、World、Actor、widget、component、输入、projectile、库存或产品 mutation。

## 3. Surface response contract

`Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse` 是一次 mutating command 的 immutable evidence，绑定：

- deterministic ResponseId；
- CommandId 与 CommandKind；
- Applied/Rejected outcome 与稳定 outcome code；
- previous/after physical surface cursor。

`TryCreate` 与 `IsValid()` 同时验证 command-kind transition、outcome 语义和 cursor 守恒。Applied 必须完成该 command 唯一允许的状态转换；Rejected 必须保持 before/after 完全一致；NoOp 不能伪造 surface response。

Response 是 consumer surface 的 attestation，不是密码学签名，也不证明真实屏幕已经显示某一帧。

## 4. Narrow consumer surface

`Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface` 只公开：

1. consumer definition identity；
2. 当前物理 surface cursor；
3. 分离的 `Show`、`Replace`、`Hide` 操作。

三种 mutation 使用独立方法，Adapter 不能把一种 command kind 静默解释成另一种。接口故意没有 `NoOp`：NoOp 只验证 cursor 并生成 port acknowledgement，不触发 renderer mutation。

surface 不拥有 delivery ledger、retry loop 或产品 authority。

## 5. Run scope 与 dispatch 规则

`TryBegin(RunId, Surface)` 要求有效 Run、稳定 consumer identity 与物理 Empty surface。exact scope replay 幂等成功；active Adapter 上更换 Run 或 surface 被拒绝。

`Apply(Command)` 固定执行：

1. 检查 active、valid、非重入、Run 与 consumer scope；
2. 检查 command previous cursor 与当前物理 cursor 一致；
3. NoOp 直接生成 Applied port response，surface call count 为 0；
4. Show/Replace/Hide 只调用一次对应的 typed surface method；
5. 读取并验证 response、实际 surface cursor 与 command expectation；
6. 将有效 Applied 或 Rejected response 映射为既有 delivery port response；
7. 保存 self-validating Adapter result。

`TryEnd(ExpectedRunId)` 要求 exact Run、无 operation、物理 surface Empty；Visible surface 不能被静默遗弃。

## 6. Fail-closed 与重入边界

- inactive、invalid、Run mismatch、consumer drift 与 cursor drift 在 surface call 前拒绝；
- valid Rejected response 映射为 Rejected port response，cursor 不推进；
- foreign/invalid response、Applied-without-mutation 与 Rejected-with-mutation 均 fail-closed；
- operation guard 阻止 surface callback 内重入 `Apply` 或 `TryEnd`；
- Adapter result 记录 status、command、Run、consumer、surface call count、surface response、port response 与 before/after cursor。

如果不合规 surface 已在外部产生副作用后才返回无效证据，Adapter 能拒绝并在后续 cursor mismatch 时阻断，但不能回滚该外部副作用；当前阶段没有补偿事务。

## 7. 与 delivery Session 的组合

fake surface 测试通过真实 Adapter port 接入既有 delivery Session，覆盖 Show -> Replace -> NoOp -> Hide：

- surface mutation 顺序严格为 Show、Replace、Hide；
- NoOp 仍经过 port，但 surface call 为 0；
- Hide 后 delivery audit cursor 为 Hidden，物理 surface cursor 为 Empty；
- valid surface rejection 被 delivery ledger 封存；
- exact outer delivery replay 不重复调用 surface。

P20.41 Host 与本 Adapter 目前仍由上层分别管理生命周期；本阶段没有创建共同 owner，也没有自动协调 Host recovery 与物理 surface recovery。

## 8. 自动化覆盖

新增 6 项 focused automation：

1. `SurfaceResponseContract`：deterministic response、合法 Applied/Rejected 与非法 outcome/code；
2. `LifecycleAndPreflight`：begin replay、scope rotation、foreign Run、consumer/cursor drift 与 end fence；
3. `ShowReplaceNoOpHide`：完整 delivery Session 集成与零调用 NoOp；
4. `SurfaceRejectionReplay`：Rejected sealing 与 exact replay 不重复 surface effect；
5. `SurfaceInvariantFailures`：invalid response、Applied-without-mutation、Rejected-with-mutation fail-closed；
6. `ReentrantSurfaceBlocked`：callback 内 Apply/end 重入封锁。

完整 `Shanmen.0_0_10` 从 P20.41 的 1078 增至 1084 项。

## 9. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationConsumerAdapter` exact-path mapping rule。5 个改动路径命中 2 条规则，求并集要求 27 个测试组；3 份正式日志全部满足映射。

- mapping self-test：377/377；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=27 / Logs=3；
- boundary scan：PASS，Adapter 2 files / 1076 lines；renderer/World/Actor/UObject/widget/component/RNG/ApplyDamage/projectile/inventory mutation identifiers 0；Show dispatch 1 / Replace dispatch 1 / Hide dispatch 1；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| consumer_adapter_final.log | Product.ThrownWeaponArcPreviewPresentationConsumerAdapter | 6/0 | CA7EEF451B5919E218C75A92D3E49BABE29BEEB297A9198F1403541EA8517374 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 6067EECBDF368CB35108FF046E1AF41EF90450554D8E8F69C2140F3CC2C91256 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1084/0 | 6FFBCA7D727B5490FDB1D5DD69E53B12A225D055ABEE1DB1213BDCCC51E75194 |

正式日志累计 1136/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1084/0。完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 区段出现非惩罚性 unresponsive 通知，随后保持 CPU 活动、恢复逐项进展并由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束；没有外层 timeout、拆组或人工中止替代结果。

构建：

- final Editor：4 actions / native 0 / 34.38 秒；
- final Game：4 actions / native 0 / 31.49 秒；
- Editor DLL：17120256 bytes，SHA-256 `728A4578584769F9AB12B92C3214B83A2CC5EA4942141021B6D9D94437B0D68C`；
- Game EXE：358251520 bytes，SHA-256 `83A8099EAB8A17CDACCE8C4361E60AD09B18E0F450FFF6D4B4F9673466F2A96C`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：deterministic surface response、Run/consumer scope、physical cursor normalization、typed Show/Replace/Hide dispatch、zero-call NoOp、Applied/Rejected mapping、preflight fences、delivery Session replay、invalid surface behavior fail-closed、typed result、callback reentrancy 与 changed-file regression。

未验证：真实 MainHUD/widget/component/renderer/visible frame、surface attestation 来源真实性、外部副作用补偿、跨线程同步、进程崩溃持久 exactly-once、Host/Adapter 共同生命周期或 recovery reconciliation、真实输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

建议 P20.43 建立 Run-scoped composition owner：把 P20.41 delivery Host 与 P20.42 consumer Adapter 绑定到同一 Run/consumer/surface，冻结 begin/update/end 顺序并明确 Host rejection recovery 与 physical surface reconciliation；继续使用 fake surface，真实 MainHUD/widget 绑定后置。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.42.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.42.r0_log.md>
