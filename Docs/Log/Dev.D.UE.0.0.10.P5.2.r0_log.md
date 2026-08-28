# Dev.D.UE.0.0.10.P5.2.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P5.2.r0`；
- 基线提交：`8c3f5d948641b6d81fe3d66cb0dd089f66a69928`（P5.1）；
- 当前分支：`agent/0.0.10-p5-2-defense-resource-commit`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口时间：`2026-08-28T17:30:08Z`。

## 目标

关闭 P0.1 receipt 与 P1 Items 之间的一个明确缺口：资源型防御层已经能输出 `bRequiresCommit + LayerId + SourceInstanceId`，底层 Items 也能 Reserve／Commit Durability 与 Charges，但此前没有 ActiveRun 关联的原子批量提交命令。

本轮只建立提交点与单向产品适配面，不增加防御装备内容，不改变现有 M01 数值，也不在没有跨权威恢复协议时接入真实产品伤害调用点。

## Repository 设计

`CommitPreparedRunResources` 使用一个有序请求提交同一 Impact 的全部资源行。

预检顺序：

1. exact replay；
2. request/content；
3. ActiveRun claim 存在且未 finalize；
4. claim 的 DeploymentLock 重建精确部署物品集合；
5. 每个资源 reservation 存在、仍 Reserved、类型为 Durability/Charges；
6. source item、deployment reservation、owner、scope 一致；
7. reserve command 成功且 authority revision 晚于 claim。

全部预检后才复制 State。每条 `ApplyCommit` 只修改 Candidate；全部成功后统一增加一次 AuthorityRevision、写一个批量 receipt 并执行完整 `ValidateState`。任何失败都不会移动 Candidate。

Exact replay 位于 finalize 检查之前。批量 receipt 的 ReservationId 保存 ActiveRunId，ReservationIds 保存 resolver 顺序，ItemInstanceId 沿用既有 aggregate receipt 约定保存 RequestId。

## 持久服务

AuthorityService 在既有 mutex 内通过 `ExecuteCommandLocked` 执行新 repository 命令。Subsystem 只在 Game Thread 且 lifecycle 为 Ready 时转发，并复用 `SynchronizeCommandState`。

故障注入验证 `WriteTemp` 失败后 snapshot 完全等于调用前；解除注入后同一请求只增加一个 authority revision 和一个 save generation；重启后返回 `Replayed` 且不再次磨损。

## 产品适配面

`BuildCommitRequest` 只接受 accepted、conserved Impact。它保持 triggered layer 顺序，把 LayerId／SourceInstanceId 转为 reservation／item identity，并限制来源必须是 correlation 中的 weapon、armor、accessory、spatial ring 或 backpack。

RequestId 命名空间为 `Shanmen.Product.DefenseResourceCommit.Request.r1`，canonical parts 为 owner、scope、ActiveRun、ImpactId、触发行数及各行身份。

`CommitTriggered` 能读取唯一 active correlation 与 authority content 后调用 durable API；当前代码库无外部调用点。P5.3 之前保留该状态，避免在缺少 crash-recovery intent 时形成“生命已提交但资源未提交”或相反的窗口。

## 实现与测试过程

### Editor 编译

- 首次完整编译：`32/32`，Succeeded，退出码 `0`，`116.50s`；
- test-only assertion 修正后：`4/4`，Succeeded，退出码 `0`，`5.26s`。

### 首次失败

前两次 Items 进程均退出 `1`。平台启动输出包含 LinuxArm64／VisionOS `MainVersion` 警告，但 retained UE 日志显示真正退出原因是新增测试第 113 行：

```text
Assertion failed: Addr < GetData() || Addr >= (GetData() + ArrayMax)
Attempting to use a container element which already comes from the container being modified
```

测试直接执行 `Array.Add(Array[0])`，触发 UE 容器自引用保护。修正为先复制 `FShanmenDefenseLayerResult` 再 Add；产品代码不变。

UE 自动保留的失败日志：

