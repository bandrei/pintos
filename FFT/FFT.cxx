/*
 * FFT.cxx
 *
 *  Created on: Feb 18, 2011
 *      Author: dr
 */

// Global includes
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <complex>

// Local includes
#include "FFT.h"
#include <math.h>
using namespace std;

Image<double> FFT::Magnitude(Image<complexd> input)
{
	Image<double> dbImg(input.numRows(),input.numCols());
	for(int i = 0; i< input.numRows(); i++)
		for(int j= 0 ; j< input.numCols();j++)
		{
			double pixMag = abs(input.getData(i,j));
			dbImg.setData(i,j,pixMag);
		}
	return dbImg;
}

void FFT::Filter(Image<complexd> &image, int type, double maxcut, double mincut)
{
  if (type == 1) cout << " low-pass ..." << endl;
  else if (type == 2) cout << " high-pass ..." << endl;
  else if (type == 3) cout << " band-pass ..." << endl;
}

Image<complexd> FFT::ForwardTransform(Image<double> image)
{
	//Image<complexd> compImg(image.numRows(),image.numCols());
	/*for(int i = 0; i< image.numRows(); i++)
		for(int j= 0 ; j< image.numCols();j++)
		{
			complexd pixMag(image.getData(i,j),0);
			compImg.setData(i,j,pixMag);
		}
	*/
	//Run_2D(compImg, 1);


cout<<"got here";
}

Image<double> FFT::ReverseTransform(Image<complexd> image)
{
	Run_2D(image, -1);
}

void FFT::Run_2D(Image<complexd> &image, int dir)
{

   for(int i = 0 ; i< image.numRows(); i++)
   {
	image.fillRow(i,Run_1D_Recursive(
				image.extractRow(i),image.numCols(),dir));
   }

   for(int i = 0; i<image.numCols(); i++)
   {
	image.fillCol(i,Run_1D_Recursive(
			image.extractCol(i),image.numRows(),dir));
   }
  // dir = +1 for forward transform
  // dir = -1 for reverse transform
}

vector<complexd> FFT::Run_1D_Recursive(const vector<complexd> &v, int n, int dir)
{
	if(n==1 || n==0) return v;
	complexd wn(0,2*M_PI/n);
	wn = pow(exp(wn),dir);
	complexd w(1,0);
	vector<complexd> a0;
	vector<complexd> a1;
	vector<complexd> y0;
	vector<complexd> y1;
	vector<complexd> y(n,complexd(0,0));
	for(unsigned int i = 0; i < v.size(); i++)
	{
		if(i%2==0)
		{
		  a0.push_back(v[i]);
		}
		else
		{
		  a1.push_back(v[i]);
		}
	} 
	y0 = Run_1D_Recursive(a0, n/2, dir);
	y1 = Run_1D_Recursive(a1, n/2, dir);
	for(int k =0 ; k< n/2; k++)
	{
		complexd t = w * y1[k];
		double div = (dir==-1) ? 2.0 : 1;
		y[k] = (y0[k] + t)/div;
		y[k+(n/2)] = (y0[k] - t)/div;
		w = w * wn;
	}

	return y;
}









