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

//
// logging.c
//

int Logging_Assertion( intptr_t result, const char *expression, qboolean error );

// Generates a warning/error if "expression" is false.
#define EF_WARN_ASSERT( expression ) Logging_Assertion( (intptr_t)( expression ), #expression, qfalse )
#define EF_ERR_ASSERT( expression ) Logging_Assertion( (intptr_t)( expression ), #expression, qtrue )

// Logging macros can be used in place of standard function definitions. They currently don't have any
// effect but may be used for future debugging features or custom purposes.

#define LOGFUNCTION_XRET( prefix1, returntype, prefix2, name, typedparams, untypedparams, logcond, entity_num ) \
	prefix1 returntype prefix2 name typedparams

// Standard returning function
#define LOGFUNCTION_RET( returntype, name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XRET( , returntype, , name, typedparams, untypedparams, logcond, -1 )

// Static returning function
#define LOGFUNCTION_SRET( returntype, name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XRET( static, returntype, , name, typedparams, untypedparams, logcond, -1 )

// Returning function with entity num tag
#define LOGFUNCTION_ERET( returntype, name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XRET( , returntype, , name, typedparams, untypedparams, logcond, entity_num )

// Static returning function with entity num tag
#define LOGFUNCTION_SERET( returntype, name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XRET( static, returntype, , name, typedparams, untypedparams, logcond, entity_num )

#define LOGFUNCTION_XVOID( prefix1, prefix2, name, typedparams, untypedparams, logcond, entity_num ) \
	prefix1 void prefix2 name typedparams

// Standard void function
#define LOGFUNCTION_VOID( name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XVOID( , , name, typedparams, untypedparams, logcond, -1 )

// Static void function
#define LOGFUNCTION_SVOID( name, typedparams, untypedparams, logcond ) \
	LOGFUNCTION_XVOID( static, , name, typedparams, untypedparams, logcond, -1 )

// Void function with entity num tag
#define LOGFUNCTION_EVOID( name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XVOID( , , name, typedparams, untypedparams, logcond, entity_num )

// Static void function with entity num tag
#define LOGFUNCTION_SEVOID( name, typedparams, untypedparams, entity_num, logcond ) \
	LOGFUNCTION_XVOID( static, , name, typedparams, untypedparams, logcond, entity_num )

#endif	// __VM_COMMON_H__
