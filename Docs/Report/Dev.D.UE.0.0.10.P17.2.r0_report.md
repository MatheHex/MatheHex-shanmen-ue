# Dev.D.UE.0.0.10.P17.2.r0 Report

## 1. 结论

P17.2 在 P 阶段边界内完成，结论为 **PASS**。

本轮关闭护心镜的终局结算缺口，并证明它完全服从既有通用物品生命周期：唯一 charge 在致死拦截时从 1 扣至 0；撤离把同一实例安全带回仓库，但保留 `Charges=0`；进程重启不会依据 Definition 的默认值补充 charge；同一实例可在后续 Run 再次部署，但零 charge 不会生成伪防御层；死亡结算把该实例转换为 `Destroyed` 零值墓碑，另一次重启后终态仍保持且不再进入准备栏投影。

本轮没有发现生产缺陷，因此没有修改生产代码、内容、资源、地图或配置；只新增一条覆盖撤离、重启、再部署、死亡和再次重启的生命周期 Automation。

最终结果：

```text
Heart Mirror settlement lifecycle:    1 Success / 0 Fail
Shanmen.0_0_10.Items:                77 Success / 0 Fail
Shanmen.0_0_10.CombatCore:            9 Success / 0 Fail
Shanmen.0_0_10 full:                751 Success / 0 Fail
Regression coverage:                  PASS (Changed=1 / Rules=1 / Required=2 / Logs=2)
Regression gate self-test:             PASS 273/273
Game + Editor Development:             PASS / native status 0
```

## 2. 既有结算语义

P17.2 没有为护心镜发明专用结算规则。既有物品权威已经规定：

- `Extraction` 把已部署并成功带出的同一实例转回 `Stored`，保留它当时的资源值；
- `Death` 把未带出的已部署实例转为 `Destroyed`，并把 Quantity、Durability 与 Charges 清零；
- Profile projection 只展示可准备的实例，Destroyed 墓碑只留在权威快照中用于审计与防止身份复用；
- Definition 的默认资源值只参与新实例建立，不能覆盖持久化实例的当前值。

测试将护心镜接入这条已存在的 generic settlement 路径，证明它没有形成饰品特判或第二套恢复逻辑。

## 3. 生命周期纵切

新增测试从 fresh Profile 与正式护心镜 Definition 开始，经 cutover、准备栏和 `StartPreparedRun` 建立真实 ShanmenItems authority。第一次 Run 中，玩家以 3 点生命承受 5 点致死伤害；既有资源防御协调器触发 `PreventLethal`，最终生命为 1，镜 charge 从 1 变为 0。

随后按以下顺序执行：

```text
fresh Profile + Heart Mirror charge 1
  -> prepared Run 1 / Deployed
  -> lethal Impact / vitality 3 -> 1 / charge 1 -> 0
  -> Extraction settlement / Stored / charge 0
  -> authority restart / Stored / charge 0
  -> reselect exact ItemInstanceId
  -> prepared Run 2 / Deployed / charge 0
  -> defense preparation returns ResourceUnavailable
  -> Death settlement / Destroyed tombstone / all resources 0
  -> authority restart / terminal tombstone preserved
```

该链没有直接写物品资源、手工重建实例或调用测试专用补充接口。

## 4. 状态转移与投影

| 阶段 | 实例状态 | Quantity | Durability | Charges | 准备栏投影 |
|---|---|---:|---:|---:|---|
| Run 1 初始 | `Deployed` | 保持 | 保持 | 1 | active Run correlation |
| 致死拦截后 | `Deployed` | 保持 | 保持 | 0 | active Run correlation |
| 撤离结算后 | `Stored` | 保持 | 保持 | 0 | 存在，`CHG 0/1` |
| 第一次重启后 | `Stored` | 保持 | 保持 | 0 | 存在，当前值仍为 0 |
| Run 2 再部署 | `Deployed` | 保持 | 保持 | 0 | active Run correlation |
| 死亡结算后 | `Destroyed` | 0 | 0 | 0 | 不存在 |
| 第二次重启后 | `Destroyed` | 0 | 0 | 0 | 不存在 |

撤离后的 `Fdemo_mapProfilePreparationStashRow` 明确携带 `Charges=0 / MaxCharges=1`，Presenter 输出 `CHG 0/1`。这证明 UI/read-model 可以区分“已耗尽但仍持有”和“新建满 charge”，而不会把 0 当作缺省值。

