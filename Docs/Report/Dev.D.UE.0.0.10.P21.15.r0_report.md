# Dev.D.UE.0.0.10.P21.15.r0 Report

## 1. 结论

P21.15 在 P 阶段边界内完成，结论为 **PASS**。

本轮让飞剑已有的长条网格跟随同一 Activation 内相邻飞行快照的真实位移方向。仅旋转无碰撞的 `Visual` 子组件；Actor 与根碰撞体的旋转保持不变，因此玩家能辨认飞剑飞向，而阻挡、命中和权威飞行状态不受表现姿态影响。

```text
Controlled-weapon focused:                    62 Success / 0 Fail
Shanmen.0_0_10 full:                        1247 Success / 0 Fail
Changed-file regression coverage:             PASS (3 files / 3 rules / 17 groups)
Regression gate self-test:                    PASS 437/437
Game + Editor Development:                    PASS / native status 0
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。因此本轮证明方向投影、Activation 边界、碰撞隔离、真实阻挡接触回归和编译闭合，不宣称最终美术朝向、镜头可读性或玩家手感已经人工验收。

## 2. 玩家可见结果

飞剑不再始终保持生成时的默认朝向。收到同一 Activation 的下一份有效飞行快照时，已有网格以两份世界位置之差为前向：

- Directed 飞行时朝向实际出剑方向；
- Returning、Redeployed 或 Orbiting 的后续位移同样可更新朝向；
- 第一份快照没有可验证的前序位移，因此保留现有姿态；
- 新 Activation 不借用旧 Activation 的位置推导方向；
- 两份位置相同则保留上一有效姿态，避免用零向量制造旋转。

表现朝向不改变飞剑 Actor transform 的旋转，也不改变根 `UBoxComponent` 的碰撞方向。

## 3. 单一事实链

本轮不创建速度、目标或朝向的第二权威。方向只由既有 canonical flight read model 的连续世界位置派生：

```text
previous valid flight read model
  + current valid flight read model
  + exact same ActivationId
  -> non-zero world-space travel delta
  -> existing collisionless Visual world rotation
