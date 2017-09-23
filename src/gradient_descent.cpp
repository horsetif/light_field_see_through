#include "gradient_descent.h"
//#include <bits/stdc++.h>
#include "timing.h"
#include<iostream>
#include<math.h>
#include<glm.hpp>
#include<algorithm>
using namespace std;

GradientDescent::GradientDescent(int width, int height, int frames, glm::vec2 * VB, glm::vec2 * VO, float * alpha, float * imgO, float * imgB, float * sequence) {
  this->width = width;
  this->height = height;
  this->frames = frames;
  this->sequence = sequence;
  this->VB = VB;
  this->VO = VO;
  this->alpha = alpha;
  this->imgO = imgO;
  this->imgB = imgB;
  this->initializeBuffers();
}

GradientDescent::~GradientDescent() {
  free(this->sequence);
  this->freeBuffers();
}

void GradientDescent::optimize() {
CPUTIMEINIT;
  for (int i = 0; i < this->GD_ROUNDS; i++) {
		CPUTIMEIT(this->objectiveFunction(), "objectiveFunction");
    for (int j = 0; j < this->MOTION_ROUNDS; j++) {
      CPUTIMEIT(this->optimizeMotionFields(), "optimizeMotionFields");
      // printf("OBJECTIVE: %f\n", this->objectiveFunction());
    }
    for (int j = 0; j < this->IMG_ROUNDS; j++) {
      CPUTIMEIT(this->optimizeImageComponents(), "optimizeImageComponents");
      // printf("OBJECTIVE: %f\n", this->objectiveFunction());
    }
  }
}

// TODO: probably should move to constructor tbh
void GradientDescent::initializeBuffers() {
  int sz = this->width * this->height;
  int sz2 = sz * this->frames;
  this->VB_gd = (glm::vec2 *) malloc(sz2 * sizeof(glm::vec2));
  this->VO_gd = (glm::vec2 *) malloc(sz2 * sizeof(glm::vec2));
  this->alpha_gd = (float *) malloc(sz * sizeof(float));
  this->imgO_gd = (float *) malloc(sz * sizeof(float));
  this->imgB_gd = (float *) malloc(sz * sizeof(float));
  this->w_1 = (float *) malloc(sz2 * sizeof(float));
  this->w_2 = (float *) malloc(sz2 * sizeof(float));
  this->w_3 = (float *) malloc(sz2 * sizeof(float));

  return;
}

void GradientDescent::freeBuffers() {
  free(this->VB_gd);
  free(this->VO_gd);
  free(this->alpha_gd);
  free(this->imgO_gd);
  free(this->imgB_gd);
  free(this->w_1);
  free(this->w_2);
  free(this->w_3);
  return;
}

static float phi(float t) {
  const float EPSILON_SQ = 1e-2;
  return sqrt(t * t + EPSILON_SQ);
}

