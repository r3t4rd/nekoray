# NekoBox

<img src="https://raw.githubusercontent.com/r3t4rd/nekoray/refs/heads/main/ahertaw34a3.jpg" width="1234" alt="NekoBox screenshot"/>

Qt-based cross-platform GUI proxy configuration manager. Backend: **sing-box**.

This repository is a maintained fork of [MatsuriDayo/nekoray](https://github.com/MatsuriDayo/nekoray), published as **[r3t4rd/nekoray](https://github.com/r3t4rd/nekoray)**.

**Current release version:** `5-2026-07-25.1`  
**Supported platforms:** Windows x64 (portable ZIP), Linux

---

## Download

Portable builds (no installer). Extract and run `nekobox.exe` (Windows) or the packaged binary (Linux).

**Releases:** https://github.com/r3t4rd/nekoray/releases

Windows asset naming example:

```text
nekoray-5-2026-07-25.1-windows64.zip
```

If Windows reports missing DLLs, install the [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

Do not remove `nekobox_core.exe` or the `geo*` files next to the GUI.

---

## What’s inside a release

| File | Role |
|------|------|
| `nekobox.exe` | Qt GUI |
| `nekobox_core.exe` | sing-box core + gRPC control plane |
| `updater.exe` | In-app update helper |
| `geoip.dat` / `geosite.dat` | v2ray-style routing lists |
| `geoip.db` / `geosite.db` | sing-box routing databases |
| Qt / OpenSSL DLLs | Runtime (Windows portable) |

---

## Stack (what we use and where it comes from)

### Application

| Component | Source | Notes |
|-----------|--------|--------|
| GUI (NekoBox) | this repo (`r3t4rd/nekoray`) | C++17, CMake, Ninja, MSVC on Windows |
| Upstream project | [MatsuriDayo/nekoray](https://github.com/MatsuriDayo/nekoray) | Original NekoRay / NekoBox |
| Core wrapper | `go/cmd/nekobox_core` | Builds `nekobox_core` |
| Updater | `go/cmd/updater` | Builds `updater` |
| gRPC bridge | `go/grpc_server` | GUI ↔ core control API |
| Version stamp | `nekoray_version.txt` | Embedded at build time (`NKR_VERSION` / Go ldflags) |
| Auto-update API | GitHub Releases of **r3t4rd/nekoray** | `https://api.github.com/repos/r3t4rd/nekoray/releases` |

### Core (proxy engine)

| Component | Source | Version / branch |
|-----------|--------|------------------|
| **sing-box** | [MatsuriDayo/sing-box](https://github.com/MatsuriDayo/sing-box) (`1.12.x`) | **`1.12.19-neko-1`** |
| Upstream sing-box | [SagerNet/sing-box](https://github.com/SagerNet/sing-box) | Base project |
| **libneko** | [MatsuriDayo/libneko](https://github.com/MatsuriDayo/libneko) | Shared Go helpers / version helpers |
| Go toolchain | Go **1.23+** (local builds may use Go 1.22.12+) | See `go/cmd/nekobox_core/go.mod` |

**Core build tags** (Windows deploy / `deploy_windows64.ps1`):

```text
with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls
```

> Note: `with_ech` is no longer used — ECH is covered by the Go stdlib in sing-box 1.12+.

Local `replace` paths used when building the core:

- `github.com/sagernet/sing-box` → sibling `sing-box` checkout  
- `github.com/matsuridayo/libneko` → sibling `libneko` checkout  

### GUI framework & C++ libraries

| Library | Source | Version / usage |
|---------|--------|-----------------|
| **Qt** | [Qt](https://www.qt.io/) | **Qt 6** (Widgets, Network, Svg, LinguistTools); Windows release uses Qt 6.5.x SDK |
| **protobuf** | [protocolbuffers/protobuf](https://github.com/protocolbuffers/protobuf) | **v21.4** (static, via `libs/deps`) |
| **gRPC / myproto** | generated from project `.proto` | GUI ↔ `nekobox_core` |
| **yaml-cpp** | [jbeder/yaml-cpp](https://github.com/jbeder/yaml-cpp) | **0.7.0** |
| **zxing-cpp** | [nu-book/zxing-cpp](https://github.com/nu-book/zxing-cpp) | **v2.0.0** (QR import) |
| **QHotkey** | [Skycoder42/QHotkey](https://github.com/Skycoder42/QHotkey) | Vendored under `3rdparty/QHotkey` |
| **OpenSSL 3** | Bundled with Qt SDK / deploy | `libcrypto-3-x64.dll`, `libssl-3-x64.dll` on Windows |
| CMake / Ninja / MSVC | Build tools | VS 2022 Build Tools on Windows |

Dependency build helpers in-tree: `build_deps.bat`, `build_deps2.bat`, `build_protobuf.bat`, `libs/deps/`.

### Geodata (downloaded at package time)

| File | Upstream |
|------|----------|
| `geoip.dat` | [Loyalsoldier/v2ray-rules-dat](https://github.com/Loyalsoldier/v2ray-rules-dat) (latest release) |
| `geosite.dat` | [v2fly/domain-list-community](https://github.com/v2fly/domain-list-community) (`dlc.dat`) |
| `geoip.db` | [SagerNet/sing-geoip](https://github.com/SagerNet/sing-geoip) (latest release) |
| `geosite.db` | [SagerNet/sing-geosite](https://github.com/SagerNet/sing-geosite) (latest release) |

### Packaging (Windows)

| Tool / script | Purpose |
|---------------|---------|
| `deploy_windows64.ps1` | Build core + GUI, `windeployqt`, fetch geodata, zip release |
| `windeployqt` | Qt DLL deployment |
| Output layout | `deployment/windows64/` and `deployment/nekoray-*-windows64.zip` |

Release ZIP layout expected by the updater: top-level folder `nekoray/`.

---

## Supported proxies

- SOCKS (4 / 4a / 5)
- HTTP(S)
- Shadowsocks
- VMess
- VLESS
- Trojan
- TUIC (sing-box)
- Hysteria2 (sing-box)
- NaïveProxy (custom core)
- Custom outbound / custom config / custom core
- Proxy chains

## Subscriptions

Raw subscription formats commonly used by Shadowsocks, Clash, and v2rayN clients.

---

## Run flags

See [docs/RunFlags.md](docs/RunFlags.md).

## Build

Technical docs:

- [docs/readme.md](docs/readme.md) — index
- [docs/Build_Windows.md](docs/Build_Windows.md) — Windows GUI
- [docs/Build_Linux.md](docs/Build_Linux.md) — Linux GUI
- [docs/Build_Core.md](docs/Build_Core.md) — Go core (`sing-box` + `libneko`)
- [docs/Run_Linux.md](docs/Run_Linux.md) — Linux runtime notes

For a full Windows x64 portable package from this tree, use:

```powershell
.\deploy_windows64.ps1
```

Typical sibling directory layout for the Go core:

```text
Working/
  nekobox/          # this repo (GUI + go/cmd/*)
  sing-box/         # MatsuriDayo/sing-box @ 1.12.19-neko-1
  libneko/          # MatsuriDayo/libneko
```

---

## Credits

**Core**

- [SagerNet/sing-box](https://github.com/SagerNet/sing-box)
- [MatsuriDayo/sing-box](https://github.com/MatsuriDayo/sing-box) (`1.12.19-neko-1`)
- [MatsuriDayo/libneko](https://github.com/MatsuriDayo/libneko)

**GUI & tooling**

- [MatsuriDayo/nekoray](https://github.com/MatsuriDayo/nekoray) (upstream)
- [Qv2ray](https://github.com/Qv2ray/Qv2ray) (historical GUI inspiration)
- [Qt](https://www.qt.io/)
- [protobuf](https://github.com/protocolbuffers/protobuf)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [zxing-cpp](https://github.com/nu-book/zxing-cpp)
- [QHotkey](https://github.com/Skycoder42/QHotkey)

**Geodata**

- [Loyalsoldier/v2ray-rules-dat](https://github.com/Loyalsoldier/v2ray-rules-dat)
- [v2fly/domain-list-community](https://github.com/v2fly/domain-list-community)
- [SagerNet/sing-geoip](https://github.com/SagerNet/sing-geoip)
- [SagerNet/sing-geosite](https://github.com/SagerNet/sing-geosite)

---

## License

See the repository license files and the licenses of third-party components listed above.
