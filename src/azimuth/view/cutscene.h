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
#ifndef AZIMUTH_VIEW_CUTSCENE_H_
#define AZIMUTH_VIEW_CUTSCENE_H_

#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

void az_draw_planet_starfield(az_clock_t clock);

void az_draw_moving_starfield(double time, double speed, double scale);

void az_draw_zenith_planet(az_clock_t clock);

void az_draw_zenith_planet_formation(double blacken, double create,
                                     az_clock_t clock);

void az_draw_planet_debris(az_clock_t clock);

void az_draw_cutscene(const az_space_state_t *state);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_CUTSCENE_H_
