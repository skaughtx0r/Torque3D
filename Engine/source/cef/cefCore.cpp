//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/module.h"

#include "cefCore.h"
#include "webApp.h"
#include "cef/webView.h"

#include "platformWin32/platformWin32.h"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"

#ifdef WIN32
   #ifdef _DEBUG
      #define SUBPROC_NAME "cefproc_DEBUG.exe"
   #else
      #define SUBPROC_NAME "cefproc.exe"
   #endif
#endif

CefCore* CefCore::sInstance = NULL;

void WebThreadFunc(void* self)
{
   CefCore::get()->run();
}

MODULE_BEGIN( GuiWebCore )

MODULE_INIT
{
   CefCore* core = new CefCore(WebThreadFunc);
   core->start();
}

MODULE_SHUTDOWN_AFTER(Sim)

MODULE_SHUTDOWN
{
   CefCore::get()->exit();

   if (!CefCore::get()->running())
   {
      delete CefCore::get();
   }
}

MODULE_END;


CefCore::CefCore(ThreadRunFunction func) : Thread(func)
{
   AssertISV(!sInstance, "Multiple Web Cores!");
   sInstance = this;
   mLockCount = 0;

}

CefCore::~CefCore()
{
   
}

void CefCore::exit()
{
   CefPostTask(TID_UI, base::Bind(&CefQuitMessageLoop));

   mMutex.lock();

   stop();

   mMutex.unlock();

   // wait up for 10 seconds for web thread to exit
   //mStopSemaphore.acquire(true, 10000);

   //join();
}

bool CefCore::running()
{
   mMutex.lock();

   bool b = isAlive();

   mMutex.unlock();

   return b;

}

void CefCore::run()
{
   CefMainArgs main_args;

   CefRefPtr<WebApp> app(new WebApp());

   CefSettings settings;
   settings.command_line_args_disabled = true;
   settings.windowless_rendering_enabled = true;
   settings.context_safety_implementation = true;
   settings.remote_debugging_port = 1337;
   CefString(&settings.browser_subprocess_path).FromASCII(SUBPROC_NAME);   

   CefInitialize(main_args, settings, app, NULL);

   CefRunMessageLoop();

   //CefShutdown();
}

void CefCore::CreateWebView(String url, CefRefPtr<WebView> webView)
{
   CefWindowInfo window_info;
   CefBrowserSettings browserSettings;
   window_info.SetAsWindowless(NULL, true);
   CefBrowserHost::CreateBrowser(window_info, webView.get(), url.c_str(), browserSettings, NULL);
}