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

#include "azimuth/state/sound.h"

#include <assert.h>
#include <stdlib.h>

#include "azimuth/util/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/sound.h"

/*===========================================================================*/

static az_sound_spec_t sound_specs[] = {
  [AZ_SND_ALARM] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.835, .env_decay = 0.24,
    .start_freq = 0.385, .freq_slide = 0.12, .freq_delta_slide = 0.1,
    .repeat_speed = 0.035, .phaser_sweep = -0.15,
    .lpf_cutoff = 0.32, .lpf_resonance = -0.43,
    .hpf_cutoff = 0.3, .hpf_ramp = -0.000113648
  },
  [AZ_SND_AZIMUTH_AWAKEN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.555, .env_decay = 0.92,
    .start_freq = 0.855, .freq_slide = 0.13, .freq_delta_slide = -0.31,
    .square_duty = 0.245, .repeat_speed = 0.155,
    .phaser_offset = -0.74, .phaser_sweep = 0.25
  },
  [AZ_SND_BEAM_FREEZE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 1.0, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253
  },
  [AZ_SND_BEAM_NORMAL] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.2943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PHASE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.3443662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BEAM_PIERCE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.3943662,
    .vibrato_depth = 0.1056338, .vibrato_speed = 0.57042253,
    .square_duty = 0.35298
  },
  [AZ_SND_BLINK_MEGA_BOMB] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.119718313217,
    .env_decay = 0.0704225376248,
    .start_freq = 0.41549295187,
    .square_duty = 0.19812,
  },
  [AZ_SND_BOSS_EXPLODE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.09, .env_sustain = 0.86,
    .env_punch = 0.3, .env_decay = 0.8,
    .start_freq = 0.105, .freq_slide = 0.15, .freq_delta_slide = -0.05,
    .vibrato_depth = 0.525, .vibrato_speed = 0.425,
    .arp_mod = 0.825791, .arp_speed = 0.04,
    .repeat_speed = 0.02, .phaser_sweep = 0.05,
    .lpf_cutoff = 0.0191205, .lpf_ramp = 0.136981, .lpf_resonance = 0.158773,
    .hpf_cutoff = 0.215, .hpf_ramp = 0.01, .volume_adjust = 0.5
  },
  [AZ_SND_BOSS_SHAKE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.905, .env_sustain = 0.04, .env_decay = 0.17,
    .start_freq = 0.2, .freq_slide = 0.05,
    .vibrato_depth = 0.05, .vibrato_speed = 0.6, .volume_adjust = -0.25
  },
  [AZ_SND_BOUNCE_FIREBALL] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_decay = 0.15, .start_freq = 0.28,
    .square_duty = 0.86, .hpf_cutoff = 0.00528305
  },
  [AZ_SND_CHARGED_GUN] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.323943674564,
    .start_freq = 0.232394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.33098590374,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGED_MISSILE_BEAM] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_decay = 0.563380300999,
    .start_freq = 0.373239427805, .freq_slide = 0.1549295187,
    .vibrato_depth = 0.760563373566, .vibrato_speed = 0.66197180748,
    .phaser_offset = 0.718309879303, .phaser_sweep = 0.436619758606,
    .lpf_cutoff = 0.6768399, .hpf_cutoff = 0.036
  },
  [AZ_SND_CHARGED_ORDNANCE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.6,
    .start_freq = 0.202394367456, .freq_slide = 0.004733738,
    .vibrato_depth = 0.33098590374, .vibrato_speed = 0.25,
    .arp_speed = -0.4154, .square_duty = -0.8286, .phaser_offset = -0.7323943,
    .volume_adjust = -0.462962989807
  },
  [AZ_SND_CHARGING_GUN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 1.0, .env_sustain = 0.34507, .env_decay = 0.246478870511,
    .start_freq = 0.1197183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511
  },
  [AZ_SND_CHARGING_ORDNANCE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.2, .env_sustain = 1.0,
    .start_freq = 0.0897183, .freq_slide = 0.118591,
    .freq_delta_slide = 0.056338,
    .square_duty = 0.40368, .duty_sweep = 0.0140844583511,
    .volume_adjust = -0.4
  },
  [AZ_SND_CLOAK_BEGIN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.27622,
    .start_freq = 0.146478870511, .freq_slide = 0.12,
    .vibrato_depth = 0.2, .vibrato_speed = 0.2,
    .square_duty = 0.53694, .volume_adjust = -0.7
  },
  [AZ_SND_CLOAK_END] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.27622,
    .start_freq = 0.201408463717, .freq_slide = -0.18,
    .vibrato_depth = 0.2, .vibrato_speed = 0.2,
    .square_duty = 0.53694, .volume_adjust = -0.7
  },
  [AZ_SND_CORE_BEAM_CHARGE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.15, .env_sustain = 0.65, .env_decay = 0.31,
    .start_freq = 0.35, .freq_slide = 0.12, .freq_delta_slide = 0.05,
    .vibrato_depth = 0.08, .vibrato_speed = 0.57,
    .phaser_offset = -0.63, .phaser_sweep = 0.06
  },
  [AZ_SND_CORE_BEAM_FIRE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.1971831, .start_freq = 0.38,
    .vibrato_depth = 0.105, .vibrato_speed = 0.57,
    .phaser_offset = 0.42, .phaser_sweep = 0.06, .volume_adjust = -0.5
  },
  [AZ_SND_CPLUS_ACTIVE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.2827, .env_punch = 0.61646, .env_decay = 1.0,
    .start_freq = 0.443661957979, .freq_delta_slide = -0.25352114439,
    .vibrato_depth = 0.27069, .vibrato_speed = 0.00366,
    .arp_mod = 0.253521084785, .arp_speed = 0.65505,
    .repeat_speed = 0.732394337654,
    .phaser_offset = -0.04278001, .phaser_sweep = -0.112676084042
  },
  [AZ_SND_CPLUS_CHARGED] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.6126761, .env_punch = 0.08549776, .env_decay = 0.204225,
    .start_freq = 0.84507, .freq_slide = 0.00128, .freq_delta_slide = 0.11465,
    .vibrato_depth = 0.4195169, .vibrato_speed = -0.39,
    .arp_mod = 0.1044, .arp_speed = -0.5518,
    .square_duty = 0.189, .duty_sweep = 0.01924883,
    .phaser_offset = -0.4636848, .phaser_sweep = 0.02882149,
    .lpf_cutoff = 0.5151533, .lpf_ramp = 0.4815216, .lpf_resonance = 0.312,
    .hpf_cutoff = 0.6101004, .hpf_ramp = -0.01561149, .volume_adjust = -0.222
  },
  [AZ_SND_CPLUS_IMPACT] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.33175, .env_punch = 0.53942, .env_decay = 0.788732409477,
    .start_freq = 0.1128691, .freq_slide = 0.05820001,
    .phaser_offset = 0.47598, .phaser_sweep = -0.25746
  },
  [AZ_SND_CPLUS_READY] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.6, .env_punch = 0.1355418,
    .start_freq = 0.5106531, .freq_delta_slide = 0.5334117,
    .vibrato_depth = -0.0002874958, .vibrato_speed = -0.161,
    .arp_mod = 0.9934, .arp_speed = -0.331,
    .square_duty = 0.8678, .duty_sweep = 0.000924009, .repeat_speed = 0.663,
    .phaser_offset = 0.0006630559, .phaser_sweep = -0.3438828,
    .lpf_cutoff = 0.4502442, .lpf_resonance = -0.4142,
    .hpf_cutoff = 0.0008809352, .hpf_ramp = -0.2542205, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_CLOSE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.401408463717, .freq_slide = -0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DOOR_OPEN] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.352112680674, .env_decay = 0.12622,
    .start_freq = 0.246478870511, .freq_slide = 0.197183,
    .square_duty = 0.53694, .volume_adjust = -0.5
  },
  [AZ_SND_DRILLING] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.74, .start_freq = 0.1056338,
    .vibrato_depth = 0.640845, .vibrato_speed = 0.56338,
    .volume_adjust = -0.5
  },
  [AZ_SND_DROP_BOMB] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.15671, .env_decay = 0.00384,
    .start_freq = 0.380281686783, .square_duty = 0.36672,
    .hpf_cutoff = 0.1, .volume_adjust = -0.314814834595
  },
  [AZ_SND_ELECTRICITY] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.55, .env_sustain = 1.0, .env_punch = 0.5, .env_decay = 0.3,
    .start_freq = 0.356815, .freq_delta_slide = 0.1,
    .vibrato_depth = 0.0768938, .vibrato_speed = 0.948503,
    .arp_mod = 0.05, .arp_speed = 0.155206, .repeat_speed = 0.51,
    .phaser_offset = 0.589003, .phaser_sweep = -0.07,
    .lpf_resonance = 0.664601, .hpf_cutoff = 0.0240503, .hpf_ramp = 0.01,
    .volume_adjust = -0.7,
  },
  [AZ_SND_ENTER_ATMOSPHERE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.275, .env_sustain = 0.225,
    .env_punch = 0.075, .env_decay = 1.0,
    .start_freq = 0.32, .freq_slide = -0.2, .repeat_speed = 0.755,
    .phaser_offset = -0.31, .phaser_sweep = -0.04, .volume_adjust = -0.4
  },
  [AZ_SND_ERUPTION] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.215, .env_decay = 0.55, .start_freq = 0.11,
    .vibrato_depth = 0.15, .vibrato_speed = 0.57, .repeat_speed = 0.23027,
    .phaser_offset = -0.3, .phaser_sweep = 0.59, .lpf_cutoff = 0.85
  },
  [AZ_SND_EXPAND_TRINE_TORPEDO] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.005, .env_decay = 0.63,
    .start_freq = 0.33, .freq_slide = 0.12, .freq_delta_slide = 0.05,
    .vibrato_depth = 0.115, .vibrato_speed = 0.595,
    .phaser_offset = 0.79, .phaser_sweep = -0.22, .volume_adjust = 0.5
  },
  [AZ_SND_EXPLODE_BOMB] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.32941, .env_punch = 0.572, .env_decay = 0.23305,
    .start_freq = 0.1548265, .freq_slide = -0.36946,
    .vibrato_depth = 0.63161, .vibrato_speed = 0.38466,
    .phaser_offset = 0.12768, .phaser_sweep = -0.28872
  },
  [AZ_SND_EXPLODE_FIREBALL_LARGE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.536445, .env_punch = 0.375285, .env_decay = 0.40906,
    .start_freq = 0.0733247, .freq_slide = 0.141984,
    .freq_delta_slide = -0.255949,
    .vibrato_depth = 0.018219, .vibrato_speed = 0.480838,
    .arp_mod = -0.727803, .arp_speed = 0.498527, .repeat_speed = 0.858864,
    .phaser_offset = -0.0352152, .phaser_sweep = 0.794248,
    .lpf_cutoff = 0.815116, .lpf_ramp = -0.051331, .lpf_resonance = 0.653169
  },
  [AZ_SND_EXPLODE_FIREBALL_SMALL] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.325, .env_punch = 0.724659, .env_decay = 0.36,
    .start_freq = 0.115, .freq_slide = -0.17, .repeat_speed = 0.543458,
    .phaser_offset = -0.43, .phaser_sweep = 0.11
  },
  [AZ_SND_EXPLODE_HYPER_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.31483, .env_punch = 0.26024, .env_decay = 0.4112,
    .start_freq = 0.161971837282, .repeat_speed = 0.3807,
    .volume_adjust = 0.333333435059
  },
  [AZ_SND_EXPLODE_MEGA_BOMB] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.4366197, .env_punch = 0.443, .env_decay = 0.3309859,
    .start_freq = 0.161395, .freq_slide = -0.0140845179558,
    .arp_mod = -0.5244799, .arp_speed = 0.74647885561, .repeat_speed = 0.59345
  },
  [AZ_SND_EXPLODE_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.29941, .env_punch = 0.69464, .env_decay = 0.3665,
    .start_freq = 0.6008111, .freq_slide = -0.25694,
    .repeat_speed = 0.7266001, .volume_adjust = 0.222222213745
  },
  [AZ_SND_EXPLODE_SHIP] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.852112650, .env_punch = 1.0, .env_decay = 0.767605662,
    .start_freq = 0.112676054239, .freq_slide = -0.0985915660858,
    .vibrato_depth = 0.281690150499, .vibrato_speed = 0.12924
  },
  [AZ_SND_FIRE_FIREBALL] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.25, .env_decay = 0.25,
    .start_freq = 0.55, .freq_slide = -0.144864, .freq_delta_slide = -0.079236,
    .phaser_sweep = 0.05,
    .lpf_cutoff = 0.24889, .lpf_ramp = -0.700087, .lpf_resonance = 0.381467,
    .hpf_cutoff = 0.000101726, .hpf_ramp = 0.198532
  },
  [AZ_SND_FIRE_FORCE_WAVE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.35, .env_decay = 0.645,
    .start_freq = 0.275, .freq_slide = -0.19,
    .arp_mod = 0.91, .arp_speed = 0.08,
    .phaser_offset = 0.72, .phaser_sweep = -0.11, .lpf_cutoff = 0.25
  },
  [AZ_SND_FIRE_GRAVITY_TORPEDO] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.3, .env_punch = 0.485, .env_decay = 0.7,
    .start_freq = 0.33, .vibrato_depth = 0.165, .vibrato_speed = 0.645,
    .arp_mod = 0.45, .arp_speed = 0.545, .duty_sweep = 0.05,
    .phaser_offset = -0.88, .phaser_sweep = 0.1, .volume_adjust = 0.25
  },
  [AZ_SND_FIRE_GUN_CHARGED_BEAM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.36394, .env_decay = 0.274647891521,
    .start_freq = 0.1338, .freq_slide = 0.2676, .freq_delta_slide = -0.140845,
    .phaser_offset = -0.323943674564, .phaser_sweep = 0.436619758606
  },
  [AZ_SND_FIRE_GUN_CHARGED_FREEZE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_attack = 0.065, .env_sustain = 0.25,
    .env_punch = 0.64, .env_decay = 0.63,
    .start_freq = 0.515, .freq_limit = 0.065,
    .freq_slide = -0.21, .freq_delta_slide = 0.1,
    .vibrato_depth = 0.16, .vibrato_speed = 0.495,
    .arp_mod = 0.8, .arp_speed = 0.605,
    .phaser_sweep = -0.5, .hpf_cutoff = 0.19, .volume_adjust = 0.34
  },
  [AZ_SND_FIRE_GUN_CHARGED_NORMAL] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.266189, .env_punch = 0.305, .env_decay = 0.67,
    .start_freq = 0.505, .freq_limit = 0.095, .freq_slide = -0.24,
    .vibrato_depth = 0.135, .vibrato_speed = 0.83, .hpf_cutoff = 0.175
  },
  [AZ_SND_FIRE_GUN_FREEZE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.13848, .env_punch = 0.12036, .env_decay = 0.21536,
    .start_freq = 0.9, .freq_limit = 0.32917, .freq_slide = -0.22554,
    .phaser_offset = -0.5, .hpf_cutoff = 0.007470001
  },
  [AZ_SND_FIRE_GUN_NORMAL] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.10346, .env_punch = 0.16836, .env_decay = 0.18956,
    .start_freq = 0.89034, .freq_limit = 0.08224, .freq_slide = -0.61922,
    .square_duty = 0.2347, .duty_sweep = 0.0176, .hpf_cutoff = 0.25965
  },
  [AZ_SND_FIRE_GUN_CHARGED_PHASE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.0333715, .env_punch = 0.19, .env_decay = 0.74,
    .start_freq = 0.615, .freq_slide = -0.54, .freq_delta_slide = -0.2,
    .vibrato_depth = 0.12, .vibrato_speed = 0.241312,
    .arp_mod = 0.453932, .arp_speed = 0.119284,
    .square_duty = 0.745, .duty_sweep = -0.46, .repeat_speed = 0.812712,
    .phaser_offset = -0.112878, .phaser_sweep = -0.1774,
    .lpf_cutoff = 0.281177, .lpf_ramp = 0.122311, .lpf_resonance = 0.809696,
    .hpf_cutoff = 0.283753, .hpf_ramp = -0.482735
  },
  [AZ_SND_FIRE_GUN_CHARGED_PIERCE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.07187, .env_sustain = 0.055,
    .env_punch = 0.0481498, .env_decay = 0.55,
    .start_freq = 0.95, .freq_slide = -0.37, .freq_delta_slide = -0.99,
    .vibrato_depth = 0.497075, .vibrato_speed = 0.658875,
    .arp_mod = -0.05, .arp_speed = 0.87, .repeat_speed = 0.745,
    .phaser_offset = 0.00186753, .phaser_sweep = -0.12,
    .lpf_cutoff = 0.642199, .lpf_ramp = -0.00728074, .lpf_resonance = 0.518503,
    .hpf_cutoff = 0.21998
  },
  [AZ_SND_FIRE_GUN_PIERCE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.23076, .env_decay = 0.05284,
    .start_freq = 0.7278, .freq_limit = 0.29536, .freq_slide = -0.29848,
    .square_duty = 0.46265, .duty_sweep = -0.18305
  },
  [AZ_SND_FIRE_HYPER_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.3, .env_punch = 0.25, .env_decay = 0.5,
    .start_freq = 0.2, .vibrato_depth = 0.5, .vibrato_speed = 0.75,
    .repeat_speed = 0.55, .phaser_offset = 0.25, .phaser_sweep = -0.25
  },
  [AZ_SND_FIRE_ICE_TORPEDO] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.085, .env_decay = 0.695,
    .start_freq = 0.25, .freq_slide = -0.19,
    .phaser_offset = 0.72, .phaser_sweep = -0.11, .lpf_cutoff = 0.25
  },
  [AZ_SND_FIRE_LASER_PULSE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.11852, .env_punch = 0.132, .env_decay = 0.183098584414,
    .start_freq = 0.535211265087, .freq_limit = 0.2, .freq_slide = -0.26748,
    .square_duty = 0.18045, .duty_sweep = 0.18344
  },
  [AZ_SND_FIRE_MISSILE_BEAM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.03084, .env_decay = 0.66197180748,
    .start_freq = 0.929577469826, .freq_slide = -0.0140845179558,
    .vibrato_depth = 0.704225361347, .vibrato_speed = 0.739436626434,
    .phaser_offset = 0.352112650871, .phaser_sweep = 0.549295783043,
    .volume_adjust = 0.4
  },
  [AZ_SND_FIRE_OTH_MINIROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.395, .env_decay = 0.605,
    .start_freq = 0.73, .freq_slide = -0.01,
    .arp_mod = 0.7, .arp_speed = 0.73,
    .phaser_offset = -0.18, .phaser_sweep = -0.17,
    .lpf_cutoff = 0.675, .lpf_ramp = -0.23, .lpf_resonance = 0.7,
    .volume_adjust = 0.5
  },
  [AZ_SND_FIRE_OTH_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.605633795261, .env_punch = 0.160833105445,
    .env_decay = 0.732394337654, .start_freq = 0.345070421696,
    .freq_slide = -0.00747528579086, .freq_delta_slide = -0.00656020781025,
    .arp_mod = 0.704225301743, .arp_speed = 0.732394337654,
    .phaser_offset = -0.16901409626, .phaser_sweep = -0.563380241394,
    .lpf_cutoff = 0.795774638653, .lpf_ramp = -0.126760542393,
    .lpf_resonance = 0.718309879303, .volume_adjust = 0.6
  },
  [AZ_SND_FIRE_OTH_SPRAY] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.0563380271196, .env_sustain = 0.309859156609,
    .env_punch = 0.0774647891521, .env_decay = 0.598591566086,
    .start_freq = 0.387323945761, .freq_slide = 0.0563380718231,
    .freq_delta_slide = -0.08450704813,
    .vibrato_depth = 0.0774647891521, .vibrato_speed = 0.654929578304,
    .arp_mod = -0.140845060349,
    .square_duty = 0.570422530174, .duty_sweep = -0.281690120697,
    .repeat_speed = 0.612676084042, .phaser_sweep = 0.774647831917,
    .lpf_cutoff = 0.718309879303, .lpf_ramp = -0.0098278503865,
    .lpf_resonance = 0.366197168827
  },
  [AZ_SND_FIRE_ROCKET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.39907, .env_punch = 0.65582, .env_decay = 0.39085,
    .start_freq = 0.7881, .freq_slide = -0.30088,
    .vibrato_depth = 0.59703, .vibrato_speed = 0.03828,
    .phaser_offset = -0.09300001, .phaser_sweep = -0.19305,
    .volume_adjust = -0.42592590332
  },
  [AZ_SND_FIRE_SEED_MULTI] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.11, .env_decay = 0.4,
    .start_freq = 0.24, .freq_slide = 0.14, .freq_delta_slide = -0.28,
    .repeat_speed = 0.495, .phaser_offset = -0.46, .phaser_sweep = -0.221755,
    .volume_adjust = 0.9
  },
  [AZ_SND_FIRE_SEED_SINGLE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.11, .env_decay = 0.345,
    .start_freq = 0.24, .freq_slide = 0.14, .freq_delta_slide = -0.28,
    .phaser_offset = -0.46, .phaser_sweep = -0.221755, .volume_adjust = 0.9
  },
  [AZ_SND_FIRE_STINGER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.2112676, .env_punch = 0.2112676, .env_decay = 0.28873238,
    .start_freq = 0.3653719, .freq_slide = 0.49295771122
  },
  [AZ_SND_FIRE_TRINE_TORPEDO] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.345, .env_decay = 0.75, .start_freq = 0.23,
    .vibrato_depth = 0.2353, .vibrato_speed = 0.483778, .volume_adjust = -0.5
  },
  [AZ_SND_FREEZE_BADDIE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.175, .env_decay = 0.405,
    .start_freq = 0.9, .freq_slide = -0.44, .freq_delta_slide = -0.24,
    .vibrato_depth = 0.00203862, .vibrato_speed = 0.0908909,
    .repeat_speed = 0.895, .phaser_offset = -0.00555425, .phaser_sweep = -0.21,
    .lpf_cutoff = 0.305, .lpf_ramp = -0.324387, .lpf_resonance = -0.124724,
    .hpf_cutoff = 0.00798911, .hpf_ramp = -0.0795003
  },
  [AZ_SND_GLASS_BREAK] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.213974, .env_punch = 0.365633, .env_decay = 0.8,
    .start_freq = 0.765, .vibrato_depth = 0.76, .vibrato_speed = 0.455,
    .arp_mod = 0.83, .arp_speed = 0.846137, .repeat_speed = 0.305,
    .phaser_offset = -0.01, .phaser_sweep = 0.01
  },
  [AZ_SND_GRAVITY_TORPEDO_IMPACT] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_decay = 0.47, .start_freq = 0.12, .freq_slide = -0.07,
    .phaser_offset = 0.65, .phaser_sweep = -0.17, .volume_adjust = 1.0
  },
  [AZ_SND_HEAT_DAMAGE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.605633795261, .freq_slide = 0.591549277306,
    .freq_delta_slide = -0.943661987782, .repeat_speed = 0.0352112688124,
    .phaser_offset = 0.309859156609, .phaser_sweep = -0.33802819252,
    .hpf_cutoff = 0.133802816272, .volume_adjust = -0.2
  },
  [AZ_SND_HIT_ARMOR] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.3, .start_freq = 0.86, .phaser_offset = -0.65,
    .lpf_cutoff = 0.75, .lpf_ramp = -0.66, .lpf_resonance = 0.5
  },
  [AZ_SND_HIT_WALL] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.03245, .env_decay = 0.10518,
    .start_freq = 0.43334, .freq_slide = -0.57352
  },
  [AZ_SND_HURT_BOUNCER] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.08, .env_sustain = 0.130676,
    .env_punch = 0.211366, .env_decay = 0.215,
    .start_freq = 0.1, .freq_slide = 0.41, .freq_delta_slide = -0.35,
    .arp_mod = 0.23, .arp_speed = 0.787091, .repeat_speed = 0.637637,
    .lpf_cutoff = 0.81, .lpf_resonance = 0.65, .hpf_cutoff = 0.31
  },
  [AZ_SND_HURT_CRAWLER] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_attack = 0.00138677, .env_sustain = 0.065, .env_decay = 0.2,
    .start_freq = 0.675, .freq_slide = -0.767741, .freq_delta_slide = 0.044613,
    .vibrato_depth = 0.861827, .vibrato_speed = 0.237579,
    .arp_mod = 0.970858, .arp_speed = 0.106144, .repeat_speed = 0.955,
    .phaser_offset = 0.0158204, .phaser_sweep = -0.518164,
    .lpf_cutoff = 0.57278, .lpf_ramp = 0.435716, .lpf_resonance = -0.375799,
    .hpf_cutoff = 0.0229528
  },
  [AZ_SND_HURT_FISH] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_attack = 0.0289518, .env_sustain = 0.165,
    .env_punch = 0.167419, .env_decay = 0.225,
    .start_freq = 0.685, .freq_slide = -0.02766, .freq_delta_slide = 0.739225,
    .vibrato_depth = 0.605578, .vibrato_speed = 0.366937,
    .arp_mod = -0.781294, .arp_speed = 0.896652, .repeat_speed = 0.0247399,
    .phaser_offset = 0.224987, .phaser_sweep = 0.00464832,
    .lpf_cutoff = 0.900347, .lpf_ramp = -0.644483, .lpf_resonance = 0.423428,
    .hpf_cutoff = 0.285813, .hpf_ramp = 0.760594, .volume_adjust = 0.5
  },
  [AZ_SND_HURT_KILOFUGE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.1, .env_decay = 0.4,
    .start_freq = 0.5, .freq_slide = -0.02674, .freq_delta_slide = 0.0555,
    .vibrato_depth = 0.480421, .vibrato_speed = 0.285732,
    .arp_mod = -0.403854, .arp_speed = 0.710617, .repeat_speed = 0.339842,
    .phaser_offset = 0.100114, .phaser_sweep = 0.000345079,
    .lpf_cutoff = 0.807726, .lpf_ramp = -0.064732, .lpf_resonance = -0.333281,
    .hpf_cutoff = 0.278757, .hpf_ramp = 0.0216441
  },
  [AZ_SND_HURT_NOCTURNE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_decay = 0.5,
    .start_freq = 0.195, .freq_slide = 0.31, .freq_delta_slide = -0.29,
    .vibrato_depth = 0.305, .vibrato_speed = 0.75, .hpf_cutoff = 0.4
  },
  [AZ_SND_HURT_OTH] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.355, .env_punch = 0.000476076, .env_decay = 0.35,
    .start_freq = 0.455, .freq_slide = 0.25, .freq_delta_slide = -0.28,
    .vibrato_depth = 0.78, .vibrato_speed = 0.625,
    .arp_mod = 0.54, .arp_speed = 0.595,
    .phaser_offset = -0.43, .hpf_cutoff = 0.485, .volume_adjust = 0.5
  },
  [AZ_SND_HURT_PLANT] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_punch = 0.164633, .env_decay = 0.28,
    .start_freq = 0.255, .freq_slide = -0.39, .phaser_sweep = -0.67,
    .lpf_cutoff = 0.785, .lpf_resonance = -0.9, .hpf_cutoff = 0.23
  },
  [AZ_SND_HURT_ROCKWYRM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.00378904, .env_sustain = 0.04,
    .env_punch = 0.14, .env_decay = 0.555,
    .start_freq = 0.5, .freq_slide = 0.16, .freq_delta_slide = 0.24,
    .vibrato_depth = 0.227081, .vibrato_speed = 0.776884,
    .repeat_speed = 0.555, .phaser_offset = -0.31, .phaser_sweep = -0.2,
    .lpf_cutoff = 0.472522, .lpf_ramp = 0.12, .lpf_resonance = -0.77,
    .hpf_ramp = 0.00133439
  },
  [AZ_SND_HURT_SHIP] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.045, .env_punch = 0.28, .env_decay = 0.25,
    .start_freq = 0.22, .freq_slide = -0.24, .freq_delta_slide = 0.37,
    .vibrato_depth = 0.29, .vibrato_speed = 0.665,
    .phaser_offset = 0.25, .phaser_sweep = 0.24, .volume_adjust = -0.2
  },
  [AZ_SND_HURT_SHIP_SLIGHTLY] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.045, .env_punch = 0.28, .env_decay = 0.15,
    .start_freq = 0.16, .freq_slide = -0.24, .freq_delta_slide = 0.37,
    .vibrato_depth = 0.29, .vibrato_speed = 0.665,
    .phaser_offset = 0.25, .phaser_sweep = 0.24, .volume_adjust = -0.46
  },
  [AZ_SND_HURT_SWOOPER] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_attack = 0.111202, .env_sustain = 0.0282855,
    .env_punch = 0.269045, .env_decay = 0.395,
    .start_freq = 0.34, .freq_slide = -0.218567, .freq_delta_slide = 0.069207,
    .vibrato_depth = 0.334984, .vibrato_speed = 0.52311,
    .arp_mod = 0.833634, .arp_speed = 0.94336, .repeat_speed = 0.0475345,
    .phaser_offset = 0.564119, .phaser_sweep = -0.570054,
    .lpf_cutoff = 0.275482, .lpf_ramp = 0.518367, .lpf_resonance = -0.89505,
    .hpf_cutoff = 0.778376
  },
  [AZ_SND_HURT_TURRET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_decay = 0.185, .start_freq = 0.2, .freq_slide = -0.1
  },
  [AZ_SND_HURT_ZIPPER] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.148994, .env_sustain = 0.0012423, .env_decay = 0.125,
    .start_freq = 0.1, .freq_slide = -0.0500972, .freq_delta_slide = 0.287,
    .vibrato_depth = 0.102431, .vibrato_speed = 0.823938,
    .arp_mod = 0.860993, .arp_speed = 0.852278, .repeat_speed = 0.235746,
    .phaser_offset = -0.265713, .phaser_sweep = 0.793911,
    .lpf_cutoff = 0.466273, .lpf_ramp = 0.176426, .lpf_resonance = 0.367827,
    .hpf_cutoff = 0.0446594, .volume_adjust = -0.5
  },
  [AZ_SND_IMPACT_CHARGED_SHOT] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.201129, .env_decay = 0.22,
    .start_freq = 0.25, .freq_slide = -0.22,
    .vibrato_depth = 0.215, .vibrato_speed = 0.57, .phaser_sweep = -0.25
  },
  [AZ_SND_KILL_ATOM] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.211267605424, .env_punch = 0.405718,
    .env_decay = 0.605633795261, .start_freq = 0.7841117,
    .freq_slide = -0.236357, .freq_delta_slide = -0.9898345,
    .vibrato_depth = 0.04728087, .vibrato_speed = -0.4434,
    .arp_mod = 0.6122, .arp_speed = 0.2002,
    .square_duty = -0.3186, .duty_sweep = -0.4922905,
    .repeat_speed = 0.4604,
    .phaser_offset = 0.6059253, .phaser_sweep = -0.3536932,
    .lpf_cutoff = 0.982626, .lpf_ramp = 0.2353423, .lpf_resonance = 0.8618,
    .volume_adjust = 0.8
  },
  [AZ_SND_KILL_BOUNCER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.18, .env_punch = 0.797245, .env_decay = 0.419678,
    .start_freq = 0.065, .freq_slide = -0.08,
    .arp_mod = 0.544511, .arp_speed = 0.673332,
    .lpf_cutoff = 0.55, .lpf_ramp = -0.25
  },
  [AZ_SND_KILL_DRAGONFLY] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.03548157, .env_sustain = 0.378881,
    .env_punch = 0.3252649, .env_decay = -0.36613,
    .start_freq = 0.05789758,
    .freq_slide = 0.5313399, .freq_delta_slide = -0.1058238,
    .vibrato_depth = 0.9644304, .vibrato_speed = 0.9172,
    .arp_mod = 0.00940001, .arp_speed = 0.281,
    .square_duty = -0.1706, .duty_sweep = 0.585956,
    .repeat_speed = 0.8049999,
    .phaser_offset = -0.5914348, .phaser_sweep = -0.8174004,
    .lpf_cutoff = 0.8984792, .lpf_ramp = 0.3468362, .lpf_resonance = -0.007,
    .hpf_cutoff = 0.1794088, .hpf_ramp = -0.1530325,
    .volume_adjust = 0.3
  },
  [AZ_SND_KILL_FIRE_CRAWLER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.42, .env_sustain = 0.0987764,
    .env_punch = 0.0323949, .env_decay = 0.665,
    .start_freq = 0.055, .freq_slide = 0.203787, .freq_delta_slide = 0.611536,
    .vibrato_depth = 0.075261, .vibrato_speed = 0.999019,
    .arp_mod = 0.0173007, .arp_speed = 0.886639, .repeat_speed = 0.734126,
    .lpf_cutoff = 0.361873, .lpf_ramp = 0.322418,
    .lpf_resonance = 0.708243, .hpf_ramp = 0.950342
  },
  [AZ_SND_KILL_FISH] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_attack = 0.34, .env_sustain = 0.09,
    .env_punch = 0.11, .env_decay = 0.55,
    .start_freq = 0.125, .freq_slide = -0.0029153, .freq_delta_slide = 0.72224,
    .vibrato_depth = 0.00973633, .vibrato_speed = 0.849479,
    .arp_mod = -0.628656, .arp_speed = 0.590676, .repeat_speed = 0.490141,
    .phaser_offset = 0.232152, .phaser_sweep = -0.101336,
    .lpf_cutoff = 0.264579, .lpf_ramp = 0.0444092,
    .lpf_resonance = -0.207566, .hpf_cutoff = 0.00052807,
    .hpf_ramp = 0.000260741, .volume_adjust = -0.48
  },
  [AZ_SND_KILL_OTH] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.000185765, .env_sustain = 0.14,
    .env_punch = 0.0539325, .env_decay = 0.48,
    .start_freq = 0.245, .freq_slide = -0.15, .freq_delta_slide = 0.0125571,
    .vibrato_depth = 0.409668, .vibrato_speed = 0.480537,
    .arp_mod = -0.212992, .arp_speed = 0.622925, .repeat_speed = 0.503951,
    .phaser_offset = 0.557146, .phaser_sweep = -0.0230222,
    .lpf_cutoff = 0.701672, .lpf_ramp = 0.410088, .lpf_resonance = 0.741268,
    .hpf_cutoff = 0.226537, .hpf_ramp = 0.0183639, .volume_adjust = 0.7
  },
  [AZ_SND_KILL_PLANT] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.13, .env_punch = 0.34, .env_decay = 0.46,
    .start_freq = 0.255, .freq_slide = -0.25,
    .vibrato_depth = 0.32, .vibrato_speed = 0.535,
    .phaser_offset = 0.81, .phaser_sweep = -0.67,
    .lpf_cutoff = 0.785, .lpf_resonance = -0.9, .hpf_cutoff = 0.23
  },
  [AZ_SND_KILL_SWOOPER] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_attack = 0.105, .env_sustain = 0.08,
    .env_punch = 0.585, .env_decay = 0.51,
    .start_freq = 0.34, .freq_slide = 0.09, .freq_delta_slide = 0.04,
    .vibrato_depth = 0.334984, .vibrato_speed = 0.52311,
    .arp_mod = 0.833634, .arp_speed = 0.94336,
    .phaser_offset = 0.564119, .phaser_sweep = -0.570054,
    .lpf_cutoff = 0.275482, .lpf_ramp = 0.518367, .lpf_resonance = -0.89505,
    .hpf_cutoff = 0.778376, .volume_adjust = 0.52
  },
  [AZ_SND_KILL_TURRET] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.10687, .env_punch = 0.24554, .env_decay = 0.463,
    .start_freq = 0.0972442, .freq_slide = 0.05804, .repeat_speed = 0.56475,
  },
  [AZ_SND_KLAXON_COUNTDOWN] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.55, .start_freq = 0.25, .freq_slide = 0.112676,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.24
  },
  [AZ_SND_KLAXON_COUNTDOWN_LOW] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.45, .start_freq = 0.32394, .freq_slide = 0.112676,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.19
  },
  [AZ_SND_KLAXON_SHIELDS_LOW] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.112676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.24
  },
  [AZ_SND_KLAXON_SHIELDS_VERY_LOW] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.4507, .start_freq = 0.32394,
    .freq_slide = 0.212676, .freq_delta_slide = -0.197183,
    .square_duty = 0.873239, .phaser_offset = 0.40845,
    .volume_adjust = -0.19
  },
  [AZ_SND_LAUNCH_OTH_RAZORS] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = -0.0173337701708, .env_sustain = 0.450704216957,
    .env_punch = 0.0839145034552, .env_decay = 0.133802816272,
    .start_freq = 0.211267605424, .freq_delta_slide = -0.464788734913,
    .vibrato_speed = 0.725352108479,
    .arp_mod = -0.0140845179558, .arp_speed = -0.966199994087,
    .repeat_speed = 0.58450704813, .phaser_sweep = 0.563380241394,
    .lpf_cutoff = 0.781690120697, .lpf_ramp = -0.464788734913,
    .lpf_resonance = 0.274647891521
  },
  [AZ_SND_LIGHTS_FLICKER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_decay = 0.115, .start_freq = 0.825, .freq_slide = -0.3,
    .phaser_offset = -0.7, .phaser_sweep = -0.04, .volume_adjust = -0.5
  },
  [AZ_SND_LIGHTS_ON] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.128036, .env_decay = 0.835,
    .start_freq = 0.145, .freq_slide = 0.05,
    .arp_mod = 0.4, .arp_speed = 0.53,
    .phaser_offset = -0.52, .phaser_sweep = -0.31
  },
  [AZ_SND_MAGBEEST_LEG_STOMP] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.4, .start_freq = 0.06,
    .vibrato_depth = 0.15, .vibrato_speed = 0.115,
    .phaser_offset = 0.7, .phaser_sweep = -0.37,
    .lpf_cutoff = 0.75, .lpf_ramp = -0.66, .lpf_resonance = 0.5,
    .volume_adjust = 0.75
  },
  [AZ_SND_MAGBEEST_MAGMA_BOMB] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_sustain = 0.0463, .env_decay = 0.175, .start_freq = 0.469
  },
  [AZ_SND_MAGBEEST_MAGNET] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.5, .start_freq = 0.2,
    .vibrato_depth = 0.23, .vibrato_speed = 0.39, .phaser_offset = 0.71
  },
  [AZ_SND_MAGBEEST_MAGNET_CHARGE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.975, .env_decay = 0.3,
    .start_freq = 0.2, .freq_slide = 0.13,
    .vibrato_depth = 0.23, .vibrato_speed = 0.65, .phaser_offset = 0.71
  },
  [AZ_SND_MAGBEEST_TUNNELLING] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.25, .start_freq = 0.1,
    .vibrato_depth = 0.645, .vibrato_speed = 0.58,
    .phaser_offset = -0.65, .lpf_cutoff = 0.8, .volume_adjust = -0.5
  },
  [AZ_SND_MENU_CLICK] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.0891609, .env_sustain = 0.155,
    .env_punch = 0.0656827, .env_decay = 0.23,
    .start_freq = 0.19, .freq_slide = 0.00708617,
    .freq_delta_slide = -0.0759178,
    .vibrato_depth = 0.00914664, .vibrato_speed = 0.881547,
    .arp_mod = -0.67998, .arp_speed = 0.284788, .repeat_speed = 0.431267,
    .phaser_offset = -0.0525242, .phaser_sweep = -0.0142853,
    .lpf_cutoff = 0.103765, .lpf_ramp = 0.240105, .lpf_resonance = 0.173952,
  },
  [AZ_SND_MENU_HOVER] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.0891609, .env_sustain = 0.155,
    .env_punch = 0.0656827, .env_decay = 0.23,
    .start_freq = 0.215, .freq_slide = 0.00708617,
    .freq_delta_slide = -0.0759178,
    .vibrato_depth = 0.00914664, .vibrato_speed = 0.881547,
    .arp_mod = -0.67998, .arp_speed = 0.284788, .repeat_speed = 0.431267,
    .phaser_offset = -0.0525242, .phaser_sweep = -0.0142853,
    .lpf_cutoff = 0.103765, .lpf_ramp = 0.240105, .lpf_resonance = 0.173952,
  },
  [AZ_SND_METAL_CLINK] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_decay = 0.260563373566, .start_freq = 0.407042229176,
    .phaser_offset = -0.197183, .lpf_cutoff = 0.2253521, .lpf_resonance = 1.0
  },
  [AZ_SND_MINOR_UPGRADE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_sustain = 0.0336143, .env_punch = 0.625, .env_decay = 0.97,
    .start_freq = 0.72, .arp_mod = -0.51, .arp_speed = 0.65,
    .repeat_speed = 0.545, .phaser_offset = -0.42, .phaser_sweep = 0.4
  },
  [AZ_SND_NPS_PORTAL] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.8, .env_sustain = 0.178638,
    .env_punch = 0.21187, .env_decay = 0.8,
    .start_freq = 0.25, .freq_slide = 0.13, .freq_delta_slide = 0.561365,
    .vibrato_depth = 0.450008, .vibrato_speed = 0.441515,
    .arp_mod = 0.11, .arp_speed = 0.456977, .repeat_speed = 0.416536,
    .phaser_offset = 0.08, .phaser_sweep = 0.62,
    .hpf_cutoff = 0.0775991, .hpf_ramp = -0.781411
  },
  [AZ_SND_ORDN_ACTIVATE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.1,
    .start_freq = 0.3, .freq_slide = 0.6,
    .lpf_cutoff = 0.735, .volume_adjust = -0.5
  },
  [AZ_SND_ORDN_DEACTIVATE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.1,
    .start_freq = 0.7, .freq_slide = -0.6,
    .lpf_cutoff = 0.735, .volume_adjust = -0.5
  },
  [AZ_SND_ORION_BOOSTER] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.338028, .env_punch = 0.4225352, .env_decay = 0.6,
    .start_freq = 0.190140843391, .freq_slide = 0.19992,
    .vibrato_depth = 0.39781, .vibrato_speed = 0.06258,
    .phaser_offset = 0.746478915215, .phaser_sweep = -0.0704225301743
  },
  [AZ_SND_OTH_SCREAM] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.2, .env_sustain = 1.0,
    .env_punch = 0.000476076, .env_decay = 1.0,
    .start_freq = 0.5, .freq_slide = 0.08, .freq_delta_slide = -0.05,
    .vibrato_depth = 0.78, .vibrato_speed = 0.625,
    .arp_mod = 0.54, .arp_speed = 0.595,
    .phaser_offset = -0.43, .hpf_cutoff = 0.485, .volume_adjust = 0.5
  },
  [AZ_SND_PICKUP_ORDNANCE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.17572, .env_decay = 0.2746479,
    .start_freq = 0.26918, .freq_slide = 0.25928,
    .square_duty = 0.01332, .repeat_speed = 0.69412
  },
  [AZ_SND_PICKUP_SHIELDS] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.1376, .env_decay = 0.288732379675,
    .start_freq = 0.225352108479, .freq_slide = 0.2832,
    .repeat_speed = 0.542253494263
  },
  [AZ_SND_PISTON_MOVEMENT] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.155, .env_decay = 0.45,
    .start_freq = 0.94, .freq_delta_slide = 0.33,
    .phaser_offset = 0.650736, .phaser_sweep = 0.04,
    .volume_adjust = -0.4
  },
  [AZ_SND_PLANET_DEFORM] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_attack = 0.985, .env_sustain = 0.535, .env_decay = 0.965,
    .start_freq = 0.08, .freq_slide = 0.10, .freq_delta_slide = -0.02,
    .vibrato_depth = 0.145, .vibrato_speed = 0.55,
    .square_duty = 0.92, .duty_sweep = -0.12,
    .phaser_sweep = 0.09, .lpf_cutoff = 0.00252298, .volume_adjust = 0.5
  },
  [AZ_SND_PLANET_EXPLODE] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.3, .env_sustain = 0.9,
    .env_punch = 0.75, .env_decay = 0.8,
    .start_freq = 0.115, .freq_slide = 0.06,
    .vibrato_depth = 0.24, .vibrato_speed = 0.54,
    .phaser_offset = 0.35, .phaser_sweep = -0.08
  },
  [AZ_SND_PRISMATIC_CHARGE] = {
    .wave_kind = AZ_SQUARE_WAVE,
    .env_sustain = 0.216365, .env_decay = 0.52,
    .start_freq = 0.405, .freq_limit = 0.338565, .freq_slide = 0.12,
    .vibrato_depth = 0.365, .vibrato_speed = 0.7,
    .hpf_cutoff = 0.5, .volume_adjust = 0.5
  },
  [AZ_SND_PRISMATIC_FIRE] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.216365, .env_decay = 0.73,
    .start_freq = 0.725, .freq_limit = 0.105, .freq_slide = -0.16,
    .vibrato_depth = 0.365, .vibrato_speed = 0.7, .volume_adjust = -0.25
  },
  [AZ_SND_REACTIVE_ARMOR] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.10495, .env_punch = 0.362849, .env_decay = 0.33,
    .start_freq = 0.560445, .freq_slide = -0.1, .freq_delta_slide = 0.042,
    .vibrato_depth = 0.167017, .vibrato_speed = 0.391652, .phaser_sweep = -0.6,
    .lpf_cutoff = 0.4, .lpf_resonance = 0.36842, .hpf_cutoff = 0.115,
  },
  [AZ_SND_ROCKWYRM_SCREAM] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_attack = 0.00378904, .env_sustain = 0.26,
    .env_punch = 0.14, .env_decay = 0.97,
    .start_freq = 0.96, .freq_slide = -0.24, .freq_delta_slide = -0.48,
    .vibrato_depth = 0.545, .vibrato_speed = 0.665, .repeat_speed = 0.675,
    .phaser_offset = -0.31, .phaser_sweep = -0.2,
    .lpf_cutoff = 0.472522, .lpf_ramp = 0.12, .lpf_resonance = -0.77,
    .hpf_ramp = 0.00133439
  },
  [AZ_SND_SHRAPNEL_BURST] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.015, .env_punch = 0.22, .env_decay = 0.29,
    .start_freq = 0.62, .phaser_offset = -0.31, .volume_adjust = -0.75
  },
  [AZ_SND_SONIC_SCREECH] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.24708, .env_punch = 0.1941, .env_decay = 0.36444,
    .start_freq = 0.6959, .freq_limit = 0.28848,
    .freq_slide = -0.29102, .freq_delta_slide = 0.352112650871,
    .vibrato_depth = 0.316901415586, .vibrato_speed = 0.563380300999,
    .phaser_offset = 0.521126747131, .phaser_sweep = -0.239436626434,
    .hpf_cutoff = 0.394366204739
  },
  [AZ_SND_SPLASH] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.302816897631, .env_decay = 0.464788734913,
    .start_freq = 0.84507, .freq_slide = 0.422535, .freq_delta_slide = -0.0986,
    .phaser_sweep = 1.0, .hpf_cutoff = 0.5
  },
  [AZ_SND_SWITCH_ACTIVATE] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.03, .env_punch = 0.4, .env_decay = 0.47,
    .start_freq = 0.305, .arp_mod = 0.68, .arp_speed = 0.56
  },
  [AZ_SND_SWITCH_CONFIRM] = {
    .wave_kind = AZ_WOBBLE_WAVE,
    .env_sustain = 0.03, .env_punch = 0.4, .env_decay = 0.7,
    .start_freq = 0.4, .arp_mod = 0.5, .arp_speed = 0.56, .repeat_speed = 0.3
  },
  [AZ_SND_THRUST] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_sustain = 0.245, .start_freq = 0.3,
    .vibrato_depth = 0.1, .vibrato_speed = 0.605,
    .phaser_offset = 0.2, .volume_adjust = -0.98
  },
  [AZ_SND_TONGUE_SHOOT] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.098, .env_decay = 0.445,
    .start_freq = 0.15, .freq_slide = -0.11, .freq_delta_slide = 0.25
  },
  [AZ_SND_TONGUE_STICK] = {
    .wave_kind = AZ_SAWTOOTH_WAVE,
    .env_sustain = 0.098, .env_decay = 0.2,
    .start_freq = 0.19, .freq_slide = -0.19
  },
  [AZ_SND_TRACTOR_BEAM] = {
    .wave_kind = AZ_WOBBLE_WAVE, .env_sustain = 1.0, .start_freq = 0.18,
    .vibrato_depth = 0.2, .vibrato_speed = 0.3, .volume_adjust = -0.4
  },
  [AZ_SND_USE_COMM_CONSOLE] = {
    .wave_kind = AZ_SINE_WAVE,
    .env_attack = 0.4, .env_sustain = 0.75, .env_decay = 0.4,
    .start_freq = 0.42, .freq_slide = -0.11, .freq_delta_slide = 0.16,
    .arp_mod = -0.43, .arp_speed = 0.665, .repeat_speed = 0.46,
    .volume_adjust = -0.8
  },
  [AZ_SND_USE_REFILL_CONSOLE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.4, .env_sustain = 0.6, .env_decay = 0.3,
    .start_freq = 0.23, .freq_slide = 0.09,
    .vibrato_depth = 0.305, .vibrato_speed = 0.615,
    .arp_mod = 0.59, .arp_speed = 0.55
  },
  [AZ_SND_USE_SAVE_CONSOLE] = {
    .wave_kind = AZ_TRIANGLE_WAVE,
    .env_attack = 0.255, .env_sustain = 0.685,
    .env_punch = 0.26, .env_decay = 0.585,
    .start_freq = 0.295, .freq_slide = -0.033641, .freq_delta_slide = -0.05,
    .vibrato_depth = 0.309356, .vibrato_speed = 0.76,
    .arp_mod = -0.519069, .arp_speed = 0.501632,
    .repeat_speed = 0.930951, .phaser_sweep = -0.00171322,
    .lpf_cutoff = 0.896005, .lpf_ramp = 0.657195, .lpf_resonance = 0.402098,
    .hpf_cutoff = 0.0432724, .hpf_ramp = 0.0389865
  },
  [AZ_SND_WINGS_FLAPPING] = {
    .wave_kind = AZ_NOISE_WAVE,
    .env_attack = 0.24, .env_decay = 0.57,
    .freq_slide = 0.68, .freq_delta_slide = 0.95,
    .vibrato_depth = 0.84, .vibrato_speed = 0.495,
    .arp_mod = 0.32, .arp_speed = 0.16, .repeat_speed = 0.54,
    .lpf_cutoff = 0.78, .lpf_ramp = 0.17
  },
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(sound_specs) == AZ_NUM_SOUND_KEYS + 1);

