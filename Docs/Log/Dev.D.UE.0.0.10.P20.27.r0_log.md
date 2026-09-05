# Dev.D.UE.0.0.10.P20.27.r0 Log

## 阶段

- 任务：P20.27 thrown-weapon Ballistic Arc editing physical input；
- 基线：`2730376f42ba478c069220e390ae49292a19c9ca`；
- 分支：`agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实设备输入/截图/Smoke/Cook/Package。

## 实现记录

1. 统一 action registry 新增 target set、apex increase、apex decrease、target clear 四个 press-only action，默认键分别为鼠标中键、右方括号、左方括号与 Delete。
2. 默认键总数 `24→28`，全部唯一；四个 binding 均通过 `Settings.GetKey(ActionId)` 建立，继承现有 remap/swap/rebuild。
3. 输入配置写出版本 `5→6`；加载旧 v5 时保留已有 override、补齐四个新 action，默认键冲突时使用既有唯一替代策略。
4. target handler 复用 `GetLastValidAimDirection()` 的 XY 分量，只调用一次 P20.26 target route。
5. apex handlers 以固定 `±0.25` 各调用一次 P20.26 adjustment route；clear handler只调用一次 P20.26 clear route。
6. 四个 handler 不直接访问 GameMode、session、reducer、command、UI、timer 或 retry；未新增协议或权威。
7. 增加一组共享 non-shipping invocation/result 证据，避免四套测试字段。
8. 新增 10 项 transient World 合成按键自动化；完整 0.0.10 总数 `969→979`。
9. changed-file map 新增物理输入规则并扩展 UnifiedInput/PlayerCombatController 要求；正反 self-test `347→349`。
10. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 首次失败与修复

首次 `demo_map.P7Integration`：`8/1`，原生退出码 255。唯一失败为 `RegistryCoversProductActions`：旧测试仍断言 action 数量 24。生产注册表、迁移断言和新测试均要求 28，因此修正旧期望为 28，不改变产品逻辑。修复后 Editor 编译成功，P7 复跑 `9/0`。

- 首败日志：`legacy_p7_integration_first_failure.log`；
- SHA-256：`A9739121B25741F85756B23F2C317C64C3F4DD9DB8E0F4F6A9B708C2A62E6E2A`。

## 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_editing_physical_input_final.log` | `10/0` | `CC538D34BFE6C9C95807AF9F6194256BA7C1EB2B389DB0E4DCAA347EBBF42247` |
| `full_0_0_10_final.log` | `979/0` | `C23BBEFD301860C87E444A5F3647387B908A89C6B11730F13561101F4DBAF4D8` |
| `legacy_full_system_loop_final.log` | `50/0` | `7DBBEBD806BE8D3C92B6B57AF4A1248736F97F88F5390DD91DCF1B38A6F449DE` |
| `legacy_p5_runtime_interface_final.log` | `9/0` | `55ADEB8AAB9DAA305E694C99BB7FAA18DAF3E345B6B494B0FF448CEC3C8C54E6` |
| `legacy_p7_integration_final.log` | `9/0` | `573470B030D9FFC5E158D8C2069063C72C47CD4B346DC7834FF55F333EB6F9BD` |
| `legacy_input_restore_final.log` | `101/0` | `5B7C676BFE01354083CD6451B242DBAE91358AACE45831C13BCD02CA5675D695` |
| `legacy_v2_ranged_final.log` | `22/0` | `AA92E02B319866707ABEB9D8E43878565FA0308E3572DE54F33C94220E4F8E57` |

最终证据合计 `1180/0`。日志审计：7 份日志各含唯一 RunTests 命令、精确成功数、零失败、原生退出码 0 与终止标记，fatal/unhandled/ensure 为 0；SHA-256 `605F6073B576DF778335CB2E62AB486754FC6B83107544F6776935E96D0351B7`。

## 流程与静态证据

- regression self-test：`349/349`；SHA-256 `3FACBF4D044380BEE2478C86060930B4CB159B3993FB62A4A1BE227850B8C9D7`；
- static audit：`30/30`；SHA-256 `D6B99D1744E31ED7F789B83268A60928968D0AA2AF7942A361C984E56697967A`；
- changed-file gate：`PASS Changed=15 Rules=7 Required=31 Logs=7`；SHA-256 `B64978E52EC08728197CD48144D78AD4199C116A8BD547B8DE730408E42E29A8`；
- `git diff --check`：PASS。

## 构建与产物

- P7 修复后 Editor：native 0 / 5.95 秒；SHA-256 `152094669032D501C9E44EC882565FC7DB97103323731C89B94B46C24A3AB220`；
- final Editor：up to date / native 0 / 0.92 秒；SHA-256 `0B7B489658435BA9152B06376F29726E1B89562B66AE5FC0B3851AFB50BC1D54`；
- final Game：36 actions / native 0 / 36.11 秒；SHA-256 `90776680828C8F0B3B3E11A484C79B4C25AFABAD024404047CCF215151A49D68`；
- Editor artifact：16,424,960 bytes，SHA-256 `9ECE45B8014876E4B8C02ACD8E06B7B2407480274D3DFCEE601D169F526BA178`；
- Game artifact：357,665,280 bytes，SHA-256 `A5611DDC772564FDEA92982EA491FE2BCA82E9CC639354784EEF23FDD58D809E`。

## P/F

PASS：四动作注册与 remap、v5→v6 冲突安全迁移、pointer/aim target、apex `±0.25`、clear、live rebuild、Straight/零 aim/input lock 失败关闭，以及完整新旧映射回归。

未验证：真实鼠标/键盘、OS 焦点、cursor hit、Editor UI/PIE/Standalone、可见轨迹与落点、真实投掷/碰撞/命中、库存、伤害、Smoke、Cook 或 Package。

## 下一步

P20.28：在既有 HUD presentation 内读取统一 input settings，显示 remap 后的 target/apex/clear 键名与 Ballistic Arc 模式提示；不建立第二套 UI 状态或输入权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input/Docs/Report/Dev.D.UE.0.0.10.P20.27.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-27-thrown-weapon-arc-editing-physical-input/Docs/Log/Dev.D.UE.0.0.10.P20.27.r0_log.md>
