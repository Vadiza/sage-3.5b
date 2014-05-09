/*
  PQLabs tracker for SAGE

  Ratko Jagodic
  June 2010
*/


#include "pqlabs.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <functional>

using namespace std;


// 0 for no sage... for testing only
#define USE_SAGE 1


/*****************************************************************************/
// globals
/*****************************************************************************/

#define round(fp) (int)((fp) >= 0 ? (fp) + 0.5 : (fp) - 0.5)


/////////  sage communication stuff  /////////
#define DIM_PORT 20005
char sageHost[100];
char pqServer[100];
char myIP[16];
char msg[1024]; // will hold queued up messages
int sock;

int touchResW = 1600;
int touchResH = 1200;

bool sageConnected = false;

TrackerClient tracker;
TIMESTAMP lastTime = -1;

#ifndef SOCKET_ERROR
#define SOCKET_ERROR    -1
#endif




/*****************************************************************************/
// SAGE connection
/*****************************************************************************/


void connectToSage(void)
{
  if (!USE_SAGE)
    return;

  // socket stuff
  int error;
  struct sockaddr_in peer;
  struct hostent *myInfo;
  char myHostname[128];

  peer.sin_family = AF_INET;
  peer.sin_port = htons(DIM_PORT);
  peer.sin_addr.s_addr = inet_addr(sageHost);

  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
  {
    printf("\nERROR: Couldn't open socket on port %d!\n", DIM_PORT);
    exit(0);
  }

  // get my IP for the message header to SAGE
  gethostname(myHostname, 128);
  myInfo = gethostbyname(myHostname);
  sprintf(myIP, "%s", inet_ntoa(*(struct in_addr*) myInfo->h_addr));

  // ignore SIGPIPE
  int set = 1;
  setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&set, sizeof(int));
  setsockopt(sock, SOL_SOCKET, TCP_NODELAY, (void *)&set, sizeof(int));


  while ((error = connect(sock, (struct sockaddr*)&peer, sizeof(peer))) != 0)
  {
    // close the failed one...
    close(sock);
    sageConnected = false;

    printf("\nTrying to reconnect to sage on %s... failed, socket error code: %d", sageHost, errno);
    mySleep(1000);

    // recreate the socket...
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // ignore SIGPIPE
    int set = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&set, sizeof(int));
    setsockopt(sock, SOL_SOCKET, TCP_NODELAY, (void *)&set, sizeof(int));
  }

  mySleep(1000);
  printf("\nConnected to sage on: %s\n", sageHost);
  sageConnected = true;
}


void disconnectFromSage(void)
{
  if (!USE_SAGE) return;
  close(sock);
  sageConnected = false;
}


void sendToSage()
{
  if (!USE_SAGE || strlen(msg) == 0) return;

  if (send(sock, msg, strlen(msg), 0) == SOCKET_ERROR)
  {
    printf("\nDisconnected from sage... reconnecting\n");
    disconnectFromSage();
    tracker.Disconnect();

    connectToSage();   // reconnect automatically

    tracker.ConnectToTracker();
  }

  strcpy(msg,"\0");
}


void queueMessage(char *newMsg)
{
  if (!USE_SAGE) return;

  if (strlen(msg) == 0) {
    sprintf(msg, "%s", newMsg);
  }
  else {
    sprintf(msg, "%s%s", msg, newMsg);
  }
}


TrackerClient::TrackerClient()
{
}


TrackerClient::~TrackerClient()
{
  delete gestureProc;
}


void TrackerClient::ConnectToTracker(bool firstTime)
{
  fprintf(stderr, "\nTrying to connect to PQ Labs server");
  int c = 0;
  while (Init(firstTime) != PQMTE_SUCESS) {
    fprintf(stderr, "\nTrying to connect to PQ Labs server", c++);
    mySleep(5000);
  }
}

void TrackerClient::Disconnect()
{
  DisconnectServer();
}