/*===========================================================================*/

static az_sound_data_t sound_datas[AZ_ARRAY_SIZE(sound_specs)];

static bool sound_data_initialized = false;

static void destroy_sound_datas(void) {
  assert(sound_data_initialized);
  sound_data_initialized = false;
  AZ_ARRAY_LOOP(data, sound_datas) {
    az_destroy_sound_data(data);
  }
}

static const az_sound_data_t *sound_data_for_key(az_sound_key_t sound_key) {
  assert(sound_data_initialized);
  const int sound_index = (int)sound_key;
  assert(sound_index >= 0);
  assert(sound_index < AZ_ARRAY_SIZE(sound_datas));
  const az_sound_data_t *sound_data = &sound_datas[sound_index];
  if (sound_data->num_samples == 0) return NULL;
  return sound_data;
}

void az_init_sound_datas(void) {
  assert(!sound_data_initialized);
  for (int i = 1; i < AZ_ARRAY_SIZE(sound_specs); ++i) {
    az_create_sound_data(&sound_specs[i], &sound_datas[i]);
  }
  sound_data_initialized = true;
  atexit(destroy_sound_datas);
  assert(sound_data_for_key(AZ_SND_NOTHING) == NULL);
}

/*===========================================================================*/

void az_play_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_play_sound_data(soundboard, sound_data_for_key(sound_key), 1);
}

void az_play_sound_with_volume(
    az_soundboard_t *soundboard, az_sound_key_t sound_key, float volume) {
  az_play_sound_data(soundboard, sound_data_for_key(sound_key), volume);
}

void az_loop_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_loop_sound_data(soundboard, sound_data_for_key(sound_key), 1);
}

void az_loop_sound_with_volume(
    az_soundboard_t *soundboard, az_sound_key_t sound_key, float volume) {
  az_loop_sound_data(soundboard, sound_data_for_key(sound_key), volume);
}

void az_persist_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_persist_sound_data(soundboard, sound_data_for_key(sound_key), 1);
}

void az_hold_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_hold_sound_data(soundboard, sound_data_for_key(sound_key));
}

void az_reset_sound(az_soundboard_t *soundboard, az_sound_key_t sound_key) {
  az_reset_sound_data(soundboard, sound_data_for_key(sound_key));
}

/*===========================================================================*/
