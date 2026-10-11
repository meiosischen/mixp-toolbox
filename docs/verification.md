# 0.1.0 验证记录

日期：2026-10-11。源码和产物位于 `mixp-toolbox` 项目目录。

## 环境和结果

使用 BCB6 安装介质中的真正 Win32 工具，通过 macOS Wine 执行：Borland C++ 5.6、MAKE 5.2、BRCC32 5.40、ILINK32 5.60 和 BPR2MAK。没有用现代编译器替代 BCB6。

| 项目 | 结果 | 证据 |
| --- | --- | --- |
| Release 构建 | 通过 | [Release 与测试日志](verification/test-build.log) |
| Debug 构建 | 通过 | [Debug 日志](verification/debug-build.log) |
| BCB6 `.bpr` 解析及构建 | BPR2MAK 转换成功，生成的 Makefile 构建成功 | [工程构建日志](verification/bpr-build.log) |
| 注册表集成测试 | 8 组通过 | [Release 与测试日志](verification/test-build.log) |
| GUI 自动测试 | 6 组通过 | [Release 与测试日志](verification/test-build.log) |
| 正式 Release EXE | 启动、Unicode 界面初始化及正常退出通过 | 日志最后的 `shipped Release executable` 记录 |
| EXE 格式和依赖 | x86 PE32、Windows GUI、仅导入系统 DLL | [PE 头和导入表](verification/pe-imports.log) |
| 界面布局检查 | 已检查实际窗口客户区；Wine 部分中文输入栏字体回退异常 | [截图](screenshot.png) |

Release 优化时有 BCB6 的 W8027 内联提示，编译和链接没有错误。CRT 静态链接，不需要 `cc3260mt.dll` 或 VCL BPL。

导入的系统 DLL 为 ADVAPI32、KERNEL32、COMCTL32、COMDLG32、SHELL32 和 USER32。使用 XP 时代的 API，递归删除通过 `RegDeleteKeyW` 实现，未导入 Vista 才提供的 `RegDeleteTreeW`。

## 已验证的行为

注册表测试使用真实注册表 API，在 HKCU/HKLM 各自的 `Software\MixpToolboxTests\...` 分支建立临时数据并清理。覆盖：

- 五种作用范围，文件类型的 HKCU 优先和 HKLM 回退，拒绝未注册扩展名。
- 中文名称、中文程序路径及参数完整往返；来源分开列出；重复标识不覆盖。
- 修改状态、Shift 标记和 `REG_EXPAND_SZ` 命令类型。
- 自动备份后调用真实 `regedit /S`，核对恢复后的整棵子树和原快照一致。
- 修改后导入备份能够删除新加的标记；删除后导入恢复整个分支。
- DWORD、含反斜杠和引号的字符串、跨行二进制及 REG_MULTI_SZ 完整恢复。
- 只删除指定来源的菜单分支；外部修改后拒绝过期保存/删除。
- 备份位置不可写时不执行修改或删除。
- DDE、DropTarget、执行处理程序和无 command 的项不作为普通静态命令编辑。
- 拒绝无效标识和空命令；带空格路径正确加引号；无法可靠拆分的旧命令保留完整编辑方式。

GUI 测试通过控件消息操作真实窗口，覆盖启动、中文标签、创建带中文路径和默认参数的项、只改名称时保持命令不变、即时搜索、状态开关、来源选择、确认删除及退出。测试构建复用正式界面代码，只额外替换注册表根路径并提供测试截图入口；正式构建没有这些入口。

最后启动正式 Release EXE，确认初始化和正常退出，未对实际 Classes 菜单配置执行写操作。

Wine 环境补充了中文字体及 Tahoma 字体回退，字体仅用于本地验证，便携包不包含字体。当前 Wine 的部分输入栏中文字仍显示方框；通过 WM_GETTEXT、注册表读取和快照对比已核对中文数据无损，真实 XP 中的字形显示尚未验证。截图来自真实控件，裁去 Wine 未捕获的窗口外框。测试中保存的是命令配置，不会真的运行示例中的 JOE。

## XP 中的验收步骤

本次没有在真实 Windows XP 中运行程序，也没有在 XP 的 BCB6 IDE 界面中实际打开工程。BPR2MAK 解析成功不等同于已经完成这两项验证。建议在你的 XP 虚拟机按以下步骤验收：

1. 打开 `MixpToolbox.bpr`，Build；确认中文界面、全部按钮和下方编辑区正常显示。
2. 在“所有文件 / 当前用户”新增 `Mixp.XpCheck`，指向真实存在的 JOE EXE，参数使用该版本支持的选项并包含 `"%1"`。
3. 右键一个带空格和中文文件名的文件，点击新增菜单；确认文件路径与参数正确传入。
4. 修改菜单名称；切换禁用和仅 Shift 显示；重新打开菜单确认变化。
5. 手动导出、删除、导入该项的备份，确认菜单和命令恢复。
6. 另测文件夹、驱动器和已有文件关联的扩展名；同一 ProgID 的扩展名会共享菜单项。
7. 管理员账户下验证 HKLM；普通账户验证 HKCU，确认无权限时有提示。
8. 完成后删除 `Mixp.XpCheck`，仅处理本次创建的项。

当前不覆盖 COM 动态菜单扩展、XP 文件夹空白处菜单、64 位 XP 注册表双视图、不同 DPI 的实际布局，以及 Explorer 多选文件的调用行为。
