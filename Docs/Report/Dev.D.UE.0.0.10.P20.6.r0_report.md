# Dev.D.UE.0.0.10.P20.6.r0 Report

## 1. 结论

P20.6 已把 P20.5 的 BallisticArc Controller 能力接入既有 `Fdemo_mapShanmenThrownWeaponProductSession`，同时完整保留 Straight 兼容路径。

会话入口现在显式区分 Straight 与 BallisticArc。Hotbar intent 只携带玩家选择几何；SessionConfig 冻结产品侧 Arc policy；Session 仍从 active Run correlation 解析唯一物品、捕获一次 AttackPower，并委托同一个 ProductController、RunCommand Router、RunHost 与物品事务完成执行。

不可达 Arc 会在取得 Run-owned Action identity 后以 `ArcPlanRejected` 确定性结束。它没有命令、物品 I/O、Actor 或恢复工作，但仍是合法会话记录，可精确重放且可正常结束 Session。

本轮没有接 ProductLifecycle、输入、UI 或预览。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`23f377d537da7ec85db58bfd5a84811ed64a6b19`（P20.5）；
- 分支：`agent/0.0.10-p20-6-thrown-weapon-arc-product-session`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 类型化 Hotbar Intent

`Fdemo_mapShanmenThrownWeaponHotbarIntent` 新增显式 trajectory kind 与 `TryCaptureArc`：

- Straight 继续使用 origin、规范化 aim direction 与正 maximum distance；
- Arc 只接受有限 origin/target、互异端点与正 apex clearance；
- Straight 的 Arc 字段必须为空，Arc 的 Straight 字段必须为空；
- `IsValid` 与 `Matches` 比较完整判别式载荷，不能把同一 SelectionId 的两种轨迹误认为相同请求；
- 一基 hotbar slot 与 active Run correlation 的绑定规则保持不变。

## 4. 不可变 SessionConfig

`Fdemo_mapShanmenThrownWeaponSessionConfig` 保留原 Straight capture，并新增 Arc capture。Arc config 冻结：

- 规范 Arc action definition；
- source gameplay tags；
- technique tier；
- gravity magnitude；
- maximum flight time。

Definition 的 launch speed 继续作为唯一最大初速度来源。Config 通过 ProductCapture 的既有验证入口构造，不在 Session 复制第二套平衡参数规则。

## 5. 单一会话执行路径

`TrySubmitHotbar` 在 identity 分配与物品 I/O 前拒绝 intent/config trajectory mismatch。通过后仍按原顺序执行：

1. 从冻结的 active Run hotbar 解析精确 item instance；
2. 捕获一次玩家 AttackPower；
3. 按 trajectory kind 构造 Straight 或 Arc product selection；
4. 按同一 kind 构造 immutable product capture；
5. 委托 P20.5 ProductController；
6. 复用唯一 Coordinator sequence、Router、Host、item Prepare/Commit/Cancel 与 replay ledger。

精确重放继续复用首次物品、AttackPower、Action identity、sequence 和 projectile Actor。同 SelectionId 异几何继续在会话层失败关闭，不消耗新 sequence。

Session `IsValid` 会按 trajectory kind 重构 selection/product 并验证互斥载荷。它同时区分“成功捕获命令”与“不可达 Arc 的确定性无命令拒绝”，避免把后者误判为会话损坏；其它已捕获 Action 却缺命令的状态仍失败关闭。

## 6. 自动化扩展

ProductSession exact 从 4 增至 6 项：

- `ContractAndBinding`：扩展 Straight/Arc intent、config、互斥字段与 trajectory mismatch；
- `ArcResolveFreezeReplayConflict`：Arc hotbar → exact item → Action → planner → Router/Host，验证扣减、首次快照、Actor 重放与几何冲突；
- `ArcPlanRejectionReplayEnd`：不可达 Arc 只记录一次身份，无 item/Host 副作用，重放复用拒绝且 Session 可直接结束；
- 原 Straight freeze/replay、busy retry 与 durable recovery 三项继续通过。

