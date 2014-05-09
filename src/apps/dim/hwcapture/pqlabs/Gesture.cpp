/*
  PQLabs tracker for SAGE

  Ratko Jagodic
  June 2010

  Many reworkings of the original TacTile tracker code by Khairi Reda
*/


#include "topHeader.h"

using namespace std;


GestureProcessor::GestureProcessor(TrackerClient *tc)
{
  currId = -1;
  run = true;
  tracker = tc;

  pthread_mutex_init(&clusterLock, NULL);
  if (pthread_create(&cleanupThread, NULL, clusterCleanupThread, (void*) this) !=0) {
    fprintf(stderr, "\n\n ERROR: No cluster cleanup will happen...");
  }
}

GestureProcessor::~GestureProcessor()
{
  run = false;
  pthread_mutex_unlock(&clusterLock);
  pthread_mutex_destroy(&clusterLock);
}

void* GestureProcessor::clusterCleanupThread(void *args)
{
  GestureProcessor *This = (GestureProcessor *) args;
  vector<Gesture> gestures;  // a list of all gestures that need to be finished...

  while(This->run)
  {
    pthread_mutex_lock( &(This->clusterLock) );

    // go through all current clusters and find ones that are older than CLUSTER_LIFE
    // if older, create the END gesture from the last cluster and send it to SAGE

    TIMESTAMP now = This->getTime();
    vector<int> toDelete;  // holds all indexes to vectors of clusters for deletion...

    for (int i=0; i<This->clusters.size(); i++)
    {
      cluster *c = &(This->clusters[i].back());
      if (c->flick) {
        float newXd = c->flickXd - c->flickXd*FLICK_DAMPENING;
        float newYd = c->flickYd - c->flickYd*FLICK_DAMPENING;

        c->gesture.x = c->gesture.x + newXd;
        c->gesture.y = c->gesture.y + newYd;

        This->keepInBounds(c->gesture.x, 0.0, 1.0);
        This->keepInBounds(c->gesture.y, 0.0, 1.0);
        gestures.push_back(c->gesture);

        //fprintf(stderr, "\nsending flick... %f, %f", c->gesture.x, c->gesture.y);

        c->flickXd = newXd;
        c->flickYd = newYd;

        if ( (fabs(newXd) < END_FLICK_THRESHOLD && fabs(newYd) < END_FLICK_THRESHOLD) ||
             c->gesture.x == 0.0 || c->gesture.x == 1.0 || c->gesture.y == 0.0 || c->gesture.y == 1.0)
          c->flick = false;
      }
      else if (now - c->time > CLUSTER_LIFE) {
        c->gesture.lifePoint = END;

        // if DOUBLE_CLICK was the last gesture, erase it but dont send END event
        if (c->gesture.gestureType != GESTURE_DOUBLE_CLICK)
          gestures.push_back(c->gesture);

        toDelete.push_back(i);
        This->lastTouchDownPoint.erase(c->clusterID);
      }
    }

    // now delete them
    for (int i=0; i<toDelete.size(); i++) {
      This->clusters.erase(This->clusters.begin()+toDelete[i]);
    }

    // keep sending flick gestures if they exist
    //This->sendFlickGestures();

    // send all the ending gestures...
    This->tracker->SendGesturesToSAGE(gestures);
    gestures.clear();

    pthread_mutex_unlock( &(This->clusterLock) );

    mySleep(30);  // in milliseconds
  }
}


vector<Gesture> GestureProcessor::feed(vector<point> points)
{
  // we are getting around 20 fps...

  vector<Gesture> gestures;  // a list of all gestures from all clusters
  vector<Gesture> clusterGestures;  // what we find in each cluster

  // make clusters out of points
  vector<cluster> newClusters = makeclusters( meanshift(points, MAX_CLUSTER_SIZE) );

  // match new clusters to old ones
  // newClusters will be integrated into main clusters vector and free of noise
  matchToOldClusters(newClusters);

  // find gestures in each cluster and assemble them into "gestures" vector
  for (int i=0; i<clusters.size(); i++)
  {
    processGestures(i, clusterGestures);
    for (int i=0; i<clusterGestures.size(); i++)
      gestures.push_back(clusterGestures[i]);
    clusterGestures.clear();
  }

  return gestures;
}


