//
// Created by Henrik Enblom on 2022-12-17.
//

#ifndef SIDPOD_DIGI_INTERFACE_H
#define SIDPOD_DIGI_INTERFACE_H

#include "stdint.h"

struct DigiDetector;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct DigiDetector *DigiDetectorHandle;
DigiDetectorHandle get_digi_detector();
void detect_sample(DigiDetectorHandle p_digi_detector, uint16_t addr, uint8_t value);
void reset_digi_detector(DigiDetectorHandle p_digi_detector,
                         uint32_t clock_rate,
                         uint8_t is_rsid,
                         uint8_t is_compatible);
void route_digi_signal(DigiDetectorHandle p_digi_detector,
                       int32_t *digi_out,
                       int32_t *outf,
                       int32_t *outo);
bool mahoney_samples_detected(DigiDetectorHandle p_digi_detector);
#ifdef __cplusplus
}
#endif
#endif //SIDPOD_DIGI_INTERFACE_H
