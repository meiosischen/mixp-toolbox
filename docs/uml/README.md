# MixP Toolbox UML 图

根据当前 **0.1.2** 源码（代码基准 `f01b82d`）绘制，采用可独立渲染的 UTF-8 PlantUML `.puml` 文件。每个文件只有一组 `@startuml` / `@enduml`，没有远程 include、插件、主题或图片依赖。

| 类型 | 文件 | 内容 |
| --- | --- | --- |
| 组件图 | [components.puml](components.puml) | 程序入口、原生 Win32 界面、注册表存储、数据类型、系统 API、备份文件与外部程序 |
| 顺序图 | [sequence-query.puml](sequence-query.puml) | 启动初始化、全部/指定范围查询、内存搜索、选择菜单 |
| 顺序图 | [sequence-create.puml](sequence-create.puml) | 进入新增模式、构建命令与 Draft、校验、写入、失败清理、通知 Shell |
| 顺序图 | [sequence-update.puml](sequence-update.puml) | 编辑保存、快照校验、修改前备份、写入失败恢复、刷新和重新选择 |
| 顺序图 | [sequence-delete.puml](sequence-delete.puml) | 删除确认、快照校验、备份、递归删除和失败恢复 |
| 顺序图 | [sequence-export.puml](sequence-export.puml) | 手动导出及用户在系统 regedit 中恢复的独立流程 |
| 对象图 | [objects.puml](objects.puml) | 保存修改前，Window、RegistryStore、Entry、Draft、Tree 和 Value 的实例关系 |

组件图表示当前源码的职责和依赖；这些组件都编译在一个 EXE 内。`RegistryStore` 所在源文件还包含快照、校验、备份、写入和恢复等辅助函数，顺序图将它们合并到同一生命线。`Window / dialogProc` 生命线合并窗口状态与 Win32 消息入口，异常提示实际由 `dialogProc` 捕获后显示。

对象图使用 README 中 JOE 菜单的示例数据，假设列表只包含一个项，并展示将 `-tab 4` 改为 `-tab 8`、构建 Draft 后且调用 `update()` 前的时刻。对象图没有采集或修改实际注册表。`Entry.snapshot` 和 `Draft` 分别表示查询时的旧内容和待保存的新内容；句柄、备份目录使用符号值，字节数组使用语义表示。

## 源码对应关系

| 源码 | 图中的职责与入口 |
| --- | --- |
| [MixpToolbox.cpp](../../MixpToolbox.cpp) | `WinMain()` 构造生产 `RegistryStore` 并调用 `runMainWindow()` |
| [src/MainWindow.cpp](../../src/MainWindow.cpp) | `Window`、`dialogProc()`、`runMainWindow()`；`refresh()`、`showList()`、`loadEditor()`；`IDC_NEW / IDC_SAVE / IDC_DELETE / IDC_EXPORT` |
| [src/MenuRegistry.h](../../src/MenuRegistry.h) | `RegistryStore`、`Entry`、`Draft`、`Tree`、`Value`、`Error`、`Scope` 和 `Source` 的声明 |
| [src/MenuRegistry.cpp](../../src/MenuRegistry.cpp) | `objectFor()`、`listAll()`、`list()`、`read()`、`create()`、`update()`、`remove()`、`exportEntry()`；`snapshot()`、`apply()`、`backup()` 和恢复辅助函数 |
| [MixpToolbox.rc](../../MixpToolbox.rc) / [src/resource.h](../../src/resource.h) | `IDD_MAIN` 对话框资源和控件 ID |

当前实现使用 Win32 资源对话框，没有 VCL `TForm` 或 `.dfm`。系统信息、控制面板和 `shellex` COM 动态菜单扩展尚未实现，因此未列为已有组件。

修改和删除的自动备份在写入前完成；手动导出也先校验当前快照。图中的失败恢复是补偿操作，没有注册表事务或并发锁保证；恢复失败时仍需按错误提示处理。`.reg` 内容先删除指定单个菜单分支，再恢复旧快照；工具没有自动启动 regedit 或内置导入功能。

## 渲染方法

用支持 PlantUML 的编辑器直接打开 `.puml` 文件，或从项目根目录执行：

```sh
# 检查全部文件的语法
java -jar /path/to/plantuml.jar -charset UTF-8 -checkonly docs/uml/*.puml

# 导出 SVG；输出目录相对每个 .puml 文件所在目录
java -jar /path/to/plantuml.jar -charset UTF-8 -tsvg -o ../../build/uml docs/uml/*.puml

# 导出 PNG
java -jar /path/to/plantuml.jar -charset UTF-8 -tpng -o ../../build/uml docs/uml/*.puml
```

组件图和对象图默认布局通常需要 Graphviz。当前 PlantUML 也可通过命令行参数 `-Playout=smetana` 使用内置布局引擎，例如：

```sh
java -jar /path/to/plantuml.jar -charset UTF-8 -Playout=smetana -tsvg -o ../../build/uml docs/uml/*.puml
```

查看中文图形需要渲染环境提供中文字体。文件默认使用 `Arial Unicode MS`；如果环境没有此字体，可将 `skinparam defaultFontName` 改为该环境已有的中文字体，例如 Windows 上的 `SimSun`。PlantUML 工具和渲染预览放在临时目录或已忽略的 `build/` 下，不加入编译工程。

## 验证结果

2026-10-11，使用 PlantUML **1.2026.8** 和 OpenJDK **21.0.10**，7 份文件均通过语法检查，并成功生成 SVG 和 PNG。组件图和对象图使用内置 Smetana 布局引擎渲染；已查看渲染结果并核对中文、连接关系和流程分支。该验证针对图源语法与渲染，未重新执行程序或修改系统注册表。
