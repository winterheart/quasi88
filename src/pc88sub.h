#ifndef PC88SUB_H_INCLUDED
#define PC88SUB_H_INCLUDED

/**** 変数 ****/

extern int sub_load_rate;

/**** 関数 ****/

void pc88sub_init(int init);
void pc88sub_term(void);
void pc88sub_bus_setup(void);

uint8_t sub_mem_read(uint16_t addr);
void sub_mem_write(uint16_t addr, uint8_t data);
uint8_t sub_io_in(uint8_t port);
void sub_io_out(uint8_t port, uint8_t data);

void subcpu_keyscan_draw(void);

#endif /* PC88SUB_H_INCLUDED */
