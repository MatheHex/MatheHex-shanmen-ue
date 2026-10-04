# 山门 Demo2.0 首个战斗切片 Development Log

日期：2026-10-04。基于提交 `4288d27a2417ace8637cf7975b47db4e0e1e121b`，在原分支开发新入口，未切换到落后且有用户修改的 playable-ui worktree。

## 实现过程

1. 读取当前 Git、P28.37 报告和当前框架，记录 113 项原文件/存档哈希。未删除旧 Demo 材料，改为独立目录和入口不复用它们。
2. 新建 Demo20 Session、World、Widget、3 项自动化，以及地图生成、本地运行、回归脚本。
3. 地图生成使用新的空 level、原生 GameMode 与 PlayerStart；不打开或复制旧地图。新地图生成进程原生退出 0，文件大小 11,440 字节。这是磁盘文件大小，不是运行内存。
4. 首次编译因 UE 局部成员遮蔽规则与 TObjectPtr 推导失败，Editor 原生 6，Game 尚未开始。修正后双目标成功。
5. 首次自动化 22 Success / 3 Fail；缺少 BasicSword.RequiredTargetTags。补齐后第二轮 24 Success / 1 Fail；缺少 DefenseLayer.LayerTags。继续补齐后专项 25 Success / 0 Fail，结束队列 25，原生 0。
6. 专项使用 `+` 组合表达式，现有回归映射工具不能将其视作单个原生测试组的覆盖证据，因此不放宽检查器，改用单个 `Shanmen` 根重新验证。
7. 首次可见窗口实际加载 Demo20；发现按钮自动换行和把 UMG 焦点交接误认为失焦。关闭本轮窗口，修正按钮不换行、先刷新界面后切换输入、按 Slate 应用激活状态识别失焦，再重新构建。
8. 扩展 `Shanmen` 根运行约 11 分 22 秒时仍停留在旧 SwordRhythm 产品测试序列；已记录 915 Success / 0 Fail，但无队列结束。核对唯一证据路径与进程后主动停止该无头进程，原生 -1、外层 1。它不是完整根回归通过，也不是源码失败。最终改为五个单组日志运行并交给原覆盖检查器，不修改通过条件。
9. 实际场景发现默认材质不支持 `Color` 参数，生成独立新材质并接入角色和场景；同时令预警圆直径匹配 260cm 攻击半径。最终 Editor/Game 再次构建原生 0。
10. 最终窗口确认颜色、入口、空击提示、闪身位移、归阵拒绝、石卫预警、受击死亡、返回入口与第二局 100 生命、暂停。存六张原始窗口截图，不冒充完整键鼠通关或双分辨率验收。Alt+F4 正常关闭，运行日志含对象子系统关闭与 LogExit；异步启动没有回收原生退出码，不捏造为 0。

## 首次失败与修正后日志

以下路径均相对于工程根 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，原件本地保留。

| 证据 | 本地路径 | SHA-256 |
| --- | --- | --- |
| 首次 Editor 构建失败，原生 6 | `Saved/FoundationRuns/Demo20.S01/BuildEditor/20261004T050233624Z-0636a3b3/stdout.log` | `73C0D23B22E47FB8E6F2B09158A192F401155246EB2DAC67501F3F91ACAA4360` |
| 首次测试 22/3 | `Saved/FoundationRuns/Demo20.S01/Automation/20261004T050700196Z-719169df/UnrealEditor.log` | `FE7C52D7184EB73CDEAD937A5C1DAFBCF10531A04C88CB601437B5891C966964` |
| 第二次测试 24/1 | `Saved/FoundationRuns/Demo20.S01/Automation/20261004T051213766Z-c876bd58/UnrealEditor.log` | `88C580D5DDE546031CFBFA02DF107281068978F19FBB6D2ADB6A81BE4395D5E7` |
| 修正后专项 25/0 | `Saved/FoundationRuns/Demo20.S01/Automation/20261004T051404119Z-5b4ccd7f/UnrealEditor.log` | `5474E18841C656B32B00538B6113ADF009A08A3D8BC500DB0BF94FAA27EBB331` |
| Fix4 Editor，原生 0 | `Saved/FoundationRuns/Demo20.S01-Fix4/BuildEditor/20261004T051641203Z-be7ab189/stdout.log` | `F5DB5BF769FD03C156A42E7A080A75E80C697797E52C4AC8790E9C00ED1F1DD9` |
| Fix4 Game，原生 0 | `Saved/FoundationRuns/Demo20.S01-Fix4/BuildGame/20261004T051656945Z-f0a5754b/stdout.log` | `46108FB7C0B5BEE89C6CCDE30879AFB86ECFCF5A77FDBCC2E0A94A6D9ED87116` |

