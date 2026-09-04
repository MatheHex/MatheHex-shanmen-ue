# Dev.D.UE.0.0.10.P20.7.r0 Report

## 1. 结论

P20.7 已把 P20.6 的 Straight/BallisticArc ProductSession 能力接入既有 `Fdemo_mapShanmenThrownWeaponProductLifecycle`。

生命周期现在可以显式选择 Straight 或 BallisticArc，但调用方只能选择轨迹种类，不能注入伤害、速度、标签、重力或飞行时限。真实 `TrainingThrowingKnife` 的共享战斗内容与 Arc policy 全部由生命周期从规范内容身份构造，再交给同一个 Session、Controller、RunCommand Router、RunHost 与物品事务执行。

旧 `TryBegin` 与旧 content capture 保持 Straight 兼容。显式 Straight 与兼容入口幂等；已激活的 Straight 生命周期不能切换为 Arc，反之也由 Session config identity 失败关闭。

新增自动化证明 Arc 可从生命周期完成 hotbar → Run item → Action identity → planner → command → Host 的成功链路，也证明不可达 Arc 只产生可重放的确定性拒绝，不扣物品、不生成 Actor、没有隐藏恢复工作。

本轮没有修改 InputAdapter、玩家手势、UI 或预览。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`3a25065e4c3bd5cb7deb28796064ab30ea0c23d7`（P20.6）；
- 分支：`agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 生命周期内容所有权

新增的 typed content capture 只接受 `Straight` 或 `BallisticArc`。无效/未指定轨迹会清空输出并失败关闭。

两种轨迹共享同一真实物品语义与战斗内容：

- item definition：`TrainingThrowingKnife`；
- formula：`Combat.Formula.ThrownWeapon.TrainingThrowingKnife.r1`；
- base damage：`12.0`；
- technique coefficient：`0.3`；
- launch speed：`900.0`；
- damage tag：`Damage.Physical.Slash`；
- required target tag：`Target.Living`；
- reject self：启用。

Straight 使用规范 Straight action definition 与 `.Straight` detector。Arc 使用规范 Arc action definition 与 `.Arc` detector，并由生命周期冻结 `Intermediate / gravity 980 / maximum flight time 4.0` policy。

因此输入层未来只能提供选择几何，不会成为第二个平衡参数权威。

## 4. 类型化生命周期绑定

新增 typed `TryBegin` 在进入 Session 前继续执行既有权威检查：

1. active lifecycle 不能切换 item authority；
2. CombatRunCoordinator 必须 ready；
3. ShanmenItems active Run 必须与 Coordinator Run 相同；
4. source Actor 必须解析为该 Run 的规范玩家实体；
5. 生命周期按请求轨迹构造规范 SessionConfig；
6. Session 以完整 config identity 执行首次绑定或精确幂等复用。

旧 `TryBegin` 只委托 typed Straight 入口，没有复制第二套绑定逻辑。生命周期还提供只读 command audit 透传，用于检查已冻结请求；它不允许调用方改写命令或绕过 Session。

## 5. Arc 成功与重放

生命周期 Arc 测试使用 hotbar slot 2 的真实 `TrainingThrowingKnife`，验证：

- RunId、item instance 与第一次 activation sequence 完全一致；
- command 使用 Arc action definition；
- origin、target、apex clearance 来自 immutable selection；
- technique tier、gravity、最大速度与飞行时限来自生命周期 policy；
- Host 进入 `InFlight`，物品权威 revision 只按既有 Prepare/Commit 增加两次；
- 同 SelectionId、同几何精确重放同一个 activation 与 projectile Actor；
- 重放不再次读取/修改物品，不分配新 sequence，不生成新 Actor；
- 同 SelectionId、异 target 以 `SelectionIdConflict` 失败关闭；
- `TryEnd` 由生命周期拥有中断与空状态收束。

## 6. Arc 规划拒绝

对超出可行包络的 Arc，生命周期保留一次 Run-owned Action identity，并返回 `ArcPlanRejected`：

- authority snapshot 前后完全相同；
- 不生成 RunCommand；
- Host 保持 Empty；
- selection ledger 只记录一个确定性拒绝；
- 精确重试复用原 activation id/sequence；
- 冲突几何不消耗新 sequence；
- 因为没有隐藏 durable recovery，生命周期可正常结束。

## 7. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `product_lifecycle_final.log` | `Product.ThrownWeaponProductLifecycle` | 5/0 | `08C3573664C82BF4EADBF3EF850CA0EBEC42C06A3D489F61B970521AEF44C544` |
| `product_session_final.log` | `Product.ThrownWeaponProductSession` | 6/0 | `917A468D655FC8E4534DB18D977707D282AE2A8FE416012D5E10B99B095DF6B4` |
| `product_controller_final.log` | `Product.ThrownWeaponProductController` | 7/0 | `32FE583484E92ED75AC78422E47DC0DB51646C13BAB175171B59FBF12DC3093C` |
| `run_command_final.log` | `Product.ThrownWeaponRunCommand` | 6/0 | `BDB7742C29118B0D90A6969D782DFC23241F8FBB315C7BBE3D08A6B69DDDD8C0` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 858/0 | `DEE41552ADCE2573BBC77C8BD21E6DD0EBF8426DB460448559207012B91ABBAD` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `297AFA6C646C8565C0D13009B0C108EFB8AAE39F42A1521B819579DC7B556BF0` |

每份有效日志都只有一个 canonical `RunTests` command、一个 native success terminal、至少一个成功结果、0 Fail，并且无 Fatal/Unhandled/Ensure：`PASS Logs=6 RecordedSuccess=928`。审计 SHA-256：`BB986130781264F6276C6B3D98301CB29C9F0FE7BC5396C8658AF4929258B093`。

0.0.10 全量从 P20.6 的 856 增至 858。首次 lifecycle exact 也为 5/0。

## 8. 失败证据透明度

首次批量启动 RunCommand exact 时，人工编排命令误写为不存在的 `Shanmen.0_0_10.ThrownWeapon.RunCommand`。UE 找到 0 个测试并以 native `255` 退出；该轮没有被计为成功。

原始失败日志保留为 `run_command_invalid_group.log`，SHA-256 `E3B41B2BFD9CA585B9C0F222DAB993ABC1A5F56E66B84794FBF0B3CF30241B54`。修正为真实 `Shanmen.0_0_10.Product.ThrownWeaponRunCommand` 后独立重跑，得到 6/0 与 native 0。源码、构建和产品测试没有发生失败。

## 9. 改动门禁与静态边界

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=12 Logs=6
```

