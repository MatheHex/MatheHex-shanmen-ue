# Dev.D.UE.0.0.10.P20.31.r0 Report

## 1. 结论

P20.31 已在既有 thrown-weapon 产品会话与 P20.30 Arc preview configuration 之间建立纯值、只读的预览捕获策略。调用方显式提供当前观察到的下一真实 activation sequence；策略只把该值冻结为证据，不持有其引用，也不执行预留、递增、提交或消耗。

预览 action 使用独立命名空间 demo_map.ShanmenThrownWeapon.ArcPreviewAction.r1 派生确定性 PreviewActivationId。策略同时用既有 FShanmenCombatIdFactory::MakeActivationId() 计算 ProspectiveRealActivationId 作为对照证据，但不把它登记到 coordinator 或 ledger。两种身份必须有效且互不相等。

成功结果携带不可变 request、两种身份、preview-only action 与 P20.30 configuration，并能从 request 完整重建和自验证。无效请求、action 建立失败与 configuration 建立失败分别返回 typed status，不伪装为可用预览。

新增聚焦自动化 8/0，完整 0.0.10 由 998 增至 1006/0。按改动文件映射运行 10 份最终自动化证据，合计 1084/0；Editor 与 Game Development 构建均成功。

本轮没有连接真实输入、UI、World、Actor、trace、collision、renderer、projectile、inventory 或 run coordinator，也没有推进真实 action sequence。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：53ce13e7a0c7e1b9ce929ec18e8631a4ae36a69e（P20.30）；
- 分支：agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 只读捕获请求

Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture() 冻结：

1. preview request、Run、player 与 source item 身份；
2. 当前 authority content stamp；
3. 调用方观察到的下一真实 activation sequence；
4. 既有 Fdemo_mapShanmenThrownWeaponSessionConfig；
5. 既有 Arc choice policy；
6. preview segment count。

请求要求全部身份和内容戳有效、sequence 位于 [1, MAX_uint64-1]、会话为 BallisticArc、定义为标准 Arc action、choice policy 有效，且 segment count 保持在既有 sampler 的 [2, 128] 范围内。复用输出在失败时清空。

该结构没有 sequence 引用、coordinator 指针或 inventory handle；Capture() 的签名无法修改调用方序号。

## 4. 身份分域

PreviewActivationId 由以下冻结输入规范派生：

- preview request、Run、player 与 source item GUID；
- observed sequence 与 authority content；
- thrown-weapon definition 的 action、detector、formula、damage 参数、launch speed、自目标规则与规范排序标签；
- session source tags、Arc technique tier、gravity、maximum flight time；
- choice policy ID 与 segment count。

ProspectiveRealActivationId 继续由既有标准 factory 以 Run、player、Arc action definition 与 observed sequence 派生。它只回答“如果此刻真实激活，标准身份会是什么”，不代表已建立真实 action。

Result::IsValid() 会重算两种身份，并拒绝二者相同或任一不匹配的结果。preview request revision 会改变预览身份和 configuration identity，但不会改变同一 observed sequence 对应的 prospective real identity。

## 5. Preview action 与 P20.30 组合

策略从 request 建立 preview-only FShanmenCombatActionSnapshot：

- Run、owner、source entity、source item 与 authority content 原样冻结；
- action definition 固定为既有 Arc definition；
- activation ID 使用 preview namespace；
- session source tags 保留，并补入既有 Source.Player 标签。

P20.30 configuration 增加 geometry-only TryCapture() 重载，接受已验证的 immutable thrown-weapon definition 与 Arc product policy。原有 product-capture 重载仍先验证 product，再委托新重载，因此既有调用行为不变。

P20.31 从 session definition capture 重建 immutable definition，再将 preview action、session Arc policy、choice policy 与 segment count 交给 P20.30。没有复制 choice 投影、ballistic planner 或 preview sampler 数学。

## 6. Typed 结果与自验证

结果状态为：

- RequestRejected：请求无效，身份、action 与 configuration 均为空；
- ActionRejected：preview identity 或 action capture 失败关闭；
- ConfigurationRejected：preview action 无法进入 P20.30 frozen configuration；
- Captured：request、双身份、action 与 configuration 全部有效。

Captured 结果会重新构造 preview action 与 configuration，并逐值比较。Matches() 比较状态、诊断、request、双身份、action 与 configuration；相同只读输入可确定性重放。

## 7. 新增自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture 新增 8 项无头自动化：

1. CanonicalCapture：标准请求、双身份分域、action/configuration 绑定与标签；
2. DeterministicReplay：相同只读输入逐值重放；
3. RevisionIsolation：preview revision 只改变预览域身份；
4. SequenceSnapshot：捕获不推进调用方 sequence，sequence 改变会进入两种身份；
5. IdentityDomain：prospective real identity 精确复用标准 activation factory；
6. RequestFences：零值、耗尽序号、Straight 会话与非法 segment 均失败关闭；
7. TypedRejection：默认无效请求产生自验证的 RequestRejected；
8. CompositionCompatibility：捕获配置可直接进入 P20.30 并生成预期 target。

首轮 Editor 编译 7 actions、native exit 0、20.76 秒；首轮聚焦自动化直接得到 8/0，无生产代码修复轮。

## 8. 改动文件回归映射

新增 ThrownWeaponArcPreviewCapture changed-file 规则，要求：

