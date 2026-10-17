#pragma once

#include <waty/web_window.hpp>

#include <memory>

namespace demo
{
    /// 注册 waty-notify 演示所需的 IPC handler。
    ///
    /// JS 侧通过 window.waty.ipc.invoke("notify.show", { actions: boolean }) 触发。
    void register_notify_demo(const std::shared_ptr<waty::WebWindow> &window);
} // namespace demo
