#include <stdbool.h>			// C standard unit needed for bool and true/false
#include <stdint.h>				// C standard unit needed for uint8_t, uint32_t, etc
#include "rpi-smartstart.h"
#include "Font8x16.h"			// Provides the 8x16 bitmap font for console 
#include "windows.h"			// This units header

uint32_t frameBufferAddress = 0x00;
uint32_t frameBufferWidth = 0x00;
uint32_t frameBufferHeight = 0x00;
uint32_t frameBufferPitch = 0x00;
uint32_t frameBufferDepth = 0x00;

bool initializeWindow (int Width,										// Screen width request (Use 0 if you wish autodetect width)
					  int Height,									// Screen height request (Use 0 if you wish autodetect height)
					  int PhysicalWidth,
					  int PhysicalHeight,
					  int Depth)									// Screen colour depth request (Use 0 if you wish autodetect colour depth) 
{
	uint32_t buffer[23];
	if ((Width == 0) || (Height == 0)) {							// Has auto width or height been requested
		if (mailbox_tag_message(&buffer[0], 5,
			MAILBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT,
			8, 0, 0, 0)) {											// Get current width and height of screen
			if (Width == 0) Width = buffer[3];						// Width passed in as zero set set Screenwidth variable
			if (Height == 0) Height = buffer[4];					// Height passed in as zero set set ScreenHeight variable
		} else return false;										// For some reason get screen physical failed
	}
	if (Depth == 0) {												// Has auto colour depth been requested
		if (mailbox_tag_message(&buffer[0], 4,
			MAILBOX_TAG_GET_COLOUR_DEPTH, 4, 4, 0)) {				// Get current colour depth of screen
			Depth = buffer[3];										// Depth passed in as zero set set current screen colour depth
		} else return false;										// For some reason get screen depth failed
	}
	if (!mailbox_tag_message(&buffer[0], 23,
		MAILBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT, 8, 8, PhysicalWidth, PhysicalHeight,
		MAILBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT, 8, 8, Width, Height,
		MAILBOX_TAG_SET_COLOUR_DEPTH, 4, 4, Depth,
		MAILBOX_TAG_ALLOCATE_FRAMEBUFFER, 8, 4, 16, 0,
		MAILBOX_TAG_GET_PITCH, 4, 0, 0))							// Attempt to set the requested settings
		return false;												// The requesting settings failed so return the failure

	frameBufferAddress = buffer[17];
	frameBufferPitch = buffer[22];									// Transfer the line pitch
	frameBufferWidth = Width;											// Transfer the screen width
	frameBufferHeight = Height;											// Transfer the screen height
	frameBufferDepth = Depth;										// Transfer the screen depth

	return true;													// Return successful
}


