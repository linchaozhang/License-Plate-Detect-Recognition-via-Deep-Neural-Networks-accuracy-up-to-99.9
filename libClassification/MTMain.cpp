// caffe
#include <caffe/caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>

// c++
#include <string>
#include <vector>
#include <fstream>
#include <direct.h>
// opencv
#include <opencv2/opencv.hpp>
// boost
#include "boost/make_shared.hpp"
//#include "classifierCaffe.h"

#include "debugTime.h"

#include "tform_fcn.h"

#include "caffe-gpu.h"

//#define CPU_ONLY
#define INTER_FAST
using namespace caffe;
using namespace cv;
using namespace std;




///////////////////////////////
extern char* trainSetPosPath;
extern char* templateName;
extern int frame_width;
extern int frame_height;
extern char *model_file;		//model_file = "LightenedCNN_C_deploy.prototxt";
extern char *trained_file;	//trained_file = "LightenedCNN_C.caffemodel";
extern int label_file;

//extern char * configFile;
//extern vector<string> imgNames;
//extern vector<string> imgLists;
//extern vector<int> imgLabels;
//extern int labelTemp;
//extern void initTrainImage();
///////////////////////////////
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> /*用到了time函数，所以要有这个头文件*/
#include <fstream>
#include <sstream>
#include <exception>
#include <vector>
#include <io.h>

//#include <highgui.h> 
//#include <cv.h>

#define showSteps 0

using namespace std;
char * configFile = "config.txt";

//读取config文件里的内容--
char* trainSetPosPath = (char *)malloc(200 * sizeof(char));
char* templateName = (char *)malloc(200 * sizeof(char));
int frame_width = 640;
int frame_height = 480;
char *model_file = (char *)malloc(200 * sizeof(char));		//model_file = "LightenedCNN_C_deploy.prototxt";
char *trained_file = (char *)malloc(200 * sizeof(char));	//trained_file = "LightenedCNN_C.caffemodel";
int label_file = 256;
void readConfig(char* configFile, char* trainSetPosPath) {
	fstream f;
	char cstring[1000];
	int readS = 0;
	f.open(configFile, fstream::in);
	char param1[200]; strcpy(param1, "");
	char param2[200]; strcpy(param2, "");
	char param3[200]; strcpy(param3, "");

	//--读取第一行：--
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %s", param1, param2, param3);
	strcpy(trainSetPosPath, param3);

	//--读取第2行：-- 对比人脸
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %s", param1, param2, param3);
	strcpy(templateName, param3);

	//--读取第3行：-- 相机宽
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %d", param1, param2, &frame_width);

	//--读取第4行：-- 相机高
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %d", param1, param2, &frame_height);

	//--读取第5行：-- 训练模型
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %s", param1, param2, param3);
	strcpy(model_file, param3);

	//--读取第6行：-- 训练权重
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %s", param1, param2, param3);
	strcpy(trained_file, param3);

	//--读取第6行：-- 特征数量
	f.getline(cstring, sizeof(cstring));
	readS = sscanf(cstring, "%s %s %d", param1, param2, &label_file);
}

//遍历config.txt里的根目录下的所有的文件，包括子目录。--
// 其中子目录的名字就是label，子目录里的文件为label对于的训练测试样本---
vector<string> imgNames;
vector<string> imgLists;
vector<int> imgLabels;
int labelTemp = 0;

void dfsFolder(string folderPath) {
	_finddata_t FileInfo;
	string strfind = folderPath + "\\*";
	long long Handle = _findfirst(strfind.c_str(), &FileInfo);
	if (Handle == -1L)
	{
		cerr << "can not match the folder path" << endl;
		exit(-1);
	}
	do {
		//判断是否有子目录--
		if (FileInfo.attrib & _A_SUBDIR) {
			//	cout<<FileInfo.name<<" "<<FileInfo.attrib<<endl;
			//这个语句很重要--
			if ((strcmp(FileInfo.name, ".") != 0) && (strcmp(FileInfo.name, "..") != 0)) {
				string newPath = folderPath + "\\" + FileInfo.name;
				cout << FileInfo.name << " " << newPath << endl;
				//根目录下下的子目录名字就是label名，如果没有子目录则其为根目录下
				labelTemp = atoi(FileInfo.name);
				//	printf("%d\n",labelTemp);
				dfsFolder(newPath);
			}
		}
		else {
			string finalName = folderPath + "\\" + FileInfo.name;
			//将所有的文件名写入一个txt文件--
			//	cout << FileInfo.name << "\t";
			//	printf("%d\t",label);
			//	cout << folderPath << "\\" << FileInfo.name  << " " <<endl;
			//将文件名字和label名字（子目录名字赋值给向量）--
			imgLabels.push_back(labelTemp);
			imgNames.push_back(finalName);


			std::stringstream ss;
			std::string str;
			ss << labelTemp;
			ss >> str;

			string finalList = FileInfo.name;
			imgLists.push_back(finalList);

		}
	} while (_findnext(Handle, &FileInfo) == 0);
	_findclose(Handle);

}

void initTrainImage() {
	readConfig(configFile, trainSetPosPath);

	string folderPath = trainSetPosPath;
	//	string folderPath = "H:\\char\\poker_rec_char_equalist_test";
	//	string folderPath = "C:\\planeSample\\recSampleData\\rec";
	//	string folderPath = "C:\\Users\\Administrator\\Desktop\\LPR\\hu\\";
	dfsFolder(folderPath);

}
////////////////////////////////////////////





// compare score
bool CompareBBox(const FaceInfo & a, const FaceInfo & b) {
  return a.bbox.score > b.bbox.score;
}

