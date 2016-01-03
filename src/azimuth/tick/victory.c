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

#include "azimuth/tick/victory.h"

#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/victory.h"
#include "azimuth/tick/baddie_core.h"
#include "azimuth/tick/baddie_forcefiend.h"
#include "azimuth/tick/baddie_nocturne.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/baddie_wyrm.h"
#include "azimuth/tick/particle.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void fly_towards(az_baddie_t *baddie, az_vector_t goal, double time,
                        double turn_rate, double max_speed,
                        double forward_accel, double lateral_decel_rate) {
  const double backward_accel = 80.0;
  const az_vector_t delta = az_vsub(goal, baddie->position);
  baddie->angle =
    az_angle_towards(baddie->angle, turn_rate * time, az_vtheta(delta));
  const az_vector_t unit = az_vpolar(1, baddie->angle);
  const az_vector_t lateral = az_vflatten(baddie->velocity, unit);
  const az_vector_t dvel =
    az_vadd(az_vcaplen(az_vneg(lateral), lateral_decel_rate * time),
            (az_vdot(unit, delta) >= 0.0 ?
             az_vmul(unit, forward_accel * time) :
             az_vmul(unit, -backward_accel * time)));
  baddie->velocity = az_vcaplen(az_vadd(baddie->velocity, dvel), max_speed);
}

