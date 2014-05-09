/*
 *  clustering.cpp
 *  cluster
 *
 *  Created by Filippo Galgani on 12/11/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

//#include "clustering.h"
#include "topHeader.h"

using namespace std;


vector<cluster> makeclusters(vector<point> p) {
  vector<cluster> c;
  vector<int> lbl;  //in lbl[x] c'è scritto qual'è l'etichetta di c[x]

  for (int k=0; k<p.size(); k++) {
    int l = p[k].getclusterID();
    int nuovo=-1;
    for(int s=0; s<lbl.size(); s++)
      if(lbl[s]==l)
        nuovo=s;
    if ( nuovo==-1 ) {
      lbl.push_back(l);
      cluster clu;
      (clu.points).push_back(p[k]);
      c.push_back(clu);
    }
    else
      (c[nuovo].points).push_back(p[k]);
  }

  for (int j=0; j<c.size(); j++) {
    float ax=0,ay=0;  //mean x and y (center of the cluster) //posso pesarlo con la duration
    float tt=0;
    for (int i=0; i<(c[j].points).size(); i++) {
      ax+=(c[j].points)[i].getx();
      ay+=(c[j].points)[i].gety();
      tt+=(c[j].points)[i].getduration();
    }
    ax = ax / (c[j].points).size();
    ay = ay / (c[j].points).size();
    float sx=0,sy=0,sxy=0;
    float simil = 0;
    for (int i=0; i<(c[j].points).size(); i++) {
      float dx=(c[j].points)[i].getx() - ax;
      float dy=(c[j].points)[i].gety() - ay;
      sx += dx*dx;
      sy += dy*dy;
      sxy += dx*dy;
      simil += dx*dx + dy*dy;
    }
    if( (c[j].points).size() > 1) {
      sx = sx / ((c[j].points).size());    // variance x
      sy = sy / ((c[j].points).size());  // variance y
      sxy= sxy / ((c[j].points).size()); // covariance xy
    }
    //    cout<<"cx,cy"<<ax<<","<<ay<<"sx,sy="<<sx<<","<<sy<<" size="<<(c[j].points).size()<<endl;
    float lam1 = (sx+sy+sqrtf(sx*sx+4*sxy*sxy+sy*sy-2*sx*sy))/2;
    float lam2 = fabs ( (sx+sy-sqrtf(sx*sx+4*sxy*sxy+sy*sy-2*sx*sy))/2 );

    c[j].numPoints = (c[j].points).size();
    c[j].cx = ax;
    c[j].cy = ay;
    c[j].varx=sx;
    c[j].vary=sy;
    c[j].sd1 = sqrtf(lam1);
    c[j].sd2 = sqrtf(lam2);
    c[j].e1x = lam1-sy;
    c[j].e1y = sxy;
    c[j].e2x = lam2-sy;
    c[j].e2y = sxy;
    if ( c[j].e1x == 0 )
      c[j].alpha = 0;
    else
      c[j].alpha = (atan(c[j].e1y/c[j].e1x))*180/3.14159265;
    c[j].sse = simil;
    //c[j].time=tt;

    // assign the cluster label to the cluster itself
    if ( (int) c[j].points.size() > 0)
      c[j].clusterID = c[j].points[0].getclusterID();
  }
  return c;
}

vector<int> getneig_ids(float eps, vector<point> allpoints, int me) {
  vector<int> neigh;
  for (int k=0; k<allpoints.size(); k++)
    if( (allpoints[me].calcdist(allpoints[k])<=eps) && (k!=me) )
      neigh.push_back(k);
  return neigh;
}

float move(vector<point>& allpoints, int me, float sigmas, bool weighted) {
  point n(0,0,0,0);
  float d=0, disp=0;
  for (int j=0; j<allpoints.size(); j++)
    if(j!=me) {
      float k=0;
      if ( (allpoints[me].calcdist(allpoints[j])) > 2*sigmas)
        k = 0;
      else {
        float x = allpoints[me].getx()-allpoints[j].getx();
        float y = allpoints[me].gety()-allpoints[j].gety();
        k = exp(-(x*x+y*y)/(sigmas*sigmas));
      }
      if(weighted == true)
        k = k * (allpoints[j]).getduration();

      n.setx( n.getx() + k*allpoints[j].getx() );
      n.sety( n.gety() + k*allpoints[j].gety() );
      d += k;
    }
  if (d!=0) {
    n.setx( n.getx() / d );
    n.sety( n.gety() / d );
    disp=allpoints[me].calcdist(n);
    allpoints[me].setx( n.getx() );
    allpoints[me].sety( n.gety() );
  }
  return disp;
}

vector<point> meanshift(vector<point> points, float sigmas) {
  vector<point> shifts;
  for (int s=0; s<points.size(); s++) {
    points[s].setclusterID(0);
    shifts.push_back(points[s]);
  }
  //Beginning of clustering process
  //1. preprocess moving the points
  int num=0;
  float max = 0;
  do {
    max = 0;
    num++;
    for (int k=0; k<shifts.size(); k++) {
      float disp;
      if(shifts[k].getduration() > 0 )
        disp = move(shifts,k,sigmas,true);
      else
        disp = move(shifts,k,sigmas,false);
      if (disp>max) max=disp;
    }
  }
  while (max>sigmas/3 && num<20);
  //2. apply a clustering algorithm that employs a distance threshold
  int curr_cl_label = 0;
  for (int c=0; c<points.size(); c++)  {
    vector<int> neig = getneig_ids(sigmas,shifts,c);
    if (neig.size()==0) //or < minimimum_dimension_of_cluster
      points[c].setclusterID(++curr_cl_label);  // so that 1 point clusters are also acceptable
    //points[c].setclusterID(0);
    else
      if (points[c].getclusterID()==0) {
        points[c].setclusterID(++curr_cl_label);
        for (int n=0; n<neig.size(); n++)
          if (points[neig[n]].getclusterID()==0)
            points[neig[n]].setclusterID(curr_cl_label);

        while (neig.empty()==false) {
          int currentP = neig.back();
          neig.pop_back();
          vector<int> result = getneig_ids(sigmas,shifts,currentP);
          for (int i=0; i<result.size(); i++)
            if (points[result[i]].getclusterID()==0) {
              neig.push_back(result[i]);
              points[result[i]].setclusterID(curr_cl_label);
            }
        }
      }
  }
  return points;
}

vector<cluster> makeclusters(vector< vector<point> > p) {
  vector<point> tot;
  for(int i=0; i<p.size(); i++)
    for(int j=0; j<p[i].size(); j++)
      tot.push_back(p[i][j]);
  return makeclusters(tot);
}
