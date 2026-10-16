#include <waty/application.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    // 测试 Application::readEmbeddedFile：读取嵌入的 PNG 二进制资源，
    // 校验 PNG 魔数；同时验证未注册的资源返回 std::nullopt。
    bool test_read_embedded_file(const waty::Application &app)
    {
        constexpr std::uint8_t png_sig[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

        const auto bytes = app.readEmbeddedFile("img/base_image.png");
        if (!bytes)
        {
            std::printf("[waty-test] readEmbeddedFile(\"img/base_image.png\") -> nullopt (FAIL)\n");
            return false;
        }

        const bool magic_ok = bytes->size() >= sizeof(png_sig) &&
                              std::equal(png_sig, png_sig + sizeof(png_sig), bytes->begin());
        std::printf("[waty-test] readEmbeddedFile(\"img/base_image.png\") size=%zu png_magic=%s\n",
                    bytes->size(), magic_ok ? "ok" : "FAIL");

        const auto missing = app.readEmbeddedFile("__no_such_resource__.bin");
        std::printf("[waty-test] readEmbeddedFile(missing) -> %s\n",
                    missing ? "value (FAIL)" : "nullopt (ok)");

        return magic_ok && !missing;
    }

    // 测试 appName 严格校验：非法值必须在 Application 构造时抛出 std::invalid_argument。
    bool test_app_name_validation()
    {
        const auto expect_reject = [](std::string_view name) {
            try
            {
                waty::ApplicationOptions opts;
                opts.appName = name;
                waty::Application app(opts);
                return false;
            }
            catch (const std::invalid_argument &)
            {
                return true;
            }
        };

        const std::string too_long(65, 'a');
        const bool ok = expect_reject("") &&
                        expect_reject("Bad Name") &&
                        expect_reject("a/b") &&
                        expect_reject("CON") &&
                        expect_reject("com1") &&
                        expect_reject("LPT9") &&
                        expect_reject(too_long);

        std::printf("[waty-test] appName 严格校验: 非法输入均被拒绝=%s\n", ok ? "ok" : "FAIL");
        return ok;
    }

    // 测试 organizationName 默认值与严格校验。
    bool test_organization_name_validation()
    {
        const waty::ApplicationOptions default_opts;
        const bool default_ok = default_opts.organizationName == "waty";

        const auto expect_reject_org = [](std::string_view org) {
            try
            {
                waty::ApplicationOptions opts;
                opts.appName          = "Valid_App";
                opts.organizationName = org;
                waty::Application app(opts);
                return false;
            }
            catch (const std::invalid_argument &)
            {
                return true;
            }
        };

        const std::string too_long(65, 'a');
        const bool reject_ok = expect_reject_org("") &&
                               expect_reject_org("Bad Org") &&
                               expect_reject_org("a/b") &&
                               expect_reject_org("CON") &&
                               expect_reject_org("LPT9") &&
                               expect_reject_org(too_long);

        std::printf("[waty-test] organizationName: default=%s invalid_rejected=%s\n",
                    default_ok ? "waty(ok)" : "FAIL", reject_ok ? "ok" : "FAIL");
        return default_ok && reject_ok;
    }

    // 测试 SysPath 新增/重命名后的 getPath 行为。
    bool test_get_paths(const waty::Application &app, std::string_view org)
    {
        const std::string app_data = app.getPath(waty::SysPath::AppData);
        const std::string config   = app.getPath(waty::SysPath::Config);
        const std::string exe      = app.getPath(waty::SysPath::Executable);
        const std::string legacy_user_data = app.getPath(waty::SysPath::UserData);
        const std::string legacy_app_dir   = app.getPath(waty::SysPath::AppDir);

#if defined(__linux__)
        const std::string org_segment = std::string("/") + std::string(org) + "/";
        const bool xdg_namespace_ok = app_data.find(org_segment) != std::string::npos &&
                                      config.find(org_segment) != std::string::npos;
#else
        const bool xdg_namespace_ok = true;
#endif

        const bool ok = !app_data.empty() && !config.empty() && !exe.empty() &&
                        legacy_user_data == config && legacy_app_dir == exe &&
                        xdg_namespace_ok;

        std::printf("[waty-test] getPath: appData=%s\n", app_data.c_str());
        std::printf("[waty-test] getPath: config=%s\n", config.c_str());
        std::printf("[waty-test] getPath: executable=%s\n", exe.c_str());
        std::printf("[waty-test] getPath: UserData==Config / AppDir==Executable / LinuxXDG(%s)=%s => %s\n",
                    std::string(org).c_str(), xdg_namespace_ok ? "ok" : "FAIL", ok ? "ok" : "FAIL");
        return ok;
    }

    // 测试全屏与窗口边框 API：验证 setter/getter 的默认态与切换逻辑。
    bool test_window_state_api(waty::WebWindow &window)
    {
        window.setFullscreen(false);
        const bool fullscreen = window.isFullscreen();

        window.setWindowFrameState(true);
        const bool framed = window.hasWindowFrame();
        window.setWindowFrameState(false);
        const bool frameless = window.hasWindowFrame();
        window.setWindowFrameState(true); // 恢复默认

        std::printf("[waty-test] window state api: isFullscreen=%s hasWindowFrame=true->%s / false->%s\n",
                    fullscreen ? "true" : "false", framed ? "true" : "false", frameless ? "true" : "false");

        return !fullscreen && framed && !frameless;
    }
} // namespace

