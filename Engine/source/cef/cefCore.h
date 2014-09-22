//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIWEBCORE_H_
#define _GUIWEBCORE_H_

#ifndef _MRECT_H_
#include "math/mRect.h"
#endif

#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

#ifndef _PLATFORM_THREADS_THREAD_H_
#include "platform/threads/thread.h"
#endif

#ifndef _PLATFORM_THREAD_SEMAPHORE_H_
#include "platform/threads/semaphore.h"
#endif

#ifndef _THREADSAFEDEQUE_H_
   #include "platform/threads/threadSafeDeque.h"
#endif

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_command_line.h>

class WebView;

// This is meant only for internal use, see GuiWebCtrl.h for some documentation

class GuiWebCore : public Thread
{

public:

   static GuiWebCore* get() { return sInstance; }

   void lock();
   void unlock();
   
   GuiWebCore(ThreadRunFunction func = 0);
   ~GuiWebCore();

   void CreateWebView(String url, CefRefPtr<WebView> webView);

   void run();
   void exit();
   bool running();

private:

   Mutex mMutex;
   Semaphore mSemaphore;
   Semaphore mStopSemaphore;

   static GuiWebCore* sInstance;

   U32 mLockCount;

};

#endif