Lifecycle 三个改动文件命中 `ThrownWeaponProductLifecycle` 映射。12 个必跑组全部由 lifecycle/session/controller/command exact、0.0.10 全量与旧物品/护甲日志覆盖；gate SHA-256：`1F30926A9126370E37125C19F9F2097678248007DED9DB5E9E2512A817D3D72F`。

对两个 production 文件的新增行扫描 direct damage、direct inventory mutation、spawn、RNG、trace/sweep、input binding、PlayerController 与 GameplayStatics：`PASS Files=2 AddedLines=85 Matches=0`；SHA-256：`49C732E29FB08D3B06C254DBE8C1CAA9C6E62AD69523660399CD8BC4CF424950`。

`git diff --cached --check`：仅暂存本轮 5 个文件后 PASS / native 0；证据 SHA-256：`65588A84A5DA0F288C80FF939ED672A855BE11AB11ED5EDE3E0B0A567E1072AC`。长期未跟踪文件未被纳入。

## 10. 构建与产物

- initial Editor：29 actions / native 0，SHA-256 `21DCCAEE47502993A96470943B2B2C43815B499F337A38D24BCBC6FCC87E544F`；
- final Editor：up to date / 0 actions / native 0，SHA-256 `D5411F056750DB7E412EAC0CF0B4B43002F18AB57611A462B348DA90C80F3A08`；
- final Game：28 actions / native 0，SHA-256 `4C742CE362CA251D5CBBE54023099446C6B8D301E24D625AA78552EF8A4A8A0E`。

产物：

- `Binaries/Win64/demo_map.exe`：357,175,808 bytes，SHA-256 `80807465617897082CE0379DC7958CB05A76E7DD5784858D1BC1B5234451466E`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,837,184 bytes，SHA-256 `5AD25F91034C3FA44856E146145E76F9ECDC6E665418C6E7A877A861894BFD9A`。

## 11. P/F 边界与下一步

P20.7 证明的是“生命周期能够用规范内容显式绑定 Straight 或 Arc，并完整复用唯一既有执行权威”。它没有证明玩家当前可以选择 Arc；现有 InputAdapter 仍只采样 aim direction 并调用 Straight 兼容入口。

建议 P20.8 扩展既有 InputAdapter，新增平台无关的 typed Arc route：一次采样 target 与 apex clearance，生成 Arc hotbar intent，并显式请求生命周期 Arc 绑定；保留当前 Straight 兼容路径。该阶段继续不接按键映射、鼠标手势、UI 或预览，先把输入采样次数、重放与失败关闭契约稳定下来。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P20.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-7-thrown-weapon-arc-product-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P20.7.r0_log.md>