// methodType : u is IoU(Intersection Over Union)
// methodType : m is IoM(Intersection Over Maximum)
std::vector<FaceInfo> MTCNN::NonMaximumSuppression(std::vector<FaceInfo>& bboxes,
                float thresh,char methodType){
  std::vector<FaceInfo> bboxes_nms;
  std::sort(bboxes.begin(), bboxes.end(), CompareBBox);

  int32_t select_idx = 0;
  int32_t num_bbox = static_cast<int32_t>(bboxes.size());
  std::vector<int32_t> mask_merged(num_bbox, 0);
  bool all_merged = false;

  while (!all_merged) {
    while (select_idx < num_bbox && mask_merged[select_idx] == 1)
      select_idx++;
    if (select_idx == num_bbox) {
      all_merged = true;
      continue;
    }

    bboxes_nms.push_back(bboxes[select_idx]);
    mask_merged[select_idx] = 1;

    FaceRect select_bbox = bboxes[select_idx].bbox;
    float area1 = static_cast<float>((select_bbox.x2-select_bbox.x1+1) * (select_bbox.y2-select_bbox.y1+1));
    float x1 = static_cast<float>(select_bbox.x1);
    float y1 = static_cast<float>(select_bbox.y1);
    float x2 = static_cast<float>(select_bbox.x2);
    float y2 = static_cast<float>(select_bbox.y2);

    select_idx++;
    for (int32_t i = select_idx; i < num_bbox; i++) {
      if (mask_merged[i] == 1)
        continue;

      FaceRect& bbox_i = bboxes[i].bbox;
      float x = std::max<float>(x1, static_cast<float>(bbox_i.x1));
      float y = std::max<float>(y1, static_cast<float>(bbox_i.y1));
      float w = std::min<float>(x2, static_cast<float>(bbox_i.x2)) - x + 1;
      float h = std::min<float>(y2, static_cast<float>(bbox_i.y2)) - y + 1;
      if (w <= 0 || h <= 0)
        continue;

      float area2 = static_cast<float>((bbox_i.x2-bbox_i.x1+1) * (bbox_i.y2-bbox_i.y1+1));
      float area_intersect = w * h;

      switch (methodType) {
        case 'u':
          if (static_cast<float>(area_intersect) / (area1 + area2 - area_intersect) > thresh)
            mask_merged[i] = 1;
          break;
        case 'm':
          if (static_cast<float>(area_intersect) / min(area1 , area2) > thresh)
            mask_merged[i] = 1;
          break;
        default:
          break;
        }
    }
  }
  return bboxes_nms;
}

void MTCNN::Bbox2Square(std::vector<FaceInfo>& bboxes){
  for(int i=0;i<bboxes.size();i++){
    float w = bboxes[i].bbox.x2 - bboxes[i].bbox.x1;
    float h = bboxes[i].bbox.y2 - bboxes[i].bbox.y1;
    float side_w = h>w ? h:w;
	float side_h = 12 / 30.0 * side_w;
	bboxes[i].bbox.x1 += (w - side_w)*0.5;
	bboxes[i].bbox.y1 += (h - side_h)*0.5;

	bboxes[i].bbox.x2 = (int)(bboxes[i].bbox.x1 + side_w);
	bboxes[i].bbox.y2 = (int)(bboxes[i].bbox.y1 + side_h);
    bboxes[i].bbox.x1 = (int)(bboxes[i].bbox.x1);
    bboxes[i].bbox.y1 = (int)(bboxes[i].bbox.y1);

  }
}

std::vector<FaceInfo> MTCNN::BoxRegress(std::vector<FaceInfo>& faceInfo,int stage){
  std::vector<FaceInfo> bboxes;
  for(int bboxId =0;bboxId<faceInfo.size();bboxId++){
      FaceRect faceRect;
      FaceInfo tempFaceInfo;
      float regh = faceInfo[bboxId].bbox.y2 - faceInfo[bboxId].bbox.y1;
      regh += (stage == 1)? 0:1;
      float regw = faceInfo[bboxId].bbox.x2 - faceInfo[bboxId].bbox.x1;
      regw += (stage == 1)? 0:1;
			faceRect.y1 = faceInfo[bboxId].bbox.y1 + regh * faceInfo[bboxId].regression[1];
			faceRect.x1 = faceInfo[bboxId].bbox.x1 + regw * faceInfo[bboxId].regression[0];
			faceRect.y2 = faceInfo[bboxId].bbox.y1 + regh * faceInfo[bboxId].regression[3];
			faceRect.x2 = faceInfo[bboxId].bbox.x1 + regw * faceInfo[bboxId].regression[2];
      faceRect.score = faceInfo[bboxId].bbox.score;

      tempFaceInfo.bbox = faceRect;
      tempFaceInfo.regression = faceInfo[bboxId].regression;
      if(stage == 3)
        tempFaceInfo.facePts = faceInfo[bboxId].facePts;
      bboxes.push_back(tempFaceInfo);
  }
  return bboxes;
}

// compute the padding coordinates (pad the bounding boxes to square)
void MTCNN::Padding(int img_w,int img_h){
  for(int i=0;i<regressed_rects_.size();i++){
    FaceInfo tempFaceInfo;
    tempFaceInfo = regressed_rects_[i];
	tempFaceInfo.bbox.y2 = (regressed_rects_[i].bbox.y2 >= img_h) ? img_h : regressed_rects_[i].bbox.y2;
	tempFaceInfo.bbox.x2 = (regressed_rects_[i].bbox.x2 >= img_w) ? img_w : regressed_rects_[i].bbox.x2;
    tempFaceInfo.bbox.y1 = (regressed_rects_[i].bbox.y1 <1) ? 1 : regressed_rects_[i].bbox.y1;
    tempFaceInfo.bbox.x1 = (regressed_rects_[i].bbox.x1 <1) ? 1 : regressed_rects_[i].bbox.x1;
    regressed_pading_.push_back(tempFaceInfo);
  }
}