void GradientDescent::optimizeImageComponents() {
  // initialize gradients to 0
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      imgO_gd[idx(x, y)] = 0.0f;
      imgB_gd[idx(x, y)] = 0.0f;
      alpha_gd[idx(x, y)] = 0.0f;
    }
  }

  // compute w terms
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        float dataTerm = 
          this->sequence[idx(x, y, t)]
          - this->imgO[warpO(x, y, t)]
          - this->alpha[warpO(x, y, t)] * this->imgB[warpB(x, y, t)];

        dataTerm *= LAMBDA_DT;

        w_1[idx(x, y, t)] = 1.0 / phi(dataTerm);
        glm::vec2 g;
        g = grad(this->imgB, x, y);
        w_2[idx(x, y, t)] = 1.0f / phi(g.x * g.x + g.y * g.y);
        g = grad(this->imgO, x, y);
        w_3[idx(x, y, t)] = 1.0f / phi(g.x * g.x + g.y * g.y);
      }
    }
  }

  // compute actual gradients
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        float dataTerm = 
          this->sequence[idx(x, y, t)]
          - this->imgO[warpO(x, y, t)]
          - this->alpha[warpO(x, y, t)] * this->imgB[warpB(x, y, t)];

        dataTerm *= LAMBDA_DT;

        imgO_gd[warpO(x, y, t)] -= dataTerm * w_1[idx(x, y)];
        alpha_gd[warpO(x, y, t)] -= dataTerm * imgB[warpB(x, y, t)] * w_1[idx(x, y)];
        imgB_gd[warpB(x, y, t)] -= dataTerm * alpha[warpO(x, y, t)] * w_1[idx(x, y)];
      }
    }
  }

  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      alpha_gd[idx(x, y)] += grad(this->alpha, x + 1, y).x + grad(this->alpha, x - 1, y).x + grad(this->alpha, x, y + 1).y + grad(this->alpha, x, y - 1).y;

      glm::vec2 g;

      g = grad(this->imgO, x + 1, y);
      imgO_gd[idx(x, y)] -= this->LAMBDA_2 * w_2[idx(x + 1, y)] * g.x;
      imgO_gd[idx(x, y)] += this->LAMBDA_3 * (g.x * g.x + g.y * g.y) * g.x;

      g = grad(this->imgO, x - 1, y);
      imgO_gd[idx(x, y)] += this->LAMBDA_2 * w_2[idx(x - 1, y)] * g.x;
      imgO_gd[idx(x, y)] += this->LAMBDA_3 * (g.x * g.x + g.y * g.y) * g.x;

      g = grad(this->imgO, x, y + 1);
      imgO_gd[idx(x, y)] -= this->LAMBDA_2 * w_2[idx(x, y + 1)] * g.y;
      imgO_gd[idx(x, y)] += this->LAMBDA_3 * (g.x * g.x + g.y * g.y) * g.y;

      g = grad(this->imgO, x, y - 1);
      imgO_gd[idx(x, y)] += this->LAMBDA_2 * w_2[idx(x, y - 1)] * g.y;
      imgO_gd[idx(x, y)] += this->LAMBDA_3 * (g.x * g.x + g.y * g.y) * g.y;

      g = grad(this->imgB, x + 1, y);
      imgB_gd[idx(x, y)] -= this->LAMBDA_2 * w_3[idx(x + 1, y)] * g.x;

      g = grad(this->imgB, x - 1, y);
      imgB_gd[idx(x, y)] += this->LAMBDA_2 * w_3[idx(x - 1, y)] * g.x;

      g = grad(this->imgB, x, y + 1);
      imgB_gd[idx(x, y)] -= this->LAMBDA_2 * w_3[idx(x, y + 1)] * g.y;

      g = grad(this->imgB, x, y - 1);
      imgB_gd[idx(x, y)] += this->LAMBDA_2 * w_3[idx(x, y - 1)] * g.y;

      if (imgO[idx(x, y)] < 0.0f) {
        imgO_gd[idx(x, y)] += this->LAMBDA_P * imgO[idx(x, y)];
      }
      if (imgB[idx(x, y)] < 0.0f) {
        imgB_gd[idx(x, y)] += this->LAMBDA_P * imgB[idx(x, y)];
      }
      if (alpha[idx(x, y)] < 0.0f) {
        alpha_gd[idx(x, y)] += this->LAMBDA_P * alpha[idx(x, y)];
      }
      if (imgO[idx(x, y)] > 1.0f) {
        imgO_gd[idx(x, y)] -= this->LAMBDA_P * (1.0f - imgO[idx(x, y)]);
      }
      if (imgB[idx(x, y)] > 1.0f) {
        imgB_gd[idx(x, y)] -= this->LAMBDA_P * (1.0f - imgB[idx(x, y)]);
      }
      if (alpha[idx(x, y)] > 1.0f) {
        alpha_gd[idx(x, y)] -= this->LAMBDA_P * (1.0f - alpha[idx(x, y)]);
      }
    }
  }

  // Update based on gradients
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      imgO[idx(x, y)] -= this->LEARNING_RATE1 * imgO_gd[idx(x, y)];
      if (imgO[idx(x, y)] >= 2.0f) {
        imgO[idx(x, y)] = 1.0f;
      } else if (imgO[idx(x, y)] <= -1.0f) {
        imgO[idx(x, y)] = 0.0f;
      }
      //printf("%f\n", imgB_gd[idx(x, y)]);
      imgB[idx(x, y)] -= this->LEARNING_RATE1 * imgB_gd[idx(x, y)];
      if (imgB[idx(x, y)] >= 2.0f) {
        imgB[idx(x, y)] = 1.0f;
      } else if (imgB[idx(x, y)] <= -1.0f) {
        imgB[idx(x, y)] = 0.0f;
      }
      alpha[idx(x, y)] -= this->LEARNING_RATE1 * alpha_gd[idx(x, y)];
      if (alpha[idx(x, y)] >= 2.0f) {
        alpha[idx(x, y)] = 1.0f;
      } else if (alpha[idx(x, y)] <= -1.0f) {
        alpha[idx(x, y)] = 0.0f;
      }
    }
  }
  return;
}

