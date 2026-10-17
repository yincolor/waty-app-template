#include "tray_demo.hpp"

#include <future>
#include <string>
#include <string_view>
#include <utility>

namespace demo
{
    TrayDemo::TrayDemo(waty::Application &app, std::shared_ptr<waty::WebWindow> window)
        : m_app(app), m_window(std::move(window))
    {
    }

    TrayDemo::~TrayDemo()
    {
        // m_tray 先于 m_context 析构；托盘资源在 UI 线程（app.run 返回后）释放。
        m_tray.reset();
    }

    void TrayDemo::register_ipc()
    {
        std::weak_ptr<TrayDemo> weak_self = weak_from_this();

        if (auto window = m_window.lock())
        {
            window->ipcHandle("tray.status",
                              [weak_self](const nlohmann::json &) -> std::future<nlohmann::json> {
                                  return std::async(std::launch::async, [weak_self]() -> nlohmann::json {
                                      if (auto self = weak_self.lock())
                                      {
                                          return self->status();
                                      }
                                      return {{"ready", false},
                                              {"error", "TrayDemo 已销毁"},
                                              {"icon", ""},
                                              {"menu", nlohmann::json::array()}};
                                  });
                              });
        }
    }

    void TrayDemo::start()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_ready)
            {
                return;
            }
        }

        if (auto result = m_context.initialize(); !result)
        {
            fail("TrayContext::initialize 失败，错误码 " +
                 std::to_string(static_cast<int>(result.error())));
            emit("tray-status", status());
            return;
        }

        auto created = waty::tray::TrayIcon::create();
        if (!created)
        {
            fail("TrayIcon::create 失败，错误码 " +
                 std::to_string(static_cast<int>(created.error())));
            emit("tray-status", status());
            return;
        }
        m_tray = std::move(*created);

        // 托盘图标使用嵌入资源 web/img/icon.png，而不是可执行文件旁的 icon.png。
        bool icon_ok = false;
        if (auto bytes = m_app.readEmbeddedFile("img/icon.png"))
        {
            if (auto icon = waty::tray::Icon::from_data(*bytes))
            {
                m_tray->set_icon(*icon);
                icon_ok = true;
            }
            else
            {
                fail("Icon::from_data(web/img/icon.png) 失败");
            }
        }
        else
        {
            fail("readEmbeddedFile(web/img/icon.png) 失败");
        }

        if (!icon_ok)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_icon_source = "fallback:applications-other";
            }
            m_tray->set_icon(waty::tray::Icon::from_theme_name("applications-other"));
        }

        waty::tray::Menu menu;
        menu.add_item({
            .id       = "show",
            .label    = "显示主窗口",
            .on_click = [weak_self = weak_from_this()] {
                if (auto self = weak_self.lock())
                {
                    self->emit("tray-menu-clicked", {{"id", "show"}, {"label", "显示主窗口"}});
                    if (auto window = self->m_window.lock())
                    {
                        window->show();
                        window->focus();
                    }
                }
            },
        });
        menu.add_item({
            .id       = "hide",
            .label    = "隐藏主窗口",
            .on_click = [weak_self = weak_from_this()] {
                if (auto self = weak_self.lock())
                {
                    self->emit("tray-menu-clicked", {{"id", "hide"}, {"label", "隐藏主窗口"}});
                    if (auto window = self->m_window.lock())
                    {
                        window->hide();
                    }
                }
            },
        });
        menu.add_item({
            .id       = "about",
            .label    = "关于 waty-tray",
            .on_click = [weak_self = weak_from_this()] {
                if (auto self = weak_self.lock())
                {
                    self->emit("tray-menu-clicked", {{"id", "about"}, {"label", "关于 waty-tray"}});
                }
            },
        });
        menu.add_item({
            .id       = "quit",
            .label    = "退出应用",
            .on_click = [weak_self = weak_from_this()] {
                if (auto self = weak_self.lock())
                {
                    self->emit("tray-menu-clicked", {{"id", "quit"}, {"label", "退出应用"}});
                    self->m_app.quit();
                }
            },
        });

        m_tray->set_tooltip("waty-app-template · waty-tray");
        m_tray->set_menu(std::move(menu));
        m_tray->show();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ready = true;
        }
        emit("tray-status", status());
    }

    nlohmann::json TrayDemo::status() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return {
            {"ready", m_ready},
            {"error", m_error},
            {"icon", m_icon_source},
            {"menu", nlohmann::json::array({"show", "hide", "about", "quit"})},
            {"hint", "右键点击托盘图标，然后选择菜单项；点击结果会打印到页面事件输出。"},
        };
    }

    void TrayDemo::emit(std::string_view event, nlohmann::json data) const
    {
        if (auto window = m_window.lock())
        {
            window->emit(event, data);
        }
    }

    void TrayDemo::fail(std::string message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_error = std::move(message);
    }
} // namespace demo
