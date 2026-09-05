# Dev.D.UE.0.0.10.P20.32.r0 Report

## 1. 结论

P20.32 已把 P20.31 的纯值 Arc preview capture 接到真实 thrown-weapon 产品会话与 combat Run coordinator 的只读边界。桥从当前 lifecycle 读取精确 hotbar item 与冻结 session config，校验 lifecycle/coordinator 指向同一 ready Run，并在 P20.31 捕获前后各读取一次下一真实 activation sequence。

成功结果要求两次序号读数相同、库存权威快照不变、product Host 仍为空且没有捕获真实 selection。由此证明一次 live Arc preview 可以使用真实产品配置和 prospective sequence 构造，但不会预留、递增、提交或消耗真实 action/inventory 状态。

新增 7 项自动化，完整 0.0.10 由 1006 增至 1013/0。按 changed-file 映射执行 7 份最终自动化证据，合计 1092/0；Editor 与 Game Development 构建均成功。

本轮没有连接 UI/HUD、真实输入、World trace、collision、renderer、projectile、launch、inventory reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：9af9905a0a9077e52cdca2b24dab40db0bc174b9（P20.31）；
- 分支：agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 产品会话只读边界

Fdemo_mapShanmenThrownWeaponProductSession 新增 TryCaptureReadOnlyHotbarBinding()：

1. 先清空复用输出；
2. 只接受 active、self-validating session 与 [1, HotbarSlotCount] 槽位；
3. 从冻结 Run correlation 复制该槽位的 item instance ID；
4. 按值复制 session config，并要求 item/config 均有效；
5. 不接收 item authority、Actor 或 coordinator，也不改变 session/Host。

Fdemo_mapShanmenThrownWeaponProductLifecycle 只做有效性检查和委托，不复制另一套产品真值。桥因此只能读取 lifecycle 已拥有的 canonical binding，不能自行查询或改写库存。

## 4. Live Arc preview 产品桥

Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture() 接收 hotbar slot、当前不可变 choice state、choice policy、segment count，以及 const lifecycle/coordinator。

固定顺序为：

1. 校验 slot、policy、segment 与 BallisticArc choice；
2. 读取 canonical ProductContentStamp；
3. 从 lifecycle 捕获精确 item/config；
4. 校验 lifecycle Run 与 ready coordinator Run 相同并读取 player identity；
5. 读取 sequence-before；
6. 建立确定性 preview request identity，并调用既有 P20.31 pure capture；
7. 读取 sequence-after；
8. 只有两次读数相同且 nested capture 有效时返回 Captured。

桥没有 mutable item authority 参数。生产边界扫描确认 forbidden reservation/submit/recovery/launch/delivery/World/random 调用为 0，coordinator sequence getter 恰好出现两次。

## 5. 确定性身份与自验证

PreviewRequestId 使用独立命名空间 demo_map.ShanmenThrownWeapon.ArcPreviewProductRequest.r1，并由以下 canonical inputs 派生：

- hotbar slot；
- choice state ID、last command、revision、target intent 与 apex adjustment；
- choice policy ID 与 segment count；
- authority content version/digest；
- Run、player、source item 与 observed next sequence；
- definition action/detector/formula、damage 参数、launch speed、自目标规则与规范排序标签；
- Arc technique tier、gravity、maximum flight time 与 session source tags。

Captured result 会重算 PreviewRequestId，要求 lifecycle/coordinator Run 一致、sequence read count 为 2、before/after 相等，并逐层验证 P20.31 request/capture。相同 live 读数可逐值重放；choice revision 改变只进入 preview-domain identity，不改变同一 prospective real activation identity。

## 6. Typed 状态

桥区分：Invalid、InputRejected、ChoiceRejected、ProductUnavailable、RunUnavailable、SequenceUnavailable、RequestRejected、CaptureRejected、SequenceChanged 与 Captured。

无效 slot/policy/segment、非 Arc choice、空 hotbar、inactive lifecycle、Run 不一致、耗尽序号、request/capture 拒绝或捕获期间序号变化均失败关闭。拒绝结果不会伪装成可呈现 preview。

## 7. 自动化覆盖与修正记录

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge 新增 7 项：

1. CanonicalReadOnlyCapture：真实 lifecycle/coordinator 进入自验证 preview，前后库存、序号与 Host 不变；
2. DeterministicReplay：相同 live 读数逐值重放且不产生 selection；
3. ChoiceRevisionIsolation：choice revision 只改变 preview-domain identity；
4. InputAndChoiceFences：非法 slot 与非 Arc choice 在产品读取前拒绝；
5. ProductFences：空 hotbar 与 inactive lifecycle 失败关闭；
6. RunFence：inactive/foreign coordinator 不借用产品 Run；
7. CompositionCompatibility：live bridge 输出直接进入 P20.30 pure geometry composition。

