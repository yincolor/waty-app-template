#include "notify_demo.hpp"

#include <waty/notify.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace demo
{
    namespace
    {
        std::string read_string(const nlohmann::json &args, std::string_view key, std::string fallback)
        {
            if (args.is_object())
            {
                const std::string key_string(key);
                const auto it = args.find(key_string);
                if (it != args.end() && it->is_string())
                {
                    return it->get<std::string>();
                }
            }
            return fallback;
        }

        bool read_bool(const nlohmann::json &args, std::string_view key, bool fallback)
        {
            if (args.is_object())
            {
                const std::string key_string(key);
                const auto it = args.find(key_string);
                if (it != args.end() && it->is_boolean())
                {
                    return it->get<bool>();
                }
            }
            return fallback;
        }
    } // namespace

    void register_notify_demo(const std::shared_ptr<waty::WebWindow> &window)
    {
        std::weak_ptr<waty::WebWindow> weak_window = window;

        window->ipcHandle("notify.show",
                          [weak_window](const nlohmann::json &args) -> std::future<nlohmann::json> {
                              const std::string title =
                                  read_string(args, "title", "waty 通知演示");
                              const std::string body =
                                  read_string(args, "body", "来自 waty-notify 的系统通知");
                              const bool with_actions = read_bool(args, "actions", false);

                              return std::async(std::launch::async,
                                                [weak_window, title, body, with_actions]() -> nlohmann::json {
                                  const auto emit = [weak_window](std::string_view event,
                                                                  nlohmann::json data) {
                                      if (auto window = weak_window.lock())
                                      {
                                          window->emit(event, data);
                                      }
                                  };

                                  waty::notify::notification notification;
                                  notification.title   = title;
                                  notification.body    = body;
                                  notification.tag     = "waty-app-template-notify";
                                  notification.timeout = std::chrono::seconds{8};
                                  if (with_actions)
                                  {
                                      notification.actions = {
                                          {.id = "open", .label = "查看"},
                                          {.id = "ignore", .label = "忽略"},
                                      };
                                  }

                                  notification.on_action = [emit](std::string_view action_id) {
                                      emit("notify-action", {{"id", std::string(action_id)}});
                                  };
                                  notification.on_closed = [emit](waty::notify::close_reason reason) {
                                      emit("notify-closed", {{"reason", static_cast<int>(reason)}});
                                  };
                                  notification.on_error = [emit](const waty::notify::error &error) {
                                      emit("notify-error", {{"code", static_cast<int>(error.code)},
                                                            {"message", error.message}});
                                  };

                                  if (auto result = waty::notify::send(std::move(notification)); !result)
                                  {
                                      return {{"ok", false},
                                              {"code", static_cast<int>(result.error().code)},
                                              {"message", result.error().message}};
                                  }
                                  return {{"ok", true}};
                              });
                          });
    }
} // namespace demo