/////////////////////////// functions ///////////////////////////////////
int TrackerClient::Init(bool firstTime)
{
  int err_code = PQMTE_SUCESS;

  if (firstTime) {
    // initialize the handle functions of gestures;
    InitFuncOnTG();
    // set the functions on server callback
    SetFuncsOnReceiveProc();
  }

  // connect server
  if((err_code = ConnectServer(pqServer)) != PQMTE_SUCESS){
    cout << "...failed, socket error code: " << err_code << endl;
    return err_code;
  }
  // send request to server
  fprintf(stderr, "...Connected!");
  TouchClientRequest tcq = {0};
  tcq.app_id = GetTrialAppID();
  tcq.type = RQST_RAWDATA_ALL | RQST_GESTURE_ALL;
  if((err_code = SendRequest(tcq)) != PQMTE_SUCESS){
    cout << " send request fail, error code:" << err_code << endl;
    return err_code;
  }
  //////////////you can set the move_threshold when the tcq.type is RQST_RAWDATA_INSIDE;
  //send threshold
  int move_threshold = 10;// 1 pixel
  if((err_code = SendThreshold(move_threshold)) != PQMTE_SUCESS){
    cout << " send threadhold fail, error code:" << err_code << endl;
    return err_code;
  }
  //
  ////////////////////////
  //get server resolution
  if((err_code = GetServerResolution(OnGetServerResolution, NULL)) != PQMTE_SUCESS){
    cout << " get server resolution fail,error code:" << err_code << endl;
    return err_code;
  };
  //
  // start receiving
  fprintf(stderr, "\nListening for touches...\n");
  return PQMTE_SUCESS;
}


void TrackerClient::InitFuncOnTG()
{
  gestureProc = new GestureProcessor(this);
}


void TrackerClient::SetFuncsOnReceiveProc()
{
  PFuncOnReceivePointFrame old_rf_func = SetOnReceivePointFrame(&TrackerClient::OnReceivePointFrame,this);
  PFuncOnReceiveGesture old_rg_func = SetOnReceiveGesture(&TrackerClient::OnReceiveGesture,this);
  PFuncOnServerBreak old_svr_break = SetOnServerBreak(&TrackerClient::OnServerBreak,NULL);
  PFuncOnReceiveError old_rcv_err_func = SetOnReceiveError(&TrackerClient::OnReceiveError,NULL);
}

void TrackerClient:: OnReceiveGesture(const TouchGesture & ges, void * call_back_object)
{
}


void TrackerClient:: OnReceivePointFrame(int frame_id, int time_stamp, int moving_point_count, const TouchPoint * moving_point_array, void * call_back_object)
{
  TrackerClient * tracker = static_cast<TrackerClient*>(call_back_object);
  assert(tracker != NULL);

  fprintf(stderr, "\n\n- - - - - - - - - - - - - - - - - - - - - - - \n");

  // convert PQLabs points into our points for the clustering algorithm
  vector<point> touches;
  for(int i = 0; i < moving_point_count; ++ i){
    TouchPoint tp = moving_point_array[i];
    float fx = tp.x / (float)touchResW;
    float fy = 1 - tp.y / (float)touchResH;
    point p = point(fx, fy);
    p.dx = tp.dx;
    p.dy = tp.dy;
    p.id = tp.id;
    p.point_event = tp.point_event;
    touches.push_back( p );
  }

  // feed to gesture processor
  // =========================
  pthread_mutex_lock(&(tracker->gestureProc->clusterLock));
  vector<Gesture> gestures = tracker->gestureProc->feed(touches);
  fprintf(stderr, "\nGestures %d, Touches %d", (int)gestures.size(), (int)touches.size());

  tracker->SendGesturesToSAGE(gestures);
  pthread_mutex_unlock(&(tracker->gestureProc->clusterLock));
}


