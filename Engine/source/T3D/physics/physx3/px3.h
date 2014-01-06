//-----------------------------------------------------------------------------
// Copyright (c) 2013 Timothy C Barnes
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

#ifndef _PHYSX3_H_
#define _PHYSX3_H_

//-------------------------------------------------------------------------
//defines to keep PhysX happy and compiling
#if defined(TORQUE_OS_MAC) && !defined(__APPLE__)
   #define __APPLE__
#elif defined(TORQUE_OS_LINUX) && !defined(LINUX)
   #define LINUX
#elif defined(TORQUE_OS_WIN32) && !defined(WIN32)
   #define WIN32
#endif

//-------------------------------------------------------------------------

#include <PxPhysicsAPI.h>
#include <extensions\PxExtensionsAPI.h>
#include <extensions\PxDefaultErrorCallback.h>
#include <extensions\PxDefaultAllocator.h>
#include <extensions\PxDefaultSimulationFilterShader.h>
#include <extensions\PxDefaultCpuDispatcher.h>
#include <extensions\PxShapeExt.h>
#include <extensions\PxSimpleFactory.h>
#include <foundation\PxFoundation.h>
#include <characterkinematic\PxController.h>
#include <common\PxIO.h> 


//man i'm lazy :-), must remove this after testing
#ifdef _MSC_VER
#ifdef TORQUE_DEBUG
	#pragma comment(lib, "PhysX3DEBUG_x86.lib")
	#pragma comment(lib, "PhysX3CommonDEBUG_x86.lib")
	#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
	#pragma comment(lib, "PhysX3CookingDEBUG_x86.lib")
	#pragma comment(lib, "PxTaskDEBUG.lib")
	#pragma comment(lib, "PhysX3CharacterKinematicDEBUG_x86.lib")
	#pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib")
	#pragma comment(lib, "PhysXProfileSDKDEBUG.lib")
#else
	#pragma comment(lib, "PhysX3_x86.lib")
	#pragma comment(lib, "PhysX3Common_x86.lib")
	#pragma comment(lib, "PhysX3Extensions.lib")
	#pragma comment(lib, "PhysX3Cooking_x86.lib")
	#pragma comment(lib, "PxTask.lib")
	#pragma comment(lib, "PhysX3CharacterKinematic_x86.lib")
	#pragma comment(lib, "PhysXVisualDebuggerSDK.lib")
	#pragma comment(lib, "PhysXProfileSDK.lib")
#endif
#endif


extern physx::PxPhysics* gPhysics3SDK;


#endif