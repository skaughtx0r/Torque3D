#include "AssetResourceHandler.h"
#include "platform/platform.h"
#include "core/fileObject.h"

CefRefPtr<CefResourceHandler> AssetSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request)
{
   std::string url = request->GetURL().ToString().substr(13);
   String redirectUrl = String::ToString("file:///%s/%s", Platform::getCurrentDirectory(), url.c_str());
   if (browser.get())
      browser->GetMainFrame()->LoadURL(redirectUrl.c_str());
   return new AssetResourceHandler();
}

bool AssetResourceHandler::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback)
{
   return true;
}

void AssetResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl)
{
}

bool AssetResourceHandler::ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback)
{
   return false;
}

void AssetResourceHandler::Cancel()
{

}