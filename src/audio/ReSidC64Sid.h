//
// Created by Henrik Enblom on 2025-04-07.
//

#ifndef RESIDC64SID_H
#define RESIDC64SID_H

#include "c64/c64sid.h"
#include "reSID.h"

class ReSidC64Sid final : public c64sid {
private:
    reSID m_sid;

protected:
    virtual uint8_t read(uint_least8_t addr) {
        return m_sid.read(addr);
    }

    virtual void writeReg(uint_least8_t addr, uint8_t data) {
        m_sid.write(addr, data);
    }

public:
    ReSidC64Sid() = default;

    ~ReSidC64Sid() override = default;

    void reset() {
        m_sid.reset();
    }

    void reset(uint8_t volume) override {
        printf("ReSidC64Sid::reset(volume)\n");
        m_sid.reset();
        m_sid.enable_filter(true);
        m_sid.enable_external_filter(true);
        m_sid.set_sampling_parameters(CLOCKFREQ, SAMPLE_FAST, SAMPLE_RATE);
    }

    void output(short *buffer, uint_least32_t len) {
        cycle_count delta_t = static_cast<cycle_count>(static_cast<float>(CLOCKFREQ)) / (
                                  static_cast<float>(SAMPLE_RATE) / static_cast<float>(len));
        m_sid.clock(delta_t, buffer, static_cast<int>(len));
    }
};

#endif //RESIDC64SID_H