- Product.ThrownWeaponArcPreviewCapture；
- Product.ThrownWeaponArcPreviewComposition；
- Product.ThrownWeaponProductSession；
- Product.ThrownWeaponProductController；
- CombatRuntime.ThrownWeaponPreview.Arc；
- CombatRuntime.ThrownWeaponArc；
- CombatRuntime.ThrownWeapon；
- CombatCore；
- broad Shanmen.0_0_10。

同时修改了 P20.30 composition，因此其既有规则额外要求 ArcChoiceProjection。正反映射 self-test 由 355 增至 357/357。最终 gate 对 7 个实现、测试与流程路径求并集：Changed=7 Rules=2 Required=10 Logs=10，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_capture_final.log | Product.ThrownWeaponArcPreviewCapture | 8/0 | 57D627802011CC7BA756D6FF82194712683DA5885D2653FED68806BD0CA64D56 |
| arc_preview_composition_final.log | Product.ThrownWeaponArcPreviewComposition | 7/0 | 1B4128622DBD39739E91CB87C57249F377FF37F6604DA6CD9F99382B8068406E |
| thrown_product_session_final.log | Product.ThrownWeaponProductSession | 6/0 | FC169EF4E8126C4282C7FBDC6170395B1DF8CC5B98295227CE34350E30A3B839 |
| thrown_product_controller_final.log | Product.ThrownWeaponProductController | 7/0 | 60AAB04CB454CA1BE93B78CD1D889E5D85CC797AB64CACEEF88A514E44E43EA4 |
| arc_choice_projection_final.log | Product.ThrownWeaponArcChoiceProjection | 5/0 | A3E95C026C5EC3C848127197097CD62466962ED99E5A82DBAD780E11F750D515 |
| arc_preview_sampler_final.log | CombatRuntime.ThrownWeaponPreview.Arc | 6/0 | 6E86A9FB8FAC86C4B3550CF705416DFFCA3BB4B084717A096D1D80F66B3DC7FF |
| arc_planner_final.log | CombatRuntime.ThrownWeaponArc | 10/0 | 1EA507E19797C5A48B9E33F09A11CE011DD576A322839E93816041EF6EE72943 |
| thrown_weapon_runtime_final.log | CombatRuntime.ThrownWeapon | 20/0 | E6D40BB1CD15655C4A895B72175FE422D638095C02FEFD76FE56D944DFAAB7F5 |
| combat_core_final.log | CombatCore | 9/0 | C11244BEED015EC52AC002FE3AB050A754E7843F29FC235AA1328AE9D3D61303 |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1006/0 | 2F9C9D36FCF36F593818AE78756D21A9A293DD97720BE27A10EBF6321EF65D30 |

最终自动化证据合计 1084/0。每份日志均有唯一 RunTests、零失败、唯一 UE 5.8 原生成功终止标记与零 fatal/unhandled/ensure/no-match。

- automation audit：PASS，SHA-256 BE17C58DD27A1C7CAC51544ED9D825D24F76F295315C054C6814CC58FB012C5B；
- regression self-test：357/357，SHA-256 EDC0861BC9C8BAED583376C4F291DA3C60E9FAF75F7C70C64832738B40DBD6FF；
- boundary scan：PASS，0 个 coordinator/inventory/World/random/mutation API 命中，SHA-256 80F7EB62E5EB6CCD13465C7F7B0A99362D43CB16B7AC4A2BE9B6F72CC0D8718F；
- changed-file gate：PASS Changed=7 Rules=2 Required=10 Logs=10，SHA-256 64757088CAD6D6B84B40D1162D6F199392E9D9F2EFD0803FDA111CA8B36F0B43；
- staged diff check：PASS，9 个精确暂存文件、0 个空白错误，SHA-256 3ECFF37E568E2AE6BC903448F793CD28963DE4959E2CA09FBAFFFF210ECE4050；
- final Editor：up to date / native 0 / 0.99 秒，SHA-256 F255294F0B1FDFC63ADA6F22D488F05211BE94BBBEF4C9A98105BEB0E5A66034；
- final Game：6 actions / native 0 / 19.76 秒，SHA-256 CBE0877A1FB58CAA805619F0C2A09A4531682B384F4578775F891AE2475BC230。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,542,208 bytes，SHA-256 90CD59B7E6263CAF459C144BE834B6E95FAF33DD269B9063EDD45DDA73F0CFAB；
- Binaries/Win64/demo_map.exe：357,777,920 bytes，SHA-256 CA0F72CD7F3C2EDAEB7A802C42E5F5D097341C4AA17A96D60CCED3BBAE922149。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：只读 request capture、observed sequence 冻结、preview/real activation identity 分域、preview-only action、session-to-P20.30 geometry capture、typed rejection、自验证、确定性重放，以及完整 0.0.10 与全部直接依赖回归。

未验证：从真实 run coordinator 读取当前 sequence 的产品 adapter、真实 product session/current choice 的 live 组合、UI/HUD presentation、World trace/collision/occlusion、真实地形落点、可见轨迹、真实设备输入、Editor UI/PIE/Standalone、产品可执行文件、projectile 对齐、库存 reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.32 增加只读产品桥：从当前 product session、choice revision 与 coordinator getter 组装 P20.31 request，并证明一次预览读取前后真实 activation sequence 与库存权威均不变；桥稳定后再连接 presentation。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture/Docs/Report/Dev.D.UE.0.0.10.P20.31.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture/Docs/Log/Dev.D.UE.0.0.10.P20.31.r0_log.md>
