#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <baseapi.h>
#include <allheaders.h>
#include <Windows.h>
#include <list>

int countursCount = 0;
int count_contr;

// считаем количество контуров
int countConturs(IplImage* plateImage) {
	countursCount = 0;
	IplImage* plate = cvCreateImage(cvGetSize(plateImage), IPL_DEPTH_8U, 1);
	cvConvertImage(plateImage, plate, CV_BGR2BGRA);
	cvThreshold(plate, plate, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	cvErode(plate, plate, NULL, 1);
	cvDilate(plate, plate, NULL, 1);
	cvCanny(plate, plate, 100, 50, 3); // находим границы
									   // хранилище пам€ти дл€ контуров 
	CvMemStorage* storage1 = cvCreateMemStorage(0);
	CvSeq* contour1 = 0;
	CvSeq* contourLow1 = 0;

	cvFindContours(plate, storage1, &contour1, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_TC89_KCOS, cvPoint(0, 0));
	assert(storage1 != 0);
	contourLow1 = cvApproxPoly(contour1, sizeof(CvContour), storage1, CV_POLY_APPROX_DP, 1, 1);

	for (; contourLow1 != 0; contourLow1 = contourLow1->h_next) {
		CvRect rect1;
		CvPoint pt11, pt21;
		rect1 = cvBoundingRect(contourLow1, NULL);
		pt11.x = rect1.x;
		pt21.x = (rect1.x + rect1.width);
		pt11.y = rect1.y;
		pt21.y = (rect1.y + rect1.height);

		if ((rect1.height > rect1.width)) {
			if ((plate->height) < (3 * rect1.height)) {
				countursCount = countursCount + 1;
			}
		}
	}

	// освобождаем ресурсы
	cvReleaseMemStorage(&storage1);
	//cvReleaseImage(&sub_img);
	return  countursCount;
}

/// ¬ыводим буквы и цифры
void plateNumber(IplImage* plateImage) {
	cvSaveImage("sub_img.jpg", plateImage);
	assert(plateImage != 0);

	IplImage* plateMarked = cvCreateImage(cvGetSize(plateImage), IPL_DEPTH_8U, 1);
	plateMarked = cvCloneImage(plateImage);
	
	IplImage* number = cvCreateImage(cvGetSize(plateImage), IPL_DEPTH_8U, 1);
	// конвертируем в градации серого
	cvConvertImage(plateImage, number, CV_BGR2BGRA);
	cvThreshold(number, number, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	cvErode(number, number, NULL, 1);
	cvDilate(number, number, NULL, 1);
	cvCanny(number, number, 100, 50, 3); // находим границы

	tesseract::TessBaseAPI *myOCR = new tesseract::TessBaseAPI();
	myOCR->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
	myOCR->SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz");
	if (myOCR->Init(NULL, "eng")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		cvWaitKey(0);
		system("pause");
		exit(1);
	}

	// хранилище пам€ти дл€ контуров
	CvMemStorage* countursStorage = cvCreateMemStorage(0);
	CvSeq* counturs = 0;
	CvSeq* countursOptimized = 0;

	cvFindContours(number, countursStorage, &counturs, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_TC89_KCOS, cvPoint(0, 0));
	assert(counturs != 0);
	// ќптимизируем контуры - уменьшаем количество точек
	countursOptimized = cvApproxPoly(counturs, sizeof(CvContour), countursStorage, CV_POLY_APPROX_DP, 1, 1);

	std::list<std::pair<int, char>> digitsList;

	for (; countursOptimized != 0; countursOptimized = countursOptimized->h_next) {
		CvRect rect = cvBoundingRect(countursOptimized, NULL);
		double ratio = (rect.width / rect.height);

		if ((rect.height > rect.width)) {
			if ((plateImage->height) < (3 * rect.height)) {
				cvRectangle(plateMarked, CvPoint(rect.x, rect.y), CvPoint(rect.x+rect.width,rect.y+rect.height), cvScalar(0, 0, 255), 2, 8, 0);
				cvSetImageROI(plateImage, rect);
				IplImage* digit = cvCreateImage(cvGetSize(plateImage), plateImage->depth, plateImage->nChannels);
				cvCopy(plateImage, digit, NULL);
				IplImage* digitWithBorder = cvCreateImage(cvSize(digit->width + 20, digit->height + 20), digit->depth, digit->nChannels);
				cvCopyMakeBorder(digit, digitWithBorder, cvPoint(10, 10), IPL_BORDER_CONSTANT, cvScalar(250));
				cvSaveImage("number_1.jpg", digitWithBorder);
				char outText[10000];
				myOCR->SetImage((uchar*)digitWithBorder->imageData, digitWithBorder->width, digitWithBorder->height, digitWithBorder->nChannels, digitWithBorder->widthStep);
				myOCR->Recognize(0);
				lstrcpy(outText, myOCR->GetUTF8Text());
//				printf(outText);
				digitsList.push_back( { rect.x, outText[0] });
				myOCR->Clear();
				//cvReleaseImage( &sub_img );
			}
		}
	}

	digitsList.sort([](std::pair<int, char> &a, std::pair<int, char> &b) { return a.first < b.first; });
	for (std::list<std::pair<int, char>>::iterator it = digitsList.begin(); it != digitsList.end(); ++it)
		printf("%c", it->second);

	// ѕодчищаем сделанные изменени€
	cvResetImageROI(plateImage);

	// выводим изображение с разметкой на символы в знаке номера
	cvNamedWindow("chars-positions", 1);
	cvShowImage("chars-positions", plateMarked);
	cvSaveImage("sub_img_markup.jpg", plateMarked);
	myOCR->End();
}

// находит и показывает рамку номера
void findPlate(IplImage* _image) {
	assert(_image != 0);
	IplImage* bin = cvCreateImage(cvGetSize(_image), IPL_DEPTH_8U, 1); // создаем новое изображение
	cvConvertImage(_image, bin, CV_BGR2GRAY); // конвертируем в градации серого
	cvSmooth(bin, bin, CV_GAUSSIAN, 3, 0, 0, 0);  // сглаживание гаусса
	cvThreshold(bin, bin, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU); // пороговое значение
	cvAdaptiveThreshold(bin, bin, 250, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 7, 1);// адаптивное пороговое значение
	cvErode(bin, bin, NULL, 1);  // эрози€ 
	cvDilate(bin, bin, NULL, 1); // дилатаци€
								 //cvNamedWindow( "cvThreshold", 1 );
								 //cvShowImage("cvThreshold", bin);
	cvCanny(bin, bin, 100, 50, 3); // находим границы дл€ поиска контуров
								   //cvNamedWindow( "bin", 1 );
								   //cvShowImage("bin", bin);
								   // хранилище пам€ти дл€ контуров
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contour = 0;
	CvSeq* contourLow = 0;
	cvFindContours(bin, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0)); // поиск контуров
	assert(contour != 0);
	contourLow = cvApproxPoly(contour, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 0.5, 12); // оптимизируем контуры
	int i = 1;

	for (; contourLow != 0; contourLow = contourLow->h_next) {
		double area = fabs(cvContourArea(contourLow));
		double perim = cvContourPerimeter(contourLow);
		if ((area / perim) > 4) {
			CvRect rect;
			CvPoint pt1, pt2;
			rect = cvBoundingRect(contourLow, NULL);
			pt1.x = rect.x;
			pt2.x = (rect.x + rect.width);
			pt1.y = rect.y;
			pt2.y = (rect.y + rect.height);
			double ratio = rect.width / rect.height;

			if ((2.0 < fabs(ratio) && fabs(ratio) < 8.0)) {
				// cvRectangle(_image, pt1,pt2, cvScalar(0, 0, 255), 1, 8, 0);
				// отправл€ем рамку на поиск номера
				cvSetImageROI(_image, rect);  // вырезаем рамку номера
				IplImage* sub_img = cvCreateImage(cvGetSize(_image), _image->depth, _image->nChannels);
				//  опирование вырезанного объекта в созданный образ
				cvCopy(_image, sub_img, NULL);

				if ((sub_img->height) < 124) {
					// ”величиваем размер изображени€
					double koeff1 = 124 / sub_img->height;
					IplImage* new_sub_img = cvCreateImage(cvSize(sub_img->width*koeff1, sub_img->height*koeff1), sub_img->depth, sub_img->nChannels);
					cvResize(sub_img, new_sub_img);
					cvReleaseImage(&sub_img);
					sub_img = new_sub_img;
				}

				count_contr = countConturs(sub_img);

				if (count_contr < 5)
					continue;
				else {
					// выводим буквы
					cvSaveImage("sub_img.jpg", sub_img);
					plateNumber(sub_img);
				}

				cvReleaseImage(&sub_img);  // освобождаем ресурсы
			}
		}
	}

	// освобождаем ресурсы
	cvReleaseMemStorage(&storage);
	cvReleaseImage(&bin);
}

int main(int argc, char* argv[]) {
	IplImage *src = 0;

	// им€ картинки задаЄтс€ первым параметром
	char* filename = argc >= 2 ? argv[1] : "1064a7383029d7718635ed2cae534e37.jpg";
	// получаем картинку
	src = cvLoadImage(filename, 1); // загружаем картинку
	assert(src != 0);
	cvNamedWindow("original", 1); // покажем изображение
	cvShowImage("original", src);
	findPlate(src);                // поиск рамки
	cvNamedWindow("plate", 1);
	cvShowImage("plate", src);
	cvWaitKey(0); // ждЄм нажати€ клавиши

	cvReleaseImage(&src); // освобождаем ресурсы
	cvDestroyAllWindows(); // удал€ем окна
	return 0;
}