void MTCNN::GenerateBoundingBox(Blob<float>* confidence,Blob<float>* reg,
      float scale,float thresh,int image_width,int image_height){
  int stride_w = 20;
  int stride_h = 2;
  int cellSize_w = 30;
  int cellSize_h = 12;

  int stride = 2;
  int cellSize = 12;


  int curr_feature_map_w_ = (std::ceil((image_width - cellSize)*1.0 / stride) + 1) - 10 + 1;
  int curr_feature_map_h_ = std::ceil((image_height - cellSize)*1.0 / stride) + 1;

  //std::cout << "Feature_map_size:"<< curr_feature_map_w_ <<" "<<curr_feature_map_h_<<std::endl;
  int regOffset = curr_feature_map_w_*curr_feature_map_h_;
  // the first count numbers are confidence of face
  int count = confidence->count()/2;
  const float* confidence_data = confidence->cpu_data();
  //for (int i = 0; i < confidence->count(); i++){
	 // if (confidence_data[i] >= thresh)
	 // cout << i <<"-"<< confidence_data[i] << " " ;
  //}


  confidence_data += count;
  const float* reg_data = reg->cpu_data();

  condidate_rects_.clear();

  //cv::Mat image =  imread("imgs\\001.jpg");

  for(int i=0;i<count;i++){
    if(*(confidence_data+i)>=thresh){
      int y = i / curr_feature_map_w_;
      int x = i - curr_feature_map_w_ * y;

	  float xTop = (int)((x*stride + 1) / scale);
	  float yTop = (int)((y*stride + 1) / scale);
	  float xBot = (int)((x*stride + cellSize_w - 1 + 1) / scale);
	  float yBot = (int)((y*stride + cellSize_h - 1 + 1) / scale);

	 // cout << xTop << " " << yTop << " " << xBot << " " << yBot << endl;


	 /* float xx = xTop;
	  float yy = yTop;
	  float w = xBot - xTop;
	  float h = yBot - yTop;

	  if (xx < 0) xx = 0;	if (yy < 0) yy = 0;
	  if ((yy + h) > image.rows) yy = image.rows - h;
	  if ((xx + w) > image.cols) xx = image.cols - w;
	  cv::rectangle(image, cv::Rect(xx, yy, w, h), cv::Scalar(255, 0, 0), 2);
	  cv::imshow("Extracted frame", image);*/
	 // int c = waitKey(0);

      FaceRect faceRect;
      faceRect.x1 = xTop;
      faceRect.y1 = yTop;
      faceRect.x2 = xBot;
      faceRect.y2 = yBot;
       faceRect.score  = *(confidence_data+i);
      FaceInfo faceInfo;
      faceInfo.bbox = faceRect;
      faceInfo.regression = cv::Vec4f(reg_data[i+0*regOffset],reg_data[i+1*regOffset],reg_data[i+2*regOffset],reg_data[i+3*regOffset]);
      condidate_rects_.push_back(faceInfo);
    }
  }
}

MTCNN::MTCNN(const std::string &proto_model_dir){
#ifdef CPU_ONLY
  Caffe::set_mode(Caffe::CPU);
#else
  Caffe::set_mode(Caffe::GPU);
#endif
  /* Load the network. */
  PNet_.reset(new Net<float>((proto_model_dir+"/det1.prototxt"), TEST));
  PNet_->CopyTrainedLayersFrom(proto_model_dir+"/det1.caffemodel");

  CHECK_EQ(PNet_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(PNet_->num_outputs(),2) << "Network should have exactly two output, one"
                                     " is bbox and another is confidence.";

  #ifdef CPU_ONLY
  RNet_.reset(new Net<float>((proto_model_dir+"/det2.prototxt"), TEST));
  #else
  RNet_.reset(new Net<float>((proto_model_dir+"/det2_input.prototxt"), TEST));
  #endif
  RNet_->CopyTrainedLayersFrom(proto_model_dir+"/det2.caffemodel");

//  CHECK_EQ(RNet_->num_inputs(), 0) << "Network should have exactly one input.";
//  CHECK_EQ(RNet_->num_outputs(),3) << "Network should have exactly two output, one"
//                                     " is bbox and another is confidence.";

  #ifdef CPU_ONLY
  ONet_.reset(new Net<float>((proto_model_dir+"/det3.prototxt"), TEST));
  #else
  ONet_.reset(new Net<float>((proto_model_dir+"/det3_input.prototxt"), TEST));
  #endif
  ONet_->CopyTrainedLayersFrom(proto_model_dir+"/det3.caffemodel");

//  CHECK_EQ(ONet_->num_inputs(), 1) << "Network should have exactly one input.";
//  CHECK_EQ(ONet_->num_outputs(),3) << "Network should have exactly three output, one"
//                                     " is bbox and another is confidence.";

  Blob<float>* input_layer;
  input_layer = PNet_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1) << "Input layer should have 1 or 3 channels.";
}

void MTCNN::WrapInputLayer(std::vector<cv::Mat>* input_channels,
        Blob<float>* input_layer, const int height, const int width) {
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
}

