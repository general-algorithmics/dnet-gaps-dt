#ifndef GAPS_COMMON_H
#define GAPS_COMMON_H

#include <stdint.h>
#include <stddef.h>     // size_t
#include <sys/types.h>  // ssize_t


#ifdef __cplusplus
extern "C" {
#endif




//---------------------------------------------------------------------------
// BASIC GENERIC TYPES
//---------------------------------------------------------------------------

#ifndef __GAPS_GENERIC_TYPES_DEFINED
#define __GAPS_GENERIC_TYPES_DEFINED

//
// NOTE:
//  No long variant here to avoid precision confusion.
//
typedef int8_t            INT8,    *PINT8;
typedef int16_t           INT16,   *PINT16;
typedef int32_t           INT32,   *PINT32;
typedef int64_t           INT64,   *PINT64;
typedef uint8_t           UINT8,   *PUINT8;
typedef uint16_t          UINT16,  *PUINT16;
typedef uint32_t          UINT32,  *PUINT32;
typedef uint64_t          UINT64,  *PUINT64;
typedef size_t            SIZE_T,  *PSIZE_T;
typedef ssize_t           SSIZE_T, *PSSIZE_T;

typedef int8_t            S8,   *PS8;
typedef int16_t           S16,  *PS16;
typedef int32_t           S32,  *PS32;
typedef int64_t           S64,  *PS64;
typedef uint8_t           U8,   *PU8;
typedef uint16_t          U16,  *PU16;
typedef uint32_t          U32,  *PU32;
typedef uint64_t          U64,  *PU64;

typedef float             FLT, *PFLT;
typedef double            DBL, *PDBL;

typedef void              VOID,  *PVOID;

typedef char              CHAR,  *PCHAR;
typedef unsigned char     UCHAR, *PUCHAR;
typedef UCHAR             BYTE,  *PBYTE;
typedef bool              BOOL,  *PBOOL;

#endif // !__GAPS_GENERIC_TYPES_DEFINED




//---------------------------------------------------------------------------
// UTILITY MACROS
//---------------------------------------------------------------------------

#ifndef G_NUMBER_OF
#define G_NUMBER_OF(_A_)          (sizeof(_A_)/sizeof((_A_)[0]))
#endif

#ifndef G_UNUSED_VAR
#define G_UNUSED_VAR(_var_)       (void)(_var_)
#endif

#ifndef G_IGNORE_RESULT
#define G_IGNORE_RESULT(_CALL_)   if (_CALL_){} 
#endif




#ifdef __cplusplus
}
#endif


#endif  // GAPS_COMMON_H


