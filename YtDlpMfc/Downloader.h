#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>

// Runs yt-dlp (python -m yt_dlp) as a child process.
// Progress is parsed from yt-dlp --newline progress lines.
class Downloader {
public:
    using LogFn = std::function<void(const std::wstring&)>;
    using StatusFn = std::function<void(const std::wstring&)>;
    using ProgressFn = std::function<void(int percent)>;
    using DoneFn = std::function<void(const std::wstring& message)>;
    using ErrorFn = std::function<void(const std::wstring& message)>;

    Downloader();
    ~Downloader();

    void SetCallbacks(LogFn log, StatusFn status, ProgressFn progress, DoneFn done, ErrorFn error);
    void SetOptions(bool useCookies, const std::wstring& browser, bool writeThumbnail,
                    const std::wstring& mergeFormat);

    bool IsRunning() const { return running_.load(); }
    void Cancel();

    // Non-blocking: starts worker thread.
    bool Start(const std::wstring& url, const std::wstring& outDir);

    static bool FfmpegOk();
    static std::wstring FindPython();
    static std::wstring FindYtDlp();
    // Directory containing local ffmpeg.exe/ffprobe.exe (exe dir or deps/), or empty.
    static std::wstring FindFfmpegDir();

private:
    void Worker(std::wstring url, std::wstring outDir);
    bool SpawnAndWait(const std::wstring& cmdline, std::wstring& lastLine);

    LogFn log_;
    StatusFn status_;
    ProgressFn progress_;
    DoneFn done_;
    ErrorFn error_;

    bool useCookies_ = false;
    std::wstring browser_ = L"chrome";
    bool writeThumbnail_ = true;
    std::wstring mergeFormat_ = L"mp4";

    std::atomic<bool> running_{false};
    std::atomic<bool> cancel_{false};
    std::thread worker_;
    HANDLE process_ = nullptr;
};