void MTCNN::ClassifyFace(const std::vector<FaceInfo>& regressed_rects,cv::Mat &sample_single,
        boost::shared_ptr<Net<float> >& net,double thresh,char netName){
  int numBox = regressed_rects.size();
  Blob<float>* crop_input_layer = net->input_blobs()[0];
  int input_channels = crop_input_layer->channels();
  int input_width  = crop_input_layer->width();
  int input_height = crop_input_layer->height();
  crop_input_layer->Reshape(1, input_channels, input_height,input_width );
  net->Reshape();

  condidate_rects_.clear();
  // load crop_img data to datum
  for(int i=0;i<numBox;i++){
    std::vector<cv::Mat> channels;
    WrapInputLayer(&channels,net->input_blobs()[0],input_height,input_width);
    int pad_left   = std::abs(regressed_pading_[i].bbox.x1 - regressed_rects[i].bbox.x1);
    int pad_top  = std::abs(regressed_pading_[i].bbox.y1 - regressed_rects[i].bbox.y1);
    int pad_bottom = std::abs(regressed_pading_[i].bbox.y2 - regressed_rects[i].bbox.y2);
    int pad_right= std::abs(regressed_pading_[i].bbox.x2 - regressed_rects[i].bbox.x2);

	//cout << "Range0: " << sample_single.cols << " " << sample_single.rows << endl;
	//cout << "Range: " <<i<<" "<< regressed_pading_[i].bbox.y1 - 1 << " " << regressed_pading_[i].bbox.y2 - 1 << " "
	//	<< regressed_pading_[i].bbox.x1 - 1 << " " << regressed_pading_[i].bbox.x2 - 1 << endl;

	/*if ((regressed_pading_[i].bbox.y1 - 1 <= 0) || (regressed_pading_[i].bbox.y2 - 1 <= 0) ||
		(regressed_pading_[i].bbox.x1 - 1 <= 0) || (regressed_pading_[i].bbox.x2 - 1 <= 0) ||
		(regressed_pading_[i].bbox.x1  > regressed_pading_[i].bbox.x2 -1 ) ||
		(regressed_pading_[i].bbox.y1  > regressed_pading_[i].bbox.y2 -1 )
		
		)
		continue;*/
    cv::Mat crop_img = sample_single(cv::Range(regressed_pading_[i].bbox.y1-1,regressed_pading_[i].bbox.y2-1),
                         cv::Range(regressed_pading_[i].bbox.x1-1,regressed_pading_[i].bbox.x2-1));
			cv::copyMakeBorder(crop_img, crop_img,pad_top, pad_bottom,  pad_left, pad_right, cv::BORDER_CONSTANT, cv::Scalar(0));
#ifdef INTER_FAST
    cv::resize(crop_img,crop_img,cv::Size(input_width,input_height),0,0,cv::INTER_NEAREST);
#else
    cv::resize(crop_img,crop_img,cv::Size(input_width,input_height),0,0,cv::INTER_AREA);
#endif
    crop_img = (crop_img-127.5)*0.0078125;
    cv::split(crop_img,channels);

    CHECK(reinterpret_cast<float*>(channels.at(0).data) == net->input_blobs()[0]->cpu_data())
          << "Input channels are not wrapping the input layer of the network.";
    /*net->Forward();*/
	net->ForwardPrefilled(); 

    int reg_id = 0;
    int confidence_id = 1;
	if (netName == 'o')
		confidence_id = 2;
    const Blob<float>* reg = net->output_blobs()[reg_id];
    const Blob<float>* confidence = net->output_blobs()[confidence_id];
	//ONet_ points_offset != NULL
    const Blob<float>* points_offset = net->output_blobs()[1];

	
    const float* confidence_data = confidence->cpu_data() + confidence->count()/2;
    const float* reg_data = reg->cpu_data();
	const float* points_data = { 0 };
	if (netName == 'o') points_data = points_offset->cpu_data();

	/*if (netName == 'o'){
		cout << "***** :" << *(confidence->cpu_data()) << " " << *(confidence->cpu_data() + 1)  << endl;
			cout << "------- :" << (confidence->count()) << endl;

			cout << "############:" << confidence->cpu_data() + confidence->count() / 2 << endl;
			cout << "#######thresh#####:" << thresh << endl;

			for (int j = 0; j < 8; j++){
				cout <<  *(points_data + j) << " " ;
			}



		}*/

    if(*(confidence_data) > thresh){
      FaceRect faceRect;
      faceRect.x1 = regressed_rects[i].bbox.x1;
      faceRect.y1 = regressed_rects[i].bbox.y1;
      faceRect.x2 = regressed_rects[i].bbox.x2;
      faceRect.y2 = regressed_rects[i].bbox.y2 ;
      faceRect.score  = *(confidence_data);
      FaceInfo faceInfo;
      faceInfo.bbox = faceRect;
      faceInfo.regression = cv::Vec4f(reg_data[0],reg_data[1],reg_data[2],reg_data[3]);

      // x y x y x y x y 
      if(netName == 'o'){
        FacePts face_pts;
        float h = faceRect.y2 - faceRect.y1 + 1;
        float w = faceRect.x2 - faceRect.x1 + 1;
        for(int j=0;j<4;j++){
		  face_pts.x[j] = faceRect.x1 + *(points_data + 2 * j) * w - 1;
          face_pts.y[j] = faceRect.y1 + *(points_data+2*j+1) * h - 1;      
        }
        faceInfo.facePts = face_pts;
      }
      condidate_rects_.push_back(faceInfo);
    }
  }
  regressed_pading_.clear();
}

// multi test image pass a forward
void MTCNN::ClassifyFace_MulImage(const std::vector<FaceInfo>& regressed_rects, cv::Mat &sample_single,
	boost::shared_ptr<Net<float> >& net, double thresh, char netName) {
	condidate_rects_.clear();

	int numBox = regressed_rects.size();
	std::vector<Datum> datum_vector;

	boost::shared_ptr<MemoryDataLayer<float> > mem_data_layer;
	mem_data_layer = boost::static_pointer_cast<MemoryDataLayer<float> >(net->layers()[0]);
	int input_width = mem_data_layer->width();
	int input_height = mem_data_layer->height();

	// load crop_img data to datum
	for (int i = 0; i<numBox; i++) {
		int pad_left = std::abs(regressed_pading_[i].bbox.x1 - regressed_rects[i].bbox.x1);
		int pad_top = std::abs(regressed_pading_[i].bbox.y1 - regressed_rects[i].bbox.y1);
		int pad_bottom = std::abs(regressed_pading_[i].bbox.y2 - regressed_rects[i].bbox.y2);
		int pad_right = std::abs(regressed_pading_[i].bbox.x2 - regressed_rects[i].bbox.x2);

		cv::Mat crop_img = sample_single(cv::Range(regressed_pading_[i].bbox.y1 - 1, regressed_pading_[i].bbox.y2 - 1),
			cv::Range(regressed_pading_[i].bbox.x1 - 1, regressed_pading_[i].bbox.x2 - 1));
		cv::copyMakeBorder(crop_img, crop_img, pad_top, pad_bottom, pad_left, pad_right, cv::BORDER_CONSTANT, cv::Scalar(0));

#ifdef INTER_FAST
		cv::resize(crop_img, crop_img, cv::Size(input_width, input_height), 0, 0, cv::INTER_NEAREST);
#else
		cv::resize(crop_img, crop_img, cv::Size(input_width, input_height), 0, 0, cv::INTER_AREA);
#endif
		crop_img = (crop_img - 127.5)*0.0078125;
		Datum datum;
		CvMatToDatumSignalChannel(crop_img, &datum);
		datum_vector.push_back(datum);
	}
	regressed_pading_.clear();

	/* extract the features and store */
	mem_data_layer->set_batch_size(numBox);
	mem_data_layer->AddDatumVector(datum_vector);
	/* fire the network */
	float no_use_loss = 0;
	// net->Forward(&no_use_loss);
	net->ForwardPrefilled(&no_use_loss);
	//  CHECK(reinterpret_cast<float*>(crop_img_set.at(0).data) == net->input_blobs()[0]->cpu_data())
	//          << "Input channels are not wrapping the input layer of the network.";

	// return RNet/ONet result
	std::string outPutLayerName = (netName == 'r' ? "conv5-2" : "conv6-2");
	std::string pointsLayerName = "conv6-3";

	const boost::shared_ptr<Blob<float> > reg = net->blob_by_name(outPutLayerName);
	const boost::shared_ptr<Blob<float> > confidence = net->blob_by_name("prob1");
	// ONet points_offset != NULL
	const boost::shared_ptr<Blob<float> > points_offset = net->blob_by_name(pointsLayerName);

	const float* confidence_data = confidence->cpu_data();
	const float* reg_data = reg->cpu_data();
	const float* points_data;
	if (netName == 'o') points_data = points_offset->cpu_data();

	for (int i = 0; i<numBox; i++) {
		if (*(confidence_data + i * 2 + 1) > thresh) {
			FaceRect faceRect;
			faceRect.x1 = regressed_rects[i].bbox.x1;
			faceRect.y1 = regressed_rects[i].bbox.y1;
			faceRect.x2 = regressed_rects[i].bbox.x2;
			faceRect.y2 = regressed_rects[i].bbox.y2;
			faceRect.score = *(confidence_data + i * 2 + 1);
			FaceInfo faceInfo;
			faceInfo.bbox = faceRect;
			faceInfo.regression = cv::Vec4f(reg_data[4 * i + 0], reg_data[4 * i + 1], reg_data[4 * i + 2], reg_data[4 * i + 3]);

			//x y x y x y x y 
			if (netName == 'o') {
				FacePts face_pts;
				float h = faceRect.y2 - faceRect.y1 + 1;
				float w = faceRect.x2 - faceRect.x1 + 1;
				for (int j = 0; j<4; j++) {
					face_pts.x[j] = faceRect.x1 + *(points_data + (2*j) + 8 * i) * w - 1;
					face_pts.y[j] = faceRect.y1 + *(points_data + (2*j + 1) + 8 * i) * h - 1;				
				}
				faceInfo.facePts = face_pts;
			}
			condidate_rects_.push_back(faceInfo);
		}
	}
}


