/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * Copyright (c) 2014 by Paul Stoffregen <paul@pjrc.com> (Transaction API)
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include <Arduino.h>

// SPI_HAS_TRANSACTION means SPI has beginTransaction(), endTransaction(),
// usingInterrupt(), and the actual clock speed names
#define SPI_HAS_TRANSACTION 1

// define SPI_AVR_EIMSK for AVR boards with external interrupt pins
#if defined(__AVR__)
#if defined(EIMSK)
  #define SPI_AVR_EIMSK	 EIMSK
#elif defined(GICR)
  #define SPI_AVR_EIMSK	 GICR
#elif defined(GIMSK)
  #define SPI_AVR_EIMSK	 GIMSK
#endif
#endif

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

class SPIClass {
public:
	// Initialize the SPI library
	static void begin();

	// If SPI is to used from within an interrupt, this function registers
	// that interrupt with the SPI library, so beginTransaction() can
	// prevent conflicts.  The input interruptNumber is the number used
	// with attachInterrupt.  If SPI is used from a different interrupt
	// (eg, a timer), interruptNumber should be 255.
	static void usingInterrupt(uint8_t interruptNumber);
	#if defined(__arm__) && defined(CORE_TEENSY)
	static void usingInterrupt(IRQ_NUMBER_t interruptName);
	#endif

	// Before using SPI.transfer() or asserting chip select pins,
	// this function is used to gain exclusive access to the SPI bus
	// and configure the correct settings.
	inline static void beginTransaction(uint8_t clockDiv, uint8_t bitOrder, uint8_t dataMode) {
		if (interruptMode > 0) {
			#ifdef SPI_AVR_EIMSK
			if (interruptMode == 1) {
				interruptSave = SPI_AVR_EIMSK;
				SPI_AVR_EIMSK &= ~interruptMask;
			} else
			#endif
			{
				interruptSave = SREG;
				cli();
			}
		}
		// these long expressions each compile to only 2 instructions
		SPCR = _BV(SPE) | _BV(MSTR) | ((bitOrder == LSBFIRST) ? _BV(DORD) : 0) |
			(dataMode & SPI_MODE_MASK) | (clockDiv & SPI_CLOCK_MASK);
		SPSR = (clockDiv >> 2) & SPI_2XCLOCK_MASK;
	}

	// Write to the SPI bus (MOSI pin) and also receive (MISO pin)
	inline static byte transfer(byte data) {
		SPDR = data;
		asm volatile("nop");
		while (!(SPSR & _BV(SPIF))) ; // wait
		return SPDR;
	}
	inline static void transfer(void *buf, size_t count) {
		if (count == 0) return;
		uint8_t *p = (uint8_t *)buf;
		SPDR = *p;
		while (--count > 0) {
			uint8_t out = *(p + 1);
			while (!(SPSR & _BV(SPIF))) ;
			uint8_t in = SPDR;
			SPDR = out;
			*p++ = in;
		}
		while (!(SPSR & _BV(SPIF))) ;
		*p = SPDR;
	}

	// After performing a group of transfers and releasing the chip select
	// signal, this function allows others to access the SPI bus
	inline static void endTransaction(void) {
		if (interruptMode > 0) {
			#ifdef SPI_AVR_EIMSK
			if (interruptMode == 1) {
				SPI_AVR_EIMSK = interruptSave;
			} else
			#endif
			{
				SREG = interruptSave;
			}
		}
	}

	// Disable the SPI bus
	static void end();

	// This function is deprecated.	 New applications should use
	// beginTransaction() to configure SPI settings.
	inline static void setBitOrder(uint8_t bitOrder) {
		if (bitOrder == LSBFIRST) SPCR |= _BV(DORD);
		else SPCR &= ~(_BV(DORD));
	}
	// This function is deprecated.	 New applications should use
	// beginTransaction() to configure SPI settings.
	inline static void setDataMode(uint8_t dataMode) {
		SPCR = (SPCR & ~SPI_MODE_MASK) | dataMode;
	}
	// This function is deprecated.	 New applications should use
	// beginTransaction() to configure SPI settings.
	inline static void setClockDivider(uint8_t clockDiv) {
		SPCR = (SPCR & ~SPI_CLOCK_MASK) | (clockDiv & SPI_CLOCK_MASK);
		SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((clockDiv >> 2) & SPI_2XCLOCK_MASK);
	}
	// These undocumented functions should not be used.  SPI.transfer()
	// polls the hardware flag which is automatically cleared as the
	// AVR responds to SPI's interrupt
	inline static void attachInterrupt() { SPCR |= _BV(SPIE); }
	inline static void detachInterrupt() { SPCR &= ~_BV(SPIE); }

#if defined(__arm__) && defined(CORE_TEENSY)
	inline void setMOSI(uint8_t pin) __attribute__((always_inline)) { SPCR.setMOSI(pin); }
	inline void setMISO(uint8_t pin) __attribute__((always_inline)) { SPCR.setMISO(pin); }
	inline void setSCK(uint8_t pin) __attribute__((always_inline)) { SPCR.setSCK(pin); }
#endif

private:
	static uint8_t interruptMode; // 0=none, 1=mask, 2=global
	static uint8_t interruptMask; // which interrupts to mask
	static uint8_t interruptSave; // temp storage, to restore state
};

extern SPIClass SPI;


#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

// mapping of interrupt numbers to bits within SPI_AVR_EIMSK
#ifdef SPI_AVR_EIMSK
#if defined(__AVR_ATmega32U4__)
  #define SPI_INT0_MASK	 (1<<INT0)
  #define SPI_INT1_MASK	 (1<<INT1)
  #define SPI_INT2_MASK	 (1<<INT2)
  #define SPI_INT3_MASK	 (1<<INT3)
  #define SPI_INT4_MASK	 (1<<INT6)
#elif defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
  #define SPI_INT0_MASK	 (1<<INT0)
  #define SPI_INT1_MASK	 (1<<INT1)
  #define SPI_INT2_MASK	 (1<<INT2)
  #define SPI_INT3_MASK	 (1<<INT3)
  #define SPI_INT4_MASK	 (1<<INT4)
  #define SPI_INT5_MASK	 (1<<INT5)
  #define SPI_INT6_MASK	 (1<<INT6)
  #define SPI_INT7_MASK	 (1<<INT7)
#elif defined(EICRA) && defined(EICRB) && defined(EIMSK)
  #define SPI_INT0_MASK	 (1<<INT4)
  #define SPI_INT1_MASK	 (1<<INT5)
  #define SPI_INT2_MASK	 (1<<INT0)
  #define SPI_INT3_MASK	 (1<<INT1)
  #define SPI_INT4_MASK	 (1<<INT2)
  #define SPI_INT5_MASK	 (1<<INT3)
  #define SPI_INT6_MASK	 (1<<INT6)
  #define SPI_INT7_MASK	 (1<<INT7)
#else
  #ifdef INT0
  #define SPI_INT0_MASK	 (1<<INT0)
  #endif
  #ifdef INT1
  #define SPI_INT1_MASK	 (1<<INT1)
  #endif
  #ifdef INT2
  #define SPI_INT2_MASK	 (1<<INT2)
  #endif
#endif
#endif //SPI_AVR_EIMSK


#endif
