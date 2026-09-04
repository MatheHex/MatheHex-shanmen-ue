# Dev.D.UE.0.0.10.P20.17.r0 Development Log

## 1. 目标与基线

- 基线：`dd9860efe442fb07612e06c1f1c9c568d6c63254`（P20.16）；
- 分支：`agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter`；
- 目标：复用唯一 1–9 hotbar 入口，Straight 保持原 route，Arc 生成 logical confirmation event 并进入 P20.16；
- 约束：不新增物理 binding、UI/preview、World/item/Run/inventory 权威、projectile、碰撞或伤害路径。

## 2. 接入前审计

`UseHotbarSlot1–9` 已由 9 条 `BindKey` 指向同一 `UseHotbarSlot`。该函数原先在 gameplay gate 后直接调用 GameMode Straight InputAdapter，再按 pass-through 顺序进入治疗 item 和 generic quick-slot。

P20.11 GameMode session 是唯一 choice 真值；P20.12–P20.14 已完成 Arc projection/composition/source basis；P20.15 已提供 PlayerController launch route 与 stale-choice fence；P20.16 已提供 bounded explicit confirmation owner。剩余缺口正是 hotbar 入口没有依据 choice 分流，也没有实际 logical event producer 与单一 policy source。

## 3. 轨迹感知 adapter

新增 controller-local `Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter`。固定顺序为 slot、input、consumer、choice；Straight 调用原 route 一次且零 Arc 工作，Arc 消费一个新 ordinal、生成确定性 event ID、读 policy 一次、捕获 P20.16 intent 并提交一次。

结果保存 choice/policy read、Straight/Arc route、ordinal/event、intent 与 P20.16 evidence。pass-through 不重做 item 判断，只从有效 Straight result 或有效 Arc wrapper 链末端读取原 InputAdapter result。所有 wrapper mismatch 与 sequence exhaustion 失败关闭。

## 4. policy、序列与 PlayerController 接入

产品 policy 集中在 `Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource`：forward 400–1200、lateral 300、apex 100–500。logical event ID 使用独立命名空间和 controller-local ordinal；0/MAX 为 sentinel，Arc 失败也消费本次 ordinal，Straight 不消费。

PlayerController 新增 adapter 成员与 `RouteThrownWeaponHotbarConfirmationInput`。只有 gameplay/surface/mode 全允许时才解析 GameMode；`UseHotbarSlot` 改为只调用这一入口一次，再按原顺序进入治疗与 generic item。九条现有物理 binding 未变。

## 5. 新增自动化

新增 exact 7 项：policy/identity、gate/choice、Straight pass-through/handled、Arc pass-through/handled、协议失败、序列耗尽、PlayerController boundary。

首次 Editor build：20 actions / native 0 / 42.60 秒。首次 exact：`7/0`、fatal/unhandled/ensure 0，SHA-256 `2C306A42D942C97BBBC9B98A42800E587BA9E00726A49161DAB72BF0940B6C34`。产品源码与测试断言均无返工。

## 6. 回归映射与流程自测

新增 `ThrownWeaponHotbarConfirmationAdapter` path rule，要求本轮 exact、P20.16、P20.15、P20.14、composition、projection、choice、旧 InputAdapter 与 full；PlayerCombatController rule 同步加入本轮 exact。

流程自测增加 broad-full 正例与 unrelated-item 失败关闭反例，最终 `327/327`，SHA-256 `C80FD0D0D48CFF7656B696E447F8DD9DD27DEA69DF0D9D51C06BC31F729D47C8`。JSON 解析通过。

