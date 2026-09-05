# Dev.D.UE.0.0.10.P20.31.r0 Log

## 阶段

- 任务：P20.31 thrown-weapon Arc preview read-only identity/capture policy；
- 基线：53ce13e7a0c7e1b9ce929ec18e8631a4ae36a69e；
- 分支：agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增不可变 Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest。
2. 请求冻结 preview request、Run、player、source item、authority content、observed next activation sequence、session config、choice policy 与 segment count。
3. sequence 是按值证据；策略不持有 coordinator、序号引用、reservation 或 inventory authority。
4. preview action ID 使用 demo_map.ShanmenThrownWeapon.ArcPreviewAction.r1 独立命名空间与完整 canonical inputs。
5. prospective real action ID 仅调用既有 FShanmenCombatIdFactory::MakeActivationId() 计算对照，不登记或消耗。
6. 双身份必须有效、不同且可重算；preview request revision 不污染 prospective real identity。
7. preview action 复用既有 combat action snapshot，保留 session source tags 并加入 Source.Player。
8. P20.30 configuration 增加 immutable definition + Arc policy 的 geometry-only 重载；原 product capture 重载委托该实现。
9. P20.31 从既有 session definition capture 重建 immutable definition，并复用 P20.30 configuration。
10. typed status 区分 RequestRejected、ActionRejected、ConfigurationRejected 与 Captured。
11. 成功结果重建 preview action/configuration 自验证；相同输入可逐值重放。
12. 新增 8 项纯值自动化，完整 0.0.10 总数 998→1006。
13. changed-file map 新增捕获规则；正反 self-test 355→357。
14. 当前 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 执行结果

首轮 Editor 构建直接成功：7 actions、native exit 0、20.76 秒。首轮聚焦自动化直接得到 8/0；无需修复生产代码或测试。

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_capture_final.log | 8/0 | 57D627802011CC7BA756D6FF82194712683DA5885D2653FED68806BD0CA64D56 |
| arc_preview_composition_final.log | 7/0 | 1B4128622DBD39739E91CB87C57249F377FF37F6604DA6CD9F99382B8068406E |
| thrown_product_session_final.log | 6/0 | FC169EF4E8126C4282C7FBDC6170395B1DF8CC5B98295227CE34350E30A3B839 |
| thrown_product_controller_final.log | 7/0 | 60AAB04CB454CA1BE93B78CD1D889E5D85CC797AB64CACEEF88A514E44E43EA4 |
| arc_choice_projection_final.log | 5/0 | A3E95C026C5EC3C848127197097CD62466962ED99E5A82DBAD780E11F750D515 |
| arc_preview_sampler_final.log | 6/0 | 6E86A9FB8FAC86C4B3550CF705416DFFCA3BB4B084717A096D1D80F66B3DC7FF |
| arc_planner_final.log | 10/0 | 1EA507E19797C5A48B9E33F09A11CE011DD576A322839E93816041EF6EE72943 |
| thrown_weapon_runtime_final.log | 20/0 | E6D40BB1CD15655C4A895B72175FE422D638095C02FEFD76FE56D944DFAAB7F5 |
| combat_core_final.log | 9/0 | C11244BEED015EC52AC002FE3AB050A754E7843F29FC235AA1328AE9D3D61303 |
| full_0_0_10_final.log | 1006/0 | 2F9C9D36FCF36F593818AE78756D21A9A293DD97720BE27A10EBF6321EF65D30 |

最终证据合计 1084/0。每份日志均有唯一 RunTests、精确成功数、零失败、唯一成功终止标记与零 fatal/unhandled/ensure/no-match。

## 流程与静态证据

- automation audit：PASS；SHA-256 BE17C58DD27A1C7CAC51544ED9D825D24F76F295315C054C6814CC58FB012C5B；
- regression self-test：357/357；SHA-256 EDC0861BC9C8BAED583376C4F291DA3C60E9FAF75F7C70C64832738B40DBD6FF；
- boundary scan：PASS；禁止 API 命中 0；SHA-256 80F7EB62E5EB6CCD13465C7F7B0A99362D43CB16B7AC4A2BE9B6F72CC0D8718F；
- changed-file gate：PASS Changed=7 Rules=2 Required=10 Logs=10；SHA-256 64757088CAD6D6B84B40D1162D6F199392E9D9F2EFD0803FDA111CA8B36F0B43。
- staged diff check：PASS；9 个精确暂存文件、0 个空白错误；SHA-256 3ECFF37E568E2AE6BC903448F793CD28963DE4959E2CA09FBAFFFF210ECE4050。

## 构建与产物

- first Editor：7 actions / native 0 / 20.76 秒；
- final Editor：up to date / native 0 / 0.99 秒；SHA-256 F255294F0B1FDFC63ADA6F22D488F05211BE94BBBEF4C9A98105BEB0E5A66034；
- final Game：6 actions / native 0 / 19.76 秒；SHA-256 CBE0877A1FB58CAA805619F0C2A09A4531682B384F4578775F891AE2475BC230；
- Editor artifact：16,542,208 bytes；SHA-256 90CD59B7E6263CAF459C144BE834B6E95FAF33DD269B9063EDD45DDA73F0CFAB；
- Game artifact：357,777,920 bytes；SHA-256 CA0F72CD7F3C2EDAEB7A802C42E5F5D097341C4AA17A96D60CCED3BBAE922149。

## P/F

PASS：只读 request、sequence snapshot、preview/real identity 分域、preview-only action、session-to-P20.30 capture、typed rejection、自验证、deterministic replay，以及完整直接依赖回归。

未验证：真实 coordinator getter/product adapter、live product session/current choice、UI/HUD、World、trace/collision、可见轨迹、真实输入、Editor UI/PIE/Standalone、产品可执行文件、projectile、库存/投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.32：建立只读产品桥，从当前 product session、choice revision 与 coordinator getter 组装 P20.31 request；自动化证明预览读取前后真实 sequence 与库存权威均不变，再进入 presentation。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture/Docs/Report/Dev.D.UE.0.0.10.P20.31.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-31-thrown-weapon-arc-preview-capture/Docs/Log/Dev.D.UE.0.0.10.P20.31.r0_log.md>
