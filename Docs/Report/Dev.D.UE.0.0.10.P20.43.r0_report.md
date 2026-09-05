# Dev.D.UE.0.0.10.P20.43.r0 Report

## 1. 结论

P20.43 已建立 Run-scoped thrown-weapon Arc preview presentation composition owner。Owner 共同持有 P20.41 delivery Host 与 P20.42 consumer Adapter，并把两者原子绑定到同一 Run、consumer definition 与 renderer-facing surface，冻结 begin、update、rejection recovery、end 的唯一顺序。

正常 update 只调用一次 Host；Host 只通过 owned Adapter 投递。Rejected command 保留在 Host pending slot，显式 recovery 每次最多重试一次 Adapter，并只在得到 Applied receipt 后调用一次 Host recovery。整个 Owner 不包含循环重试、静默状态推进或第二套 presentation authority。

本阶段仍是 P 阶段无头契约：使用 fake surface 验证组合生命周期与恢复语义，没有连接真实 MainHUD、widget、component、World 或可见帧。

## 2. 基线、分支与改动范围

- 基线：`53a7f2b21659fcfef485b775df06883509f64df8`（P20.42）；
- 分支：`agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未改变 Host、Adapter、delivery Session、command、ledger 或 presentation state 的 public contract；
- 未连接真实 HUD、World、Actor、widget、component、输入、projectile、库存或产品 mutation。

## 3. 原子组合生命周期

`TryBegin(RunId, Surface)` 先在 candidate Owner 上完成 Adapter begin 与 Host begin，只有两者 Run、consumer 与归一化 cursor 全部一致时才提交 candidate。由此，半完成 begin 不会泄漏到 active Owner。

exact Run/surface begin replay 幂等成功；active scope 上更换 Run 或 surface 被拒绝。Visible physical surface 不能直接绑定，因为 Owner 没有足够证据判断其来源或所属 Run。

`TryEnd(ExpectedRunId)` 同样在 candidate copy 上依次执行 Host end 与 Adapter end，只有两者均回到 empty 才提交。foreign Run、pending rejection、Visible surface、operation in progress 或失同步状态都不能被静默 teardown。

surface pointer 是 non-owning；调用方必须保证它覆盖整个 active Owner scope。

## 4. 单路径 update

`TryUpdate(...)` 固定执行：

1. 验证 Owner active、valid、非重入；
2. 记录 desired state、Host cursor、physical cursor 与 pending command 的 before evidence；
3. 调用 Host `TryUpdate` 恰好一次；
4. 从 Host 返回的 port response 推导 Adapter 是否被调用，预算只能为 0 或 1；
5. 交叉验证 Host result、Adapter result、Run、consumer、command 与所有 before/after cursor；
6. 分类为 HostRejected、RejectedPendingRecovery、Applied、ApplicationReplayed 或 InvariantViolation；
7. 生成 immutable、self-validating Owner result。

Owner 不绕过 Host 直接修改 ledger，也不绕过 Adapter 直接调用 surface。exact outer replay 保持在 Host/ledger 上方，不重复 physical effect。

## 5. 有界 rejection recovery

`TryRecoverRejected()` 只读取 Host 当前 exact pending command：

- 无 pending 时返回 `NoRecoveryPending`，Adapter 与 Host recovery 调用数均为 0；
- 有 pending 时调用 Adapter 恰好一次；
- Adapter 再次 Rejected 时保留同一 pending，Host recovery 调用数为 0；
- Adapter Applied 时封装 exact Applied receipt，并调用 Host recovery 恰好一次；
- Host 接受后清除 pending 并推进 ledger cursor；
- 任一证据不一致均进入 `InvariantViolation`，不循环、不猜测、不合成成功。

newer update 在 pending recovery 期间被 Host fence，不能越过未解决的 command。

## 6. Cursor normalization 与同步不变量

Host 保存 delivery audit cursor，physical surface 只保存 Empty 或 Visible。Host 的 Hidden audit state 在 Owner 边界归一化为 physical Empty；除此之外，Host ledger cursor 必须与 Adapter surface cursor精确一致。

`IsValid()` 要求：

- Host 与 Adapter 同时 empty，或同时 active；
- active 时 Run、consumer definition 完全相同；
- normalized Host cursor 等于 physical surface cursor；
- 两个 child contract 各自有效。

`IsSynchronized()` 进一步要求 Host desired state、ledger cursor 与 pending 状态同步。合法 pending rejection 时 Owner 仍可有效，但明确不是 synchronized，直到有界 recovery 完成。

## 7. Fail-closed 与重入边界

- inactive、invalid、consumer drift 与 cursor drift 在 child call 前拒绝；
- operation guard 阻止 surface callback 内重入 Owner update、recovery 或 end；
- result 保存 Host/Adapter call count、嵌套 evidence、desired state、Host cursor、surface cursor、pending command 与 owner-valid-after；
- result 字段私有且构造后不可变，`Validate()` 结果缓存，避免重复递归验证同一深层 Host evidence；
- runtime 在创建 result 前仍完整验证一次 Host 与 Adapter 的真实返回证据。

如果不合规 surface 先产生 mutation 再返回 Rejected，Host 仍保留旧 cursor，而 physical cursor 已推进。Owner 会检测该分歧并变为 invalid，后续自动 recovery 被阻断；本阶段没有跨 renderer 的补偿或 rollback transaction。

## 8. 自动化覆盖

新增 5 项 focused automation：

1. `LifecycleAndPreflight`：default empty、Visible bind fence、exact begin replay、scope drift 与 atomic end；
2. `OrderedAppliedReplay`：Show -> Replace -> NoOp -> replay -> Hide，验证唯一 dispatch 顺序与零 mutation NoOp；
3. `RejectionRecovery`：pending fence、Rejected retry、Applied retry、exact Host recovery 与 no-pending zero-call；
4. `DivergenceDetected`：mutated-then-rejected 导致 invariant failure，并阻断后续恢复；
5. `ReentrantSurfaceBlocked`：surface callback 内 update/recovery/end 全部被 operation guard 阻断。

完整 `Shanmen.0_0_10` 从 P20.42 的 1084 增至 1089 项。

## 9. 修正记录

首次 candidate focused run 在 `OrderedAppliedReplay` 中出现明显高 CPU 长耗时。根因不是死锁或产品循环，而是外层 result `IsValid()` 多次递归重验同一 Host result，Host result 又重验完整嵌套 evidence，形成高成本重复遍历。

修正为：result 字段保持 private immutable；构造时完成一次验证并缓存 `bValidated`；Owner runtime 在构造外层 result 前仍对 Host/Adapter 返回做完整验证，外层 result 之后只验证已冻结的关联形状与状态分类。未完成 candidate 日志被保留为失败前证据，没有冒充正式结果。修正后 focused 5/0，完整套件 1089/0。

## 10. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationCompositionOwner` exact-path mapping rule。5 个改动路径命中 2 条规则，求并集要求 28 个测试组；3 份正式日志全部满足映射。

