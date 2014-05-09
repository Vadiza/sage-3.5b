//+---------------------------------------------------------------------------
//
//  PQLabs tracker for SAGE
//
//  Author: Ratko Jagodic
//          Modification from the PQLabs sample code...
//
//----------------------------------------------------------------------------


#ifndef PQLABS_TRACKER_CLIENT_H_
#define PQLABS_TRACKER_CLIENT_H_

#include "PQMTClient.h"
#include "topHeader.h"
#include <pthread.h>

using namespace PQ_SDK_MultiTouch;


class Gesture;
class GestureProcessor;

class TrackerClient {
public:
  TrackerClient();
  ~TrackerClient();

  int Init(bool firstTime);
  void ConnectToTracker(bool firstTime=false);
  void Disconnect();
  void SendGesturesToSAGE(std::vector<Gesture> gestures);

private:
  //////////////////////call back functions///////////////////////
  // OnReceivePointFrame: function to handle when recieve touch point frame
  //  the unmoving touch point won't be sent from server. The new touch point with its pointevent is TP_DOWN
  //  and the leaving touch point with its pointevent will be always sent from server;
  static void OnReceivePointFrame(int frame_id,int time_stamp,int moving_point_count,const TouchPoint * moving_point_array, void * call_back_object);
  static void OnServerBreak(void * param, void * call_back_object);
  static void OnReceiveGesture(const TouchGesture & ges, void * call_back_object);
  static void OnReceiveError(int err_code,void * call_back_object);
  static void OnGetServerResolution(int x, int y, void * call_back_object);
  //////////////////////call back functions end ///////////////////////

  GestureProcessor *gestureProc;

  void InitFuncOnTG();
  void SetFuncsOnReceiveProc();


}; // end of namespace
#endif // end of header
