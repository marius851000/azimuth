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

#include "azimuth/tick/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void drift_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_speed, double ship_force, double wall_force) {
  // Determine the drift vector.
  const az_vector_t pos = baddie->position;
  az_vector_t drift = AZ_VZERO;
  if (az_ship_is_present(&state->ship)) {
    drift = az_vwithlen(az_vsub(state->ship.position, pos), ship_force);
  }
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    const az_vector_t delta = az_vsub(pos, door->position);
    const double dist = az_vnorm(delta) - AZ_DOOR_BOUNDING_RADIUS -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      drift = az_vadd(drift, az_vwithlen(delta, 2 * wall_force));
    } else drift = az_vadd(drift, az_vwithlen(delta, wall_force * exp(-dist)));
  }
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    const az_vector_t delta = az_vsub(pos, wall->position);
    const double dist = az_vnorm(delta) - wall->data->bounding_radius -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      drift = az_vadd(drift, az_vwithlen(delta, 2 * wall_force));
    } else drift = az_vadd(drift, az_vwithlen(delta, wall_force * exp(-dist)));
  }
  // Drift along the drift vector.
  baddie->velocity =
    az_vcaplen(az_vadd(baddie->velocity, az_vmul(drift, time)), max_speed);
}

/*===========================================================================*/

// How long it takes a baddie's armor flare to die down, in seconds:
#define AZ_BADDIE_ARMOR_FLARE_TIME 0.3
// How long it takes a baddie to unfreeze, in seconds.
#define AZ_BADDIE_THAW_TIME 8.0

static void tick_baddie(az_space_state_t *state, az_baddie_t *baddie,
                        double time) {
  // Allow the armor flare to die down a bit.
  assert(baddie->armor_flare >= 0.0);
  assert(baddie->armor_flare <= 1.0);
  baddie->armor_flare =
    fmax(0.0, baddie->armor_flare - time / AZ_BADDIE_ARMOR_FLARE_TIME);
  // Allow the baddie to thaw a bit.
  assert(baddie->frozen >= 0.0);
  assert(baddie->frozen <= 1.0);
  baddie->frozen = fmax(0.0, baddie->frozen - time / AZ_BADDIE_THAW_TIME);
  if (baddie->frozen > 0.0) {
    baddie->velocity = AZ_VZERO;
    return;
  }
  // Cool down the baddie's weapon.
  baddie->cooldown = fmax(0.0, baddie->cooldown - time);
  // Apply velocity.
  baddie->position = az_vadd(baddie->position,
                             az_vmul(baddie->velocity, time));
  // Perform kind-specific logic.
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_LUMP:
      baddie->angle = az_mod2pi(baddie->angle + 3.0 * time);
      break;
    case AZ_BAD_TURRET:
      // Aim gun:
      baddie->components[0].angle =
        fmax(-1.0, fmin(1.0, az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, 2.0 * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                       baddie->angle)));
      // Fire:
      if (baddie->cooldown <= 0.0 &&
          fabs(az_mod2pi(baddie->angle + baddie->components[0].angle -
                         az_vtheta(az_vsub(state->ship.position,
                                           baddie->position)))) < 0.1) {
        az_projectile_t *proj;
        if (az_insert_projectile(state, &proj)) {
          const double angle = baddie->angle + baddie->components[0].angle;
          az_init_projectile(proj, AZ_PROJ_GUN_NORMAL, true,
                             az_vadd(baddie->position, az_vpolar(20.0, angle)),
                             angle);
          baddie->cooldown = 0.5;
        }
      }
      break;
    case AZ_BAD_ZIPPER:
      {
        az_impact_t impact;
        az_ray_impact(state, baddie->position, az_vpolar(20.0, baddie->angle),
                      AZ_IMPF_BADDIE, baddie->uid, &impact);
        if (impact.type != AZ_IMP_NOTHING) {
          baddie->angle = az_mod2pi(baddie->angle + AZ_PI);
        }
        baddie->velocity = az_vpolar(200.0, baddie->angle);
      }
      break;
    case AZ_BAD_BOUNCER:
      {
        az_impact_t impact;
        az_circle_impact(state, 15.0, baddie->position,
                         az_vpolar(150.0 * time, baddie->angle),
                         AZ_IMPF_BADDIE, baddie->uid, &impact);
        const double normal_theta = az_vtheta(impact.normal);
        baddie->position =
          az_vadd(impact.position, az_vpolar(0.01, normal_theta));
        if (impact.type != AZ_IMP_NOTHING) {
          baddie->angle = az_mod2pi(2.0 * normal_theta -
                                    baddie->angle + AZ_PI);
        }
      }
      break;
    case AZ_BAD_ATOM:
      drift_towards_ship(state, baddie, time, 70, 100, 100);
      for (int i = 0; i < baddie->data->num_components; ++i) {
        az_component_t *component = &baddie->components[i];
        component->angle = az_mod2pi(component->angle + 3.5 * time);
        component->position = az_vpolar(4.0, component->angle);
        component->position.x *= 5.0;
        component->position =
          az_vrotate(component->position, i * AZ_DEG2RAD(120));
      }
      break;
    case AZ_BAD_SPINER:
      drift_towards_ship(state, baddie, time, 40, 10, 100);
      baddie->angle = az_mod2pi(baddie->angle + 0.4 * time);
      if (baddie->cooldown <= 0.0 &&
          az_vwithin(baddie->position, state->ship.position, 200)) {
        az_projectile_t *proj;
        for (int i = 0; i < 360; i += 45) {
          if (az_insert_projectile(state, &proj)) {
            const double angle = baddie->angle + AZ_DEG2RAD(i);
            az_init_projectile(proj, AZ_PROJ_SPINE, true,
                               az_vadd(baddie->position,
                                       az_vpolar(18.0, angle)), angle);
          }
        }
        baddie->cooldown = 2.0;
      }
      break;
    case AZ_BAD_CLAM:
      {
        const double ship_theta =
          az_vtheta(az_vsub(state->ship.position, baddie->position));
        baddie->angle =
          az_angle_towards(baddie->angle, 0.2 * time, ship_theta);
        const double max_angle = AZ_DEG2RAD(40);
        const double old_angle = baddie->components[0].angle;
        const double new_angle =
          (baddie->cooldown <= 0.0 &&
           fabs(az_mod2pi(baddie->angle - ship_theta)) < AZ_DEG2RAD(10) ?
           fmin(max_angle, max_angle -
                (max_angle - old_angle) * pow(0.00003, time)) :
           fmax(0.0, old_angle - 1.0 * time));
        baddie->components[0].angle = new_angle;
        baddie->components[1].angle = -new_angle;
        if (baddie->cooldown <= 0.0 && new_angle >= 0.95 * max_angle) {
          az_projectile_t *proj;
          for (int i = -1; i <= 1; ++i) {
            if (az_insert_projectile(state, &proj)) {
              const double angle = baddie->angle + AZ_DEG2RAD(i * 5);
              az_init_projectile(
                  proj, (i == 0 ? AZ_PROJ_FIREBALL_SLOW :
                         AZ_PROJ_FIREBALL_FAST), true,
                  az_vadd(baddie->position, az_vpolar(
                      0.5 * baddie->data->main_body.bounding_radius, angle)),
                  angle);
            }
          }
          baddie->cooldown = 3.0;
        }
      }
      break;
    case AZ_BAD_BOX:
    case AZ_BAD_ARMORED_BOX:
      // Do nothing.
      break;
  }
}

void az_tick_baddies(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    assert(baddie->health > 0.0);
    tick_baddie(state, baddie, time);
  }
}

/*===========================================================================*/
