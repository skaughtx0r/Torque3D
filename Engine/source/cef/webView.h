#pragma once

#include "include/cef_client.h"
#include "include/base/cef_lock.h"

class WebView :   public CefClient,
                        public CefRenderHandler,
                        public CefContextMenuHandler,
                        public CefDisplayHandler,
                        public CefDownloadHandler,
                        public CefDragHandler,
                        public CefGeolocationHandler,
                        public CefKeyboardHandler,
                        public CefLifeSpanHandler,
                        public CefLoadHandler,
                        public CefRequestHandler
{
 public:
      WebView();
      virtual ~WebView();
      CefRefPtr<CefBrowser> GetBrowser();

#pragma region CefClient
      // since we are letting the base implementations handle all of the heavy lifting,
      // these functions just return the this pointer
      virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler () OVERRIDE;
      virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler () OVERRIDE;
      virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler () OVERRIDE;
      virtual CefRefPtr<CefDragHandler> GetDragHandler () OVERRIDE;
      virtual CefRefPtr<CefGeolocationHandler> GetGeolocationHandler () OVERRIDE;
      virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler () OVERRIDE;
      virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler () OVERRIDE;
      virtual CefRefPtr<CefLoadHandler> GetLoadHandler () OVERRIDE;
      virtual CefRefPtr<CefRequestHandler> GetRequestHandler () OVERRIDE;
      virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE;
#pragma endregion // CefClient

#pragma region CefLoadHandler
      virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
      virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode);
#pragma endregion

#pragma region CefRenderHandler
      virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
      virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height);
      virtual void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor) {}
#pragma endregion

#pragma region CefDownloadHandler
      // this function is virtual and must be implemented; we do nothing in it, so downloading files won't work as the callback function isn't invoked
      virtual void OnBeforeDownload (CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback);
#pragma endregion // CefDownloadHandler 

#pragma region CefLifeSpanHandler
      // cache a reference to the browser
      virtual void OnAfterCreated (CefRefPtr<CefBrowser> browser) OVERRIDE;
      // release the browser reference
      virtual void OnBeforeClose (CefRefPtr<CefBrowser> browser) OVERRIDE;
#pragma endregion // CefLifeSpanHandler  

 protected:
      // the browser reference
      CefRefPtr<CefBrowser> browser;
      bool mIsLoaded;

      // Include the default reference counting implementation.
      IMPLEMENT_REFCOUNTING (WebView);
};