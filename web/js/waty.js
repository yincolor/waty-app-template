const out = (s) => { document.getElementById("out").textContent += s + "\n"; };

document.getElementById("ipc").onclick = async () => {
    const r = await window.waty.ipc.invoke("greet", "waty");
    out("ipc: " + JSON.stringify(r));
};

document.getElementById("os").onclick = async () => {
    const platform = await window.waty.os.platform();
    const homedir = await window.waty.os.homedir();
    const userData = await window.waty.os.getPath("config");
    out(`platform=${platform} homedir=${homedir} config=${userData}`);
};

document.getElementById("fs").onclick = async () => {
    const stats = await window.waty.fs.stat("index.html");
    out("fs.stat: " + JSON.stringify(stats));
};

document.getElementById("win").onclick = async () => {
    const size = await window.waty.window.getSize();
    out("window.getSize: " + JSON.stringify(size));
};

document.getElementById("emit").onclick = () => {
    out("等待 C++ emit...");
};

if (window.waty.ipc) {
    window.waty.ipc.on("server-ping", (data) => {
        out("收到 C++ 推送: " + JSON.stringify(data));
    });

    // 页面加载后自动调用一次 IPC，验证 JS → C++ 桥
    window.waty.ipc.invoke("greet", "auto-test").then((r) => {
        console.log("IPC 结果: " + JSON.stringify(r));
        out("自动 IPC 结果: " + JSON.stringify(r));
    }).catch((e) => {
        console.error("IPC 错误: " + JSON.stringify(e));
        out("自动 IPC 错误: " + JSON.stringify(e));
    });

    if (window.waty.fs) {
        window.waty.fs.stat("/tmp").then((s) => {
            console.log("fs.stat /tmp: " + JSON.stringify(s));
        }).catch((e) => {
            console.error("fs.stat 错误: " + JSON.stringify(e));
        });
    }

    if (window.waty.os) {
        Promise.all([
            window.waty.os.getPath("appData"),
            window.waty.os.getPath("config"),
            window.waty.os.getPath("executable")
        ]).then(([appData, config, executable]) => {
            console.log("os.getPath: appData=" + appData + " config=" + config + " executable=" + executable);
        }).catch((e) => {
            console.error("os.getPath 错误: " + JSON.stringify(e));
        });
    }
}