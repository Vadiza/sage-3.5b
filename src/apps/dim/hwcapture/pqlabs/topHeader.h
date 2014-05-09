
#ifndef GESTURES_H
#define GESTURES_H


#include <vector>
#include <map>
#include <pthread.h>
#include "misc.h"
#include <iostream>
#include <math.h>
#include "pqlabs.h"
#include <algorithm>

//#define EXAGGERATE 0

//using namespace std;


// zoom gesture
//static const float ZOOM_DISTANCE_MIN =  0.0005f;
//static const float ZOOM_DISTANCE_MAX =  0.02f;
#ifdef EXAGGERATE
static const float ZOOM_FACTOR = 0.3f;
#else
static const float ZOOM_FACTOR = 5.0f;
#endif

// flick
static const float MIN_FLICK_THRESHOLD = 0.006f;  // distance between last two touches to count as a flick
static const float MAX_FLICK_THRESHOLD = 0.03f;
static const float END_FLICK_THRESHOLD = 0.001f;
static const float FLICK_DAMPENING = 0.1f;  // bigger num = more dampened

// swipe
static const float SWIPE_DISTANCE_Y_MIN = 0.015f;  // minimum difference betwen two points to consider it a swipe
static const float SWIPE_DISTANCE_X_MIN = SWIPE_DISTANCE_Y_MIN/2.0f;  // minimum difference betwen two points to consider it a swipe
static const float SWIPE_FACTOR = 1.0f;

// double click
static const TIMESTAMP DOUBLE_CLICK_MAX_TIME = 400000; //CLUSTER_LIFE;
static const TIMESTAMP DOUBLE_CLICK_MIN_TIME = 5000;
static const float DOUBLE_CLICK_MAX_DIST = 0.013f;

// clusters
#ifdef EXAGGERATE
static const float MAX_CLUSTER_SIZE = 0.3f;  // how big can the cluster of points be in normalized pixels
static const float SAME_CLUSTER_DISTANCE = 0.1f;  // how far can two clusters be in order to be considered the same in two different frames
#else
static const float MAX_CLUSTER_SIZE = 0.15f;  // how big can the cluster of points be in normalized pixels
static const float SAME_CLUSTER_DISTANCE = 0.05f;  // how far can two clusters be in order to be considered the same in two different frames
#endif
static const TIMESTAMP CLUSTER_LIFE = 200000;  // microseconds before the cluster is erased from history

static const int MAX_HISTORY_SIZE = 6;  // how many previous clusters are buffered until we determine the GESTURE_TYPE?
static const int MAX_GESTURE_COUNT = 200;



static void mySleep(unsigned long millis) {
  usleep(millis*1000);
};



// forward class declaration
class TrackerClient;
class GestureProcessor;


// GESTURE TYPES
enum GESTURE_TYPE
  {
    GESTURE_NULL,           // nothing... default... unrecognizable gesture
    GESTURE_SINGLE_TOUCH,   // 1 finger at a time
    GESTURE_DOUBLE_CLICK,   // 1 finger, twice in a row
    GESTURE_BIG_TOUCH,      // 1 big blob
    GESTURE_ZOOM,           // 2 finger pinch gesture
    GESTURE_MULTI_TOUCH_HOLD,  // 4 or more fingers at a time in one place
    GESTURE_MULTI_TOUCH_SWIPE, // 4 or more fingers at a time moving
  };


// where are we in the gesture's life?
// is this the first event? last?
enum LIFE_POINT
  {
    BUFFERING,  // we haven't begun yet... still collecting history
    BEGIN,
    MIDDLE,
    END,
  };



class point  {
protected:
  int pid; //pictureid
  float t; //time(in seconds)
  int clusterID;  //cluster id
  float duration;  //duration of fixation (if appropriate) in seconds
  float pupilo,pupilv; //horizontal and vertical diameter of pupil
public :
  point ();
  point (float x, float y);
  point (int pid, float t, float x, float y);

  float x,y;  //x and y coordinates

  // added for compatilibility with TouchPoint struct from PQLabs
  unsigned short  dx;
  unsigned short  dy;
  unsigned short  point_event;
  unsigned short  id;
  // end compatibility

  friend class GestureProcessor;

