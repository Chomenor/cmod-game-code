// Common header included for all game modules

#ifndef __VM_COMMON_H__
#define __VM_COMMON_H__

//
// vm_extensions.c
//

qboolean VMExt_GVCommand( char *buffer, unsigned int bufferSize, const char *command );
int VMExt_GVCommandInt( const char *command, int defaultValue );

qboolean VMExt_FNAvailable_LAN_ServerStatus_Ext( void );
int VMExt_FN_LAN_ServerStatus_Ext( const char *serverAddress, char *serverStatus, int maxLen, char *extString, int extLen );
qboolean VMExt_FNAvailable_AltSwap_SetState( void );
void VMExt_FN_AltSwap_SetState( qboolean swapState );

#endif	// __VM_COMMON_H__
