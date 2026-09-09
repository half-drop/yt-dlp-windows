#pragma once

#include "resource.h"
#include "Config.h"
#include "Downloader.h"

#include <memory>

// Custom messages posted from the download worker to the UI thread.
#define WM_APP_LOG      (WM_APP + 1)
#define WM_APP_STATUS   (WM_APP + 2)
#define WM_APP_PROGRESS (WM_APP + 3)
#define WM_APP_DONE     (WM_APP + 4)
#define WM_APP_ERROR    (WM_APP + 5)

class CYtDlpMfcDlg : public CDialogEx {
public:
    explicit CYtDlpMfcDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_YTDLP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnPaste();
    afx_msg void OnBrowse();
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg void OnOpenDir();
    afx_msg void OnClearLog();
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg LRESULT OnWorkerLog(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerStatus(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerDone(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerError(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    void AppendLog(const CString& text);
    void SetRunning(bool running);
    void SaveConfig();
    void LoadConfigToUi();
    CString GetBrowser() const;
    bool CookiesChecked() const;
    void UpdateFfmpegLabel();

    CEdit urlEdit_;
    CEdit dirEdit_;
    CEdit logEdit_;
    CButton cookiesCheck_;
    CComboBox browserCombo_;
    CProgressCtrl progress_;
    CStatic statusLabel_;
    CStatic ffmpegLabel_;
    CButton startBtn_;
    CButton stopBtn_;

    AppConfig config_;
    std::unique_ptr<Downloader> downloader_;
    HICON icon_ = nullptr;
    bool downloading_ = false;
};

// Heap string carried in posted messages (deleted on UI thread).
struct PostedString {
    CString text;
};