int GestureProcessor::getNewClusterId()
{
  if (currId > 999)
    currId = 2;
  return ++currId;
}


void GestureProcessor::matchToOldClusters(vector<cluster> &newClusters)
{
  // match new clusters to old ones by TIME and DISTANCE
  // go through all the current clusters and find their corresponding
  // clusters from the previous frame (if any)
  // if found, copy the old cluster's history information into the new one
  cluster lastCluster, newCluster; // of certain clusterID
  int numFingers = 1;
  bool found = false;
  int historySize = 1;
  TIMESTAMP now = getTime();


  // for each new cluster...
  for (int i=0; i<newClusters.size(); i++) {
    newCluster = newClusters[i];
    newCluster.time = now;
    newCluster.flick = false;
    newCluster.lifePoint = BUFFERING;  // mark as not ready yet... might change
    found = false;


    // loop through *clusters* and look at the last item in each vector
    for( int j=0; j<clusters.size(); j++) {
      historySize = clusters[j].size();  // length of history for this clusterID
      lastCluster = clusters[j].back();   // last cluster of this clusterID...

      if ( closeClusters(newCluster, lastCluster) ) {
        if (lastCluster.flick) {  // allow new clusters over flick clusters
          fprintf(stderr, "\n\nALLOWING NEW CLUSTER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          break;
        }

        found = true;
        newCluster.clusterID = lastCluster.clusterID;

        // dont use the new data if the cluster already ended... life after DOUBLE_CLICK
        if (lastCluster.gesture.gestureType == GESTURE_DOUBLE_CLICK) {
          clusters[j].back().time = now; // update the time though so we stay in this cluster...
          break;
        }

        // double click hackery
        if (newCluster.numPoints == 1 &&
            newCluster.points[0].point_event == TP_DOWN &&
            lastTouchDownPoint.count(newCluster.clusterID) == 0)
          lastTouchDownPoint[newCluster.clusterID] = newCluster.points[0];


        //------ process single finger touches after two clicks appear
        if (historySize == 3 && lastCluster.numPoints==1 && newCluster.numPoints==1) {
          newCluster.lifePoint = BEGIN;
          clusters[j].push_back(newCluster);
        }

        //------ not enough info for determining gesture types... keep collecting
        else if (historySize < MAX_HISTORY_SIZE && lastCluster.lifePoint == BUFFERING) {
          clusters[j].push_back(newCluster);
        }

        //------ we have enough history... but of what?
        else if(historySize == MAX_HISTORY_SIZE && lastCluster.lifePoint == BUFFERING) {
          newCluster.lifePoint = BEGIN;
          clusters[j].push_back(newCluster);

          // which number of fingers spent the most time on the wall?
          numFingers = determineNumberOfFingers(j);
          fprintf(stderr, "\nNUMBER OF FINGERS: %d", numFingers);
          // adjust the newest cluster to match that
          fixTouchNoise(numFingers, j);
        }

        //------ most cases where the gesture is already under way...
        else {
          // erase the first cluster
          clusters[j].erase(clusters[j].begin());

          // add the new cluster
          newCluster.lifePoint = MIDDLE;
          clusters[j].push_back(newCluster);

          // number of fingers we want ideally... because of the gesture type
          // if not right, fix it :-)
          numFingers = lastCluster.numPoints;
          fixTouchNoise(numFingers, j);
        }

        break;
      }
    }
    if (!found) {
      newCluster.clusterID = getNewClusterId();  // ever increasing id... up to 1000, then resets

      // double click hackery
      if (newCluster.numPoints == 1 &&
          newCluster.points[0].point_event == TP_DOWN &&
          lastTouchDownPoint.count(newCluster.clusterID) == 0)
        lastTouchDownPoint[newCluster.clusterID] = newCluster.points[0];

      fprintf(stderr, "\n========================  NEW cluster <<< %d >>>  =======================", newCluster.clusterID);
      vector <cluster> newList;
      newList.push_back(newCluster);
      clusters.push_back(newList);
    }
  }
}


int GestureProcessor::determineNumberOfFingers(int clusterIndex)
{
  // given the history of clusters with this clusterID,
  // find which gesture occured most often and designated this
  // clusterID to be of that GESTURE_TYPE until it dies

  // index of array represents how many fingers...
  // the value at the index represents how many times this number of touches occured so far
  int numTouches[30] = {0};
  int max = 0;  // how many times it occured
  int numFingers = 1;  // how many fingers was that? ... this will determine our GESTURE_TYPE
  cluster c;
  int lastClusterNumPoints = clusters[clusterIndex].back().numPoints;
  int maxNumFingers = 0;

  // figure out how many times each n number of fingers occured
  for (int i=0; i<clusters[clusterIndex].size(); i++) {
    c = clusters[clusterIndex][i];
    numTouches[ c.numPoints ]++;
    if (maxNumFingers < c.numPoints)
      maxNumFingers = c.numPoints;
  }

  // find the max of those... give preference to the last cluster
  for (int i=0; i<maxNumFingers+1; i++) {
    if (numTouches[i] > max ||
        (numTouches[i] == max && i==lastClusterNumPoints)) {
      max = numTouches[i];
      numFingers = i;
    }
  }

  // we dont use 3 fingers so most likely it was 2 fingers we wanted
  if (numFingers == 3)// && max<4)
    numFingers = 2;

  return numFingers;
}


void GestureProcessor::fixTouchNoise(int numFingersWeWant, int clusterIndex)
{
  // fix the number of points in the latest cluster to match the gesture we are using (numFingers)
  int numFingersWeHave = clusters[clusterIndex].back().numPoints;

  //if (numFingersWeHave > numFingersWeWant)    // in case the last received cluster has different number of fingers
  //reduceNumberOfFingers(numFingersWeWant, clusterIndex); // discard some fingers... cluster MODIFIED IN PLACE!!!

  //else if(numFingersWeHave < numFingersWeWant)
  if (numFingersWeWant == numFingersWeHave)
    return;
  else if (numFingersWeWant == 2 && numFingersWeHave == 3)
    reduceToTwoPoints(clusters[clusterIndex].back());
  else if (numFingersWeWant < 3)
    replaceClusterWithOlderOne(numFingersWeWant, clusterIndex);  // discard the cluster and use the last one of correct type
  else          // more fingers dont matter because we aren't looking at the points themselves
    clusters[clusterIndex].back().numPoints = numFingersWeWant;
}


void GestureProcessor::reduceNumberOfFingers(int numFingersWeWant, int clusterIndex)
{
  // in a perfect world...
}


void GestureProcessor::replaceClusterWithOlderOne(int numFingersWeWant, int clusterIndex)
{
  cluster c;
  TIMESTAMP t;  // to preserve the time of the new cluster...
  LIFE_POINT l;

  // loop through the history of this cluster and find the latest one with correct number of fingers
  for (int i=clusters[clusterIndex].size()-2; i>=0; i--) {
    c = clusters[clusterIndex][i];
    if (c.numPoints == numFingersWeWant) {
      t = clusters[clusterIndex].back().time;
      l = clusters[clusterIndex].back().lifePoint;
      if (l == BEGIN)
        l = MIDDLE;
      clusters[clusterIndex].back() = c;
      clusters[clusterIndex].back().time = t;
      clusters[clusterIndex].back().lifePoint = l;
      break;
    }
  }
}


bool GestureProcessor::closeClusters(cluster c1, cluster c2)
{
  // compares the distance between the centers of two clusters and
  // returns true if they are closer than SAME_CLUSTER_DISTANCE
  float d = dist(c1.cx, c1.cy, c2.cx, c2.cy);

  if ( d > SAME_CLUSTER_DISTANCE ) {
    //fprintf(stderr, "\n---- DIST %f", d);
    return false;
  }
  else if( abs(c1.time - c2.time) < CLUSTER_LIFE) // compare the time as well
    return true;
  else {
    //fprintf(stderr, "\n---- TIME  %lli", abs(c1.time - c2.time));
    return false;
  }
}




void GestureProcessor::processGestures(int clusterIndex, vector<Gesture> &clusterGestures)
{
  // don't process gestures that don't have enough collected history information
  if (clusters[clusterIndex].back().lifePoint == BUFFERING ||
      clusters[clusterIndex].back().lifePoint == END)
    return;


  cluster c = clusters[clusterIndex].back();
  cluster prevCluster = *(clusters[clusterIndex].end()-2); // second to last element
  GESTURE_TYPE prevGestureType = prevCluster.gesture.gestureType;

  int numTouches = c.numPoints;
  float amount;  // every gesture has this

  // our new gesture...
  Gesture g = Gesture();
  g.time = c.time;
  g.points = c.points;
  g.id = c.clusterID;
  g.lifePoint = c.lifePoint;


  // process gestures based on number of fingers involved
  if (numTouches == 1) {
    int big = 30;
    int normal = 15;
    point tp = g.points[0]; // the point we are dealing with...
    g.x = c.cx;
    g.y = c.cy;
    float d = 1.0; // distance between two TP_DOWN points

    if (c.points[0].point_event == TP_DOWN) {
      if (lastTouchDownPoint.count(c.clusterID) == 0)
        lastTouchDownPoint[c.clusterID] = c.points[0];
      else
        d = dist(lastTouchDownPoint[c.clusterID], c.points[0]);
      //fprintf(stderr, "\nTP_DOWN DISTANCE: %f", d);
    }

    if ((tp.dx > big || tp.dy > big) || prevGestureType == GESTURE_BIG_TOUCH)
      g.gestureType = GESTURE_BIG_TOUCH;
    else if(d < DOUBLE_CLICK_MAX_DIST) {
      g.gestureType = GESTURE_DOUBLE_CLICK;
      clusters[clusterIndex].back().lifePoint = END;  // dont allow any more events here...
      g.lifePoint = END;
    }
    else
      g.gestureType = GESTURE_SINGLE_TOUCH;

    // for figuring out if we should flick the gesture...
    if (c.points[0].point_event == TP_UP && g.gestureType == GESTURE_SINGLE_TOUCH) {
      cluster ppCluster = *(clusters[clusterIndex].end()-3);
      float flickD = dist(ppCluster.points[0], c.points[0]);
      if (flickD > MIN_FLICK_THRESHOLD) {
        clusters[clusterIndex].back().flick = true;
        clusters[clusterIndex].back().flickXd = c.points[0].x - ppCluster.points[0].x;
        clusters[clusterIndex].back().flickYd = c.points[0].y - ppCluster.points[0].y;
      }
    }
  }

  else if(numTouches == 2) {

    if (processZoom(c, prevCluster, amount)) {
      g.gestureType = GESTURE_ZOOM;
      g.x = c.cx;
      g.y = c.cy;
      g.amount = amount;
    }
  }

  else if(numTouches > 3) {
    float dX, dY;

    cluster firstMultiTouchCluster = prevCluster;
    for (int i=0; i<clusters[clusterIndex].size(); i++) {
      if (clusters[clusterIndex][i].numPoints > 3) {
        firstMultiTouchCluster = clusters[clusterIndex][i];
        break;
      }
    }

    bool swipe = processMultiTouchSwipe(c, firstMultiTouchCluster, dX, dY);

    if ((c.lifePoint == BEGIN && swipe) ||
        (c.lifePoint != BEGIN && prevGestureType == GESTURE_MULTI_TOUCH_SWIPE))
    {
      g.gestureType = GESTURE_MULTI_TOUCH_SWIPE;
      g.x = c.cx;
      g.y = c.cy;
      g.dX = dX;
      g.dY = dY;
      g.amount = amount;
    }
    else {
      g.gestureType = GESTURE_MULTI_TOUCH_HOLD;
      g.x = c.cx;
      g.y = c.cy;
    }
  }

  else {
    // if no gestures were found, return an empty one with points only
    // and find the center of the cluster... this will be the location of gesture
    for (int i=0; i<c.points.size(); i++)
      c.points[i].id = c.clusterID;
    g.points = c.points;  // re-assign the points
    g.x = c.cx;
    g.y = c.cy;
  }

  // keep it around for later
  clusters[clusterIndex].back().gesture = g;

  clusterGestures.push_back(g);
}

bool GestureProcessor::processZoom(cluster &c, cluster &lastCluster, float & zoomAmount)
{
  if (c.lifePoint == MIDDLE)
  {
    float dDistance = dist(c.points[0], c.points[1]) - dist(lastCluster.points[0], lastCluster.points[1]);
    zoomAmount = ZOOM_FACTOR * dDistance;
  }
  else
    zoomAmount = 0.0;
  return true;
}


bool GestureProcessor::processMultiTouchSwipe(cluster &c, cluster &lastCluster, float &dX, float &dY)
{
  float _dx = c.cx - lastCluster.cx;
  float _dy = c.cy - lastCluster.cy;

  if (fabs(_dx) > SWIPE_DISTANCE_X_MIN || fabs(_dy) > SWIPE_DISTANCE_Y_MIN) {
    dX = SWIPE_FACTOR * _dx;
    dY = SWIPE_FACTOR * _dy;
    return true;
  }
  else
    return false;
}


float GestureProcessor::dist(point & p1, point & p2)
{
  return sqrt( pow( p1.getx() - p2.getx(), 2) + pow( p1.gety() - p2.gety(), 2) );
}


float GestureProcessor::dist(float x1, float y1, float x2, float y2)
{
  return sqrt( pow( x1-x2, 2) + pow( y1-y2, 2) );
}


point GestureProcessor::halfway(point &p1, point &p2)
{
  return point( p1.x+(p2.x-p1.x)/2.0, p1.y+(p2.y-p1.y)/2.0 );
}


void GestureProcessor::keepInBounds(float &val, float lower, float upper)
{
  if (val < lower)
    val = lower;
  else if(val > upper)
    val = upper;
}


void GestureProcessor::reduceToTwoPoints(cluster &c)
{
  // basically, find two closest points and merge them into one

  // only reduce 3 to 2 points
  if (c.points.size() != 3)
    return;

  // find the two points with the min distance between them
  point mergedPoint, p1, p2;
  vector<point> pts = c.points;

  // distances between all points
  float d1 = dist(pts[0], pts[1]);
  float d2 = dist(pts[1], pts[2]);
  float d3 = dist(pts[0], pts[2]);

  // the smallest of the distances
  float minD = min(d1, min(d2,d3));

  // the two closest points
  if (minD == d1) {
    p1 = pts[0];
    p2 = pts[1];
    c.points.erase(c.points.begin());
    c.points.erase(c.points.begin()+1);
  }
  else if(minD == d2) {
    p1 = pts[1];
    p2 = pts[2];
    c.points.erase(c.points.begin()+1);
    c.points.erase(c.points.begin()+2);
  }
  else {
    p1 = pts[0];
    p2 = pts[2];
    c.points.erase(c.points.begin());
    c.points.erase(c.points.begin()+2);
  }

  // merge the two points into one... in the center
  c.points.push_back( halfway(p1, p2) );
  c.numPoints = 2;
}