```

Actor 位置仍由现有 world lifecycle 设置；命中仍由现有根碰撞和 RunHost delivery 负责。表现层不写 flight controller、target vitality、Impact ledger 或资源事务。

## 4. 生命周期与失败关闭

`TryPresentFlightReadModel()` 在覆盖 Actor 当前快照前保存上一份只读模型，再调用 `RefreshTravelFacing()`。只有两份模型都有效、ActivationId 完全相同且位移非零时，才把 `TravelDelta.Rotation()` 写到 `Visual`。

无上一快照、无效快照、Activation 切换和零位移均直接返回。该规则故意不猜测目标方向，也不从 Actor 当前旋转、碰撞法线或目标位置建立隐含权威。

`Visual` 是根碰撞体的无碰撞子组件；本轮没有调用 Actor 旋转接口。阶段色、近身威胁色和 P21.14 committed-impact 色仍由原有 `RefreshPresentation()` 独立解析。

## 5. 测试覆盖

扩展既有 `ControlledWeaponRunHost.BlockingContactTerminal` 真实阻挡接触测试，而不是创建绕过产品链的测试 fixture：

- 敌人从默认前方改置于世界 `+Y` 方向；
- 飞剑以 `FVector::RightVector` 发射，避免默认 `+X` 朝向造成恒真测试；
- 从同一 Activation 的 Directed 与 Returning read model 计算真实位移方向；
- 断言网格前向精确跟随该 `+Y` 位移；
- 断言飞剑 Actor/根碰撞旋转与发射前完全一致；
- 既有阻挡、伤害提交、返航、材质反馈与下一 Activation 清理断言继续通过。

专项组保持 62 项，因为本轮强化现有真实端到端路径，没有复制 fixture。结果为 `62 Success / 0 Fail`。

## 6. 自动化证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon focused | 62 | 0 | 333,027 | `D20325CF1270E50E4B9821FA2241BA3D93068FEB51BDEDC0ACD6F563E2CABD50` |
| `Shanmen.0_0_10` full | 1247 | 0 | 1,902,750 | `842B13B21F98D78CE8809D596F54420789FDEE72AE25DA53FE4DB2D5D0FCD868` |

最终全量日志包含 1,247 次测试派发和 1,247 个 `Result={Success}`，`Result={Fail}` 与 Fatal 均为 0，队列自然清空，进程原生退出码为 0。首项从 `2026.09.08-19.22.41:507` 开始，末项于 `2026.09.08-20.41.25:214` 完成。

执行期间保留了一份未用于放行的诊断日志：第一次全量运行在 736/0 时人工终止，以验证 UE `-NoEOS` 是否能移除无关联网探测；该日志为 1,230,561 bytes，SHA-256 `E80AA6A58EFE48B852A28ED5DD64BF02630668BBDF115498518DF90A492E8693`。最终重跑证明 `-NoEOS` 只移除了 EOS SDK 日志，仍有 `generate_204` 探测，因此本 Report 不把它宣称为有效提速方案，也不以未完成日志替代最终证据。

## 7. 改动文件回归门

覆盖门以三个实际改动路径推导要求，而非按本轮主题手选：

- changed files：3；
- matched mapping rules：3；
- required groups：17；
- evidence logs：1 个完整 `Shanmen.0_0_10` 日志；
- result：`PASS Changed=3 Rules=3 Required=17 Logs=1`。

门禁日志为 2,033 bytes，SHA-256 `56B447358F6E8B9A7495948A539D7ED8498968217B51D808C1C850ABBEFD28F4`。门禁自测为 `PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 12 / 35.10s | 2,803 | `54021BA129637766E19CDBA57007CA1E710A5825ECF7A0B5127079A463CB511C` |
| Game Development final | Succeeded / native 0 | 11 / 59.97s | 2,786 | `F99236CC492D33DA35039D1C5022F4E05CE741A775E02AA7BF60E0B3C3F58C8A` |
| Editor Development final | Succeeded / native 0 | 0 / 1.20s | 1,021 | `0D76CEA193CDAF3525646CF7B84C69554DBF724536D755FA2629CF67F2FDCDFF` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,535,104 bytes，SHA-256 `47FAAE37377962A80DCD80E5FAC4F416BE68E5EA1716BBF3E03B2A3B157EAA0F`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,726,400 bytes，SHA-256 `A28C9A9495916DAB9E1863070DD9F28E176037D4DBE1D61B785107A662AF0A3B`。

实现/测试 diff 为 3 files、`+54 / -3`。新增生产行中 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；`git diff --check` 原生退出码 0，仅有工作树 LF→CRLF 提示；验证后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：同一 Activation 的连续非零世界位移控制已有无碰撞网格朝向；第一快照、新 Activation 和零位移失败关闭；网格前向跟随非默认 `+Y` 实际飞行方向；Actor/根碰撞旋转不变；既有阻挡、伤害、返航、阶段色、威胁色和命中反馈链保持通过；专项、全量、覆盖门、自测及双目标编译全部通过。

未声明：最终飞剑美术资产的模型轴是否与当前 Cube 原型完全一致；真实地图中俯仰/翻滚观感、镜头可读性、动画插值、轨迹、音效或玩家手感。若要证明这些内容，需要另行授权实际 UI/PIE 验收。

## 10. 提交边界与 GitHub

本阶段只提交飞剑 Actor 头/实现、现有真实阻挡接触测试、本 Report 与本 Development Log，共 5 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.15` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-15-flying-sword-travel-facing>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-15-flying-sword-travel-facing/Docs/Report/Dev.D.UE.0.0.10.P21.15.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-15-flying-sword-travel-facing/Docs/Log/Dev.D.UE.0.0.10.P21.15.r0_log.md>
