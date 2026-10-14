#include <whiz/application.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <future>

namespace
{
    // 测试 Application::readEmbeddedFile：读取嵌入的 PNG 二进制资源，
    // 校验 PNG 魔数；同时验证未注册的资源返回 std::nullopt。
    bool test_read_embedded_file(const whiz::Application &app)
    {
        constexpr std::uint8_t png_sig[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

        const auto bytes = app.readEmbeddedFile("img/logo_dark.png");
        if (!bytes)
        {
            std::printf("[whiz-test] readEmbeddedFile(\"img/logo_dark.png\") -> nullopt (FAIL)\n");
            return false;
        }

        const bool magic_ok = bytes->size() >= sizeof(png_sig) &&
                              std::equal(png_sig, png_sig + sizeof(png_sig), bytes->begin());
        std::printf("[whiz-test] readEmbeddedFile(\"img/logo_dark.png\") size=%zu png_magic=%s\n",
                    bytes->size(), magic_ok ? "ok" : "FAIL");

        const auto missing = app.readEmbeddedFile("__no_such_resource__.bin");
        std::printf("[whiz-test] readEmbeddedFile(missing) -> %s\n",
                    missing ? "value (FAIL)" : "nullopt (ok)");

        return magic_ok && !missing;
    }

    // 测试全屏与窗口边框 API：验证 setter/getter 的默认态与切换逻辑。
    bool test_window_state_api(whiz::WebWindow &window)
    {
        window.setFullscreen(false);
        const bool fullscreen = window.isFullscreen();

        window.setWindowFrameState(true);
        const bool framed = window.hasWindowFrame();
        window.setWindowFrameState(false);
        const bool frameless = window.hasWindowFrame();
        window.setWindowFrameState(true); // 恢复默认

        std::printf("[whiz-test] window state api: isFullscreen=%s hasWindowFrame=true->%s / false->%s\n",
                    fullscreen ? "true" : "false", framed ? "true" : "false", frameless ? "true" : "false");

        return !fullscreen && framed && !frameless;
    }
} // namespace

// whiz-app-template：whiz 样例模板。
//
// 前端位于 web/，构建时由 whiz_embed 嵌入到可执行文件；
// 入口页面为 page/index.html，通过 whiz 的 JS 桥（window.whiz.*）与 C++ 交互。
// 页面底部按钮分别演示：ipc.invoke("greet") / os.* / fs.stat / window.getSize，
// 以及 C++ 侧主动推送的 server-ping 事件。
int main()
{
    whiz::ApplicationOptions app_opts;
    app_opts.appName = "Whiz Template";
    app_opts.icon    = "resource/icon.png";         // 相对可执行文件目录（构建时复制自项目根目录 icon.png）
    app_opts.permissions.os       = true;  // 启用 os 模块（platform / homedir / getPath）
    app_opts.permissions.fs.read  = true;  // 启用文件读取（fs.stat 等）
    app_opts.permissions.window   = true;  // 启用 JS 窗口控制（window.getSize 等）
    app_opts.permissions.devtools = true;  // 允许打开 DevTools
    app_opts.permissions.log      = true;  // 转发 JS console 到 C++ 日志

    whiz::Application app(app_opts);

    // 测试 readEmbeddedFile（编译测试：不依赖图形界面即可验证嵌入资源读取）
    if (!test_read_embedded_file(app))
    {
        std::printf("[whiz-test] readEmbeddedFile 测试未通过\n");
        return 1;
    }
    std::printf("[whiz-test] readEmbeddedFile 测试通过\n");

    whiz::WebWindowOptions win_opts;
    win_opts.title     = "Whiz Template";
    win_opts.width     = 480;
    win_opts.height    = 720;
    win_opts.minWidth  = 400;
    win_opts.minHeight = 640;

    auto window = app.createWindow(win_opts);

    // 测试 setFullscreen / isFullscreen / setWindowFrameState / hasWindowFrame
    if (!test_window_state_api(*window))
    {
        std::printf("[whiz-test] 窗口状态 API 测试未通过\n");
        return 1;
    }
    std::printf("[whiz-test] 窗口状态 API 测试通过\n");

    // IPC：响应前端 window.whiz.ipc.invoke("greet", ...)。
    // 回调运行在线程池，可执行耗时操作。
    window->ipcHandle("greet", [](const nlohmann::json &args) -> std::future<nlohmann::json> {
        const std::string name = (args.is_array() && !args.empty() && args[0].is_string())
                                     ? args[0].get<std::string>()
                                     : "world";
        return std::async(std::launch::async, [name]() -> nlohmann::json {
            return nlohmann::json{{"message", "Hello, " + name + "!"}};
        });
    });

//    // 测试创建重复事件响应
//    // IPC：响应前端 window.whiz.ipc.invoke("greet", ...)。
//    // 回调运行在线程池，可执行耗时操作。
//    window->ipcHandle("greet", [](const nlohmann::json &args) -> std::future<nlohmann::json> {
//        const std::string name = (args.is_array() && !args.empty() && args[0].is_string())
//                                     ? args[0].get<std::string>()
//                                     : "world";
//        return std::async(std::launch::async, [name]() -> nlohmann::json {
//            return nlohmann::json{{"message", "Hello, " + name + "!"}};
//        });
//    });

    // 事件循环：累计 50 轮后向 JS 推送一条消息（前端通过
    // window.whiz.ipc.on("server-ping", ...) 接收）。
    int tick = 0;
    app.onEventLoopTick([&] {
        if (++tick == 50)
        {
            window->emit("server-ping", {{"tick", tick}});
        }
    });

    // ready 在窗口 realize 之后、UI 线程触发，可安全打开 DevTools
    app.on("ready", [window](whiz::Event &, const nlohmann::json &){
        std::printf("[whiz] 开启网页调试窗口\n");
        window->openDevTools();
    });

    // 加载嵌入资源中的入口页（web/page/index.html）并显示窗口
    window->loadFile("page/index.html");
    window->show();
    return app.run();
}
