# Dev.D.UE.0.0.10.P20.7.r0 Development Log

## 1. 目标与基线

- 基线：`3a25065e4c3bd5cb7deb28796064ab30ea0c23d7`（P20.6）；
- 分支：`agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle`；
- 目标：让既有 ProductLifecycle 用规范内容显式绑定 Straight 或 BallisticArc ProductSession；
- 约束：不修改 InputAdapter、输入绑定、UI、预览、Controller、Router、Host、planner 或物品事务。

## 2. 接入前审计

P20.6 已让 ProductSession 接受判别式 Straight/Arc hotbar intent 与 immutable config，但 ProductLifecycle 仍只构造 Straight action/detector，并只调用 Straight SessionConfig capture。

InputAdapter 仍经旧 `TryBegin` 和 direction/range intent 工作。为保持兼容，本轮没有替换旧入口，而是把它们改为 typed Straight 入口的薄委托。

生命周期是第一个了解真实 `TrainingThrowingKnife` content identity 的产品边界，因此 Arc 数值 policy 也应在这里冻结；不能让未来输入层注入 gravity、speed 或 flight time。

## 3. Typed content capture

新增 overload 接受 `Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind`：

- 仅接受 Straight/BallisticArc；
- 始终先清空输出；
- 两种轨迹都验证真实 `TrainingThrowingKnife` 与 `ThrownWeapon` 语义；
- 共享 formula、base damage、coefficient、launch speed、tags 与 self fence；
- Straight 使用 canonical Straight action/detector；
- Arc 使用 canonical Arc action/detector，并冻结 Intermediate/980/4.0 policy；
- capture 失败时不泄露半有效 config。

旧 content capture 委托 typed Straight，不复制内容定义。

## 4. Typed lifecycle begin

新增 typed `TryBegin` 并保留原有 authority、Run correlation、Coordinator 与 player source 检查。它用请求轨迹构造规范 config，再进入唯一 Session `TryBegin`。

旧 `TryBegin` 委托 typed Straight。Session 的完整 config matching 提供幂等与策略切换隔离：同一绑定重复 Straight 成功，active Straight 请求 Arc 失败关闭，且不改变既有生命周期。

新增 `FindCapturedCommand` 只读透传现有 Session/Controller audit seam，用于验证被冻结的 Arc request；没有新增命令写入口。

## 5. 自动化扩展

ProductLifecycle exact 从 3 增至 5 项：

- `ContentAndBinding` 扩展 Straight/Arc 规范内容、共享值、Arc policy、invalid kind 清空、显式 Straight 幂等与 active policy switch fence；
- `ArcHotbarRouteReplayAndEnd` 验证真实 hotbar item、Run/Action/command、Arc request、Host、权威 revision、Actor 精确重放、几何冲突与结束；
- `ArcPlanRejectionReplayAndEnd` 验证不可达 Arc 的一次身份、零物品/Host 副作用、无命令、拒绝重放、冲突与无隐藏恢复；
- `FailClosedBoundaries` 新增 Straight lifecycle 在任何 product work 前拒绝 Arc intent；
- 原 Straight route/replay/end 保持通过。

0.0.10 全量从 856 增至 858。

## 6. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `product_lifecycle_final.log` | 5/0 | `08C3573664C82BF4EADBF3EF850CA0EBEC42C06A3D489F61B970521AEF44C544` |
| `product_session_final.log` | 6/0 | `917A468D655FC8E4534DB18D977707D282AE2A8FE416012D5E10B99B095DF6B4` |
| `product_controller_final.log` | 7/0 | `32FE583484E92ED75AC78422E47DC0DB51646C13BAB175171B59FBF12DC3093C` |
| `run_command_final.log` | 6/0 | `BDB7742C29118B0D90A6969D782DFC23241F8FBB315C7BBE3D08A6B69DDDD8C0` |
| `full_0_0_10_final.log` | 858/0 | `DEE41552ADCE2573BBC77C8BD21E6DD0EBF8426DB460448559207012B91ABBAD` |
| `legacy_item_armor_final.log` | 46/0 | `297AFA6C646C8565C0D13009B0C108EFB8AAE39F42A1521B819579DC7B556BF0` |

证据审计：`PASS Logs=6 RecordedSuccess=928`；SHA-256 `BB986130781264F6276C6B3D98301CB29C9F0FE7BC5396C8658AF4929258B093`。

首次 lifecycle exact 也为 5/0。首次编译成功，没有源码或产品测试失败。

## 7. 首次编排错误

批量 exact 运行中的 RunCommand 组名首次误写为 `Shanmen.0_0_10.ThrownWeapon.RunCommand`。UE 明确报告 no tests matched、Success=0，并以 native 255 退出。

该日志保留为 `run_command_invalid_group.log`，SHA-256 `E3B41B2BFD9CA585B9C0F222DAB993ABC1A5F56E66B84794FBF0B3CF30241B54`。修正为 `Shanmen.0_0_10.Product.ThrownWeaponRunCommand` 后独立重跑 6/0。无效日志未进入 regression gate 或成功计数。

## 8. Changed-file regression 与静态边界

三个改动路径全部命中 `ThrownWeaponProductLifecycle` 规则：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=12 Logs=6
```

- regression gate SHA-256：`1F30926A9126370E37125C19F9F2097678248007DED9DB5E9E2512A817D3D72F`；
- production boundary：`PASS Files=2 AddedLines=85 Matches=0`；
- boundary SHA-256：`49C732E29FB08D3B06C254DBE8C1CAA9C6E62AD69523660399CD8BC4CF424950`；
- `git diff --cached --check`：精确暂存 5 个本轮文件后 PASS / native 0；证据 SHA-256 `65588A84A5DA0F288C80FF939ED672A855BE11AB11ED5EDE3E0B0A567E1072AC`。

边界扫描覆盖新增 production 行中的 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics。

## 9. 构建与产物

- initial Editor：29 actions / native 0，SHA-256 `21DCCAEE47502993A96470943B2B2C43815B499F337A38D24BCBC6FCC87E544F`；
- final Editor：0 actions / native 0，SHA-256 `D5411F056750DB7E412EAC0CF0B4B43002F18AB57611A462B348DA90C80F3A08`；
- final Game：28 actions / native 0，SHA-256 `4C742CE362CA251D5CBBE54023099446C6B8D301E24D625AA78552EF8A4A8A0E`；
- `demo_map.exe`：357,175,808 bytes / `80807465617897082CE0379DC7958CB05A76E7DD5784858D1BC1B5234451466E`；
- `UnrealEditor-demo_map.dll`：15,837,184 bytes / `5AD25F91034C3FA44856E146145E76F9ECDC6E665418C6E7A877A861894BFD9A`。

## 10. P/F 边界与后续判断

本轮仅证明 ProductLifecycle 的 typed Arc content/session binding。InputAdapter 仍只采样 Straight aim direction，未开放任何玩家 Arc 手势或 UI。

P20.8 应扩展既有 InputAdapter，新增一次性采样 target/apex 的平台无关 Arc route，并显式绑定 Arc lifecycle；保留 Straight compatibility。输入设备绑定、滚轮弧度、目标预览与呈现继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P20.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P20.7.r0_log.md>
