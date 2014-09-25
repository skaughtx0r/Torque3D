//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "gui/core/guiCanvas.h"
#include "gui/web/guiWebCtrl.h"
#include "cef/cefCore.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include <fstream>

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"


IMPLEMENT_CONOBJECT(GuiWebCtrl);

GFXTextureProfile GFXWebGuiProfile("GFXWebGuiProfile",
   GFXTextureProfile::DiffuseMap,
   GFXTextureProfile::Static | GFXTextureProfile::NoMipmap,
   GFXTextureProfile::NONE);

ConsoleDocClass(GuiWebCtrl,
   "@brief A gui control that is used to display an image.\n\n"
   
   "The image is stretched to the constraints of the control by default. However, the control can also\n"
   "tile the image as well.\n\n"

   "The image itself is stored inside the GuiBitmapCtrl::bitmap field. The boolean value that decides\n"
   "whether the image is stretched or tiled is stored inside the GuiBitmapCtrl::wrap field.\n"
   
   "@tsexample\n"
   "// Create a tiling GuiBitmapCtrl that displays \"myImage.png\"\n"
   "%bitmapCtrl = new GuiBitmapCtrl()\n"
   "{\n"
   "   bitmap = \"myImage.png\";\n"
   "   wrap = \"true\";\n"
   "};\n"
   "@endtsexample\n\n"
   
   "@ingroup GuiControls"
);

GuiWebView::GuiWebView(GuiWebCtrl *webCtrl) : mWebCtrl(webCtrl)
{
}

GuiWebView::~GuiWebView()
{
}

void GuiWebView::Destroy()
{
   GetBrowser()->GetHost()->CloseBrowser(false);
   mWebCtrl = NULL;
   Release();
}

void GuiWebView::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
   WebView::OnAfterCreated(browser);
   if (mWebCtrl->isAwake()) {
      GetBrowser()->GetHost()->WasHidden(false);
      GetBrowser()->GetHost()->SendFocusEvent(true);
   }
   else
      GetBrowser()->GetHost()->WasHidden(true);
}

GuiWebCtrl::GuiWebCtrl(void)
 : mUrl("http://www.youtube.com"),
   mStartPoint( 0, 0 ),
   mTextureObject(512, 512, GFXFormatR8G8B8A8, &GFXWebGuiProfile, "WebGuiTexture"),
   mBuffer1(NULL),
   mBuffer2(NULL),
   mUpdateTexture(false),
   mPushedCursor(false)
{
}

GuiWebCtrl::~GuiWebCtrl()
{
}

bool GuiWebCtrl::onAdd()
{
   mBuffer1 = new Buffer();
   mBuffer2 = new Buffer();
   mGuiWebView = new GuiWebView(this);
   CefCore::get()->CreateWebView(mUrl, mGuiWebView);
   //setBounds(0, 0, 512, 512);
   return Parent::onAdd();
}

void GuiWebCtrl::onRemove()
{
   //GetBrowser()->GetHost()->CloseBrowser(true);
   MutexHandle mutex;
   mutex.lock(&mMutex, true);
   if (mBuffer1) {
      delete mBuffer1;
      mBuffer1 = NULL;
   }
   if (mBuffer2) {
      delete mBuffer2;
      mBuffer2 = NULL;
   }
   CefPostTask(TID_UI, base::Bind(&GuiWebView::Destroy, mGuiWebView));
   //mGuiWebView->Destroy();
   Parent::onRemove();
}

bool GuiWebCtrl::setUrlParam(void *object, const char *index, const char *data)
{
   static_cast<GuiWebCtrl *>(object)->setUrl(data);
   return false;
}

void GuiWebCtrl::initPersistFields()
{
   addGroup( "Web" );
   
   addProtectedField("url", TypeRealString, Offset(mUrl, GuiWebCtrl),
      &setUrlParam, &defaultProtectedGetFn,
         "The url of the webpage to display in the control." );
      
   endGroup( "Web" );

   Parent::initPersistFields();
}

