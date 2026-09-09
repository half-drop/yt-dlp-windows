#include "pch.h"
#include "YtDlpMfc.h"
#include "YtDlpMfcDlg.h"
#include "Filenames.h"

#include <afxdialogex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

const wchar_t* kBrowsers[] = {
    L"chrome", L"edge", L"firefox", L"brave", L"vivaldi", L"opera"
};

void PostString(HWND hwnd, UINT msg, const CString& text) {
    auto* ps = new PostedString{ text };
    if (!::PostMessageW(hwnd, msg, 0, reinterpret_cast<LPARAM>(ps))) {
        delete ps;
    }
}

}  // namespace

CYtDlpMfcDlg::CYtDlpMfcDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_YTDLP_DIALOG, pParent) {
    icon_ = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CYtDlpMfcDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URL, urlEdit_);
    DDX_Control(pDX, IDC_DIR, dirEdit_);
    DDX_Control(pDX, IDC_LOG, logEdit_);
    DDX_Control(pDX, IDC_COOKIES, cookiesCheck_);
    DDX_Control(pDX, IDC_BROWSER, browserCombo_);
    DDX_Control(pDX, IDC_PROGRESS, progress_);
    DDX_Control(pDX, IDC_STATUS, statusLabel_);
    DDX_Control(pDX, IDC_FFMPEG, ffmpegLabel_);
    DDX_Control(pDX, IDC_START, startBtn_);
    DDX_Control(pDX, IDC_STOP, stopBtn_);
}

BEGIN_MESSAGE_MAP(CYtDlpMfcDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_PASTE, &CYtDlpMfcDlg::OnPaste)
    ON_BN_CLICKED(IDC_BROWSE, &CYtDlpMfcDlg::OnBrowse)
    ON_BN_CLICKED(IDC_START, &CYtDlpMfcDlg::OnStart)
    ON_BN_CLICKED(IDC_STOP, &CYtDlpMfcDlg::OnStop)
    ON_BN_CLICKED(IDC_OPEN_DIR, &CYtDlpMfcDlg::OnOpenDir)
    ON_BN_CLICKED(IDC_CLEAR_LOG, &CYtDlpMfcDlg::OnClearLog)
    ON_MESSAGE(WM_APP_LOG, &CYtDlpMfcDlg::OnWorkerLog)
    ON_MESSAGE(WM_APP_STATUS, &CYtDlpMfcDlg::OnWorkerStatus)
    ON_MESSAGE(WM_APP_PROGRESS, &CYtDlpMfcDlg::OnWorkerProgress)
    ON_MESSAGE(WM_APP_DONE, &CYtDlpMfcDlg::OnWorkerDone)
    ON_MESSAGE(WM_APP_ERROR, &CYtDlpMfcDlg::OnWorkerError)
END_MESSAGE_MAP()

BOOL CYtDlpMfcDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    if (icon_) {
        SetIcon(icon_, TRUE);
        SetIcon(icon_, FALSE);
    }

    // Localized UI strings (RC uses ASCII placeholders for encoding safety).
    SetWindowText(L"YT-DLP Downloader (MFC)");
    SetDlgItemText(IDC_LABEL_URL, L"视频链接");
    SetDlgItemText(IDC_LABEL_DIR, L"保存目录");
    SetDlgItemText(IDC_LABEL_BROWSER, L"浏览器");
    SetDlgItemText(IDC_PASTE, L"粘贴");
    SetDlgItemText(IDC_BROWSE, L"选择文件夹");
    SetDlgItemText(IDC_COOKIES, L"使用浏览器 Cookies（年龄限制 / 403 时有用）");
    SetDlgItemText(IDC_START, L"开始下载");
    SetDlgItemText(IDC_STOP, L"停止");
    SetDlgItemText(IDC_OPEN_DIR, L"打开保存目录");
    SetDlgItemText(IDC_CLEAR_LOG, L"清空日志");
    SetDlgItemText(IDC_NOTE,
        L"功能：最佳视频+最佳音频自动下载，完成后交给 ffmpeg 合并；同时抓取最高分辨率封面。");

    for (auto b : kBrowsers) {
        browserCombo_.AddString(b);
    }

    config_ = AppConfig::Load(GetDefaultConfigPath());
    if (config_.outputDir.empty()) {
        config_.outputDir = GetExeDir() + L"\\downloads";
    }
    LoadConfigToUi();

    progress_.SetRange32(0, 100);
    progress_.SetPos(0);

    downloader_ = std::make_unique<Downloader>();
    UpdateFfmpegLabel();
    SetRunning(false);

    urlEdit_.SetFocus();
    return FALSE;  // we set focus
}

