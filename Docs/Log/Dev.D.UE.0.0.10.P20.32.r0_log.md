# Dev.D.UE.0.0.10.P20.32.r0 Log

## 阶段

- 任务：P20.32 thrown-weapon Arc preview live product read-only bridge；
- 基线：9af9905a0a9077e52cdca2b24dab40db0bc174b9；
- 分支：agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. ProductSession 新增 TryCaptureReadOnlyHotbarBinding()，从冻结 correlation 复制精确 slot item 与 session config。
2. accessor 先清空复用输出，只接受 active/self-validating session、合法 slot、有效 item/config。
3. ProductLifecycle 只做有效性检查并委托 session，不建立第二份产品权威。
4. 新增 ArcPreviewProductBridge；输入只有值类型以及 const lifecycle/coordinator。
5. 桥内部读取 canonical ProductContentStamp，不接收可注入 content stamp。
6. lifecycle Run、coordinator Run 与 player identity 必须形成同一 ready Run。
7. coordinator next activation sequence 在 P20.31 pure capture 前后各读取一次；两次不同返回 SequenceChanged。
8. ProductRequestId 使用独立 r1 命名空间和完整 choice/content/Run/item/sequence/config canonical inputs。
9. Captured result 重算 request ID，并验证 nested P20.31 request/capture 绑定。
10. typed status 区分 input、choice、product、Run、sequence、request、capture、sequence race 与成功。
11. 桥不持有 mutable item authority，不执行 reserve/submit/recovery/launch/delivery/World/random 调用。
12. 新增 7 项自动化；完整 0.0.10 总数 1006→1013。
13. changed-file map 新增 bridge 规则与正/反 fixture；self-test 357→359。
14. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

首轮聚焦自动化：6/1，UE TEST COMPLETE exit -1（native 255）。CompositionCompatibility 的 choice fixture 超出 canonical TrainingThrowingKnife 的既有可达 Arc；桥本身已经 Captured，失败位于后续 geometry assertion。

只把 fixture 改为可达目标与最低 apex，未改生产代码。第二轮聚焦自动化 7/0，正式最终轮再次 7/0。

- first failure log：SHA-256 065545FA3FC53606385D34D9AB9D28A1AC082E1DDB158D24992A5F575D58CC5E；
- corrected candidate log：SHA-256 ABE2E207BF81A68E951F0ECDEA091A468E5BA2BBBA4C036B1364A329851DF7AA。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_product_bridge_final.log | 7/0 | 35D3E7C01D201C4A5305128A4F8B6391E7809B8D77D2BAF21353A1451E7D052B |
| thrown_product_lifecycle_final.log | 5/0 | E5965AA6DB56DCA30248C4FE806CB5D4CE7312989BB8DE31F8A2A4820393016D |
| thrown_product_session_final.log | 6/0 | 66F40F9813C8B48E71188229817AAB7A96D6B173DBC060FCF0F9AC27A31CA888 |
| arc_preview_capture_final.log | 8/0 | 08F91C449502D32939A9117E3BF952A0D989FAD158E59068790F76B32B4E9280 |
| arc_preview_composition_final.log | 7/0 | 94E7556F39F32A79FF6E887040A6AB9EBA6C61255C43894B156AF8179E0B148E |
| item_use_and_armor_final.log | 46/0 | 7B436E3D45C3DD90FAE09368A2045B4BBAA465DCE16766C7C803426E47F13F95 |
| full_0_0_10_final.log | 1013/0 | 6F701E16120AAA0D9DB5F48F26B364ECF08B47E5E189169837D0D37E2D88095A |

最终证据合计 1092/0。每份日志均有唯一 RunTests、零失败、唯一成功终止标记与零 fatal/unhandled/ensure/no-match。

## 流程与静态证据

- automation audit：PASS；SHA-256 0D950F8803EDA219929ECAB6D9D4AEB0DEE5934F824CBB36CD6A4336B3CF25B8；
- regression self-test：359/359；SHA-256 D4F78BFEB4CAD0C9F07181614C9B9AD4CFF740C914945D6E6596E9937BD6E994；
- boundary scan：PASS；forbidden mutable call 0、sequence read 2、mutable authority parameter 0、placeholder 0；SHA-256 0FCFE06C5F95B44FDD0CA90A40C7D64DE54E3452AE642A8BBCF786901C06504D；
- changed-file gate：PASS Changed=9 Rules=3 Required=18 Logs=7；SHA-256 E1F103AEC0304353CC86D9F62EE71884E567F2DFC6037C0D0FEF481663F79BFB；
- staged diff check：PASS；11 个精确暂存文件、0 个空白错误；SHA-256 A7AFA40990ACDB1F50B0F50E3006B86D9945BF9021490F1CA6E748774CB3B5FE。

## 构建与产物

- candidate Editor：57 actions / native 0 / 38.55 秒；
- fixture correction Editor：4 actions / native 0 / 5.94 秒；
- final Editor：up to date / native 0 / 3.65 秒；SHA-256 E7B86EA924BAE5CC12CA55038C835B566EA01ED423E8A1A5042D2DAAA3F92EC1；
- final Game：56 actions / native 0 / 78.19 秒；SHA-256 9275225A7617829CF9EE15846D476BB6F53BE1F80E7B97BF4271AAD4AD8D1139；
- Editor artifact：16,581,120 bytes；SHA-256 130AE81117B6CEA310B97CA33FA0648AD1C573C93E121895DFFCF83BBB5B293B；
- Game artifact：357,812,224 bytes；SHA-256 5821CDBA6AFCF3FE498BA95962A62AD14D8FCC2225B58D713A501739C365AA79。

## P/F

PASS：live product hotbar/config 只读捕获、Run 对齐、prospective sequence 双读、product request identity、typed rejection、自验证、deterministic replay、库存/Host/selection 不变，以及完整 changed-file 回归。

未验证：UI/HUD、真实设备 choice、World trace/collision/renderer、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.33：建立 renderer-agnostic Arc preview presentation snapshot，只消费 P20.32 Captured result 并支持 revision-aware replace/clear；先不接真实 HUD 或 World renderer。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge/Docs/Report/Dev.D.UE.0.0.10.P20.32.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-32-thrown-weapon-arc-preview-product-bridge/Docs/Log/Dev.D.UE.0.0.10.P20.32.r0_log.md>
