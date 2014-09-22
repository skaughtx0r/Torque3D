#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_command_line.h>
#include "webApp.h"

#ifdef WIN32

// Program entry-point function.
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCommandShow)
{
   // Structure for passing command-line arguments.
   // The definition of this structure is platform-specific.
   CefMainArgs main_args(hInstance);

   // Optional implementation of the CefApp interface.
   CefRefPtr<WebApp> app(new WebApp());

   // Execute the sub-process logic. This will block until the sub-process should exit.
   return CefExecuteProcess(main_args, app.get(), NULL);
}

#else

int main(int argc, const char **argv)
{
   // Structure for passing command-line arguments.
   // The definition of this structure is platform-specific.
   CefMainArgs main_args(argc, argv);

   // Optional implementation of the CefApp interface.
   CefRefPtr<WebApp> app(new WebApp());

   // Execute the sub-process logic. This will block until the sub-process should exit.
   return CefExecuteProcess(main_args, app.get(), NULL);
}

#endif