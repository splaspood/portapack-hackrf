/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PORTAPACK_IO_H__
#define __PORTAPACK_IO_H__

#include <cstdint>
#include <cstddef>
#include <array>

#include "gpio.hpp"
#include "ui.hpp"

namespace portapack {

class IO {
public:
	enum class TouchPinsConfig : uint8_t {
		XN_BIT = (1 << 0),
		XP_BIT = (1 << 1),
		YN_BIT = (1 << 2),
		YP_BIT = (1 << 3),

		XN_OE = (1 << 4),
		XP_OE = (1 << 5),
		YN_OE = (1 << 6),
		YP_OE = (1 << 7),

		XN_IN = XN_BIT,
		XN_OUT_1 = XN_OE | XN_BIT,
		XN_OUT_0 = XN_OE,

		XP_IN = XP_BIT,
		XP_OUT_1 = XP_OE | XP_BIT,
		XP_OUT_0 = XP_OE,

		YN_IN = YN_BIT,
		YN_OUT_1 = YN_OE | YN_BIT,
		YN_OUT_0 = YN_OE,

		YP_IN = YP_BIT,
		YP_OUT_1 = YP_OE | YP_BIT,
		YP_OUT_0 = YP_OE,

		/* Allow pins to be pulled up by CPLD pull-ups. */
		Float = XP_IN | XN_IN | YP_IN | YN_IN,

		/* Drive one plane to 0V, other plane is pulled up. Watch for when pulled-up
		 * plane falls to ~0V.
		 */
		WaitTouch = XP_OUT_0 | XN_OUT_0 | YP_IN | YN_IN,

		/* Create a voltage divider between X plane, touch resistance, Y plane. */
		SensePressure = XP_IN | XN_OUT_0 | YP_OUT_1 | YN_IN,

		/* Create a voltage divider across X plane, read voltage from Y plane. */
		SenseX = XP_OUT_1 | XN_OUT_0 | YP_IN | YN_IN,

		/* Create a voltage divider across Y plane, read voltage from X plane. */
		SenseY = XP_IN | XN_IN | YP_OUT_1 | YN_OUT_0,
	};

	constexpr IO(
		GPIO gpio_dir,
		GPIO gpio_lcd_rd,
		GPIO gpio_lcd_wr,
		GPIO gpio_io_stbx,
		GPIO gpio_addr,
		GPIO gpio_rot_a,
		GPIO gpio_rot_b
	) : gpio_dir { gpio_dir },
		gpio_lcd_rd { gpio_lcd_rd },
		gpio_lcd_wr { gpio_lcd_wr },
		gpio_io_stbx { gpio_io_stbx },
		gpio_addr { gpio_addr },
		gpio_rot_a { gpio_rot_a },
		gpio_rot_b { gpio_rot_b }
	{
	};

	void init();

	void lcd_backlight(const bool value);
	void lcd_reset_state(const bool active);

	void lcd_data_write_command_and_data(
		const uint_fast8_t command,
		const std::initializer_list<uint8_t>& data
	) {
		lcd_command(command);
		for(const auto d : data) {
			lcd_write_data_fast(d);
		}
	}

	void lcd_data_read_command_and_data(
		const uint_fast8_t command,
		uint16_t* const data,
		const size_t data_count
	) {
		lcd_command(command);
		for(size_t i=0; i<data_count; i++) {
			data[i] = lcd_read_data_id();
		}
	}

	void lcd_write_word(const uint32_t w) {
		lcd_write_data_fast(w);
	}

	void lcd_write_words(const uint16_t* const w, size_t n) {
		for(size_t i=0; i<n; i++) {
			lcd_write_data_fast(w[i]);
		}
	}

	void lcd_write_pixel(const ui::Color pixel) {
		lcd_write_data_fast(pixel.v);
	}

	uint32_t lcd_read_word() {
		return lcd_read_data_frame_memory();
	}
/*
	// NOTE: Pixels read in RGB24 format, not the RGB565 format used
	// to write pixels to frame memory. This makes reading very tricky!
	ui::Color lcd_read_pixel() {
		return ui::Color { lcd_read_data_frame_memory() };
	}
*/
	void lcd_write_pixels(const ui::Color pixel, size_t n) {
		while(n--) {
			lcd_write_data_fast(pixel.v);
		}
	}

	void lcd_write_pixels(const ui::Color* const pixels, size_t n) {
		for(size_t i=0; i<n; i++) {
			lcd_write_pixel(pixels[i]);
		}
	}

	uint32_t io_read() {
		io_stb_assert();
		dir_read();
		addr_0();
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		const auto switches_raw = data_read();
		io_stb_deassert();

		return switches_raw;
	}

	uint32_t io_update(const TouchPinsConfig write_value);

	uint32_t lcd_te() {
		return gpio_rot_a.read();
	}

private:
	const GPIO gpio_dir;
	const GPIO gpio_lcd_rd;
	const GPIO gpio_lcd_wr;
	const GPIO gpio_io_stbx;
	const GPIO gpio_addr;
	const GPIO gpio_rot_a;
	const GPIO gpio_rot_b;

