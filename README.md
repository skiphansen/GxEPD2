# Skip's GxEPD2 Fork

This is my fork of ZinggJM's excellent [GxEPD2](https://github.com/ZinggJM/GxEPD2) project which adds
support for Pervasive Display's 7.4" BWRY EPD (E2741QS0B3).

Unfortunately (and understandably) ZinggJM doesn't accept PRs.

While there is public documentation and a driver for the display but it is
disappointing.  

[BWRY Tech Doc](https://docs.pervasivedisplays.com/knowledge/Hardware/epd-usage/Screens/BWRY_Medium/index.html)<br>
[BWRY Display driver](https://github.com/PervasiveDisplays/Pervasive_BWRY_Medium)


Unlike GoodDisplay PDI doesn't document the controller 
their displays use nor any of the registers.

From their FAQ:

<b> "Q: Where can I find a detailed document on the registers?"

"A: Aside from the application notes, we do not share details on the register values to discourage tampering with the embedded values. This is to guarantee optimal functional and optical performance of the EPDs. The application notes should suffice when followed."</b>


# WHY?

I salvaged several of these displays from some e-waste that I bought on ebay and
I wanted to use this display for a number of projects, primarily for [weather displays](https://github.com/skiphansen/esp32-weather-epd).

I've also written an [driver](https://github.com/skiphansen/Tag_FW_EFR32xG22/tree/custom_support/custom/example) for this display for
the [OEPL](https://github.com/OpenEPaperLink) project.

# PDI 7.4" BWRY EPD (E2741QS0B3)

<img width="689" height="541" alt="image" src="https://github.com/user-attachments/assets/05bdffa6-5abf-482a-b1c5-934e5a7e8078" />

## Notes

The JD79665AA is the closest controller documentation found for this display

The driver reads all of the configuration values from OTP, but for the first
batch of displays I obtained the OTP values used are the same.  The only 
differences appear to be in the two ASCII text strings (batch number perhaps?)

The following is a decode of the captured data stream from the ported oepl driver.

```
C: 70 (REV): REVISION register ??? JD79665AA shows 3 bytes
D: 0D
D: 04

C: 90 (PGM): Program Mode

C: A2 (PGM_CFG): MTP Program Config Register
D: 00
D: 15
D: 00
D: 00
D: E0

C: 92 (RMTP): Read MTP Data
D: 00 dummy
D: A5 Otp data...
1D 87 07 01 53 50 51 31 41 39 04 FF FF FF FF 07 
07 2B 01 E0 03 20 40 40 40 07 AB FF FF 00 00 00 
3C 00 00 00 00 08 37 03 03 00 01 1E 06 0A 0F 19 
0F 09 01 3F 05 0B 0F 19 1A 0F 01 04 01 01 3F FF 
FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 
FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 53 
31 32 37 34 31 51 53 30 42 33 31 41 39 FF 
D: FF	End of Otp data

C: E6	?? not defined for JD79665AA  perhaps TSSET (ed2208) or PLL (SDD1963)
D: 19

C: E0	(CCSET): Cascade Setting ??? b1 is not defined for JD79665AA
D: 02

C: A5	?? not defined for JD79665AA

C: 01

D: 07	(PWR): Power setting Register ?? JD79665AA shows 6 parameters


C: 00  (PSR): Panel setting Register
D: 07 	// 00 0 0 0 0 1 1 1 
	//  00 RES: - 800 x 600
	//  0 PST_MODE: Power switching time in the period of frame scanning.
	//  0 X
	//  0 UD: Scan down
	//  1 SHL: shift left
	//  1 SHD_N: Booster on
	//  1 RST_N: not reset
D: AB   // 1 0 1 0 1 0 1 1
	// 1 LUT_EN: Using LUT from register
	// 0 X
	// 1 FOPT: No scan after waveform finished and switch the source channel output to Hiz.
	// 0 VCMZ 0: VCOM status function: no effect
	// 1 TS_AUTO: When RST_N low to high,Temperature Sensor will be activated automatically one time. (
	// 0 TIEG: VGN power off status function 0 no effect
	// 1 NORG: After refreshing display, VCOM is tied to GND before power off
	// 1 VC_LUTZ: After refreshing display, the output of VCOM is set to floating automatically


C: 61  TRES
D: 01 0x1e0: 480
D: E0
D: 03 0x320: 800
D: 20

C: 00  (PSR): Panel setting Register
D: 07 	// 00 0 0 0 0 1 1 1 
	//  00 RES: - 800 x 600
	//  0 PST_MODE: Power switching time in the period of frame scanning.
	//  0 X
	//  0 UD: Scan down
	//  1 SHL: shift left
	//  1 SHD_N: Booster on
	//  1 RST_N: not reset

D: 2B   // 0 0 1 0 1 0 1 1
	// 0 LUT_EN: Using LUT from MTP
	// 0 X
	// 1 FOPT: No scan after waveform finished and switch the source channel output to Hiz.
	// 0 VCMZ 0: VCOM status function: no effect
	// 1 TS_AUTO: When RST_N low to high,Temperature Sensor will be activated automatically one time. (
	// 0 TIEG: VGN power off status function 0 no effect
	// 1 NORG: After refreshing display, VCOM is tied to GND before power off
	// 1 VC_LUTZ: After refreshing display, the output of VCOM is set to floating automatically

C: 06	BTST
D: 40
D: 40
D: 40

C: 03  ?? not defined for JD79665AA perhaps POFS (various JD devices
D: 00
D: 00
D: 00

C: E7 ?? not defined for JD79665AA perhaps SPI2 enable (ST7789)
D: 3C

C: 65 (GSST): Gate/Source Start Setting Register
D: 00
D: 00
D: 00
D: 00

C: 30 (PLL): PLL Control Register
D: 08

C: 50 (CDI): VCOM and DATA interval setting Register
D: 37

C: 60 ?? not defined for JD79665AA perhaps TCON ? (various controllers)
D: 03
D: 03

C: E3 (PWS): Power Saving Register ??? JD79665AA shows 2 argments
D: 00

C: FF ?? not defined for JD79665AA
D: A5

C: EF ?? not defined for JD79665AA
D: 01
D: 1E
D: 06
D: 0A
D: 0F
D: 19
D: 0F
D: 09

C: DC ?? not defined for JD79665AA perhaps RDID3 ?
D: 01

C: DD ?? not defined for JD79665AA 
D: 04

C: DE ?? not defined for JD79665AA  
D: 01

C: E8 ?? not defined for JD79665AA perhaps PWCTRL2 ?
D: 01

C: DA ?? not defined for JD79665AA perhaps ILI9341_RDID1
D: 3F

C: FF ?? not defined for JD79665AA
D: E3

C: E9 ?? not defined for JD79665AA perhaps ST7789_EQCTRL
D: 01

C: 10 (DTM): Data Start transmission Register
D: 55 ...

C: 04 (PON): Power ON Command

C: 12 (DRF): Display Refresh Command
D: 00
```

