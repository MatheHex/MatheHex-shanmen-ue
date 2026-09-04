# Dev.D.UE.0.0.10.P20.8.r0 Report

## 1. 结论

P20.8 已把 P20.7 的 BallisticArc 生命周期能力接入既有 `Fdemo_mapShanmenThrownWeaponInputAdapter`，同时完整保留 Straight 兼容入口。

输入适配器现在拥有一条共享的 typed hotbar 管线：Straight 只采样一次 aim direction；BallisticArc 依次只采样一次 target 与 apex clearance。两者继续复用同一 hotbar 分类、物品权威快照、Run correlation、玩家 source、Action gate、SelectionId 序列、ProductLifecycle、Session、Controller、RunCommand、RunHost 与物品事务。

生命周期的轨迹类型必须与输入请求精确一致。Straight 生命周期不能接受 Arc 输入，Arc 生命周期也不能接受 Straight 输入；不匹配会在 target/aim、apex、Action 授权、ordinal 消耗和产品工作之前失败关闭。

新增自动化证明 Arc 成功路由、非暗器槽透传、不可达计划的确定性拒绝，以及 target/apex/action conflict 的采样和副作用边界。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`c7879574a7ec1e898f163282ee831a3d303a18f4`（P20.7）；
- 分支：`agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 单一 typed 输入管线

旧 `RouteHotbarInput` 保持原签名与 Straight 语义，只委托新的 private typed route。新增 `RouteArcHotbarInput` 显式选择 `BallisticArc`，但只接收 target 与 apex 的惰性采样回调，不能注入伤害、速度、重力、标签、技术等级或飞行时限。

共享管线继续按原顺序验证：

1. hotbar 编号与 item authority；
2. active Run correlation 与权威 snapshot；
3. 空槽/非暗器槽直接透传；
4. canonical thrown-weapon item evidence；
5. lifecycle Run 与 trajectory identity；
6. canonical player source 与 selection sequence；
7. typed geometry；
8. immutable intent capture；
9. player Action gate；
10. ordinal commit 与唯一 ProductLifecycle submission。

因此 Arc 没有第二套输入分类、Action、库存或执行权威。

## 4. Arc 几何采样契约

Arc 输入先从 source Actor 与固定 origin height 得到 origin，再调用 target sampler。target 必须有限且与 origin 不同；失败时不会调用 apex sampler 或 Action gate。

只有 target 有效后才调用 apex sampler。apex clearance 必须是有限正数；失败时不会调用 Action gate。两者有效后才构造稳定 SelectionId 与 immutable Arc hotbar intent。

结果结构显式记录 trajectory kind、target 是否采样、apex 是否采样；Straight 的既有 `bAimSampled` 保持不变。这样自动化能直接证明每个外部采样器是否被调用，避免靠最终状态猜测输入路径。

## 5. 生命周期轨迹护栏

ProductSession 与 ProductLifecycle 新增只读 trajectory kind 透传。InputAdapter 在 source 与几何采样前核对 active lifecycle：

- Run 不匹配返回 `ProductRunMismatch`；
- trajectory 不匹配返回 `ProductTrajectoryMismatch`；
- 两类拒绝都不采样 aim/target/apex，不调用 Action gate，不消耗 selection ordinal，也不进入产品执行。

该护栏防止 Arc intent 被送入 Straight policy 后再由深层偶然拒绝，也防止调用方利用兼容入口把一种轨迹降级为另一种。

## 6. 成功、拒绝与冲突语义

Arc 成功测试使用真实 hotbar `TrainingThrowingKnife`，验证 target、apex、Action gate 各调用一次，并核对 exact item、Run、SelectionId、Action、Arc request、产品 policy、命令与 Host。物品 authority revision 只按既有 Prepare/Commit 增加两次。

不可达 Arc 在几何与 Action 均有效后取得一次 Run-owned Action identity，返回 `ArcPlanRejected`：不修改物品、不生成命令、不进入 Host，也没有隐藏恢复工作；该次已授权物理输入的 selection ordinal 被明确消费。

失败关闭测试还证明：

- 非有限/同源 target：不采样 apex，不授权，不消费 ordinal；
- 非有限或非正 apex：不授权，不消费 ordinal；
- Action conflict：target/apex/action 各采样一次，但不消费 ordinal、不进入产品；
- lifecycle trajectory mismatch：所有外部采样与授权均为零。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | 7/0 | `F0EFB68E1A32FC8ABE6186E777DFF84EA382B9732F36E5BAA5D582E3E1EA12D9` |
| `product_lifecycle_final.log` | `Product.ThrownWeaponProductLifecycle` | 5/0 | `806D52E35A3623E2AE2D86FC65265DE46A214D9F5A5EEA6AF9656E94850D7312` |
| `product_session_final.log` | `Product.ThrownWeaponProductSession` | 6/0 | `1723AB3A45FF17011CEC5C0591096823E900AE8E77B92CD3B7226D9634115850` |
| `product_controller_final.log` | `Product.ThrownWeaponProductController` | 7/0 | `4B822B825C2352BF58BDD3AE4544B4EB4E068B2661F78AABDB2FD9B95B2AC0F7` |
| `run_command_final.log` | `Product.ThrownWeaponRunCommand` | 6/0 | `D73A6D7045D3F81947181B5A1B6D8C63563F50506DE01D2DD588D71941601034` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 861/0 | `7372B1857BD115F6AB61EBF828D82891CEFC425C52F304FA5FF443D48EA533AE` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `E331EB77220D83C7BFDF24FFA50B30036690ABCD00EF068E1A5BE4D34F8FD19C` |

每份最终日志都只有一个 canonical `RunTests` command、一个 native success terminal、至少一个 Success、0 Fail，并且无 Fatal/Unhandled/Ensure：`PASS Logs=7 RecordedSuccess=938 Invalid=0`。审计 SHA-256：`E79D3A929B816D22C62DEE573ADC155F9A7C493B8012D6E28EA3318479450056`。

InputAdapter exact 首次即为 7/0，SHA-256 `2C370909D99D22053941C82B21CC50C66742F687A8C7DBBEB6BF021B3A39943A`。0.0.10 全量从 P20.7 的 858 增至 861。本轮没有源码、构建或产品自动化失败。

## 8. 改动门禁与静态边界

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=3 Required=13 Logs=7
```