bool GuiWebCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   setActive(true);
   if (mGuiWebView && mGuiWebView->GetBrowser()) {
      mGuiWebView->GetBrowser()->GetHost()->WasHidden(false);
      mGuiWebView->GetBrowser()->GetHost()->SendFocusEvent(true);
   }
   return true;
}

void GuiWebCtrl::onSleep()
{
   mGuiWebView->GetBrowser()->GetHost()->WasHidden(true);

   Parent::onSleep();
}

//-------------------------------------
void GuiWebCtrl::inspectPostApply()
{
   // if the extent is set to (0,0) in the gui editor and appy hit, this control will
   // set it's extent to be exactly the size of the bitmap (if present)
   Parent::inspectPostApply();

   MutexHandle mutex;
   mutex.lock(&mMutex, true);
   if ((getExtent().x == 0) && (getExtent().y == 0) && mTextureObject)
   {
      setExtent( mTextureObject->getWidth(), mTextureObject->getHeight());
   }
}

void GuiWebCtrl::setUrl(const char *url)
{
   mUrl = url;
   if ( !isAwake() )
      return;

   if (mGuiWebView && mGuiWebView->GetBrowser())
      mGuiWebView->GetBrowser()->GetMainFrame()->LoadURL(mUrl.c_str());
}

bool GuiWebCtrl::resize(const Point2I &newPosition, const Point2I &newExtent)
{
   bool resized = Parent::resize(newPosition, newExtent);
   if (resized)
   {
      if (mGuiWebView && mGuiWebView->GetBrowser())
         mGuiWebView->GetBrowser()->GetHost()->WasResized();
   }
   return resized;
}

bool GuiWebView::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
   if (mWebCtrl)
      return mWebCtrl->GetViewRect(browser, rect);
   return false;
}

bool GuiWebCtrl::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
   rect = CefRect(0, 0, getWidth(), getHeight());
   return true;
}

void GuiWebView::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
   if (mWebCtrl)
      mWebCtrl->OnPaint(browser, type, dirtyRects, buffer, width, height);
}

void GuiWebCtrl::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height)
{
   MutexHandle mutex;
   mutex.lock(&mMutex, true);
   if (mBuffer1) {
      mBuffer1->resize(width, height);
      mBuffer1->set(buffer, width, height);
      mUpdateTexture = true;
   }
   setUpdate();
}

void GuiWebCtrl::updateSizing()
{
   if(!getParent())
      return;
   // updates our bounds according to our horizSizing and verSizing rules
   RectI fakeBounds( getPosition(), getParent()->getExtent());
   parentResized( fakeBounds, fakeBounds);
}

void GuiWebCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   MutexHandle mutex;
   mutex.lock(&mMutex, true);
   bool update = false;
   if (mUpdateTexture) {
      mUpdateTexture = false;
      Buffer* temp = mBuffer2;
      mBuffer2 = mBuffer1;
      mBuffer1 = temp;
      temp = NULL;
      update = true;
   }
   mutex.unlock();

   if (update && mTextureObject)
   {
      if (mTextureObject.getWidth() != mBuffer2->mWidth || mTextureObject.getHeight() != mBuffer2->mHeight)
         mTextureObject.set(mBuffer2->mWidth, mBuffer2->mHeight, GFXFormatR8G8B8A8, &GFXWebGuiProfile, "WebGuiTexture");
      GFXLockedRect* rect = mTextureObject.lock();
      dMemcpy(rect->bits, mBuffer2->mBuffer, mBuffer2->mImageSize);
      mTextureObject.unlock();
   }
   
   if (mTextureObject)
   {
      GFX->getDrawUtil()->clearBitmapModulation();
      RectI rect(offset, getExtent());
      GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect, GFXBitmapFlip_None, GFXTextureFilterLinear, false);
   }

   if (mProfile->mBorder || !mTextureObject)
   {
      RectI rect(offset.x, offset.y, getExtent().x, getExtent().y);
      GFX->getDrawUtil()->drawRect(rect, mProfile->mBorderColor);
   }

   renderChildControls(offset, updateRect);
}

