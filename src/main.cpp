#include <whiz/application.hpp>

#include <future>

// whiz-app-template：whiz 样例模板。
//
// 前端位于 web/，构建时由 whiz_embed_resources 嵌入到可执行文件；
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

    // 应用生命周期事件
    app.on("ready", [](whiz::Event &, const nlohmann::json &) {
        // 应用就绪：可在此初始化资源、创建窗口等
    });

    whiz::WebWindowOptions win_opts;
    win_opts.title     = "Whiz Template";
    win_opts.width     = 480;
    win_opts.height    = 720;
    win_opts.minWidth  = 400;
    win_opts.minHeight = 640;

    auto window = app.createWindow(win_opts);

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

    // 事件循环：累计 50 轮后向 JS 推送一条消息（前端通过
    // window.whiz.ipc.on("server-ping", ...) 接收）。
    int tick = 0;
    app.onEventLoopTick([&] {
        if (++tick == 50)
        {
            window->emit("server-ping", {{"tick", tick}});
        }
    });

    // 加载嵌入资源中的入口页（web/page/index.html）并显示窗口
    window->loadFile("page/index.html");
    window->show();

    return app.run();
}
