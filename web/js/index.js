const outEl = document.getElementById("out");
const videoEl = document.getElementById("video");
const videoStatusEl = document.getElementById("video-status");
const trayStatusEl = document.getElementById("tray-status-view");

let videoInfo = null;

function formatBytes(value) {
    if (!Number.isFinite(value)) {
        return "-";
    }
    const units = ["B", "KB", "MB", "GB"];
    let n = value;
    let unit = 0;
    while (n >= 1024 && unit < units.length - 1) {
        n /= 1024;
        unit += 1;
    }
    return `${n.toFixed(unit === 0 ? 0 : 2)} ${units[unit]}`;
}

function log(label, value) {
    const time = new Date().toLocaleTimeString();
    let text = `[${time}] ${label}`;
    if (value !== undefined) {
        text += " " + (typeof value === "string" ? value : JSON.stringify(value));
    }
    outEl.textContent += text + "\n";
    outEl.parentElement.scrollTop = outEl.parentElement.scrollHeight;
}

function logError(label, error) {
    log(label, error && (error.message || error.name || String(error)));
}

async function invoke(event, args) {
    return window.waty.ipc.invoke(event, args);
}

// ---------------------------------------------------------------------------
// 对话框（waty core）
// ---------------------------------------------------------------------------
document.getElementById("dialog-message").addEventListener("click", async () => {
    try {
        const result = await window.waty.dialog.showMessageBox({
            type: "question",
            title: "waty 原生消息框",
            message: "这是 window.waty.dialog.showMessageBox 演示。",
            detail: "按钮、复选框与返回值都由平台原生对话框提供。",
            buttons: ["确定", "取消"],
            defaultButtonIndex: 0,
            cancelButtonIndex: 1,
            checkboxLabel: "记住这次演示",
            checkboxChecked: false,
        });
        log("dialog.showMessageBox", result);
    } catch (error) {
        logError("dialog.showMessageBox 失败", error);
    }
});

const fileFilters = [
    { name: "图片", extensions: ["png", "jpg", "jpeg", "gif", "webp"] },
    { name: "视频", extensions: ["mp4", "webm", "mkv"] },
    { name: "所有文件", extensions: ["*"] },
];

async function openDialog(options, label) {
    try {
        const result = await window.waty.dialog.showOpenDialog(options);
        log(label, result);
    } catch (error) {
        logError(`${label} 失败`, error);
    }
}

document.getElementById("dialog-open-file").addEventListener("click", () => {
    openDialog({ title: "选择单个文件", multiSelect: false, filters: fileFilters }, "dialog.showOpenDialog(单个文件)");
});

document.getElementById("dialog-open-files").addEventListener("click", () => {
    openDialog({ title: "选择多个文件", multiSelect: true, filters: fileFilters }, "dialog.showOpenDialog(多个文件)");
});

document.getElementById("dialog-open-dir").addEventListener("click", () => {
    openDialog({ title: "选择目录", directory: true }, "dialog.showOpenDialog(目录)");
});

document.getElementById("dialog-save").addEventListener("click", async () => {
    try {
        const result = await window.waty.dialog.showSaveDialog({
            title: "保存文件",
            defaultName: "waty-demo.txt",
            filters: [{ name: "文本", extensions: ["txt"] }],
        });
        log("dialog.showSaveDialog", result);
    } catch (error) {
        logError("dialog.showSaveDialog 失败", error);
    }
});

// ---------------------------------------------------------------------------
// 系统 / 窗口（waty core）
// ---------------------------------------------------------------------------
document.getElementById("core-os").addEventListener("click", async () => {
    try {
        const [platform, arch, homedir, config, executable] = await Promise.all([
            window.waty.os.platform(),
            window.waty.os.arch(),
            window.waty.os.homedir(),
            window.waty.os.getPath("config"),
            window.waty.os.getPath("executable"),
        ]);
        log("os", { platform, arch, homedir, config, executable });
    } catch (error) {
        logError("os 失败", error);
    }
});

document.getElementById("core-win-size").addEventListener("click", async () => {
    try {
        const size = await window.waty.window.getSize();
        log("window.getSize", size);
    } catch (error) {
        logError("window.getSize 失败", error);
    }
});

document.getElementById("core-devtools").addEventListener("click", async () => {
    try {
        await window.waty.window.openDevTools();
        log("window.openDevTools", "已请求打开 DevTools");
    } catch (error) {
        logError("window.openDevTools 失败", error);
    }
});

// ---------------------------------------------------------------------------
// waty-notify
// ---------------------------------------------------------------------------
async function sendNotification(withActions) {
    try {
        const result = await invoke("notify.show", {
            title: "waty-notify 演示",
            body: withActions
                ? "这是一条带按钮的通知。点击通知上的“查看”或“忽略”会回传事件。"
                : "这是一条来自 waty-notify 的系统通知。",
            actions: withActions,
        });
        log(withActions ? "notify.show(带按钮)" : "notify.show", result);
    } catch (error) {
        logError("notify.show 失败", error);
    }
}

