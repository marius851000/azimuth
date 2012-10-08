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
#ifndef AZIMUTH_SCREEN_H_
#define AZIMUTH_SCREEN_H_

/*===========================================================================*/

// The dimensions of the screen:
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

// The distance from the center of the screen to the corner:
#define SCREEN_RADIUS 400
#if (SCREEN_WIDTH/2)*(SCREEN_WIDTH/2)+(SCREEN_HEIGHT/2)*(SCREEN_HEIGHT/2) != \
    SCREEN_RADIUS*SCREEN_RADIUS
#error Incorrect SCREEN_RADIUS value.
#endif

/*===========================================================================*/

#endif // AZIMUTH_SCREEN_H_