  int getpid() {
    return pid;
  }
  float gett() {
    return t;
  }
  float getx() {
    return x;
  }
  float gety() {
    return y;
  }
  void setx(float newx) {
    this->x = newx;
  }
  void sety(float newy) {
    this->y = newy;
  }
  void setclusterID(int label) {
    this->clusterID=label;
  }
  int getclusterID() {
    return clusterID;
  }
  void setduration (float dur){
    this->duration=dur;
  }
  float getduration (){
    return duration;
  }
  void setpupil(float orizontal, float vertical) {
    this->pupilo=orizontal;
    this->pupilv=vertical;
  }
  float getpupilo() {
    return pupilo;
  }
  float getpupilv() {
    return pupilv;
  }
  virtual void print();
  float calcdist(point p);
  float calctempdist(point p);


  bool operator== (point p) {
    if (x == p.x && y == p.y)
      return true;
    else
      return false;
  }
};



class Gesture
{
public:
  Gesture(): gestureType(GESTURE_NULL), lifePoint(BEGIN), amount(0.0f), x(0.0f), y(0.0f) {}

  GESTURE_TYPE gestureType;
  GESTURE_TYPE getGestureType() const { return gestureType; }

  int id;  // gesture id... same as clusterID
  TIMESTAMP time;
  LIFE_POINT lifePoint;  // beginning, middle or end?
  float amount;
  float x;
  float y;
  float dX;
  float dY;

  std::vector<point> points;  // points that make up the gesture... or not
};




struct cluster  {
  std::vector<point> points;

  int clusterID;  // same as the points that are in there
  Gesture gesture;
  LIFE_POINT lifePoint;  // none, beginning, middle or end?

  float flickXd, flickYd; // flick direction... if any
  bool flick;  // true/false

  int numPoints; //number of points in the cluster
  float cx,cy; //coordinates of the center
  float varx,vary; //variances (sigma^2)
  float sse; //similarity = sum(C-p)^2
  float sd1,sd2; //eigenvalues of the correlation matrix
  float e1x,e1y,e2x,e2y; //eigenstd::vectors of the correlation matrix
  float alpha; //angle between e1 and x axe in degree
  TIMESTAMP time; //time when the cluster was created
};


/* class Gesture */
class GestureProcessor
{
public:
  GestureProcessor(TrackerClient *tracker);
  ~GestureProcessor();
  std::vector<Gesture> feed(std::vector<point> points);

  TIMESTAMP getTime() { return getIntTime(); };

  pthread_mutex_t clusterLock;

private:

  // stores std::vectors of clusters, each std::vector corresponds to one clusterID
  // and holds a history of clusters... basically a trail of touches that
  // are considered the same cluster... history size is HISTORY_SIZE
  std::vector< std::vector<cluster> > clusters;

  TrackerClient *tracker;  // pointer to the tracker object

  // for generating new cluster IDs
  int currId;

  // gesture cleanup thread
  pthread_t cleanupThread;
  bool run;

  // for double click events... keeps track of the last time we got TP_DOWN from pqlabs
  std::map<int, point> lastTouchDownPoint;  // key=clusterID, value=last TP_DOWN time

  // gesture finding
  void processGestures(int clusterIndex, std::vector<Gesture> &clusterGestures);
  bool processZoom(cluster &c, cluster &lastCluster, float & zoomAmount);
  bool processMultiTouchSwipe(cluster &c, cluster &lastCluster, float &dX, float &dY );
  int determineNumberOfFingers(int clusterIndex);

  // getting rid of noise in touches... false positive touches and missing touches
  void fixTouchNoise(int numFingersWeWant, int clusterIndex);
  void reduceNumberOfFingers(int numFingersWeWant, int clusterIndex);
  void replaceClusterWithOlderOne(int numFingersWeWant, int clusterIndex);
  void reduceToTwoPoints(cluster &c);

  // cluster finding
  void matchToOldClusters(std::vector<cluster> &newClusters);
  bool closeClusters(cluster c1, cluster c2);
  int getNewClusterId();
  static void * clusterCleanupThread(void *args);

  // helper funcs
  float dist(point &p1, point &p2);
  float dist(float x1, float y1, float x2, float y2);
  point halfway(point &p1, point &p2);
  void keepInBounds(float &val, float lower, float upper);
};









//return a std::vector of cluster from a std::vector of points(that have a valid cluster label)
std::vector<cluster> makeclusters(std::vector<point> p);
std::vector<cluster> makeclusters(std::vector< std::vector<point> > p);

//meanshift algorithm
std::vector<point> meanshift(std::vector<point> points, float sigmas);
float move(std::vector<point>& allpoints, int me, float sigmas, bool weighted);
std::vector<int> getneig_ids(float eps, std::vector<point> allpoints, int me); //used by expandcluster and meanshift

#endif
