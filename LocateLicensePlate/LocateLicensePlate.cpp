// LocateLicensePlate.cpp : Нахождение номерного знака на изображении


#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <stdlib.h>
#include <stdio.h>

// находит и показывает рамку на изображении
void findPlate(IplImage* _image) {
	assert(_image != 0);
	IplImage* temp = cvCreateImage(cvGetSize(_image), IPL_DEPTH_8U, 1);
	// конвертируем в градации серого
	cvConvertImage(_image, temp, CV_BGR2GRAY);
	// смотрим что получилось
	cvNamedWindow("CV_BGR2GRAY", 1);
	cvShowImage("CV_BGR2GRAY", temp);
	// делаем гауссовское сглаживание
	cvSmooth(temp, temp, CV_GAUSSIAN, 3, 0, 0, 0);
	// Эрозию
	cvErode(temp, temp, NULL, 1);
	// расширение
	cvDilate(temp, temp, NULL, 1);

	// находим границы
	cvCanny(temp, temp, 100, 50, 3);

	cvNamedWindow("temp", 1);
	cvShowImage("temp", temp);

	// хранилище памяти для контуров
	CvMemStorage* storageContours = cvCreateMemStorage(0);
	CvSeq* contour = 0;
	CvSeq* contourLow = 0;

	assert(storageContours != 0);

	cvFindContours(temp, storageContours, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	//оптимизируем контуры
	contourLow = cvApproxPoly(contour, sizeof(CvContour), storageContours, CV_POLY_APPROX_DP, 0.5, 12);
	// бегаем по контурам 
	for (; contourLow != 0; contourLow = contourLow->h_next) {
		//находим соотношения площади к периметру контура
		double area = fabs(cvContourArea(contourLow));
		double perim = cvContourPerimeter(contourLow);
		if ((area / perim)>4) {
			CvRect rect;
			CvPoint pt1, pt2;
			rect = cvBoundingRect(contourLow, NULL); // ищем среди оставшихся прямоугольники
			pt1.x = rect.x;
			pt2.x = (rect.x + rect.width);
			pt1.y = rect.y;
			pt2.y = (rect.y + rect.height);
			double ratio = rect.width / rect.height;

			if ((2.0 < fabs(ratio) && fabs(ratio) < 8.0)) {
				//Show result.
				cvRectangle(_image, pt1, pt2, cvScalar(0, 0, 255), 1, 8, 0);
			}
		}
	}
	// освобождаем ресурсы
	cvReleaseMemStorage(&storageContours);
	cvReleaseImage(&temp);
}

int main(int argc, char* argv[]) {
	IplImage *img = 0;

	// имя картинки задаётся первым параметром
	char* filename = argc >= 2 ? argv[1] : "960.jpg";
	// получаем картинку
	img = cvLoadImage(filename, 1);
	assert(img != 0);

	// покажем изображение
	cvNamedWindow("original", 1);
	cvShowImage("original", img);

	findPlate(img);
	cvNamedWindow("findplate", 1);
	cvShowImage("findplate", img);
	// ждём нажатия клавиши
	cvWaitKey(0);
	// освобождаем ресурсы
	cvReleaseImage(&img);
	// удаляем окна
	cvDestroyAllWindows();
	return 0;
}