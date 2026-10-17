#include "notify_demo.hpp"
#include "tray_demo.hpp"
#include "video_demo.hpp"

#include <waty/application.hpp>
#include <waty/web_window.hpp>

#include <memory>

// waty-app-template：waty 组件演示应用。
//
// 模块拆分：
//   - notify_demo.*：waty-notify 系统通知
//   - tray_demo.*：waty-tray 托盘图标与菜单
//   - video_demo.* / video_stream_server.*：resource/ 下视频的流式播放
//   - web/js/index.js：所有按钮、对话框、文件系统与事件输出
int main()
{
    waty::ApplicationOptions app_opts;
    app_opts.appName          = "Waty_App_Template";
    app_opts.organizationName = "Acme_Corp";
    app_opts.icon             = "img/icon.png";  // 使用嵌入资源 web/img/icon.png

    app_opts.permissions.os       = true;  // 系统信息 / getPath
    app_opts.permissions.fs.read  = true;  // 视频流式读取探测
    app_opts.permissions.window   = true;  // JS 窗口控制
    app_opts.permissions.dialog   = true;  // 原生消息框 / 文件 / 目录选择
    app_opts.permissions.devtools = true;  // DevTools 按钮
    app_opts.permissions.log      = true;  // JS console 转发到 C++ 日志

    waty::Application app(app_opts);

    waty::WebWindowOptions win_opts;
    win_opts.title     = "waty 组件演示";
    win_opts.width     = 1100;
    win_opts.height    = 820;
    win_opts.minWidth  = 760;
    win_opts.minHeight = 600;

    auto window = app.createWindow(win_opts);

    // waty-notify：JS 通过 ipc.invoke("notify.show", ...) 触发。
    demo::register_notify_demo(window);

    // waty-tray：托盘在 UI 线程（ready 事件）初始化，菜单点击回传 JS。
    auto tray_demo = std::make_shared<demo::TrayDemo>(app, window);
    tray_demo->register_ipc();

    // 视频流：resource/video.mp4 由本地 HTTP Range 服务按需读取。
    auto video_demo = std::make_shared<demo::VideoDemo>(app, window);
    video_demo->register_ipc();

    // Subscription 必须保存；否则临时对象析构会立即取消订阅。
    auto ready_sub = app.on("ready", [tray_demo](waty::Event &, const nlohmann::json &) {
        tray_demo->start();
    });

    window->loadFile("page/index.html");
    window->show();

    const int exit_code = app.run();

    (void)ready_sub;
    return exit_code;
}