bool MTCNN::CvMatToDatumSignalChannel(const cv::Mat& cv_mat, Datum* datum){
  if (cv_mat.empty())
    return false;
  int channels = cv_mat.channels();

  datum->set_channels(cv_mat.channels());
  datum->set_height(cv_mat.rows);
  datum->set_width(cv_mat.cols);
  //datum->set_label(0);
  datum->clear_label();
  datum->clear_data();
  datum->clear_float_data();
  datum->set_encoded(false);

  int datum_height = datum->height();
  int datum_width  = datum->width();
  if(channels == 3){
    for(int c = 0;c < channels;c++){
      for (int h = 0; h < datum_height; ++h){
        for (int w = 0; w < datum_width; ++w){
          const float* ptr = cv_mat.ptr<float>(h);
          datum->add_float_data(ptr[w*channels+c]);
        }
      }
    }
  }

  return true;
}


void MTCNN::Detect(const cv::Mat& image,std::vector<FaceInfo>& faceInfo,int minSize,double* threshold,double factor){

	getStartTime();
  // 2~3ms
  // invert to RGB color space and float type
  cv::Mat sample_single,resized;
  image.convertTo(sample_single,CV_32FC3);
//		cv::cvtColor(sample_single, sample_single, cv::COLOR_BGR2RGB); MATLAB版本
//  sample_single = sample_single.t();

  int height = image.rows;
  int width  = image.cols;
  int minWH = min(height,width);
  int factor_count = 0;
  double m = 12.0/minSize;
  minWH *= m;
  std::vector<double> scales;
  while (minWH >= 30)
  {
    scales.push_back(m * std::pow(factor,factor_count));
    minWH *= factor;
    ++factor_count;
  }

  // 11ms main consum
  Blob<float>* input_layer = PNet_->input_blobs()[0];
  for(int i=0;i<factor_count;i++)
  {
    double scale = scales[i];
			int ws = std::ceil(width*scale);
			int hs = std::ceil(height*scale);

		

		//	cout << "**********:" << ws << " " << hs << endl;
    // wrap image and normalization using INTER_AREA method
#ifdef INTER_FAST
    cv::resize(sample_single,resized,cv::Size(ws,hs),0,0,cv::INTER_NEAREST);
#else
    cv::resize(sample_single,resized,cv::Size(ws,hs),0,0,cv::INTER_AREA);
#endif
    resized.convertTo(resized, CV_32FC3, 0.0078125,-127.5*0.0078125);

    // input data
    input_layer->Reshape(1, 3, hs, ws);
    PNet_->Reshape();
    std::vector<cv::Mat> input_channels;
    WrapInputLayer(&input_channels,PNet_->input_blobs()[0],hs,ws);
    cv::split(resized,input_channels);

    // check data transform right
    CHECK(reinterpret_cast<float*>(input_channels.at(0).data) == PNet_->input_blobs()[0]->cpu_data())
        << "Input channels are not wrapping the input layer of the network.";
    //PNet_->Forward();
	PNet_->ForwardPrefilled();

    // return result
    Blob<float>* reg = PNet_->output_blobs()[0];
    //const float* reg_data = reg->cpu_data();
    Blob<float>* confidence = PNet_->output_blobs()[1];
    GenerateBoundingBox(confidence, reg, scale, threshold[0],ws,hs);
    std::vector<FaceInfo> bboxes_nms = NonMaximumSuppression(condidate_rects_,0.5,'u');
    total_boxes_.insert(total_boxes_.end(),bboxes_nms.begin(),bboxes_nms.end());
  }

  int numBox = total_boxes_.size();
  if (numBox != 0){
	  total_boxes_ = NonMaximumSuppression(total_boxes_, 0.7, 'u');
	  regressed_rects_ = BoxRegress(total_boxes_, 1);
	  total_boxes_.clear();

	  Bbox2Square(regressed_rects_);
	  Padding(width, height);

	  //cv::Mat image = imread("imgs\\001.jpg");
	  for (int i = 0; i < regressed_rects_.size(); i++){
		  int pad_left = std::abs(regressed_pading_[i].bbox.x1 - regressed_rects_[i].bbox.x1);
		  int pad_top = std::abs(regressed_pading_[i].bbox.y1 - regressed_rects_[i].bbox.y1);
		  int pad_bottom = std::abs(regressed_pading_[i].bbox.y2 - regressed_rects_[i].bbox.y2);
		  int pad_right = std::abs(regressed_pading_[i].bbox.x2 - regressed_rects_[i].bbox.x2);

		  //cout << "Range0" << sample_single.cols << " " << sample_single.rows << endl;
		  //cout << "Range" << regressed_pading_[i].bbox.y1 - 1 << " " << regressed_pading_[i].bbox.y2 - 1 << " "
			 //  << regressed_pading_[i].bbox.x1 - 1 << " " << regressed_pading_[i].bbox.x2 - 1 << endl;

		  //float xx = regressed_pading_[i].bbox.x1;
		  //float yy = regressed_pading_[i].bbox.y1;
		  //float w = regressed_pading_[i].bbox.x2 - regressed_pading_[i].bbox.x1;
		  //float h = regressed_pading_[i].bbox.y2 - regressed_pading_[i].bbox.y1;

		  //if (xx < 0) xx = 0;	if (yy < 0) yy = 0;
		  //if ((yy + h) > image.rows) yy = image.rows - h;
		  //if ((xx + w) > image.cols) xx = image.cols - w;
		  //cv::rectangle(image, cv::Rect(xx, yy, w, h), cv::Scalar(255, 0, 0), 2);
		  //cv::imshow("Extracted frame0", image);
		  //int c = waitKey(0);
	}

    /// Second stage
    #ifdef CPU_ONLY
    ClassifyFace(regressed_rects_,sample_single,RNet_,threshold[1],'r');
    #else
	  ClassifyFace_MulImage(regressed_rects_,sample_single,RNet_,threshold[1],'r');
    #endif
	///////只需要使用stage1 stage2的话开启    ////////////////
	//regressed_rects_ = BoxRegress(condidate_rects_, 2);
	//faceInfo = NonMaximumSuppression(regressed_rects_,0.7,'m');
	/////////////////////////////


	///////需要stage3的话开启  1  ////////////////
	
	condidate_rects_ = NonMaximumSuppression(condidate_rects_, 0.7, 'u');
	regressed_rects_ = BoxRegress(condidate_rects_, 2);
	/////////////////////////////
	getEndTime();
	printf("%f ms\n",dfTim);
	/*cout << "three stage :" << condidate_rects_.size() << endl;
	for (int iNum = 0; iNum < condidate_rects_.size(); iNum++){
		cout << "condidate_rects_ :" << condidate_rects_[iNum].bbox.score << " " << condidate_rects_[iNum].bbox.y1 << " "
			<< condidate_rects_[iNum].bbox.x1 << " " << condidate_rects_[iNum].bbox.x1 << " " << condidate_rects_[iNum].bbox.y2 << endl;
	}*/

	Bbox2Square(regressed_rects_);
	Padding(width, height);

	//cout << "three stage :" << regressed_rects_.size() << endl;
	/*for (int iNum = 0; iNum < regressed_rects_.size(); iNum++){
		cout << regressed_rects_[iNum].bbox.score << " " << regressed_rects_[iNum].bbox.y1 << " "
			<< regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.y2 << endl;
	}



	for (int iNum = 0; iNum < regressed_rects_.size(); iNum++){
		cout << "************** " << regressed_rects_[iNum].bbox.score << " " << regressed_rects_[iNum].bbox.y1 << " "
			<< regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.y2 << endl;
	}*/
	 
    /// three stage
    numBox = regressed_rects_.size();
    if(numBox != 0){
      #ifdef CPU_ONLY
      ClassifyFace(regressed_rects_,sample_single,ONet_,threshold[2],'o');
      #else
		ClassifyFace_MulImage(regressed_rects_,sample_single,ONet_,threshold[2],'o');
      #endif
      regressed_rects_ = BoxRegress(condidate_rects_,3);
      faceInfo = NonMaximumSuppression(regressed_rects_,0.7,'m');

	 // cout << "three stage over :" << regressed_rects_.size() << endl;
	 /* for (int iNum = 0; iNum < regressed_rects_.size(); iNum++){
		  cout << regressed_rects_[iNum].bbox.score << " " << regressed_rects_[iNum].bbox.y1 << " "
			  << regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.x1 << " " << regressed_rects_[iNum].bbox.y2 << endl;
	  }*/

    }
  }
  regressed_pading_.clear();
  regressed_rects_.clear();
  condidate_rects_.clear();
}

