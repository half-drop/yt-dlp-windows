#include "pch.h"
#include "Downloader.h"
#include "Config.h"

#include <sstream>
#include <vector>
#include <cstdlib>

namespace {

std::wstring Quote(const std::wstring& s) {
    std::wstring o = L"\"";
    for (wchar_t c : s) {
        if (c == L'"') o += L"\\\"";
        else o += c;
    }
    o += L"\"";
    return o;
}

bool FileExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool ExeOnPath(const wchar_t* name) {
    wchar_t buf[MAX_PATH] = {};
    return SearchPathW(nullptr, name, nullptr, MAX_PATH, buf, nullptr) != 0;
}

// Prefer tools sitting next to the exe (portable layout) over PATH.
std::vector<std::wstring> LocalToolDirs() {
    std::vector<std::wstring> dirs;
    const std::wstring exeDir = GetExeDir();
    dirs.push_back(exeDir);
    dirs.push_back(exeDir + L"\\deps");
    return dirs;
}

std::wstring FindLocalExe(const wchar_t* name) {
    for (const auto& dir : LocalToolDirs()) {
        const std::wstring p = dir + L"\\" + name;
        if (FileExists(p)) return p;
    }
    return {};
}

}  // namespace

Downloader::Downloader() = default;

Downloader::~Downloader() {
    Cancel();
    if (worker_.joinable()) worker_.join();
}

void Downloader::SetCallbacks(LogFn log, StatusFn status, ProgressFn progress, DoneFn done, ErrorFn error) {
    log_ = std::move(log);
    status_ = std::move(status);
    progress_ = std::move(progress);
    done_ = std::move(done);
    error_ = std::move(error);
}

void Downloader::SetOptions(bool useCookies, const std::wstring& browser, bool writeThumbnail,
                            const std::wstring& mergeFormat) {
    useCookies_ = useCookies;
    browser_ = browser;
    writeThumbnail_ = writeThumbnail;
    mergeFormat_ = mergeFormat.empty() ? L"mp4" : mergeFormat;
}

void Downloader::Cancel() {
    cancel_.store(true);
    if (process_) {
        TerminateProcess(process_, 1);
    }
}

bool Downloader::FfmpegOk() {
    for (const auto& dir : LocalToolDirs()) {
        if (FileExists(dir + L"\\ffmpeg.exe") && FileExists(dir + L"\\ffprobe.exe")) {
            return true;
        }
    }
    return ExeOnPath(L"ffmpeg.exe") && ExeOnPath(L"ffprobe.exe");
}

// Local portable dir containing both ffmpeg.exe and ffprobe.exe, or empty.
std::wstring Downloader::FindFfmpegDir() {
    for (const auto& dir : LocalToolDirs()) {
        if (FileExists(dir + L"\\ffmpeg.exe") && FileExists(dir + L"\\ffprobe.exe")) {
            return dir;
        }
    }
    return {};
}

std::wstring Downloader::FindPython() {
    // Prefer `py -3`, then `python`.
    if (ExeOnPath(L"py.exe")) return L"py";
    if (ExeOnPath(L"python.exe")) return L"python";
    if (ExeOnPath(L"python3.exe")) return L"python3";
    return {};
}

std::wstring Downloader::FindYtDlp() {
    std::wstring local = FindLocalExe(L"yt-dlp.exe");
    if (!local.empty()) return local;
    if (ExeOnPath(L"yt-dlp.exe")) return L"yt-dlp";
    return {};
}

bool Downloader::Start(const std::wstring& url, const std::wstring& outDir) {
    if (running_.load()) return false;
    cancel_.store(false);
    if (worker_.joinable()) worker_.join();
    running_.store(true);
    worker_ = std::thread(&Downloader::Worker, this, url, outDir);
    return true;
}

