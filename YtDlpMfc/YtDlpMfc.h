#pragma once

#ifndef __AFXWIN_H__
#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CYtDlpMfcApp : public CWinApp {
public:
    CYtDlpMfcApp();

public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CYtDlpMfcApp theApp;
