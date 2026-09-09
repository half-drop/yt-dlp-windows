# yt-dlp-windows

用 **MFC（Microsoft Foundation Classes）** 重写的 yt-dlp 下载器 GUI。  
C++ / 原生 Windows 对话框，通过子进程调用 `yt-dlp`。

开箱即用：Release 压缩包内自带 `YtDlpMfc.exe` + `deps\`（`yt-dlp.exe`、`ffmpeg.exe`、`ffprobe.exe`），解压即可运行，无需另装 Python / VC++ 运行库。

## 功能

- 最佳视频 + 音频合并（自带 ffmpeg）
- 优先使用 exe 旁 `deps\` 工具，找不到再回落 PATH / Python 模块
- 可选写入封面（`--write-thumbnail`）
- 浏览器 Cookies（`--cookies-from-browser`）
- 取消下载（TerminateProcess）
- 进度条 + 实时日志
- 配置持久化（`config.json`，与 exe 同目录）
- 静态链接 MFC，单 exe 可独立运行

## 下载（推荐）

从 [Releases](../../releases) 下载 `yt-dlp-windows-win-x64.zip`，解压后直接运行 `YtDlpMfc.exe`。

发布物由 GitHub Actions 在打 `v*` 标签时自动构建并附带运行时依赖。

## 本地开发

### 环境要求（仅编译时）

| 组件 | 说明 |
|---|---|
| Visual Studio 2022 | 工作负荷：**使用 C++ 的桌面开发** |
| MFC / ATL | 组件：**C++ MFC for latest v143 build tools** |
| Windows 10/11 SDK | 随 VS 安装 |

### 拉取运行时依赖

```powershell
.\scripts\fetch-deps.ps1
```

会把 `yt-dlp.exe`、`ffmpeg.exe`、`ffprobe.exe` 下载到仓库根目录 `deps\`（已 gitignore）。

### 编译

```powershell
.\scripts\build.ps1
# 或在 Developer PowerShell 中
msbuild YtDlpMfc.sln /p:Configuration=Release /p:Platform=x64 /m
```

输出：`bin\x64\Release\YtDlpMfc.exe`

### 运行（便携布局）

把 exe 和 `deps\` 放在一起即可：

```
YtDlpMfc.exe
deps\
  yt-dlp.exe
  ffmpeg.exe
  ffprobe.exe
```

也可直接跑 `bin\x64\Release\YtDlpMfc.exe`（会向上/同级找 `deps\`，否则回落 PATH）。

## 发布

推送 `v*` 标签会触发 Actions：

```powershell
git tag v1.0.0
git push origin v1.0.0
```

工作流：`MSBuild Release|x64` → `fetch-deps.ps1` 打包依赖 → 上传 artifact / 创建 Release 附带 zip。  
手动构建（`workflow_dispatch`）只产出 artifact，不发 Release。

## 项目结构

```
yt-dlp-windows/
  YtDlpMfc.sln
  .github/workflows/release.yml
  scripts/build.ps1
  scripts/fetch-deps.ps1
  YtDlpMfc/
    YtDlpMfc.vcxproj     # 静态 MFC
    YtDlpMfc.rc          # 对话框模板 + 版本资源
    resource.h
    YtDlpMfc.h/.cpp      # CWinApp
    YtDlpMfcDlg.h/.cpp   # 主对话框 UI / 事件
    Downloader.h/.cpp    # 子进程 yt-dlp + 本地 deps 发现 + 取消
    Config.h/.cpp        # config.json
    Filenames.h/.cpp     # Windows 安全文件名
    framework.h / pch.*
```

## 工具查找顺序

1. `<exe 目录>\yt-dlp.exe` / `ffmpeg.exe` / `ffprobe.exe`
2. `<exe 目录>\deps\` 下同名文件
3. 系统 PATH（`yt-dlp`、`ffmpeg`）
4. 最后回落 `python -m yt_dlp`（需已装 Python + yt-dlp）

找到本地 ffmpeg 时自动追加 `--ffmpeg-location`。

## 已知限制

- 下载进度解析依赖 yt-dlp `--newline` 输出格式，极新版本若改格式需同步调整。
- 封面文件名由 yt-dlp 默认模板生成（`标题 [ID].webp` 等），不保证与 GUI 安全文件名完全一致。
- 对话框为固定初始布局，窗口可拉伸但控件不随窗口重排（可后续加 `OnSize`）。
