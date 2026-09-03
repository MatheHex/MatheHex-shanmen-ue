# Dev.D.UE.0.0.10.P18.5.r0 Development Log

## 1. 目标与基线

- 基线提交：`f4053b7897900be766b5e913ebd19b1e26d99a0f`（P18.4 Sword Qi Run Controller）；
- 分支：`agent/0.0.10-p18-5-sword-qi-command-adapter`；
- 目标：在不绑定真实输入的前提下，建立一次逻辑输入事件到 P18.4 唯一产品入口的薄型、可重放适配层；
- 约束：仅做 P 阶段，不创建 Input Action/Mapping Context，不读取装备/属性，不修改库存/伤害/生命，不投放正式视觉、声音或关卡内容。

## 2. 既有模式审计

审计了 `SpiritEvasionInputAdapter`、`WeaponGuardInputAdapter` 与 `ThrownWeaponInputAdapter`：前两者采用无状态 callback seam，在采样前完成 gameplay/route 门禁；后者因热栏 ordinal 生命周期而持有少量 Run 状态。

P18.5 选择无状态模式。原因是本轮不拥有物理设备，也不应猜测一次硬件事件的生命周期；稳定 InputEventId 由未来逻辑命令所有者提供。适配器只把该事件放入 active Run 命名空间，再交给 P18.4 Controller 冻结产品数据。

## 3. 新增输入契约

新增：

- `demo_mapShanmenSwordQiInputAdapter.h/.cpp`；
- `demo_mapShanmenSwordQiInputAdapterTests.cpp`。

`Fdemo_mapShanmenSwordQiInputSample` 保存有限 origin 与规范化非零 aim。`Fdemo_mapShanmenSwordQiInputResult` 明确记录是否采样、是否调用产品 route、InputEventId/RunId/IntentId、Sample、Intent、Product result 与诊断。

状态分类为：Applied、GameplayBlocked、ProductRouteUnavailable、RunUnavailable、EventIdentityInvalid、SpatialSampleRejected、IntentCaptureRejected、ProductRejected。前六个状态均可在不伪造产品成功的情况下准确定位输入链断点。

## 4. 确定性与调用纪律

`MakeIntentId` 使用 `demo_map.SwordQi.InputIntent.r1` 命名空间，并编码 RunId 与 InputEventId。空间轨迹不进入身份派生，保证同一事件若被配上不同轨迹，会触发 P18.4 的 payload conflict，而不是悄然生成另一产品动作。

`RouteStartInput` 在采样前依次验证 gameplay、产品 route、Run 与事件身份；通过后只调用一次空间采样。样本有效才捕获 immutable Intent，并只调用一次产品 callback。适配器没有重试循环，也不持有 sequence、Actor、item、attribute、inventory 或 damage authority。

## 5. GameMode 集成

`Ademo_mapGameMode::RouteSwordQiStartInput` 增加唯一组合入口。它从现有 GameMode 状态判断 Controller/Coordinator/玩家/物品/属性依赖是否一致可用，然后延迟执行 origin 与 aim callback，最后调用既有 `RouteSwordQiIntent`。

该入口没有修改 Run begin/end、occupancy、terminal retirement 或命中链；这些生命周期仍由 P18.4 Controller 与 P18.3 Session/Host 持有。没有新增 GameMode 成员或平行状态机。

## 6. Automation 与 fixture

新增四项 exact Automation：

1. `DeterministicIdentity`：同 Run/事件稳定，不同 Run 或事件分离，无效 identity 失败关闭；
2. `GatesBeforeSampling`：四类 availability fence 的 sample/route 都为 0；无效样本只读一次且 route 为 0；
3. `AppliedReplayAndConflict`：一次采样/路由、非轴向归一化、装备与属性漂移后的冻结重放、同事件异轨迹冲突；
4. `BusyRetry`：第二事件在 HostBusy 时保留冻结 command，第一发 retirement 后 exact retry 启动同一 command。

Fixture 使用 unattended GamePreview world、NullRHI、现有 ItemAuthority/AttributeComponent/CombatRunCoordinator/PlayerActionArbitration 与真实 P18.4 Controller。测试不启动 UI 或产品。

## 7. 复查修正

初始 Editor build 为 27/27，exact 4/0、full 777/0、legacy 443/0。静态复查随后发现 Result invariant 对二次规范化后的 FVector 使用 exact equality，轴向 fixture 未暴露潜在浮点误差。

