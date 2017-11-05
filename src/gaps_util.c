#include "gaps_util.h"


bool
PathExists(
  const CHAR* path
  )
{
  bool ret = false;
  if ( path )
  {
    if ( FILE* file = fopen( path, "r" ) )
    {
      fclose( file );
      ret = true;
    }
  }
  return ret;
}




U32 
StringTrimR(
  STDSTR& str,
  CHAR    c
  )
{
  U32 ret = 0;
  if ( !str.empty() )
  {
    U32 idx = str.size();
    while ( idx > 0 && c == str[idx-1] )
    {
      str[idx-1] = '\0';
      --idx;
      ++ret;
    }
    // let the string updates itself
    str = str.c_str();
  }
  return ret;
}




STDSTR
ExecCmd(
  const CHAR* cmd
  )
{
  CHAR   buf[2048];
  STDSTR ret;

  FILE* pipe = popen( cmd, "r" );
  if ( pipe )
  {
    while ( !feof( pipe ) )
    {
      if ( fgets( buf, G_NUMBER_OF( buf ), pipe ) != NULL )
      {
        ret += buf;
      }
    }
    pclose( pipe );
  }
  return ret;
}




STDSTR
ParseYouTubeURL(
  const CHAR* yurl
  )
{
  STDSTR ret = ExecCmd( "which youtube-dl" );
  STDSTR url;

  StringTrimR( ret, '\n' );

  if ( PathExists( ret.c_str() ) )
  {
    // get link to the highest quality mp4
    STDSTR cmd = "youtube-dl -f 22 -g ";
    cmd += yurl;
    url  = ExecCmd( cmd.c_str() );
  }
  else
  {
    printf( "YouTube URL parser not available.\n" );
  }
  return url;
}




S32 test( const CHAR* url )
{
 return 10;
}