void TrackerClient::SendGesturesToSAGE(vector<Gesture> gestures)
{
  Gesture g;
  char msgData[256];
  char lp[15];

  for (int i=0; i<gestures.size(); i++) {
    g = gestures[i];
    strcpy(msgData, "\0");

    if (g.lifePoint == BEGIN) sprintf(lp, "BEGIN");
    else if(g.lifePoint == MIDDLE) sprintf(lp, "MIDDLE");
    else sprintf(lp, "END");

    if (g.gestureType == GESTURE_SINGLE_TOUCH) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d\n",
              myIP, g.id, GESTURE_SINGLE_TOUCH, g.x, g.y, g.lifePoint);
      fprintf(stderr, "\nSINGLE_TOUCH <<< %d >>>: life = %s", g.id, lp);
    }
    else if (g.gestureType == GESTURE_DOUBLE_CLICK) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d\n",
              myIP, g.id, GESTURE_DOUBLE_CLICK, g.x, g.y, g.lifePoint);
      fprintf(stderr, "\nDOUBLE_CLICK <<< %d >>>: life = %s", g.id, lp);
    }
    else if(g.gestureType == GESTURE_BIG_TOUCH) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d\n",
              myIP, g.id, GESTURE_BIG_TOUCH, g.x, g.y, g.lifePoint);
      fprintf(stderr, "\nBIG_TOUCH: <<< %d >>>: life = %s", g.id, lp);
    }
    else if(g.gestureType == GESTURE_MULTI_TOUCH_HOLD) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d %d\n",
              myIP, g.id, GESTURE_MULTI_TOUCH_HOLD, g.x, g.y, (int)g.points.size(), g.lifePoint);
      fprintf(stderr, "\nGESTURE_MULTI_TOUCH_HOLD: <<< %d >>>: life = %s", g.id, lp);
    }
    else if(g.gestureType == GESTURE_MULTI_TOUCH_SWIPE) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %f %f %d %d\n",
              myIP, g.id, GESTURE_MULTI_TOUCH_SWIPE, g.x, g.y, g.dX, g.dY, (int)g.points.size(), g.lifePoint);
      fprintf(stderr, "\nGESTURE_MULTI_TOUCH_SWIPE: <<< %d >>>: life = %s", g.id, lp);
    }
    else if(g.gestureType == GESTURE_ZOOM) {
      sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %f %d\n",
              myIP, g.id, GESTURE_ZOOM, g.x, g.y, g.amount, g.lifePoint);
      fprintf(stderr, "\nGESTURE_ZOOM: <<< %d >>>: life = %s", g.id, lp);
    }

    queueMessage(msgData);
  }

  sendToSage(); // send all queued up messages in one
}


void TrackerClient:: OnServerBreak(void * param, void * call_back_object)
{
  // when the server break, disconenct server;
  fprintf(stderr, "\n\n======> Connection with PQ Labs server lost\n");
  if (sageConnected) {
    TrackerClient * tracker = static_cast<TrackerClient*>(call_back_object);
    tracker->ConnectToTracker();
  }
}


void TrackerClient::OnReceiveError(int err_code, void * call_back_object)
{
  switch(err_code)
  {
  case PQMTE_RCV_INVALIDATE_DATA:
    cout << " error: receive invalidate data." << endl;
    break;
  case PQMTE_SERVER_VERSION_OLD:
    cout << " error: the multi-touch server is old for this client, please update the multi-touch server." << endl;
    break;
  default:
    cout << " socket error, socket error code:" << err_code << endl;
  }
}


void TrackerClient:: OnGetServerResolution(int x, int y, void * call_back_object)
{
  cout << "   Touch screen resolution: " << x << "," << y << endl;
  touchResW = x;
  touchResH = y;
}


int main(int argc, char** argv)
{
  strcpy(msg, "\0");
  // the default pq labs server
  strcpy(pqServer, "131.193.77.104");

  // check the arguments
  if (USE_SAGE) {
    if (argc < 2)
    {
      printf("\nUSAGE: pqlabs sage_master_ip\n");
      exit(0);
    }
    else if(argc > 2)
      strcpy(pqServer, argv[2]);  // if another argument is passed, use it as the pqlabs server location

    strcpy(sageHost, argv[1]);

    // initiate connections
    connectToSage();
  }

  // connect to tracker
  tracker.ConnectToTracker(true);

  // here just wait here, not let the process exit;
  char ch  = 0;
  while(ch != 'q' && ch != 'Q'){
    cout << "press \'q\' to exit" << endl;
    ch = getchar();
  }

  return 0;
}
