<?php
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

beginModule( 'CEF' );
   global $CEF_PATH;
   // Look for the optional global from the project.conf.
   if (!$CEF_PATH)
   {
      // First look for an environment var.
      $CEF_PATH = getenv( "CEF_PATH" );

      if (strlen($CEF_PATH) == 0 || !file_exists($CEF_PATH))
      {
         // Sometimes users get confused and use this var.
         $CEF_PATH = getenv( "CEF_PATH" );

         if (strlen($CEF_PATH) == 0 || !file_exists($CEF_PATH))
         {
            // Since neither environment var worked try for 
            // the default Triton SDK install location.
            $CEF_PATH = getenv("ProgramFiles") . "/cef";

            // Last channce... try the x86 default install path.
            if (!file_exists($CEF_PATH))
               $CEF_PATH = getenv("ProgramFiles(x86)") . "/cef";
         }
      }

      // We need forward slashes for paths.
      $CEF_PATH = str_replace( "\\", "/", $CEF_PATH);

      // Remove trailing slashes.
      $CEF_PATH = rtrim($CEF_PATH, " /");
   }

   // If we still don't have the SDK path then let the user know.
   if (!file_exists($CEF_PATH))
   {
      trigger_error( 
            "\n*******************************************************************".
            "\n".
            "\n  We were not able to find a valid path to the Chromium Embedded Framework!".
            "\n".
            "\n  You must install the latest Triton Ocean SDK and set the path via a".
            "\n  \$CEF_PATH variable in your buildFiles/project.conf file".
            "\n  or by setting the CEF_PATH system environment variable".
            "\n  (may require a reboot).".
            "\n".
            "\n*******************************************************************".
            "\n", E_USER_ERROR );
   }

   includeLib( 'cefproc' );
   addSolutionProjectRef( 'cefproc' );
   
   // Includes
   addIncludePath( $CEF_PATH );
   addIncludePath( '../' . T3D_Generator::getLibSrcDir() . 'cef/include' );
   
   //Source
   addEngineSrcDir( "cef" );
   addEngineSrcDir( "gui/web" );

   // Libs
   addProjectLibDir( $CEF_PATH );
   addProjectLibDir( $CEF_PATH . "/out" );
   addProjectLibInput( "./Release/libcef.lib", "./Debug/libcef.lib" );
   addProjectLibInput( "./out/Release/lib/libcef_dll_wrapper.lib", "./out/Debug/lib/libcef_dll_wrapper.lib" );
   
   function copy_cef($src,$dst) { 
		$dir = opendir($src); 
		@mkdir($dst); 
		while(false !== ( $file = readdir($dir)) ) { 
			if (( $file != '.' ) && ( $file != '..' )) { 
				if ( is_dir($src . '/' . $file) ) { 
					copy_cef($src . '/' . $file,$dst . '/' . $file); 
				} 
				else { 
					copy($src . '/' . $file,$dst . '/' . $file); 
				} 
			} 
		} 
		closedir($dir); 
	}
	
	copy_cef($CEF_PATH . "/Resources", getcwd() . "/game");
	copy_cef($CEF_PATH . "/Release", getcwd() . "/game");

endModule();

?>
