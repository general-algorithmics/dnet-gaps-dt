#include "network.h"
#include "detection_layer.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "image.h"
#include "demo.h"
#include "gaps_util.h"
#ifdef __linux__
#include <sys/time.h>
#endif


#define WINDOW_TITLE  "General Algorithmics Demo"


#define FRAMES 3

#ifdef OPENCV
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"
image get_image_from_stream(CvCapture *cap);
void save_image_to_video_cv(image p, CvVideoWriter *vwtr);

static char**  demo_names;
static image** demo_alphabet;
static int demo_classes;

static float** probs;
static box* boxes;
static network net;
static image in;
static image in_s;
static image det;
static image det_s;
static image disp = {0};
static CvCapture* cap;
static float fps = 0;
static float demo_thresh = 0;
static float demo_hier_thresh = .5;

static float* predictions[FRAMES];
static int demo_index = 0;
static image images[FRAMES];
static float* avg;


void* fetch_in_thread( void* ptr )
{
  in = get_image_from_stream( cap );
  if( !in.data )
  {
    error("Stream closed.");
  }
  in_s = resize_image( in, net.w, net.h );
  return 0;
}

void* detect_in_thread( void* ptr )
{
  float nms = .4;

  layer  l = net.layers[net.n-1];
  float* X = det_s.data;
  float* prediction = network_predict( net, X );

  memcpy( predictions[demo_index], prediction, l.outputs*sizeof(float) );
  mean_arrays( predictions, FRAMES, l.outputs, avg );
  l.output = avg;
  printf( "l.side: %d, prediction=%p.\n", l.side, prediction );
  free_image( det_s );
  if ( l.type == DETECTION )
  {
    get_detection_boxes( l, 1, 1, demo_thresh, probs, boxes, 0 );
  }
  else if ( l.type == REGION )
  {
    get_region_boxes(
      l, 1, 1, demo_thresh, probs, boxes, 0, 0, demo_hier_thresh );
  }
  else
  {
    error( "Last layer must produce detections\n" );
  }
  if ( nms > 0 )
  {
    do_nms( boxes, probs, l.w*l.h*l.n, l.classes, nms );
  }
  printf("\033[2J");
  printf("\033[1;1H");
  printf("\n=== General Algorithmics Demo ===");
  printf("\nFPS:%.1f\n",fps);
  printf("Objects:\n\n");

  images[demo_index] = det;
  //det = images[(demo_index + FRAMES/2 + 1)%FRAMES];
  demo_index = (demo_index + 1)%FRAMES;

  draw_detections(
    det, l.w*l.h*l.n, demo_thresh, boxes, probs,
    demo_names, demo_alphabet, demo_classes );

  return 0;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

#define QUEUE_SIZE  ( 128 )

typedef struct QueueEntry
{
  volatile U32  _rcnt;  // read   counter
  volatile U32  _wcnt;  // writer counter
  image _d;
  image _ds;

  QueueEntry(
    ):
    _rcnt( 0 ),
    _wcnt( 0 ),
    _d(),
    _ds()
  {
  }
} QueueEntry_t;

typedef struct Queue
{
  volatile U32 _curridx;  // index to next entry to read
  volatile U32 _curwidx;  // index to next entry to write
  QueueEntry_t _ent[QUEUE_SIZE];

  Queue(
    ):
    _curridx( 0 ),
    _curwidx( 0 ),
    _ent()
  {
  }
} Queue_t;


static volatile U32   _stop;
static Queue_t        _queue;
static QueueEntry_t*  _currRead;


PVOID
DoReadFrame(
  PVOID  pv
  )
{
  QueueEntry_t* curr = _queue._ent;

  while ( !_stop )
  {
    if ( !( curr->_wcnt & 0x01 ) && ( curr->_wcnt == curr->_rcnt ) )
    {
      // set write counter to odd to indicate entry is now being writen
      ++curr->_wcnt;

      // get a new frame from camera
      curr->_d  = get_image_from_stream( cap );
      if ( !curr->_d.data )
      {
        error( "Stream closed." );
        break;
      }
      curr->_ds = resize_image( curr->_d, net.w, net.h );

      // set the write counter to even to indicate entry is now
      // ready for consumption
      ++curr->_wcnt;

      if ( curr == &_queue._ent[QUEUE_SIZE-1] )
      {
        curr = &_queue._ent[0];
        _queue._curwidx = 0;
      }
      else
      {
        ++curr;
        ++_queue._curwidx;
      }
    }
  }  // while

  return NULL;
}


PVOID
DoDetect(
  PVOID  pv
  )
{
  U64 detected = 0;
  U32 tgtIdx = 0;

  if ( !( _currRead->_wcnt & 0x01 ) &&
        ( 2 == ( _currRead->_wcnt - _currRead->_rcnt ) ) )
  {
    // set read counter to odd to indicate entry is now being read
    ++_currRead->_rcnt;

    //-------------------------------------------------------------
    tgtIdx = _queue._curwidx;
    if ( tgtIdx == 0 )
    {
      tgtIdx = ( QUEUE_SIZE-1 );
    }
    else
    {
      tgtIdx = ( tgtIdx - 1 );
    }
    if ( tgtIdx == _queue._curridx )
    {
      det   = _currRead->_d;
      det_s = _currRead->_ds;
      detect_in_thread( 0 );
      detected = 1;
    }
    //-------------------------------------------------------------

    // set the read counter to even to indicate entry has been read
    ++_currRead->_rcnt;

    // advance pointer
    if ( _currRead == &_queue._ent[QUEUE_SIZE-1] )
    {
      _currRead = &_queue._ent[0];
      _queue._curridx = 0;
    }
    else
    {
      ++_currRead;
      ++_queue._curridx;
    }
  }

  return (PVOID)( detected );
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////






double get_wall_time()
{
  struct timeval time;
  if ( gettimeofday( &time, NULL ) )
  {
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}


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
  )
{
    //skip = frame_skip;
    image** alphabet = load_alphabet();
    //int delay = frame_skip;
    CvVideoWriter* vwtr = NULL;
    demo_names    = names;
    demo_alphabet = alphabet;
    demo_classes  = classes;
    demo_thresh   = thresh;
    demo_hier_thresh = hier_thresh;
    printf( "General Algorithmics Demo\n" );
    net = parse_network_cfg( cfgfile );
    if ( weightfile )
    {
      load_weights( &net, weightfile );
    }
    set_batch_network( &net, 1 );

    srand( 2222222 );

    if ( filename )
    {
      printf("video file: %s\n", filename);
      cap = cvCaptureFromFile( filename );
    }
    else
    {
      cap = cvCaptureFromCAM( cam_index );
    }

    if ( !cap )
    {
      error( "Couldn't connect to webcam.\n" );
    }

    if( output )
    {
      DBL fps = cvGetCaptureProperty( cap, CV_CAP_PROP_FPS );
      DBL fw  = cvGetCaptureProperty( cap, CV_CAP_PROP_FRAME_WIDTH );
      DBL fh  = cvGetCaptureProperty( cap, CV_CAP_PROP_FRAME_HEIGHT );
      CvSize fsz = cvSize( (S32)fw, (S32)fh );
      vwtr = cvCreateVideoWriter(
        output, CV_FOURCC('M','J','P','G'), fps, fsz, 1 );
    }

    layer l = net.layers[net.n-1];
    int j;

    avg = (float *) calloc(l.outputs, sizeof(float));

    for ( j = 0; j < FRAMES; ++j )
    {
      predictions[j] = (float *)calloc( l.outputs, sizeof( float ) );
    }
    for ( j = 0; j < FRAMES; ++j )
    {
      images[j] = make_image( 1, 1, 3 );
    }

    boxes = (box*)calloc( l.w*l.h*l.n, sizeof( box ) );
    probs = (float**)calloc( l.w*l.h*l.n, sizeof( float* ) );
    for ( j = 0; j < l.w*l.h*l.n; ++j )
    {
      probs[j] = (float*)calloc( l.classes, sizeof( float ) );
    }

    pthread_t fetch_thread;
    //pthread_t detect_thread;

    fetch_in_thread( 0 );
    det   = in;
    det_s = in_s;

    fetch_in_thread(  0 );
    detect_in_thread( 0 );
    disp  = det;
    det   = in;
    det_s = in_s;

    for ( j = 0; j < FRAMES/2; ++j )
    {
      fetch_in_thread(  0 );
      detect_in_thread( 0 );
      disp  = det;
      det   = in;
      det_s = in_s;
    }

    S32 count  = 0;
    U32 countP = 0;

    if ( !prefix )
    {
      cvNamedWindow( WINDOW_TITLE, CV_WINDOW_NORMAL );
      //cvSetWindowProperty(
      //  WINDOW_TITLE, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
      cvMoveWindow( WINDOW_TITLE, 0, 0 );
      cvResizeWindow( WINDOW_TITLE, 800, 600 );
    }

    double before = get_wall_time();
    double elaps;

    if ( pthread_create( &fetch_thread, 0, DoReadFrame, 0 ) )
    {
      error("Thread creation failed");
    }
    _currRead = _queue._ent;

    while ( 1 )
    {
      ++count;

      if ( DoDetect( 0 ) )
      {
        if ( !prefix )
        {
          // save image into output video file?
          if ( vwtr )
          {
            save_image_to_video_cv( disp, vwtr );
          }

          disp = det;
          show_image( disp, WINDOW_TITLE );
          free_image( disp );

          int c = cvWaitKey( 1 );
          if ( c == 10 )
          {
            // nada
          }
        }
        // TODO - need to make this work
        //else
        //{
        //  char buff[256];
        //  sprintf( buff, "%s_%08d", prefix, count );
        //  save_image( disp, buff );
        //}

        //free_image( disp );
        //disp  = det;
        //det   = in;
        //det_s = in_s;
      }  // detected

      if ( ( count % 10 ) == 0 )
      {
        double after = get_wall_time();

        if ( before != 0 )
        {
          U32 detected = ( count - countP );
          elaps = ( after - before );
          fps   = ( detected / elaps );
          countP = count;
        }
        before = after;
      }
    }  // while

    if ( vwtr )
    {
      cvReleaseVideoWriter(&vwtr);
    }

    _stop = 1;
    pthread_join( fetch_thread,  0 );

}  // demo

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
  )
{
    //skip = frame_skip;
    image** alphabet = load_alphabet();
    int     delay    = frame_skip;
    CvVideoWriter* vwtr = NULL;

    demo_names       = names;
    demo_alphabet    = alphabet;
    demo_classes     = classes;
    demo_thresh      = thresh;
    demo_hier_thresh = hier_thresh;
    printf("General Algorithmics Demo\n");
    net = parse_network_cfg(cfgfile);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, 1);

    srand(2222222);

    if(yurl){
        STDSTR url = ParseYouTubeURL(yurl);
        StringTrimR( url, '\n' );
        printf("youtube url: [%s]\n", yurl);
        printf("----------\n");
        printf("video url: [%s]\n", url.c_str());
        cap = cvCaptureFromFile(url.c_str());

        if(!cap){
            error("Couldn't connect to url.\n");
        }

        if(output){
          DBL fps = cvGetCaptureProperty(cap, CV_CAP_PROP_FPS);
          DBL fw  = cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH);
          DBL fh  = cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT);
          CvSize fsz = cvSize((S32)fw, (S32)fh);
          vwtr = cvCreateVideoWriter(output, CV_FOURCC('M','J','P','G'), fps, fsz, 1);
        }
    }

    layer l = net.layers[net.n-1];
    int j;

    avg = (float *) calloc(l.outputs, sizeof(float));
    for(j = 0; j < FRAMES; ++j) predictions[j] = (float *) calloc(l.outputs, sizeof(float));
    for(j = 0; j < FRAMES; ++j) images[j] = make_image(1,1,3);

    boxes = (box *)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float **)calloc(l.w*l.h*l.n, sizeof(float *));
    for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = (float *)calloc(l.classes, sizeof(float));

    pthread_t fetch_thread;
    pthread_t detect_thread;

    fetch_in_thread(0);
    det = in;
    det_s = in_s;

    fetch_in_thread(0);
    detect_in_thread(0);
    disp = det;
    det = in;
    det_s = in_s;

    for(j = 0; j < FRAMES/2; ++j){
        fetch_in_thread(0);
        detect_in_thread(0);
        disp = det;
        det = in;
        det_s = in_s;
    }

    int count = 0;
    if(!prefix){
        cvNamedWindow(WINDOW_TITLE, CV_WINDOW_NORMAL); 
        cvMoveWindow(WINDOW_TITLE, 0, 0);
        cvResizeWindow(WINDOW_TITLE, 990, 740);
    }

    double before = get_wall_time();

    while(1){
        ++count;
        if(1){
            if(pthread_create(&fetch_thread, 0, fetch_in_thread, 0)){
              error("Thread creation failed");
            }
            if(pthread_create(&detect_thread, 0, detect_in_thread, 0)){
              error("Thread creation failed");
            }

            if(!prefix){
                // save image into output video file?
                if(vwtr){
                  save_image_to_video_cv(disp, vwtr);
                }
                show_image(disp, WINDOW_TITLE);
                int c = cvWaitKey(1);
                if (c == 10){
                    if(frame_skip == 0) frame_skip = 60;
                    else if(frame_skip == 4) frame_skip = 0;
                    else if(frame_skip == 60) frame_skip = 4;   
                    else frame_skip = 0;
                }
            }else{
                char buff[256];
                sprintf(buff, "%s_%08d", prefix, count);
                save_image(disp, buff);
            }

            pthread_join(fetch_thread, 0);
            pthread_join(detect_thread, 0);

            if(delay == 0){
                free_image(disp);
                disp  = det;
            }
            det   = in;
            det_s = in_s;
        }else {
            fetch_in_thread(0);
            det   = in;
            det_s = in_s;
            detect_in_thread(0);
            if(delay == 0) {
                free_image(disp);
                disp = det;
            }
            show_image(disp, WINDOW_TITLE);
            cvWaitKey(1);
        }
        --delay;
        if(delay < 0){
            delay = frame_skip;

            double after = get_wall_time();
            float curr = 1./(after - before);
            fps = curr;
            before = after;
        }
    }  // while
    if(vwtr){
        cvReleaseVideoWriter(&vwtr);
    }
}
#else
void demo(
  char* cfgfile,
  char* weightfile,
  float thresh,
  int   cam_index,
  const char* filename,
  char**      names,
  int         classes,
  int         frame_skip,
  char*       prefix,
  float       hier_thresh
  )
{
  fprintf(stderr, "Demo needs OpenCV for webcam images.\n");
}

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
  )
{
  fprintf(stderr, "Demo needs OpenCV for webcam/url images.\n");
}
#endif

