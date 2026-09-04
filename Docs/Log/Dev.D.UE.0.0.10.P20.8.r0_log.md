# Dev.D.UE.0.0.10.P20.8.r0 Development Log

## 1. 目标与基线

- 基线：`c7879574a7ec1e898f163282ee831a3d303a18f4`（P20.7）；
- 分支：`agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter`；
- 目标：把既有 InputAdapter 扩展为 Straight/BallisticArc typed route，并稳定 target/apex 的采样与失败关闭顺序；
- 约束：不修改 GameMode、输入绑定、UI、预览、Controller、Router、Host、planner 或物品事务。

## 2. 接入前审计

P20.7 已让 ProductLifecycle 显式绑定 Straight 或 BallisticArc，但 InputAdapter 仍只有 aim direction 的 Straight 入口。

现有适配器已经拥有正确的共享前置顺序：先检查 hotbar 与 authority，再读取 active Run snapshot；空槽或非暗器槽直接透传；只有 canonical thrown item 才检查生命周期、玩家 source、几何、Action gate 和产品提交。本轮保留这一顺序，把轨迹差异限制在几何采样与 immutable intent capture 两处。

## 3. Typed route 重构

新增 private `RouteTypedHotbarInput` 作为唯一执行管线。旧 `RouteHotbarInput` 委托 Straight；新 `RouteArcHotbarInput` 委托 BallisticArc。

共享管线不接受产品平衡参数。Straight 继续使用既有 maximum distance；Arc 只接收 target 与 apex clearance，实际 action definition、offense、technique tier、gravity、maximum speed 和 flight time 仍由 ProductLifecycle 的规范 config 冻结。

Result 新增 trajectory kind、target sampled 与 apex sampled 证据，既有 aim sampled 保持兼容。

## 4. 生命周期轨迹身份

ProductSession 增加 active-only 的只读 trajectory kind；ProductLifecycle 只做透传。InputAdapter 在 source、几何和 Action gate 前要求生命周期 trajectory 与请求一致。

新的 `ProductTrajectoryMismatch` 与既有 `ProductRunMismatch` 分开，便于调用方区分 Run 绑定错误和轨迹策略错误。两种错误都不会调用任何外部采样器或授权回调，也不会消费 ordinal。

## 5. Arc 采样与提交顺序

Arc 路径执行：

1. 从 canonical source Actor 计算 origin；
2. target sampler 调用一次；
3. 验证 target 有限且不同于 origin；
4. apex sampler 调用一次；
5. 验证 apex clearance 有限且为正；
6. 派生稳定 SelectionId；
7. 捕获 immutable Arc hotbar intent；
8. Action gate 调用一次；
9. 授权后递增 selection ordinal；
10. 只调用一次既有 lifecycle submission。

失败点不会执行其后的步骤。Product 返回不可达计划时，已授权输入的 ordinal 保持消费；几何或 Action gate 失败则不消费。

## 6. 自动化扩展

InputAdapter exact 从 4 增至 7 项：

- `ArcTypedRouteAndPassThrough`：空槽/非暗器零采样透传，真实 Arc 路由逐字段核对 selection、action、request、policy、command、Host 与 authority revision；
- `ArcPlanRejection`：不可达 Arc 的一次授权身份、零 item/command/Host 副作用与正常结束；
- `ArcFailClosedGeometryAndActionConflict`：invalid target、invalid apex 与 Action conflict 的严格回调/ordinal 边界；
- `FailClosedBeforeSampling`：增加 Straight lifecycle 拒绝 Arc 的全零采样与授权断言；
- 原有 deterministic identity、Straight typed route/pass-through、Straight fail-closed 和 Action conflict 继续通过。

0.0.10 全量从 858 增至 861。

## 7. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `input_adapter_final.log` | 7/0 | `F0EFB68E1A32FC8ABE6186E777DFF84EA382B9732F36E5BAA5D582E3E1EA12D9` |
| `product_lifecycle_final.log` | 5/0 | `806D52E35A3623E2AE2D86FC65265DE46A214D9F5A5EEA6AF9656E94850D7312` |
| `product_session_final.log` | 6/0 | `1723AB3A45FF17011CEC5C0591096823E900AE8E77B92CD3B7226D9634115850` |
| `product_controller_final.log` | 7/0 | `4B822B825C2352BF58BDD3AE4544B4EB4E068B2661F78AABDB2FD9B95B2AC0F7` |
| `run_command_final.log` | 6/0 | `D73A6D7045D3F81947181B5A1B6D8C63563F50506DE01D2DD588D71941601034` |
| `full_0_0_10_final.log` | 861/0 | `7372B1857BD115F6AB61EBF828D82891CEFC425C52F304FA5FF443D48EA533AE` |
| `legacy_item_armor_final.log` | 46/0 | `E331EB77220D83C7BFDF24FFA50B30036690ABCD00EF068E1A5BE4D34F8FD19C` |

证据审计：`PASS Logs=7 RecordedSuccess=938 Invalid=0`；SHA-256 `E79D3A929B816D22C62DEE573ADC155F9A7C493B8012D6E28EA3318479450056`。

首次 InputAdapter exact 同样为 7/0，SHA-256 `2C370909D99D22053941C82B21CC50C66742F687A8C7DBBEB6BF021B3A39943A`。首次编译与全部产品测试均成功，没有失败日志需要保留。

## 8. Changed-file regression 与静态边界

五个改动路径命中 InputAdapter、ProductLifecycle 与 ProductSession 规则：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=3 Required=13 Logs=7
```

- regression gate SHA-256：`3AD5AEDA0974DE44319ACAADE42FF002257FA4DD364F965BA2453C259CF4AC95`；
- production boundary：`PASS Files=4 AddedLines=169 Matches=0`；
- boundary SHA-256：`A5E18EE40F92099EE96ABD4A5C136E42D02C05EE7018A76DD5F9DB2D668B439A`；
- `git diff --cached --check`：精确暂存本轮 7 个文件后 PASS / native 0；证据 SHA-256 `D70CE3AB745809CF0A72FE49202FB1444A7734E21309F5FB96166ECE240021AE`；长期未跟踪文件不纳入。

边界扫描覆盖新增 production 行中的 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics。

## 9. 构建与产物

- initial Editor：31 actions / native 0，SHA-256 `3334592520ECA98B256A5864B897ED5E29936CB18FA18732BE4D59D162C5FD8C`；
- final Editor：0 actions / native 0，SHA-256 `8B1E0E57AF5631169A8B628BCBA644A84AF143758E4C1B7822607C69F602CCE6`；
- final Game：30 actions / native 0，SHA-256 `2F31198721D6D64C2771A7078CF78C471A98CA32B21E84611B2B75D4EF5E68B9`；
- `demo_map.exe`：357,191,168 bytes / `9915444E66212112DE266079E30C1FB6E289401F9FB4AB84C26407DB0089EE32`；
- `UnrealEditor-demo_map.dll`：15,855,104 bytes / `B46D2AFF6B3A9B2C0B275070248D2E6CA96A6B58AC7A76740F218B7E66DE501E`。

## 10. P/F 边界与后续判断

本轮仅证明平台无关 InputAdapter 的 Arc route。GameMode 当前仍以 Straight 兼容入口初始化生命周期，并只公开 Straight aim route；没有真实玩家入口。

P20.9 应在既有 GameMode 组合边界显式选择生命周期 trajectory，并增加 target/apex callback 的 Arc route，继续复用本轮适配器。按键、鼠标、滚轮、UI、轨迹预览与呈现仍后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-8-thrown-weapon-arc-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.8.r0_log.md>
