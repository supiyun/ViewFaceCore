﻿#include "seeta/FaceDetector.h"
#include "seeta/FaceLandmarker.h"
#include "seeta/FaceRecognizer.h"
#include "seeta/FaceAntiSpoofing.h"

#include <time.h>

#define View_Api extern "C" __declspec(dllexport)

using namespace std;

typedef void(_stdcall* LogCallBack)(const char* logText);

string modelPath = "./model/"; // 模型所在路径
LogCallBack logger = NULL; // 日志回调函数

// 打印日志
void WriteLog(string str) { if (logger != NULL) { logger(str.c_str()); } }

void WriteMessage(string fanctionName, string message) { WriteLog(fanctionName + "\t Message:" + message); }
void WriteModelName(string fanctionName, string modelName) { WriteLog(fanctionName + "\t Model.Name:" + modelName); }
void WriteRunTime(string fanctionName, clock_t start) { WriteLog(fanctionName + "\t Run.Time:" + to_string(clock() - start) + " ms"); }
void WriteError(string fanctionName, const std::exception& e) { WriteLog(fanctionName + "\t Error:" + e.what()); }

// 注册日志回调函数
View_Api void V_SetLogFunction(LogCallBack writeLog)
{
	logger = writeLog;
	WriteMessage("SetLogFunction", "Successed.");
}

