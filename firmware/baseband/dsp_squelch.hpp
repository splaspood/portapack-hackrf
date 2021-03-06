/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __DSP_SQUELCH_H__
#define __DSP_SQUELCH_H__

#include "buffer.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"

#include <cstdint>
#include <cstddef>

class FMSquelch {
public:
	bool execute(buffer_s16_t audio);

private:
	static constexpr size_t N = 32;
	static constexpr int16_t threshold = 3072;

	// nyquist = 48000 / 2.0
	// scipy.signal.iirdesign(wp=8000 / nyquist, ws= 4000 / nyquist, gpass=1, gstop=18, ftype='ellip')
	IIRBiquadFilter non_audio_hpf { non_audio_hpf_config };
};

#endif/*__DSP_SQUELCH_H__*/
