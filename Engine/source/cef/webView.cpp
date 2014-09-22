#include "webView.h"

WebView::WebView() : mIsLoaded(false)
{
}

WebView::~WebView()
{
}

CefRefPtr<CefBrowser> WebView::GetBrowser()
{
   return browser;
}

CefRefPtr<CefContextMenuHandler> WebView::GetContextMenuHandler()
{
   return this;
}

CefRefPtr<CefDisplayHandler> WebView::GetDisplayHandler()
{
   return this;
}

CefRefPtr<CefDownloadHandler> WebView::GetDownloadHandler()
{
   return this;
}

CefRefPtr<CefDragHandler> WebView::GetDragHandler()
{
   return this;
}

CefRefPtr<CefGeolocationHandler> WebView::GetGeolocationHandler()
{
   return this;
}

CefRefPtr<CefKeyboardHandler> WebView::GetKeyboardHandler()
{
   return this;
}

CefRefPtr<CefLifeSpanHandler> WebView::GetLifeSpanHandler()
{
   return this;
}

CefRefPtr<CefLoadHandler> WebView::GetLoadHandler()
{
   return this;
}

CefRefPtr<CefRequestHandler> WebView::GetRequestHandler()
{
   return this;
}

CefRefPtr<CefRenderHandler> WebView::GetRenderHandler()
{
   return this;
}

void WebView::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
CefRefPtr<CefDownloadItem> download_item, 
const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
   UNREFERENCED_PARAMETER (browser);
   UNREFERENCED_PARAMETER (download_item);
   callback->Continue (suggested_name, true);
}

void WebView::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  this->browser = browser;

  CefLifeSpanHandler::OnAfterCreated (browser);
}

void WebView::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  browser = NULL;

  CefLifeSpanHandler::OnBeforeClose (browser);
}

void WebView::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
{
   mIsLoaded = false;
}
void WebView::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
   mIsLoaded = true;
}

bool WebView::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
   return true;
}

void WebView::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
}