// 设置人脸模型目录
View_Api void V_SetModelPath(const char* path)
{
	modelPath = path;
	WriteMessage("SetModelPath", "Model.Path:" + modelPath);
}
// 获取人脸模型目录
View_Api bool V_GetModelPath(char** path)
{
	try
	{
		strcpy(*path, modelPath.c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		WriteError("GetModelPath", e);
		return false;
	}
}

seeta::FaceDetector* v_faceDetector = NULL;

// 人脸检测结果
static SeetaFaceInfoArray detectorInfos;
// 人脸数量检测器
View_Api int V_DetectorSize(unsigned char* imgData, int width, int height, int channels, double faceSize = 20, double threshold = 0.9, double maxWidth = 2000, double maxHeight = 2000, int type = 0)
{
	try {
		clock_t start = clock();

		SeetaImageData img = { width, height, channels, imgData };
		if (v_faceDetector == NULL) {
			seeta::ModelSetting setting;
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_detector.csta";
			if (type == 1) { modelName = "mask_detector.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("DetectorSize", modelName);
			v_faceDetector = new seeta::FaceDetector(setting);
		}

		if (faceSize != 20) { v_faceDetector->set(seeta::FaceDetector::Property::PROPERTY_MIN_FACE_SIZE, faceSize); }
		if (threshold != 0.9) { v_faceDetector->set(seeta::FaceDetector::Property::PROPERTY_THRESHOLD, threshold); }
		if (maxWidth != 2000) { v_faceDetector->set(seeta::FaceDetector::Property::PROPERTY_MAX_IMAGE_WIDTH, maxWidth); }
		if (maxHeight != 2000) { v_faceDetector->set(seeta::FaceDetector::Property::PROPERTY_MAX_IMAGE_HEIGHT, maxHeight); }

		auto infos = v_faceDetector->detect(img);
		detectorInfos = infos;

		WriteRunTime("Detector", start); // 此方法已经是人脸检测的全过程，故计时器显示为 人脸识别方法
		return infos.size;
	}
	catch (const std::exception& e)
	{
		WriteError("DetectorSize", e);
		return -1;
	}
}
// 人脸检测器
View_Api bool V_Detector(float* score, int* x, int* y, int* width, int* height)
{
	try
	{
		//clock_t start = clock();

		for (int i = 0; i < detectorInfos.size; i++, detectorInfos.data++)
		{
			*score = detectorInfos.data->score;
			*x = detectorInfos.data->pos.x;
			*y = detectorInfos.data->pos.y;
			*width = detectorInfos.data->pos.width;
			*height = detectorInfos.data->pos.height;
			score++, x++, y++, width++, height++;
		}
		detectorInfos.data = NULL;
		detectorInfos.size = NULL;

		//WriteRunTime(__FUNCDNAME__, start); // 此方法只是将 人脸数量检测器 获取到的数据赋值传递，并不耗时。故不显示此方法的调用时间
		return true;
	}
	catch (const std::exception& e)
	{
		WriteError("Detector", e);
		return false;
	}
}


seeta::FaceLandmarker* v_faceLandmarker = NULL;
/// <summary>
/// 人脸关键点数量
/// </summary>
/// <param name="type">模型类型。0：68个关键点；1：5个关键点[戴口罩]；2：5个关键点。</param>
/// <returns></returns>
View_Api int V_FaceMarkSize(int type = 0)
{
	try
	{
		clock_t start = clock();

		if (v_faceLandmarker == NULL) {
			seeta::ModelSetting setting;
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_landmarker_pts68.csta";
			if (type == 1) { modelName = "face_landmarker_mask_pts5.csta"; }
			if (type == 2) { modelName = "face_landmarker_pts5.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("FaceMarkSize", modelName);
			v_faceLandmarker = new seeta::FaceLandmarker(setting);
		}
		int size = v_faceLandmarker->number();

		WriteRunTime("FaceMarkSize", start);
		return size;
	}
	catch (const std::exception& e)
	{
		WriteError("FaceMarkSize", e);
		return -1;
	}
}
// 人脸关键点
View_Api bool V_FaceMark(unsigned char* imgData, int width, int height, int channels, int x, int y, int fWidth, int fHeight, double* pointX, double* pointY, int type = 0)
{
	try
	{
		clock_t start = clock();

		SeetaImageData img = { width, height, channels, imgData };
		SeetaRect face = { x, y, fWidth, fHeight };
		if (v_faceLandmarker == NULL) {
			seeta::ModelSetting setting;
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_landmarker_pts68.csta";
			if (type == 1) { modelName = "face_landmarker_mask_pts5.csta"; }
			if (type == 2) { modelName = "face_landmarker_pts5.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("FaceMark", modelName);
			v_faceLandmarker = new seeta::FaceLandmarker(setting);
		}
		std::vector<SeetaPointF> _points = v_faceLandmarker->mark(img, face);

		if (!_points.empty()) {
			for (auto iter = _points.begin(); iter != _points.end(); iter++)
			{
				*pointX = (*iter).x;
				*pointY = (*iter).y;
				pointX++;
				pointY++;
			}

			WriteRunTime("FaceMark", start);
			return true;
		}
		else { return false; }
	}
	catch (const std::exception& e)
	{
		WriteError("FaceMark", e);
		return false;
	}
}

seeta::FaceRecognizer* v_faceRecognizer = NULL;
// 获取人脸特征值长度
View_Api int V_ExtractSize(int type = 0)
{
	try
	{
		clock_t start = clock();

		if (v_faceRecognizer == NULL) {
			seeta::ModelSetting setting;
			setting.set_id(0);
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_recognizer.csta";
			if (type == 1) { modelName = "face_recognizer_mask.csta"; }
			if (type == 2) { modelName = "face_recognizer_light.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("ExtractSize", modelName);
			v_faceRecognizer = new seeta::FaceRecognizer(setting);
		}
		int length = v_faceRecognizer->GetExtractFeatureSize();

		WriteRunTime("ExtractSize", start);
		return length;
	}
	catch (const std::exception& e)
	{
		WriteError("ExtractSize", e);
		return -1;
	}
}
// 提取人脸特征值
View_Api bool V_Extract(unsigned char* imgData, int width, int height, int channels, SeetaPointF* points, float* features, int type = 0)
{
	try
	{
		clock_t start = clock();

		SeetaImageData img = { width, height, channels, imgData };
		if (v_faceRecognizer == NULL) {
			seeta::ModelSetting setting;
			setting.set_id(0);
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_recognizer.csta";
			if (type == 1) { modelName = "face_recognizer_mask.csta"; }
			if (type == 2) { modelName = "face_recognizer_light.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("Extract", modelName);
			v_faceRecognizer = new seeta::FaceRecognizer(setting);
		}
		int length = v_faceRecognizer->GetExtractFeatureSize();
		std::shared_ptr<float> _features(new float[v_faceRecognizer->GetExtractFeatureSize()], std::default_delete<float[]>());
		v_faceRecognizer->Extract(img, points, _features.get());

		for (int i = 0; i < length; i++)
		{
			*features = _features.get()[i];
			features++;
		}

		WriteRunTime("Extract", start);
		return true;

	}
	catch (const std::exception& e)
	{
		WriteError("Extract", e);
		return false;
	}
}
// 人脸特征值相似度计算
View_Api float V_CalculateSimilarity(float* leftFeatures, float* rightFeatures, int type = 0)
{
	try
	{
		clock_t start = clock();

		if (v_faceRecognizer == NULL) {
			seeta::ModelSetting setting;
			setting.set_id(0);
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "face_recognizer.csta";
			if (type == 1) { modelName = "face_recognizer_mask.csta"; }
			if (type == 2) { modelName = "face_recognizer_light.csta"; }
			setting.append(modelPath + modelName);
			WriteModelName("CalculateSimilarity", modelName);
			v_faceRecognizer = new seeta::FaceRecognizer(setting);
		}

		auto similarity = v_faceRecognizer->CalculateSimilarity(leftFeatures, rightFeatures);
		WriteMessage("CalculateSimilarity", "Similarity = " + to_string(similarity));
		WriteRunTime("CalculateSimilarity", start);
		return similarity;
	}
	catch (const std::exception& e)
	{
		WriteError("CalculateSimilarity", e);
		return -1;
	}
}

// 活体检测器
seeta::FaceAntiSpoofing* v_faceAntiSpoofing = NULL;
// 活体检测 - 单帧
View_Api int V_AntiSpoofing(unsigned char* imgData, int width, int height, int channels, int x, int y, int fWidth, int fHeight, SeetaPointF* points, bool global)
{
	try
	{
		clock_t start = clock();

		SeetaImageData img = { width, height, channels, imgData };
		SeetaRect face = { x, y, fWidth, fHeight };
		if (v_faceAntiSpoofing == NULL) {
			seeta::ModelSetting setting;
			setting.set_id(0);
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "fas_first.csta";
			setting.append(modelPath + modelName);
			if (global) { // 启用全局检测能力
				modelName = "fas_second.csta";
				setting.append(modelPath + modelName);
				WriteModelName("AntiSpoofing", modelName);
			}
			WriteModelName("AntiSpoofing", modelName);
			v_faceAntiSpoofing = new seeta::FaceAntiSpoofing(setting);
		}

		auto status = v_faceAntiSpoofing->Predict(img, face, points);

		WriteRunTime("AntiSpoofing", start);
		return status;
	}
	catch (const std::exception& e)
	{
		WriteError("AntiSpoofing", e);
		return -1;
	}
}
// 活体检测 - 视频
View_Api int V_AntiSpoofingVideo(unsigned char* imgData, int width, int height, int channels, int x, int y, int fWidth, int fHeight, SeetaPointF* points, bool global)
{
	try
	{
		clock_t start = clock();

		SeetaImageData img = { width, height, channels, imgData };
		SeetaRect face = { x, y, fWidth, fHeight };
		if (v_faceAntiSpoofing == NULL) {
			seeta::ModelSetting setting;
			setting.set_id(0);
			setting.set_device(SEETA_DEVICE_CPU);
			string modelName = "fas_first.csta";
			setting.append(modelPath + modelName);
			if (global) { // 启用全局检测能力
				modelName = "fas_second.csta";
				setting.append(modelPath + modelName);
				WriteModelName("AntiSpoofingVideo", modelName);
			}
			WriteModelName("AntiSpoofingVideo", modelName);
			v_faceAntiSpoofing = new seeta::FaceAntiSpoofing(setting);
		}

		auto status = v_faceAntiSpoofing->PredictVideo(img, face, points);

		WriteRunTime("AntiSpoofingVideo", start);
		return status;
	}
	catch (const std::exception& e)
	{
		WriteError("AntiSpoofingVideo", e);
		return -1;
	}
}

// 释放资源
View_Api void V_Dispose()
{
	if (v_faceDetector != NULL) delete v_faceDetector;
	if (v_faceLandmarker != NULL) delete v_faceLandmarker;
	if (v_faceRecognizer != NULL) delete v_faceRecognizer;
	if (v_faceAntiSpoofing != NULL) delete v_faceAntiSpoofing;
}