//int test1(int argc,char **argv)
//{
//  if(argc != 3){
//    std::cout << "MTMain.bin [model dir] [imagePath]"<<std::endl;
//    return 0;
//  }
//  ::google::InitGoogleLogging(argv[0]);
//  double threshold[3] = {0.6,0.7,0.7};
//  double factor = 0.709;
//  int minSize = 40;
//  std::string proto_model_dir = argv[1];
//  MTCNN detector(proto_model_dir);
//
//  std::string imageName = argv[2];
//  cv::Mat image = cv::imread(imageName);
//  std::vector<FaceInfo> faceInfo;
//  clock_t t1 = clock();
//  std::cout <<"Detect "<<image.rows<<"X"<<image.cols;
//  detector.Detect(image,faceInfo,minSize,threshold,factor);
//  #ifdef CPU_ONLY
//  std::cout<<" Time Using CPU: " << (clock() - t1)*1.0/1000<<std::endl;
//  #else
//  std::cout<<" Time Using GPU-CUDNN: " << (clock() - t1)*1.0/1000<<std::endl;
//  #endif
//  for(int i = 0;i<faceInfo.size();i++){
//    float x = faceInfo[i].bbox.x1;
//    float y = faceInfo[i].bbox.y1;
//    float h = faceInfo[i].bbox.x2 - faceInfo[i].bbox.x1 +1;
//    float w = faceInfo[i].bbox.y2 - faceInfo[i].bbox.y1 +1;
//    cv::rectangle(image,cv::Rect(y,x,w,h),cv::Scalar(255,0,0),2);
//  }
//  for(int i=0;i<faceInfo.size();i++){
//    FacePts facePts = faceInfo[i].facePts;
//    for(int j=0;j<5;j++)
//      cv::circle(image,cv::Point(facePts.y[j],facePts.x[j]),1,cv::Scalar(255,255,0),2);
//  }
//  cv::imshow("a",image);
//  cv::waitKey(0);
//
//  return 1;
//}
//
//int main2(int argc, char **argv)
//{
//	initTrainImage();
//	::google::InitGoogleLogging(argv[0]);
//	double threshold[3] = { 0.6, 0.7, 0.7 };
//	double factor = 0.709;
//	int minSize = 40;
//	std::string proto_model_dir = argv[1];
//	MTCNN detector(proto_model_dir);
//
//	int imgNum = imgNames.size();
//	for (int iNum = 0; iNum < imgNum; iNum++){
//
//		cout << endl << iNum << " " << imgNames[iNum].c_str() << endl;
//		cv::Mat image = cv::imread(imgNames[iNum].c_str());
//		if (image.data) continue;
//
//		std::vector<FaceInfo> faceInfo;
//		clock_t t1 = clock();
//		std::cout << "Detect " << image.rows << "X" << image.cols;
//		detector.Detect(image, faceInfo, minSize, threshold, factor);
//#ifdef CPU_ONLY
//		std::cout << " Time Using CPU: " << (clock() - t1)*1.0 / 1000 << std::endl;
//#else
//		std::cout << " Time Using GPU-CUDNN: " << (clock() - t1)*1.0 / 1000 << std::endl;
//#endif
//		for (int i = 0; i < faceInfo.size(); i++){
//			float x = faceInfo[i].bbox.x1;
//			float y = faceInfo[i].bbox.y1;
//			float h = faceInfo[i].bbox.x2 - faceInfo[i].bbox.x1 + 1;
//			float w = faceInfo[i].bbox.y2 - faceInfo[i].bbox.y1 + 1;
//			cv::rectangle(image, cv::Rect(y, x, w, h), cv::Scalar(255, 0, 0), 2);
//		}
//		for (int i = 0; i < faceInfo.size(); i++){
//			FacePts facePts = faceInfo[i].facePts;
//			for (int j = 0; j < 5; j++)
//				cv::circle(image, cv::Point(facePts.y[j], facePts.x[j]), 1, cv::Scalar(255, 255, 0), 2);
//		}
//		cv::namedWindow("a", 0);
//		cv::imshow("a", image);
//		cv::waitKey(0);
//	}
//
//	return 1;
//}