void CYtDlpMfcDlg::LoadConfigToUi() {
    urlEdit_.SetWindowText(config_.lastUrl.c_str());
    dirEdit_.SetWindowText(config_.outputDir.c_str());
    cookiesCheck_.SetCheck(config_.useCookies ? BST_CHECKED : BST_UNCHECKED);

    int sel = 0;
    for (int i = 0; i < (int)(sizeof(kBrowsers) / sizeof(kBrowsers[0])); ++i) {
        if (config_.browser == kBrowsers[i]) { sel = i; break; }
    }
    browserCombo_.SetCurSel(sel);
}

CString CYtDlpMfcDlg::GetBrowser() const {
    int sel = browserCombo_.GetCurSel();
    if (sel < 0) return L"chrome";
    CString s;
    browserCombo_.GetLBText(sel, s);
    return s;
}

bool CYtDlpMfcDlg::CookiesChecked() const {
    return cookiesCheck_.GetCheck() == BST_CHECKED;
}

void CYtDlpMfcDlg::UpdateFfmpegLabel() {
    if (Downloader::FfmpegOk()) {
        ffmpegLabel_.SetWindowText(L"ffmpeg: OK");
    } else {
        ffmpegLabel_.SetWindowText(L"ffmpeg: 未找到");
    }
}

void CYtDlpMfcDlg::AppendLog(const CString& text) {
    int len = logEdit_.GetWindowTextLength();
    logEdit_.SetSel(len, len);
    logEdit_.ReplaceSel(text + L"\r\n");
}

void CYtDlpMfcDlg::SetRunning(bool running) {
    downloading_ = running;
    startBtn_.EnableWindow(running ? FALSE : TRUE);
    stopBtn_.EnableWindow(running ? TRUE : FALSE);
    GetDlgItem(IDC_BROWSE)->EnableWindow(running ? FALSE : TRUE);
    GetDlgItem(IDC_PASTE)->EnableWindow(running ? FALSE : TRUE);
}

void CYtDlpMfcDlg::SaveConfig() {
    CString url, dir;
    urlEdit_.GetWindowText(url);
    dirEdit_.GetWindowText(dir);
    config_.lastUrl = url;
    config_.outputDir = dir;
    config_.useCookies = CookiesChecked();
    config_.browser = GetBrowser();
    config_.Save(GetDefaultConfigPath());
}

void CYtDlpMfcDlg::OnSysCommand(UINT nID, LPARAM lParam) {
    CDialogEx::OnSysCommand(nID, lParam);
}

void CYtDlpMfcDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - GetSystemMetrics(SM_CXICON) + 1) / 2;
        int y = (rect.Height() - GetSystemMetrics(SM_CYICON) + 1) / 2;
        dc.DrawIcon(x, y, icon_);
    } else {
        CDialogEx::OnPaint();
    }
}

HCURSOR CYtDlpMfcDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(icon_);
}