// waty-app-template: waty 样例模板。
//
// 前端位于 web/，构建时由 waty_embed 嵌入到可执行文件；
// 入口页面为 page/index.html，通过 waty 的 JS 桥（window.waty.*）与 C++ 交互。
// 页面底部按钮分别演示：ipc.invoke("greet") / os.* / fs.stat / window.getSize，
// 以及 C++ 侧主动推送的 server-ping 事件。
int main()
{
    if (!test_app_name_validation())
    {
        std::printf("[waty-test] appName 严格校验测试未通过\n");
        return 1;
    }
    std::printf("[waty-test] appName 严格校验测试通过\n");

    if (!test_organization_name_validation())
    {
        std::printf("[waty-test] organizationName 测试未通过\n");
        return 1;
    }
    std::printf("[waty-test] organizationName 测试通过\n");

    waty::ApplicationOptions app_opts;
    app_opts.appName          = "Waty_App_Template";
    app_opts.organizationName = "Acme_Corp";
    app_opts.icon    = "resource/icon.png";         // 相对可执行文件目录（构建时复制自项目根目录 icon.png）
    app_opts.permissions.os       = true;  // 启用 os 模块（platform / homedir / getPath）
    app_opts.permissions.fs.read  = true;  // 启用文件读取（fs.stat 等）
    app_opts.permissions.window   = true;  // 启用 JS 窗口控制（window.getSize 等）
    app_opts.permissions.devtools = true;  // 允许打开 DevTools
    app_opts.permissions.log      = true;  // 转发 JS console 到 C++ 日志

    waty::Application app(app_opts);

    if (!test_get_paths(app, "Acme_Corp"))
    {
        std::printf("[waty-test] getPath 测试未通过\n");
        return 1;
    }
    std::printf("[waty-test] getPath 测试通过\n");

    // 测试 readEmbeddedFile（编译测试：不依赖图形界面即可验证嵌入资源读取）
    if (!test_read_embedded_file(app))
    {
        std::printf("[waty-test] readEmbeddedFile 测试未通过\n");
        return 1;
    }
    std::printf("[waty-test] readEmbeddedFile 测试通过\n");

    waty::WebWindowOptions win_opts;
    win_opts.title     = "Waty App Template";
    win_opts.width     = 800;
    win_opts.height    = 600;
    win_opts.minWidth  = 400;
    win_opts.minHeight = 600;

    auto window = app.createWindow(win_opts);

    // 测试 setFullscreen / isFullscreen / setWindowFrameState / hasWindowFrame
    if (!test_window_state_api(*window))
    {
        std::printf("[waty-test] 窗口状态 API 测试未通过\n");
        return 1;
    }
    std::printf("[waty-test] 窗口状态 API 测试通过\n");

    // IPC：响应前端 window.waty.ipc.invoke("greet", ...)。
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
//    // IPC：响应前端 window.waty.ipc.invoke("greet", ...)。
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
    // window.waty.ipc.on("server-ping", ...) 接收）。
    int tick = 0;
    app.onEventLoopTick([&] {
        if (++tick == 50)
        {
            window->emit("server-ping", {{"tick", tick}});
        }
    });

    // ready 在窗口 realize 之后、UI 线程触发，可安全打开 DevTools
    app.on("ready", [window](waty::Event &, const nlohmann::json &){
        std::printf("[waty] 开启网页调试窗口\n");
        window->openDevTools();
    });

    // 加载嵌入资源中的入口页（web/page/index.html）并显示窗口
    window->loadFile("page/index.html");
    window->show();
    return app.run();
}