void GradientDescent::optimizeMotionFields() {
  // Initialize gradients to 0
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        this->VB_gd[idx(x, y, t)] = glm::vec2(0.0f, 0.0f);
        this->VO_gd[idx(x, y, t)] = glm::vec2(0.0f, 0.0f);
      }
    }
  }

  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        float dataTerm = 
          this->sequence[idx(x, y, t)]
          - this->imgO[warpO(x, y, t)]
          - this->alpha[warpO(x, y, t)] * this->imgB[warpB(x, y, t)];

        dataTerm *= LAMBDA_DT;

        glm::vec2 g, g2, gx, gy;
        glm::ivec2 wo, wb;

        wo = iwarpO(x, y, t);
        wb = iwarpB(x, y, t);

        g = grad(imgO, wo.x, wo.y);
        VO_gd[idx(x, y, t)] -= dataTerm * g;
        g = grad(imgB, wo.x, wo.y);
        VB_gd[idx(x, y, t)] -= dataTerm * alpha[warpO(x, y, t)] * g;
        VO_gd[idx(x, y, t)] -= dataTerm * imgB[idx(x, y)] * grad(alpha, wo.x, wo.y);
      }
    }
  }
  
  // Update based on gradients
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        VB[idx(x, y, t)] -= this->LEARNING_RATE2 * VB_gd[idx(x, y, t)];
        VO[idx(x, y, t)] -= this->LEARNING_RATE2 * VO_gd[idx(x, y, t)];
      }
    }
  }
  printf("grado(1337) = %f %f\n", VO_gd[1337].x, VO_gd[1337].y);
}

float GradientDescent::objectiveFunction() {
  float ret = 0.0f;
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        float pixRet = 0.0f;
        pixRet += this->sequence[idx(x, y, t)];
        pixRet -= this->imgO[warpO(x, y, t)];
        pixRet -= this->alpha[warpO(x, y, t)] * this->imgB[warpB(x, y, t)];
        ret += fabs(pixRet) * LAMBDA_DT;
      }
    }
  }
  printf("DATA: %f\n", ret);

  float alpha_grad_sq = 0.0f;
  float imgO_grad_abs = 0.0f;
  float imgB_grad_abs = 0.0f;
  float L_IO_IB  = 0.0f;
  for (int y = 0; y < this->height; y++) {
    for (int x = 0; x < this->width; x++) {
      glm::vec2 alpha_grad = this->grad(this->alpha, x, y);
      alpha_grad_sq += alpha_grad.x * alpha_grad.x + alpha_grad.y * alpha_grad.y;

      glm::vec2 imgO_grad = this->grad(this->imgO, x, y);
      glm::vec2 imgB_grad = this->grad(this->imgO, x, y);
      imgO_grad_abs += fabs(imgO_grad.x) + fabs(imgO_grad.y);
      imgB_grad_abs += fabs(imgB_grad.x) + fabs(imgB_grad.y);

      L_IO_IB += 
        (imgO_grad.x * imgO_grad.x + imgO_grad.y * imgO_grad.y) * 
        (imgB_grad.x * imgB_grad.x + imgB_grad.y * imgB_grad.y);
    }
  }
  ret += this->LAMBDA_1 * alpha_grad_sq;
  ret += this->LAMBDA_2 * imgO_grad_abs;
  ret += this->LAMBDA_2 * imgB_grad_abs;
  ret += this->LAMBDA_3 * L_IO_IB;
  
  // TODO: too lazy to do sparsity constraint LOL
  /*float VO_grad_abs = 0.0f;
  float VB_grad_abs = 0.0f;
  for (int t = 0; t < this->frames; t++) {
    for (int y = 0; y < this->height; y++) {
      for (int x = 0; x < this->width; x++) {
        glm::vec2 VO_gradX = gradX(this->VO, x, y, t);
        glm::vec2 VO_gradY = gradY(this->VO, x, y, t);
        glm::vec2 VB_gradX = gradX(this->VB, x, y, t);
        glm::vec2 VB_gradY = gradY(this->VB, x, y, t);
        VO_grad_abs += fabs(VO_gradX.x) + fabs(VO_gradX.y);
        VO_grad_abs += fabs(VO_gradY.x) + fabs(VO_gradY.y);
        VB_grad_abs += fabs(VB_gradX.x) + fabs(VB_gradX.y);
        VB_grad_abs += fabs(VB_gradY.x) + fabs(VB_gradY.y);
      }
    }
  }
  ret += this->LAMBDA_4 * (VO_grad_abs + VB_grad_abs);*/
  return ret;
}