## 7. 最终自动化与门禁

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `hotbar_confirmation_final.log` | `7/0` | `9783AC42DD694B0171A59D3E69D2459C1DA6201579160534C85F40BE3BD650E0` |
| `arc_confirmation_owner_final.log` | `6/0` | `85E68F7D56C8427F80994F5870B9F9BFD6D1B2BF23DAF614815AECFC1D4FD66A` |
| `arc_launch_input_final.log` | `6/0` | `ADDDDF39C388BB7DAB7DF82A49069FC7C1583F6108856AB9AB10E0DB59210E11` |
| `source_basis_adapter_final.log` | `6/0` | `67FDC54DE6CF00EE0732011D67D2DD837F9B84D8B96D8CBCBF294EC9D076140B` |
| `arc_choice_composition_final.log` | `5/0` | `F28D8811F7000048ABB873A371329F8509881DE9F442B461FB36A6B9D7C6F419` |
| `arc_choice_projection_final.log` | `5/0` | `B5DD90FD8D6D2ED10F4F9E850E2338B62834BD61A8662C220153F4B50CB9EC66` |
| `input_choice_final.log` | `9/0` | `51A9B0947AE1218AC26F7D2B93633696910A55BC2E99CFBE5EC1A00B755CE190` |
| `input_adapter_final.log` | `9/0` | `0ACFCA2C66A18B448DACE5643BA3120A3C60436535832DF597AB879DAE810B02` |
| `legacy_input_restore_final.log` | `101/0` | `91BE6DB93A31ED0708B7944FCC300F8211816660ABE4C5DC50020255B5B9E88A` |
| `legacy_v2_ranged_final.log` | `22/0` | `2F019D93401EDF5CA0EFF00B461978BB5B7B890FBC6123D6EC87E6E698BCD687` |
| `full_0_0_10_final.log` | `907/0` | `A2446B16AC4A916E5B4EC6F0DDD17DB50DC635FE22C76B497B3FE3CD999BAEF4` |

日志审计：`PASS Logs=11 RecordedSuccess=1083 RecordedFail=0 Invalid=0`，SHA-256 `6172B3F5AD45CCFF9898F8B0DE470E85323B036CF3B01B23F43204CFE28D5905`。

预 Report changed-file gate：`PASS Changed=8 Rules=3 Required=18 Logs=11`，SHA-256 `57F0F881EF576FEB2BBC65E953CD95DB9418B4D3E0559F8020C76B9611A888DD`。最终 changed-file gate：`PASS Changed=10 Rules=3 Required=18 Logs=11`，SHA-256 `1D9B7939B7882D1CF05A61EA04940506AC71E9B2C0A685C7F302C932E43B5145`。暂存区格式检查：`PASS / 10 files / native 0`。

## 8. 失败证据与静态边界

首次 Game build 命令误写项目目录为 `Dev.D.UE.0.0.B`，得到 `OtherCompilationError` / native 6；`game_build_first.log` SHA-256 `73DB85A5B60619E36EA811532042C5BEF791B295AE26A3DA35BA4972C2C89CA0`。只更正为 `Dev.D.UE.0.0.9B` 后成功，未修改源码或断言。

静态审计：adapter forbidden API 0、PlayerController 新增 physical binding 0、既有 hotbar binding 9、`UseHotbarSlot` adapter/旧直连为 1/0、controller Straight/Arc/policy source 为 1/1/1；SHA-256 `0C31E59927EA6322ECC4E498F214D625B1178A557EF55E823E4FA97729745443`。

## 9. 最终构建与产物

- Editor：0 actions / native 0 / 1.06 秒 / `81911D08EB5EEA9DB2CFEF1F98F2AFC9AC483581733F4DA21E57C2448F750728`；
- Game：19 actions / native 0 / 47.53 秒 / `382588645CC8F9E4A178A6AF10E46E50063F3D246013E0D1F10D78DD6F61A552`；
- `demo_map.exe`：357420544 bytes / `D85FA0139D68467C83DA761965159F4E2B64D0085B8FB29938AEE069613C1361`；
- `UnrealEditor-demo_map.dll`：16123392 bytes / `4E74DB0CB6346A6C2EA9E45DC33B9ADA096C7ADCB4284D2281FAD2B4A60C5F60`。

只执行编译和无头自动化；未启动 Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 10. P/F 边界与后续判断

本轮证明 existing hotbar 可在唯一 choice 下稳定分流 Straight/Arc，并保持 non-thrown generic pass-through。它不证明 trajectory choice 已有真实设备/UI 编辑入口、preview 或真实 Arc 产品运行。

P20.18 建议增加 device-independent choice-edit controller adapter，把现有 P20.10 command 经过 PlayerController gate 提交给唯一 P20.11 GameMode session；保留 revision/replay/conflict evidence，不新增物理绑定或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.17.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-17-thrown-weapon-hotbar-confirmation-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.17.r0_log.md>
