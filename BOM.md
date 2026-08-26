# Bill of Materials

Most of the electronic BOM is done by ordering assembled boards (currently from JLCPCB), and as such are not listed here. Refer to the hardware folder for the pcb project. The following are extra items that need to be purchased:

- [1x display, 320x240 IPS FPC 40pin, no touch](https://www.aliexpress.com/item/1005004629215040.html?pdp_npi=4%40dis%21CZK%21CZK%208.90%21CZK%208.90%21%21%210.36%210.36%21%402103891017375828457015857e954c%2112000035688288567%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005004629215040) - this is the FPC connector version for easier soldering. The device itself uses 16bit parallel mode. 
- [1x battery](https://www.aliexpress.com/item/1005007102975858.html), this one is only 1200mAh, but comes with JST-PH2 connector and cable length that makes it easy to put in. Alternatively [tme.eu](https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/) as 1350 mAh 503759 with different connector that will need adapting
- [1x rumbler](), spring contacts
- [1x speaker](https://cz.mouser.com/ProductDetail/Same-Sky/CMS-160903-18S-X8), spring connectors to the main pcb
- [6x screws](https://www.nerezka.cz/sroub-m-2-x-4-din-965tx-a2), M2x10
- [6x screw inserts](https://www.tme.eu/cz/en/details/b2_bn1054/threaded-insertions/bossard/1386727/) that are compatible with plastics used for the top plate. 

- [battery connector]()
- [headphones connector, sinking](https://cz.mouser.com/ProductDetail/490-SJ-43504-SMT-TR)

Things below are not needed for v3.2 but might return:

- [3x buttons](https://cz.mouser.com/ProductDetail/Omron-Electronics/B3U-3000P-B?qs=AO7BQMcsEu4JAdtnbsGArA%3D%3D), hand soldered for home button and volume up & down
- [6x screw inserts](https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/) if the top plate is made of thermoplastics, such as pla.

## Cartridges

- [1x flash memory]()
- [1x 10K resistor]()
- [1x 100nF capacitor]()

Optional cartridge extras

- [Flashlight LED](https://www.tme.eu/cz/details/l128-4080ca3500001/vykonove-diody-led-emiter/lumileds/)
- [LED Driver](https://www.tme.eu/cz/details/ap2502ktr-g1/stabilizator-napeti-obvody-dc-dc/diodes-incorporated/) or from [mouser](https://cz.mouser.com/ProductDetail/Diodes-Incorporated/AP2502KTR-G1)
- [IR LED](https://www.tme.eu/cz/details/ir204c_h16_l10/infracervene-diody-led/everlight/)


## BOM Optimization

Current BOM for 100 units is at around 2000 USD, with the following biggest offenders:

- ATTiny3217 is even more expensive than RP2350. The idea is that it can be replaced with some of the cheap Puya or similar MCUs (such as https://jlcpcb.com/partdetail/PUYA-PY32F002BF15U6TR/C7469099, or https://jlcpcb.com/partdetail/PUYA-PY32F030K28U6TR/C3018718). We need simple ADC, neopixel control, PWM for the display & rumbler, UART for debug and I2C for RP2350 communication. Those should be easy to have, the biggest task is likely the external and kinda precise RTC.
- LSM6DSV is pretty expensive and offers really nice pedometer. We can ditch the pedometer feature, or we can make the IO MCU provide a cheap pedometer filter for the ACC itself. Especially with the larger chips this can be possible.
- LTR390UV is another expensive sensor. Can be replaced with simple LED that can be status & light sensing (thanks to Paul Dietz's hack). Not enough pins for it in ATTiny3217, but might be enough on the Puya chips
- the HW switch for analog cartridge to the audio codec is also pretty expensive. Does not have to be populated though
- 220uF capacitors are expensive. Need to check if we can drop them to 100uF which gets a lot cheaper
- side buttons are also pretty expensive and I want to replace them anyways
