#include "kmeans.h"
#include<glm.hpp>
/*void separatePoints(int width, int height, bool * pointGroup1, bool * pointGroup2, bool * sparseMap, glm::ivec2 * points) {
  //glm::vec2 t1 = glm::vec2(0.0f, 0.0f);
  //glm::vec2 t2 = glm::vec2(0.0f, 0.0f);
  float t1 = 0.0f;
  float t2 = 0.0f;
  float ct1 = 0.0f;
  float ct2 = 0.0f;
  int N = width * height;
  for (int i = 0; i < N; i++) {
    if (!sparseMap[i]) continue;
    if (rand() % 2 == 0) {
      t1 += sqrt(points[i].x * points[i].x + points[i].y * points[i].y);
      ct1 += 1.0f;
    } else {
      t2 += sqrt(points[i].x * points[i].x + points[i].y * points[i].y);
      ct2 += 1.0f;
    }
  }
  t1 /= ct1;
  t2 /= ct2;
  const int ITERATIONS = 50;
  for (int j = 0; j < ITERATIONS; j++) {
    memset(pointGroup1, 0, N * sizeof(bool));
    memset(pointGroup2, 0, N * sizeof(bool));
    //glm::vec2 s1 = glm::vec2(0.0f, 0.0f);
    //glm::vec2 s2 = glm::vec2(0.0f, 0.0f);
    float s1 = 0.0f;
    float s2 = 0.0f;
    ct1 = 0.0f;
    ct2 = 0.0f;
    bool doshow = true;
    for (int i = 0; i < N; i++) {
      if (!sparseMap[i]) {
        continue;
      }
      //float d1 = (t1.x - points[i].x) * (t1.x - points[i].x) + (t1.y - points[i].y) * (t1.y - points[i].y);
      //float d2 = (t2.x - points[i].x) * (t2.x - points[i].x) + (t2.y - points[i].y) * (t2.y - points[i].y);
      float d = sqrt(points[i].x * points[i].x + points[i].y * points[i].y);
      if (doshow) {
        printf("%d %f %f\n", i, fabs(d - t1), fabs(d - t2));
        doshow = false;
      }
      if (fabs(d - t1) < fabs(d - t2)) {
        pointGroup1[i] = true;
        s1 += d;
        ct1 += 1.0f;
      } else {
        pointGroup2[i] = true;
        s2 += d;
        ct2 += 1.0f;
      }
      if (ct1 < 0.01f) {
        ct1 = 1.0f;
      }
      if (ct2 < 0.01f) {
        ct2 = 1.0f;
      }
      t1 = s1 / ct1;
      t2 = s2 / ct2;
    }
  }
  return;
}*/
void separatePoints(int width, int height, bool * pointGroup1, bool * pointGroup2, bool * sparseMap, glm::ivec2 * pointDiffs, float THRESHOLD, float ITERATIONS) {
  glm::vec2 td(0.0f, 0.0f);
  glm::vec2 sd;
  float ct = 0.0f;
  int N = width * height;

  // Initialize starting guess by randomly picking half the flows and taking their mean
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (x < 5 || y < 5 || x > width - 5 || y > height - 5) continue;
      int i = y * width + x;
      if (!sparseMap[i]) {
        continue;
      }
      if (rand() % 2 == 0) {
        td += pointDiffs[i];
        ct += 1.0f;
      }
    }
  }
  td /= ct;

  // Every iteration, take points within threshold of the guess and make that the new set
  for (int j = 0; j < ITERATIONS; j++) {
    // Blank stuff out
    memset(pointGroup1, 0, N * sizeof(bool));
    sd = glm::vec2(0.0f, 0.0f);
    ct = 0.0f;

    // For each point, if the sum of squared differences is <= threshold, add to point group
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (x < 5 || y < 5 || x > width - 5 || y > height - 5) continue;
        int i = y * width + x;
        if (!sparseMap[i]) {
          continue;
        }
        glm::vec2 d = td - glm::vec2((float)pointDiffs[i].x, (float)pointDiffs[i].y);
        float delta = d.x * d.x + d.y * d.y;
        if (delta <= THRESHOLD) {
          pointGroup1[i] = true;
          sd += pointDiffs[i];
          ct += 1.0f;
        }
      }
    }
    td = sd / ct;
  }

  // Take points not in first group, set to second group
  memset(pointGroup2, 0, N * sizeof(bool));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * width + x;
      if (x < 5 || y < 5 || x > width - 5 || y > height - 5) {
        if (sparseMap[i]) pointGroup1[i] = true;
      } else if (sparseMap[i] && !pointGroup1[i]) {
        pointGroup2[i] = true;
      }
    }
  }
  return;
}

/*vector<pair<glm::ivec2, glm::ivec2>> copyPoints(int width, int height, bool * mask, glm::ivec2 * points) {
  vector<pair<glm::ivec2, glm::ivec2>> ret;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (mask[y * width + x]) {
        ret.push_back(make_pair(glm::ivec2(x, y), points[y * width + x]));
      }
    }
  }
  return ret;
}*/
