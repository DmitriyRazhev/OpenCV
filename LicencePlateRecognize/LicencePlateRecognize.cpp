#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <stdlib.h>
#include <stdio.h>
#include <baseapi.h>
#include <allheaders.h>
int count_cont = 0;
int count_contr;
// считаем количество контуров
int count_contur(IplImage* sub_img) {
	count_cont = 0;
	IplImage* plate = cvCreateImage(cvGetSize(sub_img), IPL_DEPTH_8U, 1);
	cvConvertImage(sub_img, plate, CV_BGR2BGRA);
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
				count_cont = count_cont + 1;
			}
		}
	}

	// освобождаем ресурсы
	cvReleaseMemStorage(&storage1);
	//cvReleaseImage(&sub_img);
	return  count_cont;
}

/// ¬ыводим буквы и цифры
void plate_number(IplImage* sub_img1) {
	cvSaveImage("sub_img.jpg", sub_img1);
	assert(sub_img1 != 0);

	IplImage* number = cvCreateImage(cvGetSize(sub_img1), IPL_DEPTH_8U, 1);
	IplImage* number_1 = cvCreateImage(cvGetSize(sub_img1), IPL_DEPTH_8U, 1);
	// конвертируем в градации серого
	cvConvertImage(sub_img1, number, CV_BGR2BGRA);
	cvThreshold(number, number, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	number_1 = cvCloneImage(number);
	cvErode(number, number, NULL, 1);
	cvDilate(number, number, NULL, 1);
	cvCanny(number, number, 100, 50, 3); // находим границы

	tesseract::TessBaseAPI *myOCR =
		new tesseract::TessBaseAPI();
	myOCR->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
	myOCR->SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz");
	if (myOCR->Init(NULL, "eng")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		cvWaitKey(0);
		system("pause");
		exit(1);
	}

	// хранилище пам€ти дл€ контуров
	CvMemStorage* storage2 = cvCreateMemStorage(0);
	CvSeq* contour2 = 0;
	CvSeq* contourLow2 = 0;

	cvFindContours(number, storage2, &contour2, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_TC89_KCOS, cvPoint(0, 0));
	assert(contour2 != 0);
	//Optimize contours, reduce points
	contourLow2 = cvApproxPoly(contour2, sizeof(CvContour), storage2, CV_POLY_APPROX_DP, 1, 1);

	for (; contourLow2 != 0; contourLow2 = contourLow2->h_next)	{
		CvRect rect;
		CvPoint pt1, pt2;
		rect = cvBoundingRect(contourLow2, NULL);
		pt1.x = rect.x;
		pt2.x = (rect.x + rect.width);
		pt1.y = rect.y;
		pt2.y = (rect.y + rect.height);
		double ratio = (rect.width / rect.height);

		if ((rect.height > rect.width))	{
			if ((number_1->height) < (3 * rect.height))	{
				//cvRectangle(sub_img1, pt1,pt2, cvScalar(0, 0, 255), 2, 8, 0);
				IplImage* sub_img;
				cvSetImageROI(number_1, rect);
				IplImage* sub_img2 = cvCreateImage(cvGetSize(number_1), number_1->depth, number_1->nChannels);
				cvCopy(number_1, sub_img2, NULL);
				IplImage* number_3 = cvCreateImage(cvSize(sub_img2->width + 20, sub_img2->height + 20), sub_img2->depth, sub_img2->nChannels);

				cvCopyMakeBorder(sub_img2, number_3, cvPoint(10, 10), IPL_BORDER_CONSTANT, cvScalar(250));
				cvSaveImage("number_1.jpg", number_3);
				char outText[10000];
				myOCR->SetImage((uchar*)number_3->imageData, number_3->width, number_3->height, number_3->nChannels, number_3->widthStep);
				myOCR->Recognize(0);
				lstrcpy(outText, myOCR->GetUTF8Text());
				printf(outText);
				myOCR->Clear();
				//myOCR->End();
				//cvReleaseImage( &sub_img );
			}
		}
	}
}

// находит и показывает рамку номера
void findplate(IplImage* _image) {
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
					double koeff1 = 124 / sub_img->height;
					IplImage* new_sub_img = cvCreateImage(cvSize(sub_img->width*koeff1, sub_img->height*koeff1), sub_img->depth, sub_img->nChannels);
					cvResize(sub_img, new_sub_img);
					cvSaveImage("new_sub_img.jpg", new_sub_img);
					//cvResize(sub_img, sub_img);
					count_contr = count_contur(new_sub_img);

					if (count_contr < 5) {
						continue;
					}
					if (count_contr >= 5) {
						// выводим буквы
						//cvSaveImage("sub_img.jpg", sub_img);
						plate_number(new_sub_img);
					}
				}

				if ((sub_img->height) < 124) {
					double koeff1 = 124 / sub_img->height;
					IplImage* new_sub_img = cvCreateImage(cvSize(sub_img->width*koeff1, sub_img->height*koeff1), sub_img->depth, sub_img->nChannels);
					cvResize(sub_img, new_sub_img);
					//cvResize(sub_img, sub_img);
					count_contr = count_contur(new_sub_img);

					if (count_contr < 5)
						continue;
					else {
						// выводим буквы
						//cvSaveImage("sub_img.jpg", sub_img);
						plate_number(new_sub_img);
						// 
					}
				}

				if ((sub_img->height) >= 124) {
					count_contr = count_contur(sub_img);

					if (count_contr < 5)
						continue;
					else {
						// выводим буквы
						//cvSaveImage("sub_img.jpg", sub_img);
						plate_number(sub_img);
					}
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
	//cvNamedWindow( "original", 1 ); // покажем изображение
	//cvShowImage( "original", src );
	findplate(src);                // поиск рамки
	cvNamedWindow("plate", 1);
	cvShowImage("plate", src);
	cvWaitKey(0); // ждЄм нажати€ клавиши

	cvReleaseImage(&src); // освобождаем ресурсы
	cvDestroyAllWindows(); // удал€ем окна
	return 0;
}