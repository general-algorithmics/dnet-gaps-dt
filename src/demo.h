#ifndef DEMO
#define DEMO

#include "image.h"
void
demo(
  char*       cfgfile,
  char*       weightfile,
  float       thresh,
  int         cam_index,
  const char* filename,
  const char* output,
  char**      names,
  int         classes,
  int         frame_skip,
  char*       prefix,
  float       hier_thresh
  );

void
demo_yurl(
  char*       cfgfile,
  char*       weightfile,
  float       thresh,
  const char* yurl,
  const char* output,
  char**      names,
  int         classes,
  int         frame_skip,
  char*       prefix,
  float       hier_thresh
  );

#endif
