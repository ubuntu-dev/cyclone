#include "cyc_include.h"

 typedef int Cyc___int32_t; typedef unsigned int Cyc___uint32_t; typedef int Cyc_ptrdiff_t;
typedef unsigned int Cyc_size_t; typedef int Cyc_wchar_t; typedef unsigned int
Cyc_wint_t; typedef unsigned char Cyc_u_char; typedef unsigned short Cyc_u_short;
typedef unsigned int Cyc_u_int; typedef unsigned int Cyc_u_long; typedef
unsigned short Cyc_ushort; typedef unsigned int Cyc_uint; typedef unsigned int
Cyc_clock_t; typedef int Cyc_time_t; struct Cyc_timespec{ int tv_sec; int
tv_nsec; } ; struct Cyc_itimerspec{ struct Cyc_timespec it_interval; struct Cyc_timespec
it_value; } ; typedef int Cyc_daddr_t; typedef unsigned char* Cyc_caddr_t;
typedef unsigned short Cyc_ino_t; typedef short Cyc_dev_t; typedef int Cyc_off_t;
typedef unsigned short Cyc_uid_t; typedef unsigned short Cyc_gid_t; typedef int
Cyc_pid_t; typedef int Cyc_key_t; typedef int Cyc_ssize_t; typedef unsigned int
Cyc_mode_t; typedef unsigned short Cyc_nlink_t; typedef int Cyc_fd_mask; struct
Cyc__types_fd_set{ int fds_bits[ 8u]; } ; typedef struct Cyc__types_fd_set Cyc__types_fd_set;
typedef unsigned char* Cyc_Cstring; typedef struct _tagged_string Cyc_string;
typedef struct _tagged_string Cyc_string_t; typedef struct _tagged_string* Cyc_stringptr;
typedef int Cyc_bool; extern void exit( int); extern void* abort(); struct Cyc_Core_Opt{
void* v; } ; typedef struct Cyc_Core_Opt* Cyc_Core_opt_t; extern unsigned char
Cyc_Core_InvalidArg[ 15u]; struct Cyc_Core_InvalidArg_struct{ unsigned char* tag;
struct _tagged_string f1; } ; extern unsigned char Cyc_Core_Failure[ 12u];
struct Cyc_Core_Failure_struct{ unsigned char* tag; struct _tagged_string f1; }
; extern unsigned char Cyc_Core_Impossible[ 15u]; struct Cyc_Core_Impossible_struct{
unsigned char* tag; struct _tagged_string f1; } ; extern unsigned char Cyc_Core_Not_found[
14u]; extern unsigned char Cyc_Core_Unreachable[ 16u]; struct Cyc_Core_Unreachable_struct{
unsigned char* tag; struct _tagged_string f1; } ; extern unsigned char*
string_to_Cstring( struct _tagged_string); extern unsigned char*
underlying_Cstring( struct _tagged_string); extern struct _tagged_string
Cstring_to_string( unsigned char*); extern int system( unsigned char*); extern
struct _tagged_string Cyc_Errno_sys_err( int); extern int* __errno()
 __attribute__(( dllimport )) ; extern int _sys_nerr  __attribute__(( dllimport
)) ; extern unsigned char* _sys_errlist[ 124u]; struct _tagged_string Cyc_Errno_sys_err(
int i){ if( i < 0? 1: i > _sys_nerr){( void) _throw(( void*)({ struct Cyc_Core_InvalidArg_struct*
_temp0=( struct Cyc_Core_InvalidArg_struct*) GC_malloc( sizeof( struct Cyc_Core_InvalidArg_struct));
_temp0[ 0]=({ struct Cyc_Core_InvalidArg_struct _temp1; _temp1.tag= Cyc_Core_InvalidArg;
_temp1.f1=({ unsigned char* _temp2="sys_err: integer argument out of range";
struct _tagged_string _temp3; _temp3.curr= _temp2; _temp3.base= _temp2; _temp3.last_plus_one=
_temp2 + 39; _temp3;}); _temp1;}); _temp0;}));} return Cstring_to_string(
_sys_errlist[ _check_known_subscript_notnull( 124u, i)]);}