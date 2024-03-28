#ifndef BEEPINTF_H
#define BEEPINTF_H

#ifdef __cplusplus
extern "C" {
#endif

WRITE8_HANDLER( BEEP88_write_port_0_w );
WRITE8_HANDLER( BEEP88_control_port_0_w );

extern void BEEP88_set_volume(float volume);

#ifdef __cplusplus
};
#endif

#endif