static void tick_baddies(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    const double old_angle = baddie->angle;
    baddie->cooldown = fmax(0.0, baddie->cooldown - time);
    az_vpluseq(&baddie->position, az_vmul(baddie->velocity, time));
    switch (baddie->kind) {
      case AZ_BAD_OTH_SNAPDRAGON: {
        az_vector_t goal = {75, -50};
        if (baddie->state == 1) goal = (az_vector_t){-400, -10};
        if (baddie->state == 2) goal = (az_vector_t){100, -400};
        fly_towards(baddie, goal, time, 5.0, 300, 300, 200);
        az_tick_oth_tendrils(baddie, &AZ_OTH_SNAPDRAGON_TENDRILS, old_angle,
                             time, state->step_timer);
      } break;
      case AZ_BAD_OTH_RAZOR_1: {
        baddie->velocity = az_vpolar(300, baddie->angle);
        az_tick_oth_tendrils(baddie, &AZ_OTH_RAZOR_TENDRILS, old_angle, time,
                             state->step_timer);
      } break;
      case AZ_BAD_ROCKWYRM: {
        az_rockwyrm_move_jaws(baddie, time, (baddie->state != 0));
        const az_vector_t goal = az_vadd((az_vector_t){-360, -180},
                                         az_vmul((az_vector_t){208, 120},
                                                 state->step_timer));
        az_snake_towards(NULL, baddie, time, 2, 240.0, 50.0, goal, true);
      } break;
      case AZ_BAD_WYRM_EGG: {
        if (baddie->cooldown <= 0) {
          az_play_sound(&state->soundboard, baddie->data->death_sound);
          for (int i = 0; i < 360; i += 72) {
            const double theta = AZ_DEG2RAD(i);
            az_victory_add_particle(
                state, AZ_PAR_SHARD, baddie->data->color,
                az_vadd(baddie->position, az_vpolar(5, theta)),
                az_vpolar(20 + 5 * sin(3 * theta), theta), 1.3 * theta,
                0.7 + 0.2 * cos(4 * theta), 1.5 + 0.3 * sin(5 * theta),
                10 * cos(2 * theta));
          }
          az_init_baddie(baddie, AZ_BAD_WYRMLING, baddie->position,
                         baddie->angle);
          baddie->param = AZ_HALF_PI;
        }
      } break;
      case AZ_BAD_WYRMLING: {
        const az_vector_t goal = {-360, -280};
        az_snake_towards(NULL, baddie, time, 0, 180.0, 120.0, goal, true);
      } break;
      case AZ_BAD_OTH_GUNSHIP: {
        if (baddie->state == 0) {
          baddie->velocity = (az_vector_t){1000, 0};
          az_victory_add_particle(
              state, AZ_PAR_OTH_FRAGMENT, (az_color_t){192, 255, 192, 255},
              az_vadd(baddie->position, az_vpolar(-15.0, baddie->angle)),
              AZ_VZERO, 0, 0.3, 16, 0);
        } else {
          az_vector_t goal = {-150, 20};
          if (baddie->state == 2) {
            goal = (az_vector_t){-200, 250 - 120 * state->step_timer};
          } else if (baddie->state == 3) goal = (az_vector_t){100, 300};
          fly_towards(baddie, goal, time, AZ_DEG2RAD(285), 300, 300, 200);
        }
        az_tick_oth_tendrils(baddie, &AZ_OTH_GUNSHIP_TENDRILS, old_angle,
                             time, state->step_timer);
      } break;
      case AZ_BAD_FORCEFIEND: {
        if (baddie->state == 1) {
          baddie->angle = az_angle_towards(
              baddie->angle, AZ_DEG2RAD(180) * time, AZ_DEG2RAD(0));
          az_trail_tail_behind(baddie, 8, AZ_DEG2RAD(30), baddie->position,
                               old_angle);
        } else {
          az_vector_t goal = {-100, 350};
          if (baddie->state == 0) {
            goal = az_vadd((az_vector_t){360, -150},
                           az_vmul((az_vector_t){-300, 100},
                                   state->step_timer));
          }
          az_snake_towards(NULL, baddie, time, 8, 250, 150, goal, true);
        }
        az_forcefiend_move_claws(baddie, false, time);
      } break;
      case AZ_BAD_KILOFUGE: {
        if (baddie->state > 0) {
          baddie->position.x += 100 * time;
        } else if (baddie->position.x > 350) {
          baddie->position.x -= 80 * time;
        }
      } break;
      case AZ_BAD_NOCTURNE: {
        az_nocturne_wiggle_legs(baddie, time, state->step_timer);
        if (baddie->state == 0) {
          baddie->param = fmin(1.0, baddie->param + time / 1.2);
          const az_vector_t goal = {100, 100 - 25 * state->step_timer};
          fly_towards(baddie, goal, time, AZ_DEG2RAD(60), 15, 7, 10);
        } else {
          baddie->param = fmax(0.0, baddie->param - time / 0.8);
        }
      } break;
      case AZ_BAD_MAGBEEST_HEAD: {
        // TODO behavior
      } break;
      case AZ_BAD_OTH_SUPERGUNSHIP: {
        // TODO behavior
        az_vector_t goal = {-300, -60};
        fly_towards(baddie, goal, time, AZ_DEG2RAD(285), 300, 300, 200);
        az_tick_oth_tendrils(baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                             time, state->step_timer);
      } break;
      case AZ_BAD_OTH_DECOY: {
        // TODO behavior
        az_vector_t goal = {-300, -60};
        fly_towards(baddie, goal, time, AZ_DEG2RAD(285), 300, 300, 200);
        az_tick_oth_tendrils(baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                             time, state->step_timer);
      } break;
      case AZ_BAD_ZENITH_CORE: {
        if (baddie->state < 2) {
          baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(30) * time);
        }
        if (baddie->state == 1) {
          az_zenith_core_adjust_to_beam_configuration(baddie, time);
        } else az_zenith_core_adjust_to_closed_configuration(baddie, time);
      } break;
      default: break;
    }
  }
}

/*===========================================================================*/

static void tick_particles(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    az_tick_particle(particle, time);
  }
}

/*===========================================================================*/

static bool times_per_second(double per_second, const az_projectile_t *proj,
                             double time) {
  return (ceil(per_second * proj->age) >
          ceil(per_second * (proj->age - time)));
}

