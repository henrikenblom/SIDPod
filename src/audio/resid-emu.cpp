/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2019 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "resid-emu.h"
#include <string>

#include "resid/siddefs.h"

namespace libsidplayfp {
    ReSID::ReSID() : m_sid(*(new SID)),
                     m_voiceMask(0x07) {
        m_buffer = new short[OUTPUTBUFFERSIZE];
        reset(0);
    }

    ReSID::~ReSID() {
        delete &m_sid;
        delete[] m_buffer;
    }

    // Standard component options
    void ReSID::reset(uint8_t volume) {
        m_accessClk = 0;
        m_sid.reset();
        m_sid.write(0x18, volume);
    }

    uint8_t ReSID::read(uint_least8_t addr) {
        clock();
        return m_sid.read(addr);
    }

    void ReSID::write(uint_least8_t addr, uint8_t data) {
        clock();
        m_sid.write(addr, data);
    }

    void ReSID::clock() {
        cycle_count cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
        m_accessClk += cycles;
        m_bufferpos += m_sid.clock(cycles, (short *) m_buffer + m_bufferpos, OUTPUTBUFFERSIZE - m_bufferpos);
        // Adjust in case not all cycles have been consumed
        m_accessClk -= cycles;
    }

    void ReSID::filter(bool enable) {
        m_sid.enable_filter(enable);
    }

    void ReSID::sampling(float systemclock, float freq,
                         sampling_method method, bool fast) {
        if (!m_sid.set_sampling_parameters(systemclock, method, freq)) {
            m_status = false;
            m_error = ERR_UNSUPPORTED_FREQ;
            return;
        }

        m_status = true;
    }

    // Set the emulated SID model
    void ReSID::model(chip_model model, bool digiboost) {
        short sample = 0;

        if (digiboost) {
            m_voiceMask |= 0x08;
            sample = -32768;
        }


        m_sid.set_chip_model(model);
        m_sid.input(sample);
        m_status = true;
    }
}
