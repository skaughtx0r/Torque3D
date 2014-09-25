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

#ifndef _GUIBITMAPCTRL_H_
#define _GUIBITMAPCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#ifndef _PLATFORM_THREAD_SEMAPHORE_H_
#include "platform/threads/mutex.h"
#endif

#include "gfx/gfxTextureProfile.h"

#include "cef/webView.h"

class Buffer
{
public:
   Buffer() : mWidth(0), mHeight(0), mBufferSize(0), mImageSize(0), mBuffer(NULL) {}
   ~Buffer() {
      if (mBuffer) {
         delete mBuffer;
         mBuffer = NULL;
      }
   }

   void resize(U32 width, U32 height) {
      if (mBufferSize < width * height * 4) {
         if (mBuffer) {
            delete mBuffer;
            mBuffer = NULL;
         }
         mBuffer = new U8[width * height * 4];
         mBufferSize = width * height * 4;
      }
   }

   void set(const void* data, U32 width, U32 height) {
      mWidth = width;
      mHeight = height;
      mImageSize = width * height * 4;
      dMemcpy(mBuffer, data, mImageSize);
   }

public:
   U32 mWidth;
   U32 mHeight;
   U32 mBufferSize;
   U32 mImageSize;
   U8* mBuffer;
};

class GuiWebCtrl;

class GuiWebView : public WebView
{
public:
   GuiWebView(GuiWebCtrl *webCtrl);
   virtual ~GuiWebView();

   virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
   virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height);
   virtual void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor);
   virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser);
   void Destroy();

protected:
   GuiWebCtrl* mWebCtrl;
};

/// Renders a bitmap.
class GuiWebCtrl : public GuiControl
{
   public:
   
      typedef GuiControl Parent;

   protected:
   
      /// Name of the bitmap file.  If this is 'texhandle' the bitmap is not loaded
      /// from a file but rather set explicitly on the control.
      String mUrl;

      CefRefPtr<GuiWebView> mGuiWebView;
      
      /// Loaded texture.
      bool mUpdateTexture;
      Buffer* mBuffer1;
      Buffer* mBuffer2;
      GFXTexHandle mTextureObject;
      CefRefPtr<CefBrowser> mBrowser;
      bool mPushedCursor;

      Mutex mMutex;
      
      Point2I mStartPoint;
      
      static bool setUrlParam( void *object, const char *index, const char *data );

   public:
      
      GuiWebCtrl();
      virtual ~GuiWebCtrl();
      static void initPersistFields();

      void setUrl(const char *url);

      // GuiControl.
      bool onWake();
      void onSleep();
      void inspectPostApply();

      void updateSizing();

      virtual bool onAdd();
      virtual void onRemove();

      bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
      void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type, const CefRenderHandler::RectList &dirtyRects, const void *buffer, int width, int height);
      void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor);

      virtual bool resize(const Point2I &newPosition, const Point2I &newExtent);
      void onRender(Point2I offset, const RectI &updateRect);

      //Input
      //Mouse
      virtual void onMouseUp(const GuiEvent &event);
      virtual void onMouseDown(const GuiEvent &event);
      virtual void onMouseMove(const GuiEvent &event);
      virtual void onMouseDragged(const GuiEvent &event);
      virtual void onRightMouseDown(const GuiEvent &event);
      virtual void onRightMouseUp(const GuiEvent &event);
      virtual void onRightMouseDragged(const GuiEvent &event);
      virtual void onMiddleMouseDown(const GuiEvent &event);
      virtual void onMiddleMouseUp(const GuiEvent &event);
      virtual void onMiddleMouseDragged(const GuiEvent &event);
      virtual bool onMouseWheelUp(const GuiEvent &event);
      virtual bool onMouseWheelDown(const GuiEvent &event);

      //Keyboard
      virtual void setFirstResponder();
      virtual void onLoseFirstResponder();
      virtual bool onKeyDown(const GuiEvent &event);
      virtual bool onKeyUp(const GuiEvent &event);
      virtual bool onKeyRepeat(const GuiEvent &event);

      DECLARE_CONOBJECT(GuiWebCtrl);
      DECLARE_CATEGORY( "Gui Images" );
      DECLARE_DESCRIPTION( "A control that displays a single, static image from a file.\n"
                           "The bitmap can either be tiled or stretched inside the control." );
};

extern GFXTextureProfile GFXWebGuiProfile;

#endif
