// (c) 2008-2012 nerdfever.com

#ifndef __DEBUG_H
#define __DEBUG_H

#define	CPU_BUSY	(1)			// used with SPEEDO
#define CPU_IDLE	(0)

#define assert(exp)	(exp) ? : AssertFailed(__FILE__, __LINE__)		// force break if assert fails

// the below are for use with SPEEDO

#define SET_DEBUG_STATE(x)  (SPEEDO = (x))							// set to CPU_BUSY or CPU_IDLE
#define PUSH_DEBUG_STATE()	uint8_t pushed_debug_state = SPEEDO
#define POP_DEBUG_STATE()	(SPEEDO = pushed_debug_state)

// function prototypes

void AssertFailed(char* file, int line);

#endif // __DEBUG_H