五个改动路径命中 InputAdapter、ProductLifecycle 与 ProductSession 映射。13 个必跑组全部由 exact、0.0.10 全量与旧物品/护甲日志覆盖；gate SHA-256：`3AD5AEDA0974DE44319ACAADE42FF002257FA4DD364F965BA2453C259CF4AC95`。

对四个 production 文件的新增行扫描 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics：`PASS Files=4 AddedLines=169 Matches=0`；SHA-256：`A5E18EE40F92099EE96ABD4A5C136E42D02C05EE7018A76DD5F9DB2D668B439A`。

`git diff --cached --check`：仅暂存本轮 7 个文件后 PASS / native 0；证据 SHA-256：`D70CE3AB745809CF0A72FE49202FB1444A7734E21309F5FB96166ECE240021AE`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- initial Editor：31 actions / native 0，SHA-256 `3334592520ECA98B256A5864B897ED5E29936CB18FA18732BE4D59D162C5FD8C`；
- final Editor：up to date / 0 actions / native 0，SHA-256 `8B1E0E57AF5631169A8B628BCBA644A84AF143758E4C1B7822607C69F602CCE6`；
- final Game：30 actions / native 0，SHA-256 `2F31198721D6D64C2771A7078CF78C471A98CA32B21E84611B2B75D4EF5E68B9`。

产物：

- `Binaries/Win64/demo_map.exe`：357,191,168 bytes，SHA-256 `9915444E66212112DE266079E30C1FB6E289401F9FB4AB84C26407DB0089EE32`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,855,104 bytes，SHA-256 `B46D2AFF6B3A9B2C0B275070248D2E6CA96A6B58AC7A76740F218B7E66DE501E`。

## 10. P/F 边界与下一步

P20.8 证明的是“已绑定对应轨迹生命周期时，平台无关 InputAdapter 能以严格一次采样语义把 Arc target/apex 路由到唯一产品执行权威”。它没有证明当前 GameMode 或玩家设备已经能选择 Arc。

现有 `Ademo_mapGameMode` 仍在 Run 启动时调用 Straight 兼容 `TryBegin`，并只公开 aim-direction 的 Straight input route。因此建议 P20.9 扩展既有 GameMode 组合边界：显式选择生命周期 trajectory，并增加 target/apex callback 的 Arc route，复用本轮 InputAdapter；仍不接按键映射、鼠标手势、UI、轨迹预览或真实产品输入。完成组合缝后，再独立设计玩家选择 Arc 的设备语义与视觉反馈。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.8.r0_log.md>
