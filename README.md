Direct3D9-DebugFramework
轻量级、可扩展的 Direct3D9 应用调试框架， D3D9 渲染链路的实时捕获与调试，逆向工程、图形调试、应用定制化分析场景设计，依托即时 GUI 与 API 挂钩技术实现调试.

技术组件	作用与选型依据

Direct3D9 SDK	底层图形 API 交互，适配绝大多数 Windows 平台 D3D9 应用的渲染链路捕获
MinHook	轻量级 x86/x64 API 挂钩库（BSD 3-Clause 协议），实现 D3D9 函数的无侵入式挂钩
ImGui	即时模式 GUI 库（MIT 协议），轻量化嵌入目标应用，提供可交互的调试浮层 UI
C/C++ (Win32)	底层开发语言，适配 Windows 平台 DLL 注入、内存操作、API 挂钩等核心场景

应用技术
1. D3D9 渲染链路全量捕获
基于 MinHook 实现 D3D9 核心函数的 HOOK 封装，支持对渲染关键节点的精准拦截与数据透传：
渲染帧同步：挂钩IDirect3DDevice9::Present，实现帧级别的调试逻辑注入；
图元渲染捕获：挂钩DrawIndexedPrimitive/DrawPrimitive，解析顶点 / 索引缓冲区数据；
设备状态监控：实时捕获 D3D9 设备的渲染状态（纹理绑定、着色器参数、渲染目标等）。
2. 嵌入式调试 UI 体系
基于 ImGui 实现与目标应用渲染链路的无缝融合，无需修改目标程序源码：
实时数据可视化：帧耗时、DrawCall 数量、显存占用等核心指标实时展示；
交互式调试：支持运行时修改 D3D9 渲染参数（如视口、混合模式），即时生效；
轻量化渲染：UI 渲染与目标应用渲染链路解耦，无额外性能损耗。
3. 可扩展的调试插件体系
钩子扩展：通过统一的 HOOK 注册接口，新增任意 D3D9 函数的拦截逻辑；
UI 插件：基于 ImGui 封装独立调试面板（如纹理查看器、着色器反汇编器）；
构建使用
编译：基于项目工程文件编译生成目标平台的 DLL；
注入：将 DLL 注入目标 D3D9 应用（通用 DLL 注入工具均可）；
调试：通过默认热键（F2）唤起调试 UI，按需使用 / 扩展调试功能。
扩展开发
新增 HOOK：基于 MinHook 封装的统一挂钩接口（src/d3d9_hook.cpp），注册目标 D3D9 函数即可；
定制 UI 面板：继承框架基础 UI 类（src/imgui_panel_base.h），实现自定义渲染逻辑；
许可证
本项目基于 MIT 协议开源；
ImGui：MIT 协议（详见imgui/LICENSE.txt）；
MinHook：BSD 3-Clause 协议（详见minhook/LICENSE.txt）。
