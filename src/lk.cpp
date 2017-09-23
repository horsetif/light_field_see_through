#include <time.h>
#include "timing.h"
#include "lk.h"

pair<vector<Point2f>, vector<Point2f>> lucasKanade(int N, int width, int height, unsigned char * grad, unsigned char * img1, unsigned char * img2) {
  vector<Point2f> points[2];
  Mat mat1(Size(width, height), CV_8UC1, img1);
  Mat mat2(Size(width, height), CV_8UC1, img2);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (grad[y * width + x] == 255) {
        points[0].push_back(Point2f(x, y));
      }
    }
  }
  vector<uchar> status;
  vector<float> err;
  vector<Point2f> points_new[2];
  calcOpticalFlowPyrLK(mat1, mat2, points[0], points[1], status, err, Size(21, 21));
  for (int i = 0; i < points[1].size(); i++) {
    if (status[i] == 0) continue;
    points[1][i] -= points[0][i];
	points_new[0].push_back(points[0][i]);
	points_new[1].push_back(points[1][i]);
  }
 // return make_pair(points[0], points[1]);
  return make_pair(points_new[0], points_new[1]);
}

pair<vector<Point2f>, vector<Point2f>> lkEdgeFlow(int N, int width, int height, unsigned char * grad, unsigned char * img1, unsigned char * img2) {
  pair<vector<Point2f>, vector<Point2f>> tmp;
  CPUTIMEINIT
  CPUTIMEIT(tmp = lucasKanade(N, width, height, grad, img1, img2), "Lucas Kanade")
  return tmp;
}