0.0.10 全量从 P20.5 的 854 增至 856。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `product_session_final.log` | `Product.ThrownWeaponProductSession` | 6/0 | `06F898D69CF038FD16B4C1AD6C9AB69D80AC3F08CEA0BBF9C4C04BD423AB700F` |
| `product_controller_final.log` | `Product.ThrownWeaponProductController` | 7/0 | `3250511EFCE187F6461D18C7BB9F2FCEA052B8B73E302D255FE5148621D7B6D9` |
| `run_command_final.log` | `Product.ThrownWeaponRunCommand` | 6/0 | `3D9AFAB7F48BF04D09C773D225E33A2564A426449B23BFFAF1DCC5B17BF04A1A` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 856/0 | `D4EAC4F66BB37CD70352732E5A442747FAB1B3E07FF89AB1490882BA4DDA2386` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `67907B163E4ED443CCA19B0009A796B90C8D9095C06743B164C4B6853A78C81B` |

每份门禁日志均只有一个 canonical `RunTests` command、一个 native success terminal、至少一个成功结果、0 Fail，且无 Fatal/Unhandled/Ensure：`PASS Logs=5 RecordedSuccess=921`。审计 SHA-256：`2E3A9BD92122DC46D770260B84A1ED084E21696533A5E51F96EED5927EA03002`。

首次 Editor 编译与首次 ProductSession exact 也均成功；没有失败日志被删除或覆盖。

## 8. 改动门禁与静态边界

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=5
```

Session 三个改动文件命中 `ThrownWeaponProductSession` 映射。11 个要求组全部由 exact、0.0.10 全量与 `demo_map.ItemUseAndArmor` 日志覆盖；gate SHA-256：`ECC2C5F06810D26E9169800A51E4504AEAC5584FD21080E1C97DC4E585A4F0C6`。

对两个 production 文件的新增行执行 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics 边界扫描：`PASS Files=2 AddedLines=283 Matches=0`；SHA-256：`FE2D95E8733E7512C6D4EBDC23BBAA3A26AE9461A35D45F4003CE1EED7EE95A4`。

`git diff --cached --check`：最终 5 个提交文件 PASS / native 0；证据 SHA-256：`43539D43FBF860429DF7896823DD8295D9BEDDFD6C8C93FC3C1BEA90CFD7D953`。

## 9. 构建与产物

- initial Editor：31 actions / native 0，SHA-256 `3FE797F0FCFF685C5E861916FE0B7358ACE60894BDE6272591455D19A976E764`；
- final Editor：up to date / 0 actions / native 0，SHA-256 `15F65C53A1BD7F0513E7011A1671DCFB26752B8DF63A1E08C1FA7C42E1BA3B74`；
- final Game：30 actions / native 0，SHA-256 `BC21412A206E46F5725B19941A35391FEA525A4E91CCD3E2B8542D4C6930532C`。

产物：

- `Binaries/Win64/demo_map.exe`：357,160,960 bytes，SHA-256 `D36CB4F47660AC4345BF757E82D0E933C96CF063E522A464C51574B6B91D7603`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,824,384 bytes，SHA-256 `DBBA9F58615E0A85608D935B0051B217AEE0A1B5B843BBEA8892913C3BDED44B`。

## 10. P/F 边界与下一步

P20.6 证明的是“已知 typed hotbar geometry 与 immutable product policy 时，ProductSession 能经唯一既有权威执行 Straight 或 Arc”。它没有证明玩家可以通过当前输入链选择 Arc，也没有证明生命周期、UI、预览或真实产品体验。

建议 P20.7 只扩展既有 ProductLifecycle 的 immutable content/session binding，使它能显式选择 Straight 或 Arc SessionConfig；继续不改 InputAdapter、UI 或玩家手势。生命周期缝稳定后，再单独接滚轮弧度意图、目标预览与可视反馈。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-6-thrown-weapon-arc-product-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-6-thrown-weapon-arc-product-session/Docs/Report/Dev.D.UE.0.0.10.P20.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-6-thrown-weapon-arc-product-session/Docs/Log/Dev.D.UE.0.0.10.P20.6.r0_log.md>
