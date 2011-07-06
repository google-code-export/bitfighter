/*
 * Input.h
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */
#ifndef INPUT_H_
#define INPUT_H_

#include "tnlTypes.h"
#include "keyCode.h"

#include "SDL/SDL.h"

namespace Zap {

class Event {

public:
   Event();
   virtual ~Event();

   static void onEvent(SDL_Event* event);

   static void onInputFocus();
   static void onInputBlur();
   static void onKeyDown(SDLKey key, SDLMod mod, TNL::U16 unicode);
   static void onKeyUp(SDLKey key, SDLMod mod, TNL::U16 unicode);
   static void onMouseFocus();
   static void onMouseBlur();
   static void onMouseMoved(TNL::S32 x, TNL::S32 y, TNL::S32 relX, TNL::S32 relY, bool Left, bool Right, bool Middle);
   static void onMouseWheel(bool Up, bool Down);  //Not implemented
   static void onMouseLeftButtonDown(TNL::S32 x, TNL::S32 y);
   static void onMouseLeftButtonUp(TNL::S32 x, TNL::S32 y);
   static void onMouseRightButtonDown(TNL::S32 x, TNL::S32 y);
   static void onMouseRightButtonUp(TNL::S32 x, TNL::S32 y);
   static void onMouseMiddleButtonDown(TNL::S32 x, TNL::S32 y);
   static void onMouseMiddleButtonUp(TNL::S32 x, TNL::S32 y);
   static void onJoyAxis(TNL::U8 which, TNL::U8 axis, TNL::S16 value);
   static void onJoyButtonDown(TNL::U8 which, TNL::U8 button);
   static void onJoyButtonUp(TNL::U8 which, TNL::U8 button);
   static void onJoyHat(TNL::U8 which, TNL::U8 hat, TNL::U8 value);
   static void onJoyBall(TNL::U8 which, TNL::U8 ball, TNL::S16 xrel, TNL::S16 yrel);
   static void onMinimize();
   static void onRestore();
   static void onResize(TNL::S32 w,TNL::S32 h);
   static void onExpose();
   static void onExit();
   static void onUser(TNL::U8 type, TNL::S32 code, void* data1, void* data2);
};

}
#endif /* INPUT_H_ */
