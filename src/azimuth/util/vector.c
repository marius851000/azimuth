/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include "azimuth/util/vector.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

/*===========================================================================*/

const az_vector_t AZ_VZERO = {.x = 0, .y = 0};

static bool vfinite(az_vector_t v) {
  return isfinite(v.x) && isfinite(v.y);
}

az_vector_t az_vpolar(double magnitude, double theta) {
  assert(isfinite(magnitude));
  assert(isfinite(theta));
  return (az_vector_t){.x = magnitude * cos(theta),
                       .y = magnitude * sin(theta)};
}

az_vector_t az_vadd(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x + v2.x, .y = v1.y + v2.y};
}

az_vector_t az_vsub(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x - v2.x, .y = v1.y - v2.y};
}

az_vector_t az_vmul(az_vector_t v, double f) {
  return (az_vector_t){.x = v.x * f, .y = v.y * f};
}

az_vector_t az_vdiv(az_vector_t v, double f) {
  assert(f != 0.0);
  return (az_vector_t){.x = v.x / f, .y = v.y / f};
}

double az_vdot(az_vector_t v1, az_vector_t v2) {
  return v1.x * v2.x + v1.y * v2.y;
}

az_vector_t az_vproj(az_vector_t v1, az_vector_t v2) {
  assert(vfinite(v1));
  assert(vfinite(v2));
  const double sqnorm = az_vdot(v2, v2);
  assert(sqnorm > 0.0);
  return az_vmul(v2, az_vdot(v1, v2) / sqnorm);
}

az_vector_t az_vrotate(az_vector_t v, double radians) {
  assert(vfinite(v));
  assert(isfinite(radians));
  const double c = cos(radians);
  const double s = sin(radians);
  return (az_vector_t){.x = v.x * c - v.y * s, .y = v.y * c + v.x * s};
}

az_vector_t az_vrelative(az_vector_t v, az_vector_t pos, double angle) {
  return az_vrotate(az_vsub(v, pos), -angle);
}

double az_vnorm(az_vector_t v) {
  assert(vfinite(v));
  return hypot(v.x, v.y);
}

double az_vtheta(az_vector_t v) {
  assert(vfinite(v));
  return atan2(v.y, v.x);
}

bool az_vwithin(az_vector_t v1, az_vector_t v2, double dist) {
  assert(isfinite(dist));
  return ((v1.x - v2.x) * (v1.x - v2.x) +
          (v1.y - v2.y) * (v1.y - v2.y) <= dist * dist);
}

double az_mod2pi(double theta) {
  assert(isfinite(theta));
  return theta - AZ_TWO_PI * floor((theta + AZ_PI) / AZ_TWO_PI);
}

int az_imin(int a, int b) {
  return a <= b ? a : b;
}

int az_imax(int a, int b) {
  return a > b ? a : b;
}

double az_dmin(double a, double b) {
  return a <= b ? a : b;
}

double az_dmax(double a, double b) {
  return a > b ? a : b;
}

/*===========================================================================*/