bool Downloader::SpawnAndWait(const std::wstring& cmdline, std::wstring& lastLine) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        if (error_) error_(L"CreatePipe 失败");
        return false;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::wstring mutableCmd = cmdline;
    BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),
        nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);

    CloseHandle(hWrite);
    hWrite = nullptr;

    if (!ok) {
        CloseHandle(hRead);
        if (error_) error_(L"无法启动 yt-dlp / python。请确认已安装 yt-dlp 并在 PATH 中。");
        return false;
    }

    process_ = pi.hProcess;
    CloseHandle(pi.hThread);

    // Reader thread would be nicer; sequential read is fine for a pipe.
    char buffer[4096];
    DWORD read = 0;
    std::string pending;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, nullptr) && read > 0) {
        if (cancel_.load()) break;
        buffer[read] = 0;
        pending.append(buffer, read);
        size_t nl;
        while ((nl = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, nl);
            pending.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            int wn = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), (int)line.size(), nullptr, 0);
            std::wstring wline(wn, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, line.c_str(), (int)line.size(), wline.data(), wn);
            lastLine = wline;

            // Parse progress:  "  12.3% of ~ 10.00MiB at 1.23MiB/s ETA 00:12"
            size_t pctPos = wline.find(L'%');
            if (pctPos != std::wstring::npos && pctPos > 0) {
                size_t start = pctPos;
                while (start > 0 && (iswdigit(wline[start - 1]) || wline[start - 1] == L'.')) --start;
                if (start < pctPos) {
                    double pct = _wtof(wline.substr(start, pctPos - start).c_str());
                    if (pct >= 0 && pct <= 100 && progress_) {
                        progress_((int)pct);
                    }
                    if (status_) status_(L"下载中: " + wline);
                    continue;
                }
            }
            if (status_) status_(wline.substr(0, 120));
            if (log_) log_(wline);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(hRead);
    process_ = nullptr;
    return code == 0;
}

void Downloader::Worker(std::wstring url, std::wstring outDir) {
    auto finishOk = [&](const std::wstring& msg) {
        running_.store(false);
        if (progress_) progress_(100);
        if (done_) done_(msg);
    };
    auto finishErr = [&](const std::wstring& msg) {
        running_.store(false);
        if (error_) error_(msg);
    };

    if (cancel_.load()) {
        finishErr(L"任务已取消");
        return;
    }

    if (!FfmpegOk()) {
        finishErr(L"未找到 ffmpeg / ffprobe。\n"
                  L"请运行 scripts\\fetch-deps.ps1 下载到 deps\\，或安装并加入 PATH。");
        return;
    }

    const std::wstring ffDir = FindFfmpegDir();
    if (log_) {
        log_(ffDir.empty() ? L"ffmpeg / ffprobe: 使用 PATH"
                           : (L"ffmpeg / ffprobe: " + ffDir));
    }

    if (status_) status_(L"准备 yt-dlp 命令行");

    std::wstring exe = FindYtDlp();
    std::wstring base;
    if (!exe.empty()) {
        base = Quote(exe);
        if (log_ && exe.find(L'\\') != std::wstring::npos) {
            log_(L"yt-dlp: " + exe);
        }
    } else {
        std::wstring py = FindPython();
        if (py.empty()) {
            finishErr(L"未找到 yt-dlp 或 Python。\n"
                      L"请运行 scripts\\fetch-deps.ps1 下载 yt-dlp.exe 到 deps\\，或安装 yt-dlp / Python。");
            return;
        }
        base = Quote(py);
        if (py == L"py") base += L" -3";
        base += L" -m yt_dlp";
    }

    std::wstring outTmpl = outDir + L"\\%(title).180B [%(id)s].%(ext)s";
    std::wstring cmd = base;
    cmd += L" --newline --no-playlist --no-warnings --quiet";
    cmd += L" -f bv+ba/b";
    cmd += L" --merge-output-format " + Quote(mergeFormat_);
    cmd += L" -o " + Quote(outTmpl);
    if (!ffDir.empty()) {
        cmd += L" --ffmpeg-location " + Quote(ffDir);
    }
    if (writeThumbnail_) {
        cmd += L" --write-thumbnail";
    }
    if (useCookies_) {
        cmd += L" --cookies-from-browser " + Quote(browser_);
    }
    cmd += L" " + Quote(url);

    if (log_) log_(L"命令: " + cmd);
    if (status_) status_(L"下载视频 / 音频");

    std::wstring lastLine;
    bool ok = SpawnAndWait(cmd, lastLine);

    if (cancel_.load()) {
        finishErr(L"任务已取消");
        return;
    }
    if (!ok) {
        std::wstring msg = L"yt-dlp 执行失败";
        if (!lastLine.empty()) msg += L"\n" + lastLine;
        finishErr(msg);
        return;
    }

    finishOk(L"下载完成\n\n保存目录: " + outDir);
}
