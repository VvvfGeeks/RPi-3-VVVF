#ifndef _WINDOWS_H_
#define _WINDOWS_H_

extern uint32_t frameBufferAddress;
extern uint32_t frameBufferWidth;
extern uint32_t frameBufferHeight;
extern uint32_t frameBufferPitch;
extern uint32_t frameBufferDepth;

bool initializeWindow(int Width,										// Screen width request (Use 0 if you wish autodetect width) 
	int Height,									// Screen height request (Use 0 if you wish autodetect height) 	
	int PhysicalWidth,
	int PhysicalHeight,
	int Depth);									// Screen colour depth request (Use 0 if you wish autodetect colour depth) 
#endif