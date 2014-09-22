#pragma once

#include "include/cef_app.h"

// CefApp does all of the work, but it is an abstract base class that needs reference counting implemented;
// thus, we create a dummy class that inherits off of CefApp but does nothing
class WebApp : public CefApp
{
   public:
      WebApp ()
      {
      }
      virtual ~WebApp ()
      {
      }

   private:
      IMPLEMENT_REFCOUNTING (WebApp);
};