- `Dev.D.UE.0.0.10.P5.2.r0_items-backup-2026.08.28-17.19.18.log`，SHA `1BED9BE1196CB3B0371FA27EA484C737E023AB621FE2454B25BE45469A3A2927`；
- `Dev.D.UE.0.0.10.P5.2.r0_items-backup-2026.08.28-17.19.53.log`，SHA `1A3E58AD7C1308233004CD73CD16C55FCE18281D2DB00EB9A05F903017916619`。

两份 crash context 均精确指向 `demo_mapShanmenDefenseResourceAdapterTests.cpp:113`。

### 证据格式修正

修正源码后，首轮四组 Automation 实际完成 `125 Success / 0 Fail`、进程退出码 `0`；但 `ExecCmds` 使用 `Automation RunTests <group>;Quit`，使门禁读取的 group 带 `;Quit`，且没有正式 `Automation Test Queue Empty <n> tests performed` 收口。这四份日志被门禁拒绝，不作为最终通过证据。

最终命令移除显式 Quit：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

## 最终自动化

| 日志 | 组 | Success | Fail | Queue | Fatal | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `Dev.D.UE.0.0.10.P5.2.r0_items.log` | `Shanmen.0_0_10.Items` | 63 | 0 | 1 | 0 | `630E5C67C73DDFDC2E76A6A1315850EEB07E976A7857C06FDA32AF879DCA0CFA` |
| `Dev.D.UE.0.0.10.P5.2.r0_combatcore.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | 0 | `0E429A99B2E0CF3808C9EB3DFC0956514A9F72AA1E536E314E62D06F6CF74957` |
| `Dev.D.UE.0.0.10.P5.2.r0_itemusearmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | 0 | `D61E55AE808507E26693E9CD750D1A1C3E9E58950F24A408EE7EE5BE76607F48` |
| `Dev.D.UE.0.0.10.P5.2.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | 0 | `4C64B2386BE94D6EE6910AD4CFB745D17810FA179AE29D1D12E72C0C657F915A` |

总计 `125 Success / 0 Fail`，全部原生退出码 `0`。

## Changed-file gate

- self-test：`SELF_TEST: PASS 7/7`；
- final gate：`REGRESSION_COVERAGE: PASS Changed=14 Rules=3 Required=4 Logs=4`；
- required：`Shanmen.0_0_10.Items`、`Shanmen.0_0_10.CombatCore`、`demo_map.ItemUseAndArmor`、`demo_map.P4.Hotbar`。

映射新增 `DefenseResourceAdapter` 规则；ShanmenItems 与既有 ItemProductAdapters 规则继续提供其它三个要求组。

## Game 构建

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `29/29`，Succeeded；
- 原生退出码 `0`；
- 总耗时 `102.54s`；
- 输出 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与边界

- `git diff --cached --check`：退出码 `0`；
- staged Source/Scripts：`14 files, 1105 insertions, 5 deletions`；
- ShanmenItems 中 demo_map include、`UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`、`FMath::Rand`、`FRandomStream`：`0`；
- DefenseResourceAdapter 中 Code A／Code B、存档写、伤害 API、运行时 RNG：`0`；
- `CommitTriggered` 只有 header 声明与 cpp 定义，没有产品调用点；
- 长期未跟踪的旧 Prompt、Report、自动化文档及用户文件未暂存。

## 最终不变量

1. 一次 Impact 的资源型触发行只能形成一个有序 item-authority command。
2. 资源预约必须晚于 exact ActiveRun claim。
3. source item 必须属于该 claim 的 DeploymentLock 集合。
4. 只允许 Durability 与 Charges；Quantity/DeploymentLock 拒绝。
5. 多行要么在同一 revision 全部提交，要么全部保持原状态。
6. exact retry 不受 Run 后续 finalize 影响，且不二次消耗。
7. 写盘失败不发布内存 Candidate。
8. CombatCore 保持纯函数，不依赖 Items 或产品模块。
9. 未建立跨权威恢复前，不接入真实生命提交调用点。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-2-defense-resource-commit/Docs/Report/Dev.D.UE.0.0.10.P5.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-2-defense-resource-commit/Docs/Log/Dev.D.UE.0.0.10.P5.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-2-defense-resource-commit>
