#include "pch.h"
#include "YtDlpMfc.h"
#include "YtDlpMfcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CYtDlpMfcApp theApp;

BEGIN_MESSAGE_MAP(CYtDlpMfcApp, CWinApp)
END_MESSAGE_MAP()

CYtDlpMfcApp::CYtDlpMfcApp() {
}

BOOL CYtDlpMfcApp::InitInstance() {
    CWinApp::InitInstance();

    // Enable Common Controls 6 (progress bar, etc.)
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_PROGRESS_CLASS | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    AfxEnableControlContainer();

    CYtDlpMfcDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK) {
        // closed with OK
    } else if (nResponse == IDCANCEL) {
        // closed with Cancel
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    // application, rather than start the application's message pump.
    return FALSE;
}

int CYtDlpMfcApp::ExitInstance() {
    return CWinApp::ExitInstance();
}