static void projectile_special_logic(
    az_victory_state_t *state, az_projectile_t *proj, double time) {
  switch (proj->kind) {
    case AZ_PROJ_BOUNCING_FIREBALL: {
      if (times_per_second(15, proj, time)) {
        az_victory_add_particle(
            state, AZ_PAR_EMBER, (az_color_t){255, 128, 0, 128},
            proj->position, AZ_VZERO, proj->angle, 0.3, 15.0, 0.0);
      }
    } break;
    case AZ_PROJ_FORCE_WAVE: {
      const double new_speed = az_vnorm(proj->velocity) + 700 * time;
      if (proj->age >= 0.5) {
        proj->velocity.y = 0;
        proj->angle = 0;
      }
      proj->velocity = az_vwithlen(proj->velocity, new_speed);
      AZ_ARRAY_LOOP(baddie, state->baddies) {
        if (baddie->kind != AZ_BAD_FORCE_EGG) continue;
        if (az_vwithin(baddie->position, proj->position, 40)) {
          az_vpluseq(&baddie->velocity, az_vmul(proj->velocity, 5 * time));
        }
      }
    } break;
    case AZ_PROJ_GRAVITY_TORPEDO: {
      az_victory_add_particle(
          state, AZ_PAR_EXPLOSION, (az_color_t){128, 128, 255, 255},
          proj->position, AZ_VZERO, proj->angle, 0.5, 5, 0);
    } break;
    case AZ_PROJ_OTH_BULLET: {
      if (times_per_second(30, proj, time)) {
        az_victory_add_particle(state, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                                proj->position, AZ_VZERO, proj->angle, 0.1,
                                5.0, AZ_DEG2RAD(360));
      }
    } break;
    case AZ_PROJ_OTH_MINIROCKET: {
      az_victory_add_particle(state, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                              proj->position, AZ_VZERO, proj->angle, 0.3,
                              6.0, AZ_DEG2RAD(720));
    } break;
    case AZ_PROJ_OTH_ROCKET: {
      az_victory_add_particle(state, AZ_PAR_OTH_FRAGMENT, AZ_WHITE,
                              proj->position, AZ_VZERO, proj->angle, 0.5,
                              9.0, AZ_DEG2RAD(720));
    } break;
    default: break;
  }
}

static void tick_projectiles(az_victory_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    proj->age += time;
    projectile_special_logic(state, proj, time);
    if (proj->age > proj->data->lifetime) proj->kind = AZ_PROJ_NOTHING;
    else az_vpluseq(&proj->position, az_vmul(proj->velocity, time));
  }
}

/*===========================================================================*/

static bool timer_at(const az_victory_state_t *state, double mark,
                     double time) {
  return (state->step_timer >= mark && state->step_timer - time < mark);
}

static bool next_step_at(az_victory_state_t *state, double mark) {
  if (state->step_timer >= mark) {
    ++state->step;
    state->step_timer = 0.0;
    az_victory_clear_objects(state);
    return true;
  } else return false;
}

