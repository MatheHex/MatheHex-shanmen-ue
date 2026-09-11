# Dev.D.UE.0.0.10.P27.1.r0 Development Log

## 1. 目标

- 为 P27.0 权威命令增加 Run 级唯一产品控制器；
- 使精确请求重试回放同一命令与启动证据，不重复消耗激活序号；
- 在权威采样前拒绝 ID 冲突、第二意图和 Run 错配；
- 统一取消、Host teardown 与控制器清理；
- 完成改动驱动回归、Report/Log 提交和 GitHub 推送。

## 2. 基线与范围

- 基线：`b6becab5c254a68c964ffb270b7234231e4a3277`（P27.0）；
- 分支：`agent/0.0.10-p27-1-formation-product-controller`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不改阵法定义、材料数量、影响数值、物品 schema、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. Formation Intent

新增 `Fdemo_mapShanmenFormationIntent`：

1. 捕获调用者生成的 IntentId 与当前 RunId；
2. 复用已验证的不可变阵图定义；
3. 验证有限原点和前向，把前向投影到 XY 平面后归一化；
4. 纯垂直或无效值在权威采样前失败；
5. `Matches()` 深度比较阵图、锚点、材料需求、位姿与身份，防止相同 ID 别名到不同载荷。

## 4. Run 级产品控制器

新增 `Fdemo_mapShanmenFormationProductController`：

- `TryBegin()` 绑定一个精确 Run，活动控制器不能改绑；
- 首个意图通过 P27.0 ProductAuthority 冻结命令并启动既有 ProductHost；
- 精确重试复用冻结 preparation/start/active/begin 证据，不再次调用权威；
- 同 ID 异载荷返回 `IntentIdConflict`，第二 ID 返回 `HostBusy`；
- Host 首次启动失败时保留已预留命令，精确重试不消耗新序号；
- `TryCancelAndEnd()` 统一取消、世界 teardown 与状态清理；
- 只暴露 const ProductHost 和 const 冻结命令。

## 5. 产品测试

在既有真实物品权威 fixture 中新增两项：

- `FrozenReplayAndEnd`：首次启动、平面归一化、单序号、重放相同证据、物品 snapshot 不变、
  Host teardown 和 CombatRun 结束；
- `Fences`：无效前向、Run 错配、IntentId 冲突、HostBusy、Run 改绑拒绝及最终清理。

专项结果：2 Success / 0 Fail；SHA-256
`D6F945CAB8E70412AC4F2850C3AF463E9D74BE0225D4ABC9E61874132CAEA649`。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `D6F945CAB8E70412AC4F2850C3AF463E9D74BE0225D4ABC9E61874132CAEA649` |
| `Shanmen.0_0_10` | 1,324 | 0 | `D4F75FEE4F50B996ACBF58AE4051CC5714A004B4C89C13C7F7B11C78C61CAC83` |
| `demo_map.V3.Attributes` | 4 | 0 | `288316204410A30C18EA5EE932BD6E4E8DFA4FE379E28119921AAB5BFE4D4110` |

每份最终日志均须有至少一个 Success、0 Fail、0 Fatal/Unhandled/Ensure、终端队列完成标记和进程
退出码 0。

## 7. 改动驱动覆盖

新增 `FormationProductController` 映射规则，要求完整 0.0.10、focused controller、P27.0 authority、
CombatRun、Formation Host/Session/material adapter、Items、FormationDeployment 与 CombatCore。修改既有
Host 测试文件同时触发其历史依赖组。

最终门禁：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=38 Logs=3`。

- 覆盖日志 SHA-256：
  `AA606A003E38A3ACF4B23DC68CC9C55DFE901C990FD810CFE51D4070A551EAEA`；
- 映射器自测：`461/461` PASS；SHA-256
  `C9E4BDF084B77F0CAD4D55B6CC678589904C273AD54EBB735BCAF0677054C665`。

## 8. 构建与静态检查

| Target | Result | Duration | SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 17.74s | `1E676731323D05A395BE61108A67807E12483D9B9E1A78B57C2752E3EB339A14` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.92s | `08659575C02ECE3439852ABE1969FF1E0CFA61371F90F816979CC41F4C9237EC` |

最终二进制：

- `demo_map.exe`：360,028,672 bytes；SHA-256
  `3C49E6320D075DC631625EBF9DDC5DD178A5B5656ABEC53DFEF943B5A4E318EC`；
- `UnrealEditor-demo_map.dll`：19,352,064 bytes；SHA-256
  `3863684BD450974F797FC0A3072E05FFDC39F9ECB8D9969DB3A527B44F6454E3`。

`git diff --check`、regression map JSON 解析与控制器边界扫描均 PASS。边界扫描未发现 Actor 创建/
扫描、RNG、`ApplyDamage`、输入或 UI 绑定。

## 9. 未执行项与后续边界

没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
本轮只声明 P 阶段控制器与幂等产品契约成立，不声明阵法已经通过玩家输入进入可见世界。后续必须
复用这一唯一控制器，并把产品生命周期、输入采样和世界锚点操作作为显式边界逐步连接。

原始证据位于 `Saved/Codex/P27.1`，不进入 Git。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationProductController.h`
2. `Source/demo_map/demo_mapShanmenFormationProductController.cpp`
3. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.1.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.1.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-1-formation-product-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-1-formation-product-controller/Docs/Report/Dev.D.UE.0.0.10.P27.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-1-formation-product-controller/Docs/Log/Dev.D.UE.0.0.10.P27.1.r0_log.md>
