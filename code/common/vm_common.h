// Common header included for all game modules

#ifndef __VM_COMMON_H__
#define __VM_COMMON_H__

//
// vm_extensions.c
//

qboolean VMExt_GVCommand( char *buffer, unsigned int bufferSize, const char *command );
int VMExt_GVCommandInt( const char *command, int defaultValue );

#endif	// __VM_COMMON_H__
