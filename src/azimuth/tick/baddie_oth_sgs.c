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

#include "azimuth/tick/baddie_oth_sgs.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

enum {
  DOGFIGHT_STATE = 0,
  RETREAT_STATE,
};

// Engine constants:
#define TURN_RATE (AZ_DEG2RAD(330))
#define MAX_SPEED 350.0
#define FORWARD_ACCEL 300.0

/*===========================================================================*/

static int get_primary_state(az_baddie_t *baddie) {
  return baddie->state & 0xff;
}

static int get_secondary_state(az_baddie_t *baddie) {
  return (baddie->state >> 8) & 0xff;
}

static void set_primary_state(az_baddie_t *baddie, int primary) {
  assert(primary >= 0 && primary <= 0xff);
  baddie->state = primary;
}

static void set_secondary_state(az_baddie_t *baddie, int secondary) {
  assert(secondary >= 0 && secondary <= 0xff);
  baddie->state = (baddie->state & 0xff) | (secondary << 8);
}

/*===========================================================================*/

#define TRACTOR_MIN_LENGTH 100.0
#define TRACTOR_MAX_LENGTH 300.0
#define TRACTOR_COMPONENT_INDEX 6
#define TRACTOR_CLOAK_CHARGE_TIME 2.5
#define TRACTOR_CLOAK_DECAY_TIME 5.0

static bool tractor_locked_on(
    const az_baddie_t *baddie, az_vector_t *tractor_pos_out,
    double *tractor_length_out) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const az_component_t *component =
    &baddie->components[TRACTOR_COMPONENT_INDEX];
  const double tractor_length = component->angle;
  if (tractor_length == 0) return false;
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  if (tractor_pos_out != NULL) *tractor_pos_out = component->position;
  if (tractor_length_out != NULL) *tractor_length_out = tractor_length;
  return true;
}

static void set_tractor_node(az_baddie_t *baddie, const az_node_t *node) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (node == NULL) {
    baddie->components[TRACTOR_COMPONENT_INDEX].angle = 0;
    return;
  }
  assert(node->kind == AZ_NODE_TRACTOR);
  const double tractor_length = az_vdist(baddie->position, node->position);
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  baddie->components[TRACTOR_COMPONENT_INDEX].position = node->position;
  baddie->components[TRACTOR_COMPONENT_INDEX].angle = tractor_length;
}

static void decloak_immediately(az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (baddie->param > 1.0) {
    baddie->param = 1.0;
    az_play_sound(&state->soundboard, AZ_SND_CLOAK_END);
  }
}

/*===========================================================================*/

// Fly towards the goal position, getting as close as possible.
static void fly_towards(az_space_state_t *state, az_baddie_t *baddie,
                        double time, az_vector_t goal) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  az_fly_towards_position(state, baddie, time, goal, TURN_RATE, MAX_SPEED,
                          FORWARD_ACCEL, 200, 100);
}

static void begin_dogfight(az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  set_primary_state(baddie, DOGFIGHT_STATE);
  set_secondary_state(baddie, az_randint(10, 20));
  baddie->cooldown = 0.2;
}

static void begin_retreat(az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  set_primary_state(baddie, RETREAT_STATE);
  baddie->cooldown = 5.0;
}

/*===========================================================================*/