document.getElementById("notify-basic").addEventListener("click", () => sendNotification(false));
document.getElementById("notify-actions").addEventListener("click", () => sendNotification(true));

window.waty.ipc.on("notify-action", (data) => log("notify-action", data));
window.waty.ipc.on("notify-closed", (data) => log("notify-closed", data));
window.waty.ipc.on("notify-error", (data) => log("notify-error", data));

// ---------------------------------------------------------------------------
// waty-tray
// ---------------------------------------------------------------------------
function renderTrayStatus(data) {
    if (data.ready) {
        trayStatusEl.textContent = `托盘已就绪 · 图标: ${data.icon} · 菜单: ${(data.menu || []).join(", ")}`;
    } else {
        trayStatusEl.textContent = `托盘未就绪: ${data.error || "未知错误"}`;
    }
}

async function refreshTrayStatus() {
    try {
        const data = await invoke("tray.status");
        renderTrayStatus(data);
        log("tray.status", data);
    } catch (error) {
        logError("tray.status 失败", error);
    }
}

document.getElementById("tray-status").addEventListener("click", refreshTrayStatus);
window.waty.ipc.on("tray-menu-clicked", (data) => log("tray 菜单点击", data));
window.waty.ipc.on("tray-status", (data) => {
    renderTrayStatus(data);
    log("tray-status(事件)", data);
});

// ---------------------------------------------------------------------------
// 本地视频流式播放
// ---------------------------------------------------------------------------
async function getVideoInfo() {
    if (!videoInfo) {
        videoInfo = await invoke("video.info");
        if (videoInfo.ok) {
            videoStatusEl.textContent = `视频就绪: ${videoInfo.path}（${formatBytes(videoInfo.size)}）`;
        } else {
            videoStatusEl.textContent = `视频不可用: ${videoInfo.error || videoInfo.path}`;
        }
        log("video.info", videoInfo);
    }
    return videoInfo;
}

document.getElementById("video-play").addEventListener("click", async () => {
    try {
        const info = await getVideoInfo();
        if (!info.ok || !info.url) {
            logError("video.play 失败", info.error || "本地视频流服务未启动");
            return;
        }
        videoEl.src = info.url;
        videoEl.load();
        await videoEl.play();
        videoStatusEl.textContent = `正在流式播放: ${info.path}`;
        log("video.play", { url: info.url, path: info.path, size: formatBytes(info.size) });
    } catch (error) {
        logError("video.play 失败", error);
    }
});

document.getElementById("video-fs-probe").addEventListener("click", async () => {
    try {
        const info = await getVideoInfo();
        if (!info.exists) {
            logError("fs 流式读取失败", "视频文件不存在");
            return;
        }

        const stream = await window.waty.fs.createReadStream(info.path, { highWaterMark: 256 * 1024 });
        const reader = stream.getReader();
        let total = 0;
        for (let index = 0; index < 4; index += 1) {
            const { value, done } = await reader.read();
            if (done) {
                break;
            }
            total += value.byteLength;
            log(`fs.createReadStream 第 ${index + 1} 块`, `${formatBytes(value.byteLength)}，累计 ${formatBytes(total)}`);
        }
        await reader.cancel();
        log("fs.createReadStream 探测完成", `${formatBytes(total)} / ${formatBytes(info.size)}`);
    } catch (error) {
        logError("fs.createReadStream 失败", error);
    }
});

document.getElementById("video-stop").addEventListener("click", () => {
    videoEl.pause();
    videoEl.removeAttribute("src");
    videoEl.load();
    videoStatusEl.textContent = "视频已停止";
    log("video.stop", "已释放视频源");
});

videoEl.addEventListener("loadedmetadata", () => {
    log("video.loadedmetadata", {
        duration: Number.isFinite(videoEl.duration) ? videoEl.duration.toFixed(1) + "s" : "unknown",
        width: videoEl.videoWidth,
        height: videoEl.videoHeight,
    });
});

videoEl.addEventListener("playing", () => log("video.playing", { currentTime: videoEl.currentTime.toFixed(1) }));
videoEl.addEventListener("waiting", () => log("video.waiting", "等待更多数据"));
videoEl.addEventListener("error", () => {
    const error = videoEl.error;
    log("video.error", error ? { code: error.code, message: error.message } : "unknown");
});

videoEl.addEventListener("progress", () => {
    if (!videoEl.buffered.length) {
        return;
    }
    const end = videoEl.buffered.end(videoEl.buffered.length - 1);
    const duration = Number.isFinite(videoEl.duration) ? videoEl.duration.toFixed(1) : "?";
    videoStatusEl.textContent = `已缓冲 ${end.toFixed(1)}s / ${duration}s（按需 HTTP Range 读取）`;
});

document.getElementById("clear-log").addEventListener("click", () => {
    outEl.textContent = "";
});

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------
(async function initialize() {
    log("waty 组件演示已加载", {
        ipc: Boolean(window.waty.ipc),
        fs: Boolean(window.waty.fs),
        dialog: Boolean(window.waty.dialog),
        window: Boolean(window.waty.window),
    });
    await refreshTrayStatus();
    await getVideoInfo();
})();