测试失败时 UE 原生退出仍为 0；外层脚本因 Fail 结果退出 1。日志中进程成功不等于测试通过，未改写原始退出码。

## 本轮最终证据

最终五组共 25 Success / 0 Fail；每组队列正常结束、原生退出 0。`Test-Demo20.ps1` 外层退出 0，覆盖门结果 `PASS Changed=9 Rules=1 Required=5 Logs=5`。自测 555/555，差异检查通过，113 项原文件保护哈希未改变。

下表路径均相对于工程根；前五行为最终自动化。原始日志留本地，GitHub 保存本开发日志中的索引和哈希，不伪称原始日志已上传。

| 证据 / 数量 | 本地路径 | SHA-256 |
| --- | --- | --- |
| Demo20，3/0 | `Saved/FoundationRuns/Demo20.S01/Automation-Shanmen.Demo20/20261004T054144473Z-6d9fd37a/UnrealEditor.log` | `3E1B045F89EE33DFD21534E7A915A4F82CE725425C4943DF8F29EDB3960DD3C2` |
| CombatCore，9/0 | `Saved/FoundationRuns/Demo20.S01/Automation-Shanmen.0_0_10.CombatCore/20261004T054216090Z-a32fb180/UnrealEditor.log` | `0359236E5130C1FA9A91B927AB47C1D061B668732D37191A57BAE8E355907FFE` |
| BasicSword，4/0 | `Saved/FoundationRuns/Demo20.S01/Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T054251570Z-5b6f30ca/UnrealEditor.log` | `B97B2F21DE0346B7D58412F7D2E1D6615D84E80E5A9D0895AADF987D114162C0` |
| VitalityAuthority，5/0 | `Saved/FoundationRuns/Demo20.S01/Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T054322902Z-7c682709/UnrealEditor.log` | `DE5F6BED142CB0256C78CA47BA74AF159BFEA066FAE4BC07D7DC77B9847F89A0` |
| VitalityLedger，4/0 | `Saved/FoundationRuns/Demo20.S01/Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T054354303Z-3ad0c5a6/UnrealEditor.log` | `02F43F587154585CEDDB6536350192A5379880BBA9188A2E02B82C6034B7E5E6` |
| 最终 Editor，原生 0 | `Saved/FoundationRuns/Demo20.S01-Final/BuildEditor/20261004T053342377Z-6ba13686/stdout.log` | `9B93CC096BC22AC111B1AC7E30A3A081FD8FD078AE7100157196F2D0893EAD37` |
| 最终 Game，原生 0 | `Saved/FoundationRuns/Demo20.S01-Final/BuildGame/20261004T053358009Z-777888e5/stdout.log` | `B2D6AA8D73F7AC73D98159CA63F38DEB5DE9ED55A2FB2C72B77133FC6E946A5D` |
| 中断扩展根，不通过 | `Saved/FoundationRuns/Demo20.S01/Automation/20261004T051751325Z-00020719/UnrealEditor.log` | `76344C0AAE77B71EB415C32F71ACA41DD8EAB144E2D20E9358A9A3673B61306A` |
| 映射自测 555/555 | `Saved/Automation/Demo20/RegressionSelfTest.log` | `61FD7001F1CA6252BA8E323C0155343B6EA4B187E1A893312E64DBB19BE60385` |
| 最终实际窗口运行 | `Saved/FoundationRuns/Demo20.S01/Play/20261004T053442638Z-61b9ef49/UnrealEditor.log` | `4B2E07518B890339FC8CB4E1617EB428606EB886EA1D12AE37176A6CE4BB6268` |

## 实际验证边界

截图位于 `Docs/Demo20/Evidence`：entry、arena、telegraph、defeat、restarted、pause。验证了局部真实操作和死亡重开路径，未完成真实键鼠全清撤出、长按移动/格挡、PIE、独立双分辨率或性能/释放趋势验收。没有 Cook/Package，也没有把磁盘资产大小当运行内存。当前交付为可启动的首个战斗切片，后续应先收集实际操作反馈。