void az_tick_bad_oth_supergunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const double old_angle = baddie->angle;

  // Handle tractor beam:
  az_vector_t tractor_position;
  double tractor_length;
  if (tractor_locked_on(baddie, &tractor_position, &tractor_length)) {
    baddie->position = az_vadd(az_vwithlen(az_vsub(baddie->position,
                                                   tractor_position),
                                           tractor_length),
                               tractor_position);
    baddie->velocity = az_vflatten(baddie->velocity,
        az_vsub(baddie->position, tractor_position));
    az_loop_sound(&state->soundboard, AZ_SND_TRACTOR_BEAM);
    baddie->param += time / TRACTOR_CLOAK_CHARGE_TIME;
    if (baddie->param >= 1.0) {
      az_play_sound(&state->soundboard, AZ_SND_CLOAK_BEGIN);
      baddie->param = 1.0 + TRACTOR_CLOAK_DECAY_TIME;
      set_tractor_node(baddie, NULL);
    }
  } else if (baddie->param == 0.0) {
    az_node_t *nearest = NULL;
    double best_dist = INFINITY;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_TRACTOR) continue;
      const double dist = az_vdist(baddie->position, node->position);
      if (dist >= TRACTOR_MIN_LENGTH && dist < best_dist) {
        nearest = node;
        best_dist = dist;
      }
    }
    if (nearest != NULL && best_dist <= TRACTOR_MAX_LENGTH) {
      set_tractor_node(baddie, nearest);
    }
  } else {
    const double old_param = baddie->param;
    baddie->param = fmax(0.0, baddie->param - time);
    if (old_param > 1.0 && baddie->param <= 1.0) {
      az_play_sound(&state->soundboard, AZ_SND_CLOAK_END);
    }
  }
  if (baddie->param >= 1.0) {
    baddie->temp_properties |= AZ_BADF_NO_HOMING;
  }

  switch (get_primary_state(baddie)) {
    case DOGFIGHT_STATE: {
      az_fly_towards_ship(state, baddie, time, TURN_RATE, MAX_SPEED,
                          FORWARD_ACCEL, 200, 200, 100);
      int secondary = get_secondary_state(baddie);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
          az_can_see_ship(state, baddie)) {
        for (int i = -1; i <= 1; ++i) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                    20.0, 0.0, AZ_DEG2RAD(10) * i);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
        baddie->cooldown = 0.2;
        decloak_immediately(state, baddie);
        --secondary;
        if (secondary <= 0) {
          begin_retreat(baddie);
        } else {
          set_secondary_state(baddie, secondary);
        }
      }
    } break;
    case RETREAT_STATE: {
      if (get_secondary_state(baddie) == 0 &&
          az_ship_in_range(state, baddie, 200) &&
          az_ship_within_angle(state, baddie, AZ_PI, AZ_DEG2RAD(60)) &&
          az_baddie_has_clear_path_to_position(state, baddie,
              az_vadd(baddie->position, az_vpolar(500, baddie->angle)))) {
        az_projectile_t *proj = az_fire_baddie_projectile(
            state, baddie, AZ_PROJ_OTH_ORION_BOMB,
            baddie->data->main_body.bounding_radius, AZ_PI, 0);
        if (proj != NULL) {
          az_vpluseq(&proj->velocity, baddie->velocity);
          az_play_sound(&state->soundboard, AZ_SND_ORION_BOOSTER);
        }
        set_secondary_state(baddie, 1);
      }
      // Out of all marker nodes we can see, pick the one farthest from the
      // ship.
      double best_dist = 0.0;
      az_vector_t target = baddie->position;
      AZ_ARRAY_LOOP(node, state->nodes) {
        if (node->kind != AZ_NODE_MARKER) continue;
        if (az_baddie_has_clear_path_to_position(state, baddie,
                                                 node->position)) {
          const double dist = az_vdist(node->position, state->ship.position);
          if (dist > best_dist) {
            best_dist = dist;
            target = node->position;
          }
        }
      }
      // If we get far away enough, or if the cooldown expires, change states.
      // Otherwise, fly towards the target position.
      if (baddie->cooldown <= 0.0 || !az_ship_in_range(state, baddie, 800)) {
        baddie->cooldown = 0.0;
        for (int i = 0; i < 360; i += 15) {
          az_fire_baddie_projectile(
              state, baddie, AZ_PROJ_OTH_SPRAY,
              baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
        decloak_immediately(state, baddie);
        begin_dogfight(baddie);
      } else {
        fly_towards(state, baddie, time, target);
      }
    } break;
    default:
      begin_dogfight(baddie);
      break;
  }

  az_tick_oth_tendrils(state, baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                       time);
}

void az_on_oth_supergunship_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (damage_kind & (AZ_DMGF_ROCKET | AZ_DMGF_BOMB)) {
    decloak_immediately(state, baddie);
    AZ_ARRAY_LOOP(decoy, state->baddies) {
      if (decoy->kind != AZ_BAD_OTH_DECOY) continue;
      az_kill_baddie(state, decoy);
    }
    double theta =
      az_vtheta(az_vrot90ccw(az_vsub(baddie->position, state->ship.position)));
    if (az_randint(0, 1)) theta = -theta;
    baddie->velocity = az_vpolar(250, theta);
    az_baddie_t *decoy = az_add_baddie(state, AZ_BAD_OTH_DECOY,
                                       baddie->position, baddie->angle);
    if (decoy != NULL) {
      decoy->velocity = az_vneg(baddie->velocity);
      decoy->cooldown = 0.5;
    }
  }
}

/*===========================================================================*/

void az_tick_bad_oth_decoy(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_DECOY);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time, TURN_RATE, MAX_SPEED,
                      FORWARD_ACCEL, 200, 200, 100);
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
      az_can_see_ship(state, baddie)) {
    for (int i = -1; i <= 1; ++i) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                20.0, 0.0, AZ_DEG2RAD(10) * i);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
    baddie->cooldown = 0.2;
  }
  az_tick_oth_tendrils(state, baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                       time);
}

/*===========================================================================*/