首轮聚焦测试为 6/1。唯一失败是 CompositionCompatibility 的测试 choice 超出 canonical TrainingThrowingKnife 在既有物理参数下的可达范围；生产桥已正确捕获，失败发生在后续几何组合断言。只把 fixture 调整为可达目标/最低 apex，未修改生产代码；第二轮与正式最终轮均为 7/0。首次失败日志保留，SHA-256 065545FA3FC53606385D34D9AB9D28A1AC082E1DDB158D24992A5F575D58CC5E。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewProductBridge 规则，覆盖 bridge、P20.31 capture、P20.30 composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 18 个 required groups。

正反 mapping self-test 由 357 增至 359/359。最终 gate 对 9 个实现、测试与流程路径求并集：Changed=9 Rules=3 Required=18 Logs=7，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_product_bridge_final.log | Product.ThrownWeaponArcPreviewProductBridge | 7/0 | 35D3E7C01D201C4A5305128A4F8B6391E7809B8D77D2BAF21353A1451E7D052B |
| thrown_product_lifecycle_final.log | Product.ThrownWeaponProductLifecycle | 5/0 | E5965AA6DB56DCA30248C4FE806CB5D4CE7312989BB8DE31F8A2A4820393016D |
| thrown_product_session_final.log | Product.ThrownWeaponProductSession | 6/0 | 66F40F9813C8B48E71188229817AAB7A96D6B173DBC060FCF0F9AC27A31CA888 |
| arc_preview_capture_final.log | Product.ThrownWeaponArcPreviewCapture | 8/0 | 08F91C449502D32939A9117E3BF952A0D989FAD158E59068790F76B32B4E9280 |
| arc_preview_composition_final.log | Product.ThrownWeaponArcPreviewComposition | 7/0 | 94E7556F39F32A79FF6E887040A6AB9EBA6C61255C43894B156AF8179E0B148E |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 7B436E3D45C3DD90FAE09368A2045B4BBAA465DCE16766C7C803426E47F13F95 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1013/0 | 6F701E16120AAA0D9DB5F48F26B364ECF08B47E5E189169837D0D37E2D88095A |

最终自动化证据合计 1092/0。每份日志均有唯一 RunTests、精确成功数、零失败、唯一 UE 5.8 原生成功终止标记与零 fatal/unhandled/ensure/no-match。

- automation audit：PASS，SHA-256 0D950F8803EDA219929ECAB6D9D4AEB0DEE5934F824CBB36CD6A4336B3CF25B8；
- regression self-test：359/359，SHA-256 D4F78BFEB4CAD0C9F07181614C9B9AD4CFF740C914945D6E6596E9937BD6E994；
- boundary scan：PASS，forbidden mutable call 0、sequence read 2、mutable authority parameter 0、placeholder 0，SHA-256 0FCFE06C5F95B44FDD0CA90A40C7D64DE54E3452AE642A8BBCF786901C06504D；
- changed-file gate：PASS Changed=9 Rules=3 Required=18 Logs=7，SHA-256 E1F103AEC0304353CC86D9F62EE71884E567F2DFC6037C0D0FEF481663F79BFB；
- staged diff check：PASS，11 个精确暂存文件、0 个空白错误，SHA-256 A7AFA40990ACDB1F50B0F50E3006B86D9945BF9021490F1CA6E748774CB3B5FE；
- final Editor：up to date / native 0 / 3.65 秒，SHA-256 E7B86EA924BAE5CC12CA55038C835B566EA01ED423E8A1A5042D2DAAA3F92EC1；
- final Game：56 actions / native 0 / 78.19 秒，SHA-256 9275225A7617829CF9EE15846D476BB6F53BE1F80E7B97BF4271AAD4AD8D1139。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,581,120 bytes，SHA-256 130AE81117B6CEA310B97CA33FA0648AD1C573C93E121895DFFCF83BBB5B293B；
- Binaries/Win64/demo_map.exe：357,812,224 bytes，SHA-256 5821CDBA6AFCF3FE498BA95962A62AD14D8FCC2225B58D713A501739C365AA79。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：active product session 的精确 hotbar/config 只读捕获、canonical content stamp、lifecycle/coordinator Run 对齐、prospective sequence 前后双读、deterministic product request identity、typed rejection、自验证、确定性重放、库存/Host/selection 不变，以及完整 0.0.10 与 changed-file 回归。

未验证：UI/HUD presentation、从真实设备驱动 choice、World trace/collision/occlusion、真实地形落点、renderer 可见轨迹、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.33 建立 renderer-agnostic Arc preview presentation snapshot：只消费 P20.32 Captured result，生成可清除、可替换、revision-aware 的线段/状态数据；先不连接真实 HUD 或 World renderer。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge/Docs/Report/Dev.D.UE.0.0.10.P20.32.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge/Docs/Log/Dev.D.UE.0.0.10.P20.32.r0_log.md>
