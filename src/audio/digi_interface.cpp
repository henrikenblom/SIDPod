//
// Created by Henrik Enblom on 2022-12-17.
//

#include "digi_interface.h"
#include "digi.h"

DigiDetectorHandle get_digi_detector() {
    return DigiDetector::getInstance();
}

void detect_sample(DigiDetectorHandle p_digi_detector, uint16_t addr, uint8_t value) {
    p_digi_detector->detectSample(addr, value);
}

void reset_digi_detector(DigiDetectorHandle p_digi_detector,
                         uint32_t clock_rate,
                         uint8_t is_rsid,
                         uint8_t is_compatible) {
    p_digi_detector->reset(clock_rate, is_rsid, is_compatible);
}

void route_digi_signal(DigiDetectorHandle p_digi_detector,
                       int32_t *digi_out,
                       int32_t *outf,
                       int32_t *outo) {
    p_digi_detector->routeDigiSignal(digi_out, outf, outo);
}

bool mahoney_samples_detected(DigiDetectorHandle p_digi_detector) {
    return p_digi_detector->isMahoney();
}