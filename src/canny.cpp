#include "canny_cpu.h"
#include "timing.h"
#include <cstdio>
#include <cmath>

#define UPPERTHRESHOLD 90
#define LOWERTHRESHOLD 30

const float G_x[3 * 3] = {
  -1, 0, 1,
  -2, 0, 2,
  -1, 0, 1
};

const float G_y[3 * 3] = {
  1, 2, 1,
  0, 0, 0,
  -1, -2, -1
};

const float gaussian[5 * 5] = {
  2.f/159, 4.f/159, 5.f/159, 4.f/159, 2.f/159,
  4.f/159, 9.f/159, 12.f/159, 9.f/159, 4.f/159,
  5.f/159, 12.f/159, 15.f/159, 12.f/159, 2.f/159,
  4.f/159, 9.f/159, 12.f/159, 9.f/159, 4.f/159,
  2.f/159, 4.f/159, 5.f/159, 4.f/159, 2.f/159
};


void Canny::kernSmooth(int N, int width, int height, unsigned char * in, unsigned char * out, const float * kernel, int kernSize) {
  float c;
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      c = 0.0f;
    	for (int i = 0; i < kernSize; i++) {
    		int tx = x + i - kernSize/2;
    		for (int j = 0; j < kernSize; j++) {
    			int ty = y + j - kernSize/2;
    			if (tx >= 0 && ty >= 0 && tx < width && ty < height) {
    				c += in[ty * width + tx] * kernel[j * kernSize + i];
    			}
    		}
    	}
    	out[y * width + x] = fabs(c);
    }
  }
}

void kernGradient(int N, int width, int height, unsigned char * in, unsigned char * gradient, unsigned char * edgeDir) {
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      int idx, dx, dy;
      float Gx, Gy, grad, angle;
      idx = i * width + j;
      Gx = Gy = 0;
      for (int dy = 0; dy < 3; dy++) {
        int ty = i + dy - 1;
        for (int dx = 0; dx < 3; dx++) {
          int tx = j + dx - 1;
          if (tx >= 0 && ty >= 0 && tx < width && ty < height) {
            Gx += in[ty * width + tx] * G_x[dy * 3 + dx];
            Gy += in[ty * width + tx] * G_y[dy * 3 + dx];
          }
        }
      }
      grad = sqrt(Gx * Gx + Gy * Gy);
      angle = (atan2(Gx, Gy) / 3.14159f) * 180.0f;
      unsigned char roundedAngle;
      if (((-22.5 < angle) && (angle <= 22.5)) || ((157.5 < angle) && (angle <= -157.5))) {
        roundedAngle = 0;
      }
  		if (((-157.5 < angle) && (angle <= -112.5)) || ((22.5 < angle) && (angle <= 67.5))) {
        roundedAngle = 45;
      }
  		if (((-112.5 < angle) && (angle <= -67.5)) || ((67.5 < angle) && (angle <= 112.5))) {
        roundedAngle = 90;
      }
  		if (((-67.5 < angle) && (angle <= -22.5)) || ((112.5 < angle) && (angle <= 157.5))) {
        roundedAngle = 135;
      }
      gradient[idx] = grad;
      edgeDir[idx] = roundedAngle;
    }
  }
}



void nonMaxSuppression(int N, int width, int height, unsigned char * in, unsigned char * out) {
  int D = 1;
  for (int i = D; i < height - D; ++i) {
    for (int j = D; j < width - D; ++j) {
      int angle = in[i * width + j];
      switch(angle) {
        case 0:
          if (out[i * width + j] < out[(i + D) * width + j] || out[i * width + j] < out[(i - D) * width + j]) {
            out[i * width + j] = 0;
          }
          break;
        case 45:
          if (out[i * width + j] < out[(i + D) * width + j - D] || out[i * width + j] < out[(i - D) * width + j + D]) {
            out[i * width + j] = 0;
          }
          break;
        case 90:
          if (out[i * width + j] < out[i * width + j + D] || out[i * width + j] < out[i * width + j - D]) {
            out[i * width + j] = 0;
          }
          break;

        case 135:
          if (out[i * width + j] < out[(i + D) * width + j + D] || out[i * width + j] < out[(i - D) * width + j - D]) {
            out[i * width + j] = 0;
          }
          break;
        default:
          break;
      }
    }
  }
}

void hysteresis(int N, int width, int height, unsigned char * in) {
  for (int i = 1; i < height - 1; i++) { // simplified bounds
    for (int j = 1; j < width - 1; j++) {
      int idx = i * width + j;
      if (in[idx] > UPPERTHRESHOLD) {
        in[idx] = 255;
      } else if (in[idx] < LOWERTHRESHOLD) {
        in[idx] = 0;
      } else {
        for (int dy = -1; dy <= 1; dy++) {
          for (int dx = -1; dx <= 1; dx++) {
            int nidx = (i + dy) * width + (j + dx);
            if(0 <= (i + dy) && (i + dy) < height &&  0 <= (j + dx) && (j + dx) < width && in[nidx] > LOWERTHRESHOLD) {
              in[nidx] = 255;
            }
          }
        }
      }
    }
  }
}


unsigned char * Canny::edge(int N, int width, int height, unsigned char * in) {
	CPUTIMEINIT
  unsigned char * smooth = new unsigned char[N];
  unsigned char * gradient = new unsigned char[N];
  unsigned char * edgeDir = new unsigned char[N];

  CPUTIMEIT(Canny::kernSmooth(N, width, height, in, smooth, gaussian, 5), "kernSmooth");
  CPUTIMEIT(kernGradient(N, width, height, smooth, gradient, edgeDir), "kernGradient");
  CPUTIMEIT(nonMaxSuppression(N, width, height, edgeDir, gradient), "nonmax");
  CPUTIMEIT(hysteresis(N, width, height, gradient), "hysteresis"); // can use stream compaction

  delete[] smooth, gradient, edgeDir;

  return gradient;
}