void az_tick_victory_state(az_victory_state_t *state, double time) {
  ++state->clock;
  tick_particles(state, time);
  tick_projectiles(state, time);
  tick_baddies(state, time);
  state->step_timer += time;
  az_baddie_t *boss = &state->baddies[0];
  switch (state->step) {
    case AZ_VS_START: {
      next_step_at(state, 1.0);
    } break;
    case AZ_VS_SNAPDRAGON: {
      // Fly in from the lower-left corner of the screen:
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_OTH_SNAPDRAGON,
                       (az_vector_t){-360, -180}, AZ_DEG2RAD(30));
        boss->velocity = az_vpolar(300, boss->angle);
      }
      // Turn around and fire an Oth rocket to the left:
      if (timer_at(state, 1.7, time)) boss->state = 1;
      if (timer_at(state, 2.5, time)) {
        az_victory_add_projectile(state, AZ_PROJ_OTH_ROCKET,
                                  az_vadd(boss->position,
                                          az_vpolar(30.0, boss->angle)),
                                  boss->angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      }
      // Turn downwards to fly off the bottom of the screen, and drop three Oth
      // Razors, which will fly off the top of the screen:
      if (timer_at(state, 3.0, time)) boss->state = 2;
      if (timer_at(state, 3.5, time)) {
        for (int i = -1; i <= 1; ++i) {
          az_victory_add_baddie(state, AZ_BAD_OTH_RAZOR_1, boss->position,
                                boss->angle + AZ_PI + i * AZ_DEG2RAD(45));
        }
        az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
      }
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_ROCKWYRM: {
      // Slither in from the bottom-left corner of the screen:
      if (timer_at(state, 0.1, time)) {
        az_init_baddie(boss, AZ_BAD_ROCKWYRM,
                       (az_vector_t){-360, -180}, AZ_DEG2RAD(30));
        boss->param = AZ_HALF_PI;
      }
      // Open jaws and fire a stinger spread:
      if (timer_at(state, 1.3, time)) boss->state = 1;
      if (timer_at(state, 1.8, time)) {
        for (int i = -2; i <= 2; ++i) {
          const double radius = boss->data->main_body.bounding_radius;
          const double theta = boss->angle + i * AZ_DEG2RAD(10);
          az_victory_add_projectile(
              state, AZ_PROJ_STINGER,
              az_vadd(boss->position, az_vpolar(radius, theta)), theta);
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
        boss->state = 0;
      }
      // Drop two eggs behind:
      if (timer_at(state, 3.0, time)) {
        for (int i = 0; i < 2; ++i) {
          az_baddie_t *egg = &state->baddies[i + 1];
          const az_vector_t position =
            az_vadd(boss->position, az_vrotate(boss->components[11].position,
                                               boss->angle));
          const double angle = AZ_DEG2RAD(195) + AZ_DEG2RAD(65) * i;
          az_init_baddie(egg, AZ_BAD_WYRM_EGG, position, angle);
          egg->velocity = az_vpolar(50, egg->angle);
          egg->cooldown = 1.0 + 0.75 * i;
        }
      }
      next_step_at(state, 6.5);
    } break;
    case AZ_VS_GUNSHIP: {
      // C-plus dash in from the left side of the screen:
      if (timer_at(state, 1.0, time)) {
        az_init_baddie(boss, AZ_BAD_OTH_GUNSHIP,
                       (az_vector_t){-360, 5}, AZ_DEG2RAD(0));
        az_play_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
      }
      // Stop C-plus and turn around:
      if (timer_at(state, 1.4, time)) boss->state = 1;
      if (timer_at(state, 2.4, time)) boss->state = 2;
      // FIre off some Oth bullets and rockets:
      if (timer_at(state, 2.5, time) || timer_at(state, 2.7, time)) {
        for (int i = -1; i <= 1; ++i) {
          az_victory_add_projectile(
              state, AZ_PROJ_OTH_BULLET,
              az_vadd(boss->position, az_vpolar(18, boss->angle)),
              az_mod2pi(boss->angle + i * AZ_DEG2RAD(10)));
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
      }
      if (timer_at(state, 2.9, time) || timer_at(state, 3.1, time)) {
        az_victory_add_projectile(
            state, AZ_PROJ_OTH_MINIROCKET,
            az_vadd(boss->position, az_vpolar(18, boss->angle)), boss->angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_MINIROCKET);
      }
      if (timer_at(state, 3.3, time)) {
        az_victory_add_projectile(
            state, AZ_PROJ_OTH_ROCKET,
            az_vadd(boss->position, az_vpolar(18, boss->angle)), boss->angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
      }
      // Fly off the top of the screen:
      if (timer_at(state, 3.5, time)) boss->state = 3;
      next_step_at(state, 6.5);
    } break;
    case AZ_VS_FORCEFIEND: {
      // Swim in from the right side of the screen:
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_FORCEFIEND,
                       (az_vector_t){360, -150}, AZ_DEG2RAD(150));
        boss->param = AZ_HALF_PI;
      }
      // Fire a gravity torpedo spread, then drop an egg:
      if (timer_at(state, 1.3, time)) {
        for (int i = -1; i <= 1; ++i) {
          az_victory_add_projectile(
              state, AZ_PROJ_GRAVITY_TORPEDO,
              az_vadd(boss->position, az_vpolar(23, boss->angle)),
              az_mod2pi(boss->angle + i * AZ_DEG2RAD(20)));
        }
        az_play_sound(&state->soundboard, AZ_SND_FIRE_GRAVITY_TORPEDO);
      }
      if (timer_at(state, 1.6, time)) {
        az_victory_add_baddie(state, AZ_BAD_FORCE_EGG, boss->position,
                              boss->angle);
      }
      // Turn around and fire some force waves (which will push the egg off the
      // right side of the screen):
      if (timer_at(state, 2.6, time)) boss->state = 1;
      if (timer_at(state, 3.6, time) ||
          timer_at(state, 3.9, time) ||
          timer_at(state, 4.2, time)) {
        az_victory_add_projectile(state, AZ_PROJ_FORCE_WAVE, boss->position,
                                  AZ_DEG2RAD(50) * sin(state->step_timer * 8));
        az_play_sound(&state->soundboard, AZ_SND_FIRE_FORCE_WAVE);
      }
      // Swim off the top of the screen:
      if (timer_at(state, 4.7, time)) boss->state = 2;
      next_step_at(state, 7.0);
    } break;
    case AZ_VS_KILOFUGE: {
      // TODO: One step in, fire ice torp, second step in, brief meltbeam, then
      // quick slide out.
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_KILOFUGE,
                       (az_vector_t){520, 0}, AZ_DEG2RAD(180));
      }
      if (timer_at(state, 4.0, time)) boss->state = 1;
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_NOCTURNE: {
      // Phase in:
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_NOCTURNE,
                       (az_vector_t){-100, 50}, AZ_DEG2RAD(60));
      }
      // Fire off some nightseeds:
      if (timer_at(state, 2.0, time) || timer_at(state, 2.66667, time) ||
          timer_at(state, 3.33333, time)) {
        az_victory_add_projectile(
            state, AZ_PROJ_NIGHTSEED,
            az_vadd(boss->position, az_vpolar(45, boss->angle)),
            boss->angle);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_SEED_SINGLE);
      }
      // Launch fireball spread and phase out:
      if (timer_at(state, 4.0, time)) {
        boss->state = 1;
        for (int i = 0; i < 360; i += 36) {
          const double theta = boss->angle + AZ_DEG2RAD(i);
          az_victory_add_projectile(
              state, AZ_PROJ_BOUNCING_FIREBALL,
              az_vadd(boss->position, az_vpolar(20, theta)), theta);
        }
        az_play_sound(&state->soundboard, AZ_SND_EXPLODE_FIREBALL_LARGE);
      }
      next_step_at(state, 6.5);
    } break;
    case AZ_VS_MAGBEEST: {
      // TODO: Push down from top of screen in spider mode.  drop a magma bomb
      // (which will explode in lava spurts off the bottom of the screen);
      // meanwhile gatling guns fires and magnet does something (but no metal
      // shards; there's not enough time).  Finally, whole thing pulls back up
      // offscreen.
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_MAGBEEST_HEAD,
                       (az_vector_t){0, 50}, AZ_DEG2RAD(-90));
        az_init_baddie(&state->baddies[1], AZ_BAD_MAGBEEST_LEGS_L,
                       (az_vector_t){200, 50}, AZ_DEG2RAD(-90));
        az_init_baddie(&state->baddies[2], AZ_BAD_MAGBEEST_LEGS_R,
                       (az_vector_t){-200, 50}, AZ_DEG2RAD(-90));
      }
      next_step_at(state, 6.0);
    } break;
    case AZ_VS_SUPERGUNSHIP: {
      // TODO: choreography
      az_baddie_t *decoy = &state->baddies[1];
      if (timer_at(state, 0.5, time)) {
        az_init_baddie(boss, AZ_BAD_OTH_SUPERGUNSHIP,
                       (az_vector_t){340, -50}, AZ_DEG2RAD(180));
      }
      if (timer_at(state, 2.5, time)) {
        az_init_baddie(decoy, AZ_BAD_OTH_DECOY, boss->position, boss->angle);
        boss->velocity = (az_vector_t){-100, 200};
        decoy->velocity = (az_vector_t){-100, -200};
      }
      if (next_step_at(state, 6.0)) {
        az_init_baddie(boss, AZ_BAD_ZENITH_CORE, AZ_VZERO, 0);
      }
    } break;
    case AZ_VS_CORE: {
      if (timer_at(state, 1.0, time)) boss->state = 1;
      if (timer_at(state, 4.0, time)) boss->state = 0;
      if (next_step_at(state, 6.0)) {
        az_init_baddie(boss, AZ_BAD_ZENITH_CORE, AZ_VZERO, AZ_DEG2RAD(180));
        boss->state = 2;
        az_play_sound(&state->soundboard, AZ_SND_BOSS_SHAKE);
      }
    } break;
    case AZ_VS_EXPLODE: {
      if (timer_at(state, 1.5, time)) {
        az_play_sound(&state->soundboard, AZ_SND_BOSS_EXPLODE);
      }
      next_step_at(state, 2.5);
    } break;
    case AZ_VS_DONE: break;
  }
}

/*===========================================================================*/
