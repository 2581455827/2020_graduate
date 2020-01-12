NOTE: AVR Code addresses are word addresses
All other addresses (including ones in FLASH) are byte addresses

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                             lit   00E4   0004 =      4. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       00E4  __lit_start
       00E8  __lit_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   00E8   0017 =     23. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       00E8  __idata_start
       00FF  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   0100   0740 =   1856. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       0080  __start
       0080  __text_start
       00A2  _exit
       00A3  _delay_1us
       00A5  _delay_nus
       00B3  _delay_1ms
       00BD  _delay_nms
       00CB  _I2C_Write
       00F3  _I2C_Read
       0121  _LCD_init
       0142  _LCD_en_write
       0148  _LCD_write_command
       0166  _LCD_write_data
       0184  _LCD_set_xy
       0193  _LCD_write_string
       01A8  _LCD_write_char
       01B4  _conversion
       01F7  _main
       023D  mod16u
       023F  div16u
       0240  xdiv16u
       0259  elpm32
       026D  push_xgset300C
       0272  pop_xgset300C
       0277  push_xgsetF00C
       027E  pop_xgsetF00C
       0285  push_xgset303C
       028C  pop_xgset303C
       0293  neg32
       029C  fpint
       02C8  uint2fp
       02CA  int2fp
       02D3  ulong2fp
       02D5  long2fp
       02E8  fpdiv1
       02F1  fpdiv2
       02FB  fpdiv1x
       0307  fpdiv2x
       0398  unpacks
       03A0  tstR16_R19
       03A8  unpacku
       03BD  normalize_and_pack
       03EB  pack
       03F5  fpneg
       03FC  fpneg2
       0402  saveR8toR25
       0411  restoreR8toR25
       0420  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   0200   0017 =     23. bytes (rel,con,ram)

       Addr  Global Symbol
      -----  --------------------------------
       0200  __data_start
       0200  _display
       0217  __data_end

Area                               Start  End    Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                          vector   0000   0004 =      4. bytes (abs,ovr,rom)

Files Linked      [ module(s) ]

C:\iccv7avr\lib\crtatmega.o	[ crtatmega.s ]
AVR_GY-30.o	[ AVR_GY ]
<library>	[ div16u.s, elpm32.s, gpushx.s, gpushz.s, gpusha.s, lneg.s, fpint.s, int2fp.s, fpdiv_new.s, fputil.s ]


User Global Definitions

ram_end = 0x21ff
hwstk_size = 0x1e

User Base Address Definitions

func_lit = 0xe4
data = 0x200
eeprom:0.4096

