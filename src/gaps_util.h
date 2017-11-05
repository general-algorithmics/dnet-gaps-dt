#ifndef GAPS_UTIL_H
#define GAPS_UTIL_H

#include "gaps_common.h"
#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif




//---------------------------------------------------------------------------
// STD TYPES
//---------------------------------------------------------------------------

#ifndef STDSET
#define STDSET      std::set
#endif  // !STDSET

#ifndef STDSTR
#define STDSTR      std::string
#endif  // !STDSTR

#ifndef STDVEC
#define STDVEC      std::vector
#endif  // !STDVEC

#ifndef STDMAP
#define STDMAP      std::map
#endif  // !STDMAP

#ifndef STDLST
#define STDLST      std::list
#endif  // !STDLST

#ifndef STDQUE
#define STDQUE      std::queue
#endif  // !STDQUE

#ifndef __GAPS_STD_TYPES_DEFINED
#define __GAPS_STD_TYPES_DEFINED

typedef  STDVEC<STDSTR>   STRVEC;
typedef  STDVEC<S32>      S32VEC;
typedef  STDVEC<U32>      U32VEC;
typedef  STDVEC<S64>      S64VEC;
typedef  STDVEC<U64>      U64VEC;
typedef  STDVEC<DBL>      DBLVEC;
typedef  STDSET<S64>      S64SET;
typedef  STDSET<U64>      U64SET;

#endif // !__GAPS_STD_TYPES_DEFINED



#ifdef __cplusplus
}
#endif


bool    PathExists( const CHAR* path );
U32     StringTrimR( STDSTR& str, CHAR c );
STDSTR  ExecCmd( const CHAR* cmd );
STDSTR  ParseYouTubeURL( const CHAR* yurl );

S32 test( const CHAR* url );




#endif  // GAPS_UTIL_H