## 5. 身份、再部署与失败关闭

撤离、重启和第二次部署始终使用同一个 `HeartMirrorId`。第二个 Run 的 `AccessoryItemInstanceId` 必须等于该 ID，且 RunId 必须与第一个 Run 不同，因此测试不会以创建一件新镜子掩盖资源恢复错误。

在第二个 Run 中，`PrepareImpactDefense` 对已识别但零 charge 的护心镜返回成功处理状态 `ResourceUnavailable`：

- 不生成资源防御层；
- 不创建 reservation；
- 不向 `FShanmenDefenseSnapshot` 添加任何 layer；
- 不把 Definition 的 `MaxCharges=1` 当作当前 charge。

死亡结算后，墓碑保留 exact identity，但准备栏投影不再包含它。第二次重启同时验证终态不能被自动复活或重新选择。

## 6. Automation 覆盖与证据

新增：

- `Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorSettlementLifecycle`

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Heart Mirror settlement exact | 1 | 0 | `8967F2F86D801A46D5867E2DB14A805316C8CA69D246BABE8AEC1FD34B88C040` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `BFB936C23A3F32744C8D52847FC1A0B78CBADCA60AD1B481CE2396A588554735` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `05D40B22D0D8BDADAAEECFC0BF214A9E7454819ECE50E40E8A4DBE9333E8AEAE` |
| `Shanmen.0_0_10` full | 751 | 0 | `6311EF61B0D44F4F94160162B72817E243A0190DAEEFF4C20E4F92281D00F9DF` |

全部最终日志具有 `**** TEST COMPLETE. EXIT CODE: 0 ****`，Fail、Fatal 与 Ensure 计数均为 0。完整套件由 P17.1 的 750 增至 751；首末 Success 时间为 `2026.09.02 18:41:41.673 -> 19:09:14.493 UTC`，约 27m32.82s。

## 7. 执行观察与修正

新增测试首次真实执行即为 1/0，没有产品或断言失败。

在测试前的一次启动尝试中，`-log=<absolute path>` 只完成 UE 平台校验且没有生成 Automation 日志，因此没有把该尝试描述成测试执行或失败。将日志参数改为 UE 支持的 `-abslog=<absolute path>` 并启用 stdout 后，exact 测试正常运行并产生完整原生终止证据。该修正只涉及验证命令，不涉及源码、系统权限或产品行为。

## 8. 门禁与构建

本轮唯一修改源码路径为 `Source/demo_map/demo_mapShanmenDefenseResourceAdapterTests.cpp`。回归映射要求 `Items` 与 `CombatCore` 两组，实际证据完整覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=2 Logs=2
SELF_TEST: PASS 273/273
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 3 / 23.92s | `190357550AEFE087CB388E594F9877B08BB5B8ECFF84BAE78C90D247BF284C46` |
| Editor Development | Succeeded / up to date | 0 / 0.96s | `C22C3C62EC63149208403724059403A7784706EA3C73E40C9E599606A3AC2779` |

最终产物：

- `demo_map.exe`：356,155,904 bytes，SHA-256 `7EDB2C5BF8CBF0837123D2C862D2F2020CE983C90EE517C72FA024090E5531F4`；
- `UnrealEditor-demo_map.dll`：14,848,000 bytes，SHA-256 `4648E539FDF4E0FBE6A422B55842C6B80D0391D917FBD9A033673C3E60D5DA58`。

## 9. 修改范围与 P/F 边界

源码只修改一个 Automation 测试文件，并新增本 Report 与 Development Log，共计划提交 3 个文件。生产 C++、Content、地图、资源、配置、Windows、UE Engine 与用户设置均未修改。

本轮只执行 unattended、NullRHI 的 P 阶段 Automation、静态门禁与 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，也没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P17.2`。

## 10. 下一阶段

P17 的护心镜纵切至此完整闭合：定义、准备、致死触发、资源提交、产品攻击、重放、耗尽、撤离、再部署、死亡与跨进程终态均已有证据。后续不应在没有正式设计的情况下添加自动充能、修理、商店补充或一次性销毁例外。

下一 heartbeat 应先对照人工 0.0.10 战斗规划与现有 P0-P17 纵切，选择仍未闭合且语义已经明确的最小产品目标作为 P18；优先补真实产品接线或改动文件回归缺口，不继续为已闭合的护心镜堆叠 wrapper。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P17.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P17.2.r0_log.md>
