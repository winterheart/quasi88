#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

extern int set_version; /* バージョン強制変更 '0' 〜 '9'  */
extern int rom_version; /* (変更前の) BASIC ROMバージョン  */

extern int use_extram;        /* 128KB拡張RAMを使う  */
extern int use_jisho_rom;     /* 辞書ROMを使う   */
extern int use_built_in_font; /* 内蔵フォントを使う  */
extern int use_pcg;           /* PCG-8100サポート */
extern int font_type;         /* フォントの種類    */
extern int font_loaded;       /* ロードしたフォント種   */

extern int memory_wait; /* メモリウェイト有無  */

extern char *file_compatrom; /* P88SR emu のROMを使う*/

extern int has_kanji_rom; /* 漢字ROMの有無   */

extern int linear_ext_ram; /* 拡張RAMを連続させる  */

extern uint8_t *main_rom;               /* メイン ROM (32KB) */
extern uint8_t (*main_rom_ext)[0x2000]; /* 拡張 ROM   (8KB *4)    */
extern uint8_t *main_rom_n;             /* N-BASIC    (32KB)    */
extern uint8_t *main_ram;               /* メイン RAM (64KB) */
extern uint8_t *main_high_ram;          /* 高速 RAM(の裏) (4KB) */
extern uint8_t *sub_romram;             /* サブ ROM/RAM (32KB)    */

extern uint8_t (*kanji_rom)[65536][2]; /* 漢字 ROM   (128KB*2)   */

extern uint8_t (*ext_ram)[0x8000];   /* 拡張 RAM   (32KB*4〜)*/
extern uint8_t (*jisho_rom)[0x4000]; /* 辞書 ROM   (16KB*32)   */

extern uint8_t (*main_vram)[4]; /* VRAM[0x4000][4]  */
extern uint8_t *font_rom;       /* フォントイメージ     */
extern uint8_t *font_pcg;       /* フォントイメージ(PCG)*/
extern uint8_t *font_mem;       /* フォントイメージ(fix)*/
extern uint8_t *font_mem2;      /* フォントイメージ(2nd)*/
extern uint8_t *font_mem3;      /* フォントイメージ(3rd)*/

/* イリーガルな方法でメモリアクセスする   */
#define main_vram4 (uint32_t *)main_vram /* VRAM long word accrss*/

#define ROM_VERSION main_rom[0x79d7]

extern uint8_t *dummy_rom;             /* ダミーROM (32KB)  */
extern uint8_t *dummy_ram;             /* ダミーRAM (32KB)  */
extern uint8_t kanji_dummy_rom[16][2]; /* 漢字ダミーROM   */

int memory_allocate();
void memory_free();

void memory_reset_font();
void memory_set_font();

int memory_allocate_additional();

#endif /* MEMORY_H_INCLUDED */
