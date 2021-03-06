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

#pragma once
#ifndef AZIMUTH_STATE_SHIP_H_
#define AZIMUTH_STATE_SHIP_H_

#include <stdbool.h>

#include "azimuth/state/player.h"
#include "azimuth/state/uid.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  bool up_held, up_pressed;
  bool down_held, down_pressed;
  bool left_held;
  bool right_held;
  bool fire_held, fire_pressed;
  bool ordn_held;
  bool util_held, util_pressed;
} az_controls_t;

typedef struct {
  az_controls_t controls;
  az_player_t player;
  az_vector_t position; // pixels
  az_vector_t velocity; // pixels/second
  double angle; // radians
  double recharge_cooldown; // seconds until energy recharge can resume
  double gun_charge; // from 0.0 (uncharged) to 1.0 (fully charged)
  double ordn_charge; // from 0.0 (uncharged) to 1.0 (fully charged)
  double ordn_cooldown; // seconds until you can shoot ordnance again
  bool ordn_held;
  double shield_flare; // from 0.0 (nothing) to 1.0 (was just now hit)
  double reactive_flare; // from 0.0 (nothing) to 1.0 (was just now hit)
  double temp_invincibility; // seconds
  enum { AZ_THRUST_NONE = 0, AZ_THRUST_FORWARD, AZ_THRUST_REVERSE } thrusters;
  struct {
    enum { AZ_CPLUS_INACTIVE = 0, AZ_CPLUS_READY, AZ_CPLUS_ACTIVE } state;
    double charge; // from 0.0 (uncharged) to 1.0 (fully charged)
    double tap_time; // seconds left for second tap of key double-tap
  } cplus;
  struct {
    double tap_time; // seconds left for second tap of key double-tap
  } orion;
  struct {
    double active_time; // seconds
    double angle;
    bool locked_on;
  } radar;
  struct {
    bool active;
    az_uid_t node_uid;
    double distance;
  } tractor_beam;
  struct {
    bool active;
    double charge; // from 0.0 (uncharged) to 1.0 (fully charged)
  } tractor_cloak;
  struct {
    bool enabled;
    int8_t thrust; // 1 for forward, -1 for reverse, 0 for none
    bool cplus; // true if C-plus should be engaged
    double goal_angle;
  } autopilot;
} az_ship_t;

extern const az_polygon_t AZ_SHIP_POLYGON;

// Return true if the ship is alive (i.e. not destroyed), false otherwise.
bool az_ship_is_alive(const az_ship_t *ship);

// Return true if the ship is alive and not cloaked, false otherwise.
bool az_ship_is_decloaked(const az_ship_t *ship);

/*===========================================================================*/

// Determine if the specified point overlaps the ship.
bool az_point_touches_ship(const az_ship_t *ship, az_vector_t point);

// Determine if a ray, travelling delta from start, will hit the ship.  If it
// does, stores the intersection point in *point_out (if point_out is non-NULL)
// and the normal vector in *normal_out (if normal_out is non-NULL).
bool az_ray_hits_ship(
    const az_ship_t *ship, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out);

// Determine if a circle with the given radius, travelling delta from start,
// will hit the ship.  If it does, the function stores in *pos_out the earliest
// position of the circle at which it touches the ship (if pos_out is non-NULL)
// and the normal vector in *normal_out (if normal_out is non-NULL).
bool az_circle_hits_ship(
    const az_ship_t *ship, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out);

// Determine if a circle with the given radius, travelling in a circular path
// from start around spin_center by spin_angle radians, will hit the ship.  If
// it does, the function stores in *angle_out the angle travelled by the circle
// until impact (if angle_out is non-NULL), in *pos_out the first position of
// the circle at which it touches the ship (if pos_out is non-NULL), and a
// vector normal to the ship at the impact point in *normal_out (if normal_out
// is non-NULL).
bool az_arc_circle_hits_ship(
    const az_ship_t *ship, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *normal_out);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SHIP_H_
