/*
 *  SysMutex.h
 *  lgpt
 *
 *  Created by Marc Nostromo on 10/03/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

class SysMutex {
public:
	SysMutex() ;
	~SysMutex() ;
	bool Lock() ;
	// Returns True if lock was succesfully taken
	// Only on SDL2, SDL1 just calls Lock
	bool TryLock();
	void Unlock() ;
private:
	SDL_mutex *mutex_ ;
} ;

class SysMutexLocker {
public:
	SysMutexLocker(SysMutex &mutex) ;
	~SysMutexLocker() ;
private:
	SysMutex *mutex_ ;
} ;

#endif // _SYS_MUTEX_H_