	static constexpr ioportid_t gpio_data_port_id = 3;
	static constexpr size_t gpio_data_shift = 8;
	static constexpr ioportmask_t gpio_data_mask = 0xffU << gpio_data_shift;

	uint8_t io_reg { 0x01 };

	void lcd_rd_assert() {
		gpio_lcd_rd.set();
	}

	void lcd_rd_deassert() {
		gpio_lcd_rd.clear();
	}

	void lcd_wr_assert() {
		gpio_lcd_wr.set();
	}

	void lcd_wr_deassert() {
		gpio_lcd_wr.clear();
	}

	void io_stb_assert() {
		gpio_io_stbx.clear();
	}

	void io_stb_deassert() {
		gpio_io_stbx.set();
	}

	void addr(const bool value) {
		gpio_addr.write(value);
	}

	void addr_1() {
		gpio_addr.set();
	}

	void addr_0() {
		gpio_addr.clear();
	}

	void data_mask_set() {
		LPC_GPIO->MASK[gpio_data_port_id] = ~gpio_data_mask;
	}

	void dir_write() {
		gpio_dir.clear();
		LPC_GPIO->DIR[gpio_data_port_id] |= gpio_data_mask;
		/* TODO: Manipulating DIR[3] makes me queasy. The RFFC5072 DATA pin
		 * is also on port 3, and switches direction periodically...
		 * Time to resort to bit-banding to enforce atomicity? But then, how
		 * to change direction on eight bits efficiently? Or do I care, since
		 * the PortaPack data bus shouldn't change direction too frequently?
		 */
	}

	void dir_read() {
		LPC_GPIO->DIR[gpio_data_port_id] &= ~gpio_data_mask;
		gpio_dir.set();
	}

	void data_write_low(const uint32_t value) {
		LPC_GPIO->MPIN[gpio_data_port_id] = (value << gpio_data_shift);
	}

	void data_write_high(const uint32_t value) {
		LPC_GPIO->MPIN[gpio_data_port_id] = value;
	}

	uint32_t data_read() {
		return (LPC_GPIO->MPIN[gpio_data_port_id] >> gpio_data_shift) & 0xffU;
	}

	void lcd_command(const uint32_t value) {
		data_write_high(0);		/* Drive high byte (with zero -- don't care) */
		dir_write();			/* Turn around data bus, MCU->CPLD */
		addr(0);				/* Indicate command */
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		lcd_wr_assert();		/* Latch high byte */

		data_write_low(value);	/* Drive low byte (pass-through) */
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		lcd_wr_deassert();		/* Complete write operation */

		addr(1);				/* Set up for data phase (most likely after a command) */
	}

	void lcd_write_data_fast(const uint32_t value) __attribute__((always_inline)) {
		// NOTE: Assumes and DIR=0 and ADDR=1 from command phase.
		data_write_high(value);	/* Drive high byte */
		__asm__("nop");
		lcd_wr_assert();		/* Latch high byte */

		data_write_low(value);	/* Drive low byte (pass-through) */
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		lcd_wr_deassert();		/* Complete write operation */
	}

	uint32_t lcd_read_data_id() {
		// NOTE: Assumes ADDR=1 from command phase.
		dir_read();

		/* Start read operation */
		lcd_rd_assert();
		/* Wait for passthrough data(15:8) to settle -- ~16ns (3 cycles) typical */
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");

		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		const auto value_high = data_read();

		/* Latch data[7:0] */
		lcd_rd_deassert();
		/* Wait for latched data[7:0] to settle -- ~26ns (5 cycles) typical */
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");

		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");

		const auto value_low = data_read();
		return (value_high << 8) | value_low;
	}

	uint32_t lcd_read_data_frame_memory() {
		// NOTE: Assumes ADDR=1 from command phase.
		dir_read();

		/* Start read operation */
		lcd_rd_assert();
		/* Wait for passthrough data(15:8) to settle -- ~16ns (3 cycles) typical */
		/* Wait for read control L duration (355ns) */
		halPolledDelay(71);	// 355ns
		const auto value_high = data_read();

		/* Latch data[7:0] */
		lcd_rd_deassert();
		/* Wait for latched data[7:0] to settle -- ~26ns (5 cycles) typical */
		/* Wait for read control H duration (90ns) */
		halPolledDelay(18);	// 90ns

		const auto value_low = data_read();
		return (value_high << 8) | value_low;
	}

	void io_write(const bool address, const uint_fast16_t value) {
		data_write_low(value);
		dir_write();
		addr(address);
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		io_stb_assert();
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		io_stb_deassert();
	}
/*
	void lcd_data_write_command_and_data(
		const uint_fast16_t command,
		const uint8_t* const data,
		const size_t count
	) {
		lcd_data_write_command(command);
		for(size_t i=0; i<count; i++) {
			lcd_data_write_data(data[i]);
		}
	}
*/
};

extern IO io;

} /* namespace portapack */

#endif/*__PORTAPACK_IO_H__*/