inline int iclamp(int t, int mn, int mx) {
  return min(mx, max(t, mn));
}

int GradientDescent::idx(int x, int y, int t) {
  x = iclamp(x, 0, this->width - 1);
  y = iclamp(y, 0, this->height - 1);
  t = iclamp(t, 0, this->frames - 1);
  return t * this->width * this->height +
    y * this->width + x;
}

glm::ivec2 GradientDescent::iwarpO(int x, int y, int t) {
  glm::vec2 motion = this->VO[idx(x, y, t)];
  x = iclamp((int)motion.x, 0, this->width - 1);
  y = iclamp((int)motion.y, 0, this->height - 1);
  return glm::ivec2(x, y);
}

glm::ivec2 GradientDescent::iwarpB(int x, int y, int t) {
  glm::vec2 motion = this->VB[idx(x, y, t)];
  x = iclamp((int)motion.x, 0, this->width - 1);
  y = iclamp((int)motion.y, 0, this->height - 1);
  return glm::ivec2(x, y);
}

int GradientDescent::warpO(int x, int y, int t) {
  glm::vec2 motion = this->VO[idx(x, y, t)];
  x += motion.x;
  y += motion.y;
  return idx(x, y, 0);
}

int GradientDescent::warpB(int x, int y, int t) {
  glm::vec2 motion = this->VB[idx(x, y, t)];
  x += motion.x;
  y += motion.y;
  return idx(x, y, 0);
}

// TODO: do some caching
glm::vec2 GradientDescent::grad(float * field, int x, int y) {
  float x1 = (x == 0) ? field[idx(x, y)] : field[idx(x - 1, y)];
  float x2 = (x == this->width - 1) ? field[idx(x, y)] : field[idx(x + 1, y)];
  float y1 = (y == 0) ? field[idx(x, y)] : field[idx(x, y - 1)];
  float y2 = (y == this->height - 1) ? field[idx(x, y)] : field[idx(x, y + 1)];
  return glm::vec2(x2 - x1, y2 - y1);
}

glm::vec2 GradientDescent::gradX(glm::vec2 * field, int x, int y, int t) {
  glm::vec2 x1 = (x == 0) ? field[idx(x, y, t)] : field[idx(x - 1, y, t)];
  glm::vec2 x2 = (x == this->width - 1) ? field[idx(x, y, t)] : field[idx(x + 1, y, t)];
  return x2 - x1;
}

glm::vec2 GradientDescent::gradY(glm::vec2 * field, int x, int y, int t) {
  glm::vec2 y1 = (y == 0) ? field[idx(x, y, t)] : field[idx(x, y - 1, t)];
  glm::vec2 y2 = (y == this->height - 1) ? field[idx(x, y, t)] : field[idx(x, y + 1, t)];
  return y2 - y1;
}
