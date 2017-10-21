#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <stdlib.h>
#include <stdio.h>

// ������� � ���������� ����� �� �����������
void findplate(IplImage* _image) {
	assert(_image != 0);

	IplImage* temp = cvCreateImage(cvGetSize(_image), IPL_DEPTH_8U, 1);
	// ������������ � �������� ������
	cvConvertImage(_image, temp, CV_BGR2GRAY);
	// ������� ��� ����������
	//cvNamedWindow( "CV_BGR2GRAY", 1 );
	//cvShowImage("CV_BGR2GRAY", temp);
	// ������ ����������� �����������
	cvSmooth(temp, temp, CV_GAUSSIAN, 3, 0, 0, 0);
	// ������
	cvErode(temp, temp, NULL, 1);
	// ����������
	cvDilate(temp, temp, NULL, 1);

	// ������� �������
	cvCanny(temp, temp, 100, 50, 3);

	//cvNamedWindow( "temp", 1 );
	//cvShowImage("temp", temp);

	// ��������� ������ ��� ��������
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contour = 0;
	CvSeq* contourLow = 0;

	assert(storage != 0);

	cvFindContours(temp, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	//������������ �������
	contourLow = cvApproxPoly(contour, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 0.5, 12);
	// ������ �� �������� 
	for (; contourLow != 0; contourLow = contourLow->h_next) {
		//������� ����������� ������� � ��������� �������
		double area = fabs(cvContourArea(contourLow));
		double perim = cvContourPerimeter(contourLow);
		if ((area / perim)>4) {
			CvRect rect;
			CvPoint pt1, pt2;
			rect = cvBoundingRect(contourLow, NULL); // ���� ����� ���������� ��������������
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
	// ����������� �������
	cvReleaseMemStorage(&storage);
	cvReleaseImage(&temp);
}

int main(int argc, char* argv[]) {
	IplImage* frame = 0;

	// ��� ����� ������� ������ ����������
	char* filename = argc == 2 ? argv[1] : "Video012.mp4";

	printf("[i] file: %s\n", filename);

	// ���� ��� ����������� ��������
	cvNamedWindow("original", CV_WINDOW_AUTOSIZE);

	// �������� ���������� � �����-�����
	CvCapture* capture = cvCreateFileCapture(filename);

	while (1) {
		// �������� ��������� ����
		frame = cvQueryFrame(capture);
		if (!frame) 
			break;

		// ����� ����� ��������
		// ��������� ���������
		findplate(frame);
		// ���������� ����
		cvShowImage("original", frame);

		char c = cvWaitKey(33);
		if (c == 27) { // ���� ������ ESC - �������
			break;
		}
	}

	// ����������� �������
	cvReleaseCapture(&capture);
	// ������� ����
	cvDestroyWindow("original");
	return 0;
}