//const double DESIRED_LEFT_EYE_X = 0.2;     // Controls how much of the face is visible after preprocessing.
//const double DESIRED_LEFT_EYE_Y = 0.2;
//const double FACE_ELLIPSE_CY = 0.40;
//const double FACE_ELLIPSE_W = 0.50;         // Should be atleast 0.5
//const double FACE_ELLIPSE_H = 0.80;         // Controls how tall the face mask is.


int main(int argc, char **argv) {
	initTrainImage();

	::google::InitGoogleLogging(argv[0]);
	double threshold[3] = { 0.7, 0.5, 0.3 };
	double factor = 0.709;
	int minSize = 40;
	std::string proto_model_dir = argv[1];
	MTCNN detector(proto_model_dir);
	int imgNum = imgNames.size();
	for (int iNum = 0; iNum < imgNum; iNum++){

		cout << endl << iNum << " " << imgNames[iNum].c_str() << endl;
		cv::VideoCapture capture(imgNames[iNum].c_str());
		//cv::VideoCapture capture(0);
		//capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
		//capture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
		//
		//VideoCapture capture("F:\\MTCNN-master\\vs2013_caffe_BN_multi_label\\water_meter_caffe_old\\\img\\1.avi");
		//检测是否正常打开:成功打开时，isOpened返回ture  
		if (!capture.isOpened())
			cout << "fail to open!" << endl;
		//获取整个帧数  
		//	long totalFrameNumber = capture.get(CV_CAP_PROP_FRAME_COUNT);
		//	cout << "整个视频共" << totalFrameNumber << "帧" << endl;
		//
		//
		//设置开始帧()  
		//	long frameToStart = 300;
		//	capture.set(CV_CAP_PROP_POS_FRAMES, frameToStart);
		//	cout << "从第" << frameToStart << "帧开始读" << endl;
		//
		//
		//设置结束帧  
		//	int frameToStop = 400000;
		//
		//	if (frameToStop < frameToStart)
		//	{
		//		cout << "结束帧小于开始帧，程序错误，即将退出！" << endl;
		//		return -1;
		//	}
		//	else
		//	{
		//		cout << "结束帧为：第" << frameToStop << "帧" << endl;
		//	}
		//
		//
		//	获取帧率  
		//	double rate = capture.get(CV_CAP_PROP_FPS);
		//	cout << "帧率为:" << rate << endl;



		//定义一个用来控制读取视频循环结束的变量  
		bool stop = false;

		//显示每一帧的窗口  
		namedWindow("Extracted frame_", 0);
		//两帧间的间隔时间:  
		//int delay = 1000/rate;  
		//	int delay = 1000 / rate;
		//	if (rate == 0)  delay = 1;
		//
		//利用while循环读取帧  
		//currentFrame是在循环体中控制读取到指定的帧后循环结束的变量  
		//	long currentFrame = frameToStart;
		//
		//    VideoWriter writer;
		//    writer.open("../result/SuicideSquad.mp4",CV_FOURCC('M', 'J', 'P', 'G'), 25, Size(1280,720), true);
		//承载每一帧的图像  

		cv::Mat image;
		int frame_count = 0;
		while (!stop)
		{
			//读取下一帧  
			if (!capture.read(image))
			{
				cout << "读取视频失败" << endl;
				stop = true;
				continue;
			}

			//imshow("Live", image);
			//waitKey(0);
			//for (int i = 0; i < 100; i++)capture.read(image);
			//
			//cv::flip(image, image,-1);

			std::vector<FaceInfo> faceInfo;
			clock_t t1 = clock();
			//		std::cout << "Detect " << image.rows << "X" << image.cols;
			//
			//image = image.t();

			detector.Detect(image, faceInfo, minSize, threshold, factor);
#ifdef CPU_ONLY
					std::cout << " Time Using CPU: " << (clock() - t1)*1.0 / 1000 << std::endl;
#else
			std::cout << " Time Using GPU-CUDNN: " << (clock() - t1)*1.0 / 1000 << std::endl;
#endif
			cv::Mat dst_face;

			for (int i = 0; i < faceInfo.size(); i++){

				float x = faceInfo[i].bbox.x1;
				float y = faceInfo[i].bbox.y1;
				float w = faceInfo[i].bbox.x2 - faceInfo[i].bbox.x1 + 1;
				float h = faceInfo[i].bbox.y2 - faceInfo[i].bbox.y1 + 1;

				std::cout << "得分： " << faceInfo[i].bbox.score << std::endl;

				if (x < 0) x = 0;	if (y < 0) y = 0;
				if ((y + h) > image.rows) h = image.rows - y;
				if ((x + w) > image.cols) w = image.cols - x;
				if (w < 0) continue;
				if (h < 0) continue;

				std::cout << x << " " << y << " " << w << " " << h << std::endl;
				std::cout << image.rows << " " << image.cols << std::endl;

				dst_face = image(cv::Rect(x, y, w, h));
				char dst_name[100];
				_mkdir("face");
				sprintf_s(dst_name, "%s%d%s%d%s%d%s", "face\\", iNum, "_", frame_count++, "_", i,".jpg");
				cv::imwrite(dst_name, dst_face);


				FacePts facePts = faceInfo[i].facePts;





				cv::Point2f srcTri[4];
				cv::Point2f dstTri[4];
				cv::Mat rot_mat(2, 4, CV_32FC1);
				cv::Mat warp_mat(2, 4, CV_32FC1);

				for (int j = 0; j < 4; j++){
					srcTri[j] = cv::Point2f(facePts.x[j] - x, facePts.y[j] - y);
				}

				int padding_x = cvFloor(h * 0.04 * 5);
				int padding_y = cvFloor(h * 0.04 * 2);
				int x0 = 0;
				int y0 = 0;
				int x1 = 120;
				int y1 = 0;
				int x2 = 120;
				int y2 = 48;
				int x3 = 0;
				int y3 = 48;

				dstTri[0] = cv::Point2f(x0 + padding_x, y0 + padding_y);
				dstTri[1] = cv::Point2f(x1 + padding_x, y1 + padding_y);
				dstTri[2] = cv::Point2f(x2 + padding_x, y2 + padding_y);
				dstTri[3] = cv::Point2f(x3 + padding_x, y3 + padding_y);

				warp_mat = getAffineTransform(srcTri, dstTri);
				cv::Mat warp_dstImage = Mat::zeros(48 + 2 * padding_y, 120 + 2 * padding_x, dst_face.type());
				warpAffine(dst_face, warp_dstImage, warp_mat, warp_dstImage.size());

#ifdef showSteps
				namedWindow("dst_face", 0);
				imshow("dst_face", dst_face);

				namedWindow("warp_dstImage", 0);
				imshow("warp_dstImage", warp_dstImage);

				cv::waitKey(1);
#endif

				sprintf_s(dst_name, "%s%d%s%d%s%d%s", "face\\", iNum, "_", frame_count++, "_", i, "_warp_dstImage_.jpg");
				cv::imwrite(dst_name, warp_dstImage);

#ifdef warpPerspective

				int img_height = dst_face.rows;
				int img_width = dst_face.cols;

				vector<Point2f> corners(4);

				for (int j = 0; j < 4; j++){
					corners[j] = cv::Point2f(facePts.x[j] - x , facePts.y[j] - y);
				}

				vector<Point2f> corners_trans(4);
				corners_trans[0] = cv::Point2f(x0 + padding_x, y0 + padding_y);
				corners_trans[1] = cv::Point2f(x1 + padding_x, y1 + padding_y);
				corners_trans[2] = cv::Point2f(x2 + padding_x, y2 + padding_y);
				corners_trans[3] = cv::Point2f(x3 + padding_x, y3 + padding_y);

				getStartTime();
				Mat transform = getPerspectiveTransform(corners, corners_trans);
				//cout << transform << endl;
				Mat resultImage;
				warpPerspective(dst_face, resultImage, transform, Size(120 + 2 * padding_x, 48 + 2 * padding_y), INTER_LINEAR);

				getEndTime();
				std::cout << " 2 :" << dfTim << std::endl;

				namedWindow("warpPerspective", 0);
				imshow("warpPerspective", resultImage);

				cv::waitKey(1);



				sprintf_s(dst_name, "%s%d%s%d%s%d%s", "face\\", iNum, "_", frame_count++, "_", i, "_resultImage_.jpg");
				cv::imwrite(dst_name, resultImage);

#endif





#ifdef showSteps
				for (int j = 0; j < 4; j++){
					std::cout << facePts.x[j] - x << " " << facePts.y[j] - y << std::endl;
					cv::circle(image, cv::Point(facePts.x[j], facePts.y[j]), 1, cv::Scalar(255, 255, 0), 2);
				}

				cv::rectangle(image, cv::Rect(x, y, w, h), cv::Scalar(255, 0, 0), 2);
				std::cout << x << " " << y << " " << w << " " << h << std::endl;
#endif

			}

			cv::imshow("Extracted frame_", image);

			int c = waitKey(0);
			//按下ESC或者到达指定的结束帧后退出读取视频  
			if ((char)c == 27 /*|| currentFrame > frameToStop*/)
			{
				stop = true;
			}
			//按下按键后会停留在当前帧，等待下一次按键  
			//if (c >= 0)
			//{
			//	waitKey(0);
			//}
			//	currentFrame++;


	}

		//关闭视频文件  
		capture.release();
	
	}
	return 0;
}








