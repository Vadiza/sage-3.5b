/*
 *  point.cpp
 *  cluster
 *
 *  Created by Filippo Galgani on 23/10/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */


//#include "point.h"
#include "topHeader.h"

using namespace std;

point::point () {
  pid=0;
  t=0;
  x=0;
  y=0;
  duration=0;
  clusterID = 0; //0 = not in any cluster
  pupilo=0;
  pupilv=0;
}

point::point (float x, float y) {
  this->x=x;
  this->y=y;
  pid=0;
  t=0;
  duration=0;//1/60;
  clusterID = 0; //0 = not in any cluster
  pupilo=0;
  pupilv=0;
}

point::point (int pid, float t, float x, float y) {
  this->pid=pid;
  this->t=t;
  this->x=x;
  this->y=y;
  duration=0;//1/60;
  clusterID = 0; //0 = not in any cluster
  pupilo=0;
  pupilv=0;
}

void point::print() {
  cout<<"point pid="<<pid<<" t="<<t<<" x="<<x<<" y="<<y<<" cluster="<<clusterID<<" duration="<<duration<<" pupil= "<<pupilo<<"x"<<pupilv<<endl;
}

float point::calcdist(point p) {
  float dist = ( ((p.getx()) - (this->x) )*( (p.getx()) - (this->x)) ) + ( (p.gety()-(this->y))*(p.gety()-(this->y)) );
  dist= sqrt(dist);
  return dist;
}

float point::calctempdist(point p) {
  float dist = ((p.getx()) - (this->x) )*( (p.getx()) - (this->x)) ;
  dist += (p.gety()-(this->y))*(p.gety()-(this->y));
  dist += (p.gett()-(this->t))*(p.gett()-(this->t));
  dist= sqrt(dist);
  return dist;
}