- mapping self-test：379/379；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=28 / Logs=3；
- boundary scan：PASS，Owner 2 files / 1183 lines；World/Actor/RNG/ApplyDamage identifiers 0；Host update 1 / Adapter apply 1 / Host recovery 1 / Host end 1 / Adapter end 1；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| composition_owner_final.log | Product.ThrownWeaponArcPreviewPresentationCompositionOwner | 5/0 | 067CC0E3981D3273F6371FD2A5C71E480D3951B4493F5A248ADCC0D21EA81581 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | DD0456A0702979E5B7FC198D4C5948881B7F7D995A38550C1F0D97EF6043851A |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1089/0 | F88FE76ADEE7462654048C6E4607314DE8A66448D364A44053F8B7C6D37F63D7 |

正式日志累计 1140/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1089/0。完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 慢区段出现非惩罚性 unresponsive 通知，进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束；没有外层 timeout、拆组或人工中止替代正式结果。

构建：

- candidate Editor：4 actions / native 0 / 8.52 秒；
- final Editor：up to date / 0 actions / native 0 / 2.80 秒；
- final Game：4 actions / native 0 / 33.16 秒；
- Editor DLL：17179648 bytes，SHA-256 `7589147548605108C846A9269A290B966E9659E8ABB3549A0ABB5793554773C8`；
- Game EXE：358302720 bytes，SHA-256 `7084116AD413A4966AB48646B38B22442080E6DAF6AB6612991087A80C5BF4B6`。

## 11. P/F 边界、下一步与 GitHub

PASS 范围：Host/Adapter atomic scope、single-path update、bounded rejection recovery、exact pending command、Applied receipt forwarding、zero-call no-pending、newer-update fence、audit/physical cursor normalization、outer replay、typed evidence、divergence detection、callback reentrancy 与 changed-file regression。

未验证：真实 MainHUD/widget/component/renderer/visible frame、surface pointer lifetime enforcement、renderer recreation/adoption、外部副作用补偿、跨线程同步、进程崩溃持久 exactly-once、真实输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

建议 P20.44 建立 renderer surface recreation/adoption policy：对 startup 时 Empty、same-Run Visible、foreign Visible 做显式分类，只允许有证据的 exact-state adoption 或显式 Hide cleanup；继续使用 fake surface，禁止静默 pointer rebind，真实 MainHUD 绑定后置。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner/Docs/Report/Dev.D.UE.0.0.10.P20.43.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner/Docs/Log/Dev.D.UE.0.0.10.P20.43.r0_log.md>