void CYtDlpMfcDlg::OnPaste() {
    if (!OpenClipboard()) return;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        LPCWSTR p = static_cast<LPCWSTR>(GlobalLock(h));
        if (p) {
            urlEdit_.SetWindowText(p);
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
}

void CYtDlpMfcDlg::OnBrowse() {
    CFolderPickerDialog dlg(nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
    CString cur;
    dirEdit_.GetWindowText(cur);
    if (!cur.IsEmpty()) {
        dlg.m_ofn.lpstrInitialDir = cur;
    }
    if (dlg.DoModal() == IDOK) {
        dirEdit_.SetWindowText(dlg.GetPathName());
    }
}

void CYtDlpMfcDlg::OnOpenDir() {
    CString dir;
    dirEdit_.GetWindowText(dir);
    if (dir.IsEmpty()) {
        AfxMessageBox(L"请先设置保存目录", MB_ICONWARNING);
        return;
    }
    CreateDirectoryW(dir, nullptr);
    ShellExecuteW(GetSafeHwnd(), L"open", dir, nullptr, nullptr, SW_SHOWNORMAL);
}

void CYtDlpMfcDlg::OnClearLog() {
    logEdit_.SetWindowText(L"");
}

void CYtDlpMfcDlg::OnStart() {
    if (downloading_) {
        AfxMessageBox(L"已有任务正在下载", MB_ICONWARNING);
        return;
    }

    CString url, dir;
    urlEdit_.GetWindowText(url);
    dirEdit_.GetWindowText(dir);
    url.Trim();
    dir.Trim();
    if (url.IsEmpty()) {
        AfxMessageBox(L"请先输入视频链接", MB_ICONWARNING);
        urlEdit_.SetFocus();
        return;
    }
    if (dir.IsEmpty()) {
        AfxMessageBox(L"请先设置保存目录", MB_ICONWARNING);
        return;
    }

    CreateDirectoryW(dir, nullptr);
    SaveConfig();
    SetRunning(true);
    progress_.SetPos(0);
    statusLabel_.SetWindowText(L"准备中");
    AppendLog(L"============================================================");
    AppendLog(L"开始任务: " + url);
    AppendLog(L"保存目录: " + dir);

    HWND hwnd = GetSafeHwnd();
    downloader_->SetCallbacks(
        [hwnd](const std::wstring& s) { PostString(hwnd, WM_APP_LOG, CString(s.c_str())); },
        [hwnd](const std::wstring& s) { PostString(hwnd, WM_APP_STATUS, CString(s.c_str())); },
        [hwnd](int pct) { ::PostMessageW(hwnd, WM_APP_PROGRESS, pct, 0); },
        [hwnd](const std::wstring& s) { PostString(hwnd, WM_APP_DONE, CString(s.c_str())); },
        [hwnd](const std::wstring& s) { PostString(hwnd, WM_APP_ERROR, CString(s.c_str())); });

    downloader_->SetOptions(CookiesChecked(), GetBrowser().GetString(),
                            config_.writeThumbnail, config_.mergeFormat);

    if (!downloader_->Start(url.GetString(), dir.GetString())) {
        SetRunning(false);
        AfxMessageBox(L"无法启动下载线程", MB_ICONERROR);
    }
}

void CYtDlpMfcDlg::OnStop() {
    if (downloader_ && downloader_->IsRunning()) {
        downloader_->Cancel();
        statusLabel_.SetWindowText(L"正在取消…");
    }
}

void CYtDlpMfcDlg::OnDestroy() {
    SaveConfig();
    if (downloader_) downloader_->Cancel();
    CDialogEx::OnDestroy();
}

void CYtDlpMfcDlg::OnClose() {
    if (downloading_) {
        if (AfxMessageBox(L"正在下载，确定要退出吗？", MB_YESNO | MB_ICONWARNING) != IDYES) {
            return;
        }
        if (downloader_) downloader_->Cancel();
    }
    SaveConfig();
    CDialogEx::OnClose();
}

LRESULT CYtDlpMfcDlg::OnWorkerLog(WPARAM, LPARAM lParam) {
    auto* ps = reinterpret_cast<PostedString*>(lParam);
    if (ps) {
        AppendLog(ps->text);
        delete ps;
    }
    return 0;
}

LRESULT CYtDlpMfcDlg::OnWorkerStatus(WPARAM, LPARAM lParam) {
    auto* ps = reinterpret_cast<PostedString*>(lParam);
    if (ps) {
        statusLabel_.SetWindowText(ps->text);
        delete ps;
    }
    return 0;
}

LRESULT CYtDlpMfcDlg::OnWorkerProgress(WPARAM wParam, LPARAM) {
    progress_.SetPos(static_cast<int>(wParam));
    return 0;
}

LRESULT CYtDlpMfcDlg::OnWorkerDone(WPARAM, LPARAM lParam) {
    auto* ps = reinterpret_cast<PostedString*>(lParam);
    CString msg = ps ? ps->text : CString(L"完成");
    delete ps;
    SetRunning(false);
    statusLabel_.SetWindowText(L"完成");
    progress_.SetPos(100);
    AfxMessageBox(msg, MB_ICONINFORMATION);
    return 0;
}

LRESULT CYtDlpMfcDlg::OnWorkerError(WPARAM, LPARAM lParam) {
    auto* ps = reinterpret_cast<PostedString*>(lParam);
    CString msg = ps ? ps->text : CString(L"失败");
    delete ps;
    SetRunning(false);
    statusLabel_.SetWindowText(L"失败");
    progress_.SetPos(0);
    AfxMessageBox(msg, MB_ICONERROR);
    return 0;
}
