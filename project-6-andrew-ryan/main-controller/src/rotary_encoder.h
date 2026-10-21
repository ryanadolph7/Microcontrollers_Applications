#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>

#define ENC_A   (1U << 6)   // P2.6  physical pin 13
#define ENC_B   (1U << 5)   // P2.5  physical pin 14
#define ENC_SW  (1U << 4)   // P2.4  physical pin 15

#define ENC_NONE    0
#define ENC_CW      1
#define ENC_CCW     2
#define ENC_BUTTON  3

#define ENC_STEPS_PER_EVENT 4

extern volatile uint8_t encoder_output;

extern volatile uint8_t enc_debug_flags;
extern volatile uint8_t enc_debug_last_ab;
extern volatile uint8_t enc_debug_transition;
extern volatile int8_t  enc_debug_accum;

void init_rotary_encoder(void);

#endif
