#pragma once

#include <waty/application.hpp>
#include <waty/tray/tray.hpp>
#include <waty/web_window.hpp>

#include <nlohmann/json.hpp>

#include <memory>
#include <mutex>
#include <string>

namespace demo
{
    /// waty-tray 演示模块。
    ///
    /// - 在 UI 线程（waty 的 ready 事件）初始化 TrayContext / TrayIcon。
    /// - 托盘图标来自嵌入资源 web/img/icon.png。
    /// - 托盘菜单点击后通过 window->emit("tray-menu-clicked", ...) 回传 JS。
    class TrayDemo : public std::enable_shared_from_this<TrayDemo>
    {
      public:
        TrayDemo(waty::Application &app, std::shared_ptr<waty::WebWindow> window);
        ~TrayDemo();

        TrayDemo(const TrayDemo &)            = delete;
        TrayDemo &operator=(const TrayDemo &) = delete;

        /// 注册 tray.status IPC handler（在 main 中创建 shared_ptr 后调用）。
        void register_ipc();

        /// 在 UI 线程初始化托盘。由 app.on("ready", ...) 调用。
        void start();

        /// 供 JS 查询托盘状态。
        [[nodiscard]] nlohmann::json status() const;

      private:
        void emit(std::string_view event, nlohmann::json data) const;
        void fail(std::string message);

        waty::Application &m_app;
        std::weak_ptr<waty::WebWindow> m_window;

        waty::tray::TrayContext m_context;
        std::unique_ptr<waty::tray::TrayIcon> m_tray;

        mutable std::mutex m_mutex;
        bool m_ready = false;
        std::string m_error;
        std::string m_icon_source = "web/img/icon.png";
    };
} // namespace demo
