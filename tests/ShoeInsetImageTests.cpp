#include <opencv2/opencv.hpp>
#include <iostream>
#include "../UAX/UAXTypes.h"
#include "../UAX/ShoeInsetMask.h"
extern "C" __declspec(dllimport) void GetToolPath_Optimized_Mask(cv::Mat&,const cv::Mat&,double,double,ToolPath&,int,int);
int main(){
 auto image=cv::imread("DOC/Images/1005Debug.png",cv::IMREAD_GRAYSCALE);
 cv::Mat roi=cv::Mat::zeros(image.size(),CV_8UC1); roi(cv::Rect(400,110,650,950)).setTo(255);
 cv::Mat binary; cv::inRange(image,0,127,binary);
 cv::Mat canvas; cv::cvtColor(image,canvas,cv::COLOR_GRAY2BGR);
 for(int offset: {0,51}) {
  auto mask=BuildShoeInsetMask(binary,roi,offset);
  std::vector<std::vector<cv::Point>> cs; cv::findContours(mask,cs,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
  cv::drawContours(canvas,cs,-1,offset?cv::Scalar(0,255,0):cv::Scalar(0,0,255),2);
  ToolPath path; auto input=image.clone();
  GetToolPath_Optimized_Mask(input,roi,offset,695+5/.234856,path,127,0);
  int outside=0; auto original=BuildShoeInsetMask(binary,roi,0);
  for(auto pt:path.Path){if(original.at<uchar>(cvRound(pt.y),cvRound(pt.x))==0) ++outside; cv::circle(canvas,pt,4,offset?cv::Scalar(0,255,255):cv::Scalar(255,0,0),-1);}
  std::cout<<"offset="<<offset<<" mask_area="<<cv::countNonZero(mask)<<" components="<<cs.size()<<" sampled="<<path.Path.size()<<" outside="<<outside<<"\n";
  if(offset && (outside != 0 || path.Path.size()!=50)) throw std::runtime_error("Inset points escaped original shoe");
  cv::imwrite("tmp/inset-tests/mask"+std::to_string(offset)+".png",mask);
 }
 cv::rectangle(canvas,cv::Rect(400,110,650,950),cv::Scalar(255,255,0),2);
 cv::imwrite("tmp/inset-tests/1005_result.png",canvas);
}