修正为 `FVector::Equals`，fixture aim 改为 `(10,3,2)`，并增加 invalid spatial sample 断言。`build_editor_fix1.log` 重新编译 5 actions、15.88s，一次成功。之后 exact、完整与八组 legacy 均重新执行，旧日志被最终源码证据覆盖；没有失败日志，也没有更改产品 authority 来迁就测试。

## 8. 最终测试与门禁

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_sword_qi_input_adapter.log` | exact | 4/0 | `F6AB77E49D8DCF38065A17905659AFEA5B866EA445F3926E562C91A1461477FA` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 777/0 | `030FF8830E3E5FBCB3A9E5DF9CDB65F4D026CB1C93AE28BC61CF5ED4B6471443` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `D8E4DE6C35616A292D416E6C10B769C41CA7A302B674D0A574C95DCAD6BB2379` |
| `automation_profile.log` | legacy | 211/0 | `0F410990AB507474469B227EC9E5B5F395111350465C29BF8ECD0ABE88026C4A` |
| `automation_code_b.log` | legacy | 60/0 | `B1464D8BD764F656CB38514FD109A85557D57E7E7A4B9E18B4446019E7EE7D7C` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `9AAC574AD8EA88755D5A249D2A47CE776464630B5B3B140485CE1EDBC6A796E7` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `00B0F85E83627627DD4A56E9E0533687379F345CC0C1386C86A59268F6F4FE68` |
| `automation_v3.log` | legacy parent | 29/0 | `50BC640ED6537DE3B7867503F326DB3B6550BF29AC93111E800A9297ECDCEFD3` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `F3167D95FD35ADC1D604F641EBC82FE8EAF8851399F12383F12A7A5A601C478F` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `8D0BCA2CFE5D7F531552CDF4D8B8705A95E670C0B393E687699D40D71479FDD7` |

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=54 Logs=10
SELF_TEST: PASS 285/285
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

Coverage、自测、map、boundary SHA 分别为 `94A1818183A7B21FBE1DD0E8396254380A332A449CCF988102A12FD88F043397`、`1FCA481362D1F4B3AB04271BA030E0F7F0A964A34EF15557EE6D287F788BEB9F`、`F4493AA772FDE3EE9494C4B3CEA672029A32A02EBBE5F67E376D3C422F5F4975`、`3AE3D28D78DAA7B11F54480615608B79DF0FB3FC3832E782E85943E874C9DB63`。

## 9. 构建与产物

- Editor initial：27 actions、native 0、149.84s、SHA `55CFB0D47D99128CEB1F731125E268CBC185424A5A87C1BAD88A64B6A6B9F3E7`；
- Editor fix1：5 actions、native 0、15.88s、SHA `B45C57B00C2596C216A798FB503CBBEFE4DF0F20317B31B8189D7783265904E3`；
- Game final：26 actions、native 0、150.48s、SHA `9F2E7230C40D022F41747C03AFCA0D457EB6FDE13F7306CD7A2EEF58538EB4E9`；
- Editor final：up to date、0 actions、native 0、1.16s、SHA `9645D755F9ABDE0D6C59E50DF12A8183DB27CD0B1D4DC04FC140C8CB52DD44A1`；
- `demo_map.exe`：356,401,664 bytes、SHA `96715F6072083C58F6BF194E0214505CBAB50E5D2F562A7B4F70A011F4B7FA6A`；
- `UnrealEditor-demo_map.dll`：15,078,912 bytes、SHA `4F9DA91A767C5A373B9EE8F71878268A2AB471FA4895DEA69AB15FE977E8979B`。

## 10. 提交边界与后续

本轮计划提交 9 个文件：3 个新增源码、4 个修改文件、本 Report 与本 Development Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户资料保持未暂存；`Saved/Codex/P18.5` raw logs 不入 Git。

未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。P18.6 建议增加 Run-scoped 逻辑命令事件所有者，分配稳定 InputEventId 并调用本轮适配器，但仍不绑定真实物理按键；Enhanced Input 与正式表现保持 F 阶段边界。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-5-sword-qi-command-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-5-sword-qi-command-adapter/Docs/Report/Dev.D.UE.0.0.10.P18.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-5-sword-qi-command-adapter/Docs/Log/Dev.D.UE.0.0.10.P18.5.r0_log.md>