void GuiWebView::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor)
{
   if (mWebCtrl)
      mWebCtrl->OnCursorChange(browser, cursor);
}

void GuiWebCtrl::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor)
{
   PlatformWindow *pWindow = static_cast<GuiCanvas*>(getRoot())->getPlatformWindow();
   if (!pWindow)
      return;

   PlatformCursorController *pController = pWindow->getCursorController();
   if (!pController)
      return;
   if (mPushedCursor) {
      pController->popCursor();
      mPushedCursor = false;
   }
   if (cursor == (HCURSOR)0x00010005) {
      pController->pushCursor(PlatformCursorController::curIBeam); mPushedCursor = true;
   }
   else if (cursor == (HCURSOR)0x0001001f) {
      pController->pushCursor(PlatformCursorController::curHand); mPushedCursor = true;
   }
   
}

//Input
//Keyboard
void GuiWebCtrl::onMouseUp(const GuiEvent &event)
{
   if (!mActive)
   {
      Parent::onMouseUp(event);
      return;
   }

   mouseUnlock();
   setFirstResponder();

   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_LEFT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_LEFT, true, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMouseDown(const GuiEvent &event)
{
   if (!mActive)
   {
      Parent::onMouseDown(event);
      return;
   }

   setFirstResponder();
   mouseLock();

   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_LEFT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_LEFT, false, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMouseDragged(const GuiEvent &event)
{
   Parent::onMouseMove(event);
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_LEFT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseMoveEvent(evtMouse, false);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onRightMouseUp(const GuiEvent &event)
{
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_RIGHT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_RIGHT, true, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onRightMouseDown(const GuiEvent &event)
{
   Parent::onRightMouseDown(event);
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_RIGHT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_RIGHT, false, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onRightMouseDragged(const GuiEvent &event)
{
   Parent::onMouseMove(event);
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_RIGHT_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseMoveEvent(evtMouse, false);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMiddleMouseUp(const GuiEvent &event)
{
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_MIDDLE, true, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMiddleMouseDown(const GuiEvent &event)
{
   Parent::onMiddleMouseDown(event);
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseClickEvent(evtMouse, CefBrowserHost::MouseButtonType::MBT_MIDDLE, false, event.mouseClickCount);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMiddleMouseDragged(const GuiEvent &event)
{
   Parent::onMouseMove(event);
   CefMouseEvent evtMouse;
   evtMouse.modifiers = EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseMoveEvent(evtMouse, false);
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onMouseMove(const GuiEvent &event)
{
   Parent::onMouseMove(event);
   CefMouseEvent evtMouse;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseMoveEvent(evtMouse, false);
}

//-----------------------------------------------------------------------------

bool GuiWebCtrl::onMouseWheelUp(const GuiEvent &event)
{
   if (Parent::onMouseWheelUp(event))
      return false;
   CefMouseEvent evtMouse;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseWheelEvent(evtMouse, 0, event.fval);
   return true;
}

//-----------------------------------------------------------------------------

bool GuiWebCtrl::onMouseWheelDown(const GuiEvent &event)
{
   if (Parent::onMouseWheelDown(event))
      return false;
   CefMouseEvent evtMouse;
   evtMouse.x = event.mousePoint.x;
   evtMouse.y = event.mousePoint.y;
   mGuiWebView->GetBrowser()->GetHost()->SendMouseWheelEvent(evtMouse, 0, event.fval);
   return true;
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::setFirstResponder()
{
   Parent::setFirstResponder();

   GuiCanvas *root = getRoot();
   if (root) {
      root->setNativeAcceleratorsEnabled(false);
   }
}

//-----------------------------------------------------------------------------

void GuiWebCtrl::onLoseFirstResponder()
{
   GuiCanvas *root = getRoot();
   if (root) {
      root->setNativeAcceleratorsEnabled(true);
   }

   Parent::onLoseFirstResponder();
}

//-----------------------------------------------------------------------------

bool GuiWebCtrl::onKeyDown(const GuiEvent &event)
{
   if (!isActive())
      return false;

   U16 ascii = 0;
   U16 keyCode = TranslateKeyCodeToOS(event.keyCode);
   
   if (event.modifier & SI_SHIFT)
      ascii = Input::getAscii(event.keyCode, STATE_UPPER);
   else
      ascii = Input::getAscii(event.keyCode, STATE_LOWER);

   CefKeyEvent keyEvent;
   keyEvent.character = ascii;
   if (ascii) {
      BYTE VkCode = LOBYTE(VkKeyScanA(ascii));
      UINT scanCode = MapVirtualKey(VkCode, MAPVK_VK_TO_VSC);
      keyEvent.native_key_code = (scanCode << 16) | 1;
      keyEvent.windows_key_code = VkCode;
   }
   else {
      keyEvent.windows_key_code = keyCode;
   }

   keyEvent.type = KEYEVENT_RAWKEYDOWN;
   mGuiWebView->GetBrowser()->GetHost()->SendKeyEvent(keyEvent);
   if (ascii) {
      keyEvent.type = KEYEVENT_CHAR;
      keyEvent.windows_key_code = ascii;
      mGuiWebView->GetBrowser()->GetHost()->SendKeyEvent(keyEvent);
   }

   return true;
}

//-----------------------------------------------------------------------------

bool GuiWebCtrl::onKeyUp(const GuiEvent &event)
{
   if (!isActive())
      return false;

   U16 ascii = 0;
   U16 keyCode = TranslateKeyCodeToOS(event.keyCode);

   if (event.modifier & SI_SHIFT)
      ascii = Input::getAscii(event.keyCode, STATE_UPPER);
   else
      ascii = Input::getAscii(event.keyCode, STATE_LOWER);

   CefKeyEvent keyEvent;
   keyEvent.character = ascii;
   if (ascii) {
      BYTE VkCode = LOBYTE(VkKeyScanA(ascii));
      UINT scanCode = MapVirtualKey(VkCode, MAPVK_VK_TO_VSC);
      keyEvent.native_key_code = (scanCode << 16) | 1;
      keyEvent.native_key_code |= 0xC0000000;
      keyEvent.windows_key_code = VkCode;
   }
   else {
      keyEvent.windows_key_code = keyCode;
   }

   keyEvent.type = KEYEVENT_KEYUP;
   mGuiWebView->GetBrowser()->GetHost()->SendKeyEvent(keyEvent);

   return true;
}

//-----------------------------------------------------------------------------

bool GuiWebCtrl::onKeyRepeat(const GuiEvent &event)
{
   return onKeyDown(event);
}

//-----------------------------------------------------------------------------

static ConsoleDocFragment _sGuiWebCtrlSetBitmap1(
   "@brief Assign an image to the control.\n\n"
   "Child controls with resize according to their layout settings.\n"
   "@param filename The filename of the image.\n"
   "@param resize Optional parameter. If true, the GUI will resize to fit the image.",
   "GuiBitmapCtrl", // The class to place the method in; use NULL for functions.
   "void setBitmap( String filename, bool resize );" ); // The definition string.

static ConsoleDocFragment _sGuiWebCtrlSetBitmap2(
   "@brief Assign an image to the control.\n\n"
   "Child controls will resize according to their layout settings.\n"
   "@param filename The filename of the image.\n"
   "@param resize A boolean value that decides whether the ctrl refreshes or not.",
   "GuiBitmapCtrl", // The class to place the method in; use NULL for functions.
   "void setBitmap( String filename );" ); // The definition string.


//"Set the bitmap displayed in the control. Note that it is limited in size, to 256x256."
ConsoleMethod(GuiWebCtrl, setUrl, void, 3, 4,
   "( String filename | String filename, bool resize ) Assign an image to the control.\n\n"
   "@hide" )
{
   object->setUrl(argv[2]);
}
