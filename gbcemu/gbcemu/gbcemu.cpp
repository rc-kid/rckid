#include <rckid/apps/dialogs/ColorPicker.h>
#include "gbcemu.h"

#define A (regs8_[REG_INDEX_A])
#define F (regs8_[REG_INDEX_F])
#define B (regs8_[REG_INDEX_B])
#define C (regs8_[REG_INDEX_C])
#define D (regs8_[REG_INDEX_D])
#define E (regs8_[REG_INDEX_E])
#define H (regs8_[REG_INDEX_H])
#define L (regs8_[REG_INDEX_L])
#define AF (regs16_[REG_INDEX_AF])
#define BC (regs16_[REG_INDEX_BC])
#define DE (regs16_[REG_INDEX_DE])
#define HL (regs16_[REG_INDEX_HL])

#define PC (pc_)
#define SP (sp_)

#define IO_ADDR(REG) (& REG - hram_)

#define ADDR_JOYP 0x00
#define ADDR_SB 0x01
#define ADDR_SC 0x02
#define ADDR_DIV 0x04
#define ADDR_TIMA 0x05
#define ADDR_TMA 0x06
#define ADDR_TAC 0x07
#define ADDR_IF 0x0f
#define ADDR_WAVE_RAM 0x30
#define ADDR_LCDC 0x40
#define ADDR_STAT 0x41
#define ADDR_SCY 0x42
#define ADDR_SCX 0x43
#define ADDR_LY 0x44
#define ADDR_LYC 0x45
#define ADDR_DMA 0x46
#define ADDR_BGP 0x47
#define ADDR_OBP0 0x48
#define ADDR_OBP1 0x49
#define ADDR_WY 0x4a
#define ADDR_WX 0x4b
#define ADDR_KEY1 0x4d
#define ADDR_VBK 0x4f
#define ADDR_HDMA1 0x51
#define ADDR_HDMA2 0x52
#define ADDR_HDMA3 0x53
#define ADDR_HDMA4 0x54
#define ADDR_HDMA5 0x55
#define ADDR_RP 0x56
#define ADDR_BCPS 0x68
#define ADDR_BCPD 0x69
#define ADDR_OCPS 0x6a
#define ADDR_OCPD 0x6b
#define ADDR_SVBK 0x70
#define ADDR_PCM12 0x76
#define ADDR_PCM34 0x77
#define ADDR_IE 0xff

/** The joypad register. 
 
    Writing to the register's upper nibble chooses between dpad (bit 4) and buttons (bit 5), while the lower nibble contains the values of the buttons. When button is pressed, it should read 0. Internally this really is a button matrix.

    bit 5 = select buttons (write)
    bit 4 = select dpad (write)
    bit 3 = start / down (read)
    bit 2 = select / up (read)
    bit 1 = B / left (read)
    bit 0 = A / right (read)

    When any of the lower bits transition from 1 to 0, the joypad interrupt is raised.
 */
#define IO_JOYP (hram_[ADDR_JOYP])
static constexpr uint8_t JOYP_DPAD = 16;
static constexpr uint8_t JOYP_BUTTONS = 32;

/** When device wants to transfer a byte via the serial line, it writes it to this register. 
 */
#define IO_SB (hram_[ADDR_SB])
#define IO_SC (hram_[ADDR_SC])

/** The 16384Hz timer. This value is incremented at a steady rate. Any write to it should reset the value to 0. 
 
    The DMG runs the CPU at 4 194 304 Hz, which means it should be incremented 256 times in a second, while the CGB runs at 8 388 608 Hz, exactly double the rate, which gives 512 increments per second. 

    TODO Determine when & how to update this
 */
#define IO_DIV (hram_[ADDR_DIV])

/** Timer counter. Increments the register value at rate given by IO_TAC. When it overflows 0xff, it is being reset to IO_TMA value. 
 */
#define IO_TIMA (hram_[ADDR_TIMA])

/** Timer modulo. When IO_TIMA overflows, it is reset to this value. 
 */
#define IO_TMA (hram_[ADDR_TMA])

/** The Timer control register. 
 
    When the timer is enabled, the IO_TIMA is incremented at the rate given by the lower 2 bits of this register. Note that the IO_DIV is always counting, and unlike the IO_TIMA is not specified in M cycles, but actual Hz so will increment at the same pace in both DMG and CGB, whereas the IO_TIMA will run twice as fast on CGB for the same values. 
 */
#define IO_TAC (hram_[ADDR_TAC])
static constexpr uint8_t TAC_ENABLE = 4;
static constexpr uint8_t TAC_CLOCK_SELECT_MASK = 3;
static constexpr uint8_t TAC_CLOCK_SELECT_256M = 0;
static constexpr uint8_t TAC_CLOCK_SELECT_4M = 1;
static constexpr uint8_t TAC_CLOCK_SELECT_16M = 2;
static constexpr uint8_t TAC_CLOCK_SELECT_64M = 3;

/** The Interrupt Flag Register
 
    bit 4 = Joypad
    bit 3 = Serial
    bit 2 = Timer
    bit 1 = LCD
    bit 0 = VBLANK
 */
#define IO_IF (hram_[ADDR_IF])
static constexpr uint8_t IF_JOYPAD = 1 << 4;
static constexpr uint8_t IF_SERIAL = 1 << 3;
static constexpr uint8_t IF_TIMER = 1 << 2;
static constexpr uint8_t IF_STAT = 1 << 1;
static constexpr uint8_t IF_VBLANK = 1 << 0;

#define IO_NR10 (hram_[APU::ADDR_NR10])
#define IO_NR11 (hram_[APU::ADDR_NR11])
#define IO_NR12 (hram_[APU::ADDR_NR12])
#define IO_NR13 (hram_[APU::ADDR_NR13])
#define IO_NR14 (hram_[APU::ADDR_NR14])
//static constexpr size_t IO_NR20 = 0x15; // not used
#define IO_NR21 (hram_[APU::ADDR_NR21])
#define IO_NR22 (hram_[APU::ADDR_NR22])
#define IO_NR23 (hram_[APU::ADDR_NR23])
#define IO_NR24 (hram_[APU::ADDR_NR24])
#define IO_NR30 (hram_[APU::ADDR_NR30])
#define IO_NR31 (hram_[APU::ADDR_NR31])
#define IO_NR32 (hram_[APU::ADDR_NR32])
#define IO_NR33 (hram_[APU::ADDR_NR33])
#define IO_NR34 (hram_[APU::ADDR_NR34])
//static constexpr size_t IO_NR40 = 0x1f; // not used
#define IO_NR41 (hram_[APU::ADDR_NR41])
#define IO_NR42 (hram_[APU::ADDR_NR42])
#define IO_NR43 (hram_[APU::ADDR_NR43])
#define IO_NR44 (hram_[APU::ADDR_NR44])
#define IO_NR50 (hram_[APU::ADDR_NR50])
#define IO_NR51 (hram_[APU::ADDR_NR51])
#define IO_NR52 (hram_[APU::ADDR_NR52])
static constexpr uint8_t NR52_APU_ON = 1 << 7;
static constexpr uint8_t NR52_CHANNEL_4_ON = 1 << 3;
static constexpr uint8_t NR52_CHANNEL_3_ON = 1 << 2;
static constexpr uint8_t NR52_CHANNEL_2_ON = 1 << 1;
static constexpr uint8_t NR52_CHANNEL_1_ON = 1 << 0;
// TODO the next is 16 bytes? should be changed accordingly?
#define IO_WAVE_RAM_0 (hram_[ADDR_WAVE_RAM]) // 16 bytes

/** LCD Control Register

    bit 7 = LCD & PPU enable / disable
    bit 6 = window tilemap area ( 0 == 0x9800 - 9bff, 1 = 0x9c00 - 0x9fff)
    bit 5 = window enable 
    bit 4 = BG & Window tile data area ( 0 = 0x8800 - 0x97ff, 1 = 0x8000 - 0x8fff)
    bit 3 = BG tilemap area ( 0 = 0x9800 - 0x9bfff, 1 = 0x9c00 - 0x9fff)
    bit 2 = OBJ size (0 = 8x8, 1 = 8x16)
    bit 1 = OBJ enable
    bit 0 = BG/Win enable / priority -- CGB Specific
 */
#define IO_LCDC (hram_[ADDR_LCDC])
static constexpr uint8_t LCDC_LCD_ENABLE = 1 << 7;
static constexpr uint8_t LCDC_WINDOW_TILEMAP = 1 << 6;
static constexpr uint8_t LCDC_WINDOW_ENABLE = 1 << 5;
static constexpr uint8_t LCDC_BG_WIN_TILEDATA = 1 << 4;
static constexpr uint8_t LCDC_BG_TILEMAP = 1 << 3;
static constexpr uint8_t LCDC_OBJ_SIZE = 1 << 2;
static constexpr uint8_t LCDC_OBJ_ENABLE = 1 << 1;
static constexpr uint8_t LCDC_BG_WIN_PRIORITY = 1 << 0;

/** Status and interrupts for the LCD driver
 
    bit 6 = LYC int select
    bit 5 = mode 2 int select 
    bit 4 = mode 1 int select
    bit 3 = mode 0 int select
    bit 2 = LYC == LY
    bits 0 & 1 = PPU mode 
*/
#define IO_STAT (hram_[ADDR_STAT])
static constexpr uint8_t STAT_WRITE_MASK = 0b01111000;
static constexpr uint8_t STAT_PPU_MODE = 3;
static constexpr uint8_t STAT_LYC_EQ_LY = 1 << 2;
static constexpr uint8_t STAT_INT_MODE0 = 1 << 3;
static constexpr uint8_t STAT_INT_MODE1 = 1 << 4;
static constexpr uint8_t STAT_INT_MODE2 = 1 << 5;
static constexpr uint8_t STAT_INT_LYC = 1 << 6;

/** Scroll X and scroll Y values for the background layer. 
 
    The values can be from 0 to 255, which corresponds to 32x32 tilemap of 8x8 tile sizes. Both SCX and SCY wrap around their limits independently. 
 */
#define IO_SCY (hram_[ADDR_SCY])
#define IO_SCX (hram_[ADDR_SCX])

/** LCD Y Coordinate
     
    Contains the current coordinate of the LCD renderer. 0 to 143 is active renderering, 144 to 153 is VBLANK.  
 */
#define IO_LY (hram_[ADDR_LY])

/** LY compare
 
    When the LY value becomes identical to the LYC value, the STAT bit 2 (LYC == LY) is set and an interrupt can be triggered if enabled. 
 */
#define IO_LYC (hram_[ADDR_LYC])
/** OAM DMA address & start. 
 
    Writing XX to this register starts an OAM transfer from XX00 to FE00 (the OAM memory). During the DMA transfer, the CPU cannot access RAM at all so it usually busy waits for the required number of cycles in HRAM, which is the only accessible memory at the time. 

    For CGB this is less strict (but technically the emulator should not care that much). 

    https://gbdev.io/pandocs/OAM_DMA_Transfer.html#ff46--dma-oam-dma-source-address--start
 */
#define IO_DMA (hram_[ADDR_DMA])

/** DMG palette register. Contains color indices for the 4 available colors. 
 
    bits:            7 6 | 5 4 | 3 2 | 1 0
    pallete indices: -3- | -2- | -1- | -0-
    
    The values in the palette indices correspond to the following colors:

    00 = White 
    01 = Light Gray (green really)
    02 = Dark Gray
    03 = Black
 */
#define IO_BGP (hram_[ADDR_BGP])
#define IO_OBP0 (hram_[ADDR_OBP0])
#define IO_OBP1 (hram_[ADDR_OBP1])
#define IO_WY (hram_[ADDR_WY])
#define IO_WX (hram_[ADDR_WX])
#define IO_KEY1 (hram_[ADDR_KEY1])
#define IO_VBK (hram_[ADDR_VBK])
#define IO_HDMA1 (hram_[ADDR_HDMA1])
#define IO_HDMA2 (hram_[ADDR_HDMA2])
#define IO_HDMA3 (hram_[ADDR_HDMA3])
#define IO_HDMA4 (hram_[ADDR_HDMA4])
#define IO_HDMA5 (hram_[ADDR_HDMA5])
#define IO_RP (hram_[ADDR_RP])
#define IO_BCPS (hram_[ADDR_BCPS])
#define IO_BCPD (hram_[ADDR_BCPD])
#define IO_OCPS (hram_[ADDR_OCPS])
#define IO_OCPD (hram_[ADDR_OCPD])
#define IO_SVBK (hram_[ADDR_SVBK])
#define IO_PCM12 (hram_[ADDR_PCM12])
#define IO_PCM34 (hram_[ADDR_PCM34])
#define IO_IE (hram_[ADDR_IE])

/** Flag values to be used with the instruction macro expansion. The ins macro stores the flag values as either of:
 
    - 0 if the instructions always clears the flag
    - 1 if the instruction always sets the flag
    - -1 if the instruction does not affect the flag
    - Z, N, H C (the flag name) if the instruction updates the flag depending on its values in which case this is done explicitly in the instruction code 
 */
static constexpr int val_Z = -1;
static constexpr int val_N = -1;
static constexpr int val_H = -1;
static constexpr int val_C = -1;
static constexpr int val__ = -1;
static constexpr int val_0 = 0;
static constexpr int val_1 = 1;

namespace rckid::gbcemu {

    void GBCEmu::appendGamesFrom(char const * path, ui::ActionMenu * into) {
        fs::Folder games = fs::folderRead(path);
        for (auto & entry : games) {
            if (entry.isFile() && (fs::ext(entry.name()) == ".gb")) {
                LOG(LL_INFO, "Found game: " << entry.name());
                String eName = entry.name();
                into->add(ui::ActionMenu::Item(
                    fs::stem(eName),
                    assets::icons_64::gameboy,
                    [eName](){
                        LOG(LL_INFO, "running game: " << eName);
                        GamePak * gamepak = new gbcemu::CachedGamePak{fs::fileRead(STR("/games/" << eName))};
                        App::run<GBCEmu>(fs::stem(eName), gamepak);
                        //gbcemu::GBCEmu app{fs::stem(eName), gamepak};
                        //app.loadCartridge(new gbcemu::CachedGamePak{fs::fileRead(STR("/games/" << eName))});    
                        //app.loop();
                    }
                ));
            }
        }
    }


    GBCEmu::GBCEmu(String appName):
        appName_{std::move(appName)},
        vram_{
            new uint8_t[0x2000],
            new uint8_t[0x2000],
        },
        wram_{
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
            new uint8_t[0x1000],
        },
        oam_{ new uint8_t[160]},
        hram_{new uint8_t[256]},
        pixels_{320} {
        apu_.initialize(hram_ + ADDR_WAVE_RAM);
    }

    void GBCEmu::clear() {
        apu_.enable(false);
        delete gamepak_;
        for (uint32_t i = 0; i < 16; ++i)
            delete eram_[i];
        // TODO some more cleanup would be good here
    }

    void GBCEmu::showHomeMenu() {
        ASSERT(gamepak_ != nullptr);
        // clear gamepak caches so that the home menu has the maximum memory available
        gamepak_->clearCaches();
        //LOG(LL_INFO, "Cleared caches");
        //LOG(LL_INFO, "Unallocated memory: " << StackProtection::currentAvailableMemory());
        //RAMHeap::traceChunks();
        ModalApp<void>::showHomeMenu();
        setRomPage(romPage_);
    }

    ui::ActionMenu * GBCEmu::createHomeMenu() {
        ui::ActionMenu * m = ModalApp<void>::createHomeMenu();
        m->add(ui::ActionMenu::Generator("Style", assets::icons_64::paint_palette, [this]() {
            ui::ActionMenu * m = new ui::ActionMenu{};
            m->add(ui::ActionMenu::Item(
                "Classic",
                assets::icons_64::gameboy,
                [this](){
                    palette_[0] = ColorRGB{155, 188, 15};
                    palette_[1] = ColorRGB{139, 172, 15};
                    palette_[2] = ColorRGB{48, 98, 48};
                    palette_[3] = ColorRGB{15, 56, 15};
                }
            ));
            m->add(ui::ActionMenu::Item(
                "Pocket",
                assets::icons_64::gameboy,
                [this](){
                    palette_[0] = ColorRGB{255, 255, 255};
                    palette_[1] = ColorRGB{170, 170, 170};
                    palette_[2] = ColorRGB{85, 85, 85};
                    palette_[3] = ColorRGB{0, 0, 0};
                }
            ));
            m->add(ui::ActionMenu::Item(
                "White on Black",
                assets::icons_64::gameboy,
                [this](){
                    palette_[0] = ColorRGB{0, 0, 0};
                    palette_[1] = ColorRGB{85, 85, 85};
                    palette_[2] = ColorRGB{170, 170, 170};
                    palette_[3] = ColorRGB{255, 255, 255};
                }
            ));
            m->add(ui::ActionMenu::Generator("Customize", assets::icons_64::paint_palette, [this]() {
                ui::ActionMenu * m = new ui::ActionMenu{};
                for (uint32_t i = 0; i < 4; ++i) {
                    m->add(ui::ActionMenu::Item(
                        STR("Color " << i),
                        assets::icons_64::color_picker,
                        [this, i](){
                            auto c = App::run<ColorPicker>(palette_[i]);
                            if (c.has_value())
                                palette_[i] = c.value();
                        }
                    ));
                }
                return m;
            }));
            return m;
        }));
        // only allow entering debug mode when the device itself is in debug mode
        if (debugMode()) {
            m->add(ui::ActionMenu::Item(
                "Debug Mode",
                assets::icons_64::ladybug,
                [this](){
                    debug_ = true;
                }
            ));
            }
        return m;
    }

    void GBCEmu::saveExternalRam() {
        uint32_t numPages = externalRamPages();
        if (numPages == 0)
            return;
        auto f = fs::fileWrite(fs::join(homeFolder(), "eram.dat"));
        for (uint32_t i = 0; i < numPages; ++i)
            f.write(eram_[i], 8192);
   }

    void GBCEmu::loadExternalRam() {
        uint32_t numPages = externalRamPages();
        if (numPages == 0)
            return;
        auto f = fs::fileRead(fs::join(homeFolder(), "eram.dat"));
        if (! f.good())
            return;
        for (uint32_t i = 0; i < numPages; ++i)
            f.read(eram_[i], 8192);
    }

    GBCEmu::~GBCEmu() {
        // save external ram, if any
        if (gamepak_ != nullptr)
            saveExternalRam();
        clear();
        for (uint32_t i = 0; i < 2; ++i)
            delete [] vram_[i];
        for (uint32_t i = 0; i < 8; ++i)
            delete [] wram_[i];
        delete [] oam_;
        delete [] hram_;
    }

    void GBCEmu::save(WriteStream & into) {
        if (apu_.enabled())
            audioPause();
        serialize(into, VERSION);
        // serialize the CPU state
        serialize(into, regs8_, 8);
        serialize(into, sp_);
        serialize(into, pc_);
        serialize(into, cgb_);
        serialize(into, ime_);
        // save the internal state of the timer
        serialize(into, timerDIVModulo_);
        serialize(into, timerTIMAModulo_);
        serialize(into, timerCycles_);
        // serialize VRAM and WRAM
        serialize(into, vram_[0], 0x2000);
        serialize(into, vram_[1], 0x2000);
        for (unsigned i = 0; i < 8; ++i)
            serialize(into, wram_[i], 0x1000);
        // serialize OAM and HRAM
        serialize(into, oam_, 160);
        serialize(into, hram_, 256);
        // now we need to serialize the gamepak's data, which is the eram and the state of the MBC (if any). We do not need to serialize the gamepak as this should be handled by the app name already (i.e. different gamepaks go to different save folders as each gamepak appears as a different app)
        uint32_t eramSize = gamepak_->cartridgeRAMSize() / 8192;
        for (uint32_t i = 0; i < eramSize; ++i)
            serialize(into, eram_[i], 0x2000);
        serialize(into, getRomPage());
        serialize(into, getVideoRamPage());
        serialize(into, getWorkRamPage());
        serialize(into, getExternalRamPage());
        // save the APU state
        serialize(into, apu_);
        // v2 - add the RTCMapping
        serialize(into, rtcMapping_);
        // we do not have to save the PPU state as we only allow interrupting the game during a VBLANK period
        if (apu_.enabled())
            audioResume();
    }

    void GBCEmu::load(ReadStream & from) {
        uint8_t version = deserialize<uint8_t>(from);
        if (version > VERSION) {
            LOG(LL_WARN,  "Unsupported save version, skipping");
            return;
        }
        // load CPU state
        deserialize(from, regs8_, 8);
        deserialize(from, sp_);
        deserialize(from, pc_);
        deserialize(from, cgb_);
        deserialize(from, ime_);
        // load CPU internal state
        deserialize(from, timerDIVModulo_);
        deserialize(from, timerTIMAModulo_);
        deserialize(from, timerCycles_);
        // load VRAM and WRAM
        deserialize(from, vram_[0], 0x2000);
        deserialize(from, vram_[1], 0x2000);
        for (unsigned i = 0; i < 8; ++i)
            deserialize(from, wram_[i], 0x1000);
        // load oam and hram
        deserialize(from, oam_, 160);
        deserialize(from, hram_, 256);
        // load the eram and gamepak state and set the various memory pages properly
        uint32_t eramSize = gamepak_->cartridgeRAMSize() / 8192;
        for (uint32_t i = 0; i < eramSize; ++i)
            deserialize(from, eram_[i], 0x2000);
        setRomPage(deserialize<uint32_t>(from));
        setVideoRamPage(deserialize<uint32_t>(from));
        setWorkRamPage(deserialize<uint32_t>(from));
        setExternalRamPage(deserialize<uint32_t>(from));
        // load the APU state and turn APU on if necessary (or off)
        deserialize(from, apu_);
        // v2 - load the RTCMapping
        if (version >= 2) {
            deserialize(from, rtcMapping_);
        }
        // we do not have to load the PPU state as we only allow interrupting the game during a VBLANK period
    }

    void GBCEmu::loop() {
        // focus itself & blur parent
        focus();
        //clearTilemap();
        //clearTileset();
        //setBreakpoint(0xc2a6);
        while (!shouldExit()) {
#if (GBCEMU_ENABLE_BKPT == 1)
            if (mem8(PC) == 0xfd) // bkpt
                break; 
#endif
            setPPUMode(2); // OAM scan
            runCPU(cgb_ ? DOTS_MODE_2 * 2 : DOTS_MODE_2);
#if (GBCEMU_ENABLE_BKPT == 1)
            if (mem8(PC) == 0xfd) // bkpt
                break; 
#endif
            setPPUMode(3); // VRAM scan
            renderLine();
            runCPU(cgb_ ? DOTS_MODE_3 * 2 : DOTS_MODE_3);
#if (GBCEMU_ENABLE_BKPT == 1)
            if (mem8(PC) == 0xfd) // bkpt
                break; 
#endif
            setPPUMode(0); // HBlank
            runCPU(cgb_ ? DOTS_MODE_0 * 2 : DOTS_MODE_0);

            moveToNextScanline();
            // if we have reached the end of the frame, check the home menu during the VBLANL period
            //if (IO_LY == 144)
            //    ModalApp::update();
        }
        // we are done, should blur ourselves, and refocus parent (if any)
        blur();
    }

    uint32_t GBCEmu::convertAddressToAbsolute(uint16_t addr) {
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        switch (page) {
            case 0: // rom bank 0
            case 1:
            case 2:
            case 3:
                return 0x10000000 + addr;
            case 4: // rom bank 1 or selected
            case 5:
            case 6:
            case 7:
                return 0x10000000 + (romPage_ * 0x4000) + addr - 0x4000;
            case 8: // video ram
            case 9:
                UNIMPLEMENTED;
            case 10: // external ram
            case 11:
                UNIMPLEMENTED;
            case 12: // work ram bank 0
                return 0x20000000 + addr - 0xc000;
            case 13: // work ram bank 1 or selected
                // TODO for GBC this is actually selectable 
                return 0x20000000 + addr - 0xc000;
            case 14: // echo ram, same as WRAM0
                // TODO do we want to distinguish this comes from echo ram? 
                return 0x20000000 + addr - 0xe000;
            case 15:
                if (addr > 0xe00)
                    // TODO selected bank of WRAM for CGB
                    return 0x20000000 + addr - 0xe000;
                else
                    return addr;
            default:
                UNREACHABLE;
        }
    }


    void GBCEmu::runCPU(uint32_t cycles) {
        for (uint32_t c = 0; c < cycles; ) {
#if (GBCEMU_INTERACTIVE_DEBUG == 1)
            markAsVisited(PC);           
            if (PC == breakpoint_ || debug_) {
                debugWrite() << "===== BREAKPOINT ===== (pc " << hex(pc_) << ")\n";
                logDisassembly(PC, PC + 10);
                logState();
                debugInteractive();
            } else if (PC == overBreakpoint_) {
                debugInteractive();
            }
#endif
#if (GBCEMU_ENABLE_BKPT == 1)
            if (mem8(PC) == 0xfd) {
                exit(); // we'll leave the app too
                return;
            }
#endif
            c += step();
        }
    }

    void GBCEmu::onFocus() {
        App::onFocus();
        setSpeedMax();
        initializeDisplay();
        // continue playing audio if enabled
        if (apu_.enabled())
            audioResume();
    }

    void GBCEmu::onBlur() {
        // pause audio (w/o the game running there is no-one to generate samples)
        if (apu_.enabled())
            audioPause();
        setSpeedPct(100);
        App::onBlur();
    }

    void GBCEmu::loadCartridge(GamePak * game) {
        clear();
        App::enableStandaloneMode();
        gamepak_ = game;
        if (gamepak_ == nullptr) {
            mbc_ = MBC::None;
            // TODO do we want to clear memmap? or not necessary?
        } else {
            mbc_ = game->cartridgeMBC();
            switch (mbc_) {
                case MBC::None:
                case MBC::Type1:
                case MBC::Type2:
                case MBC::Type3:
                case MBC::Type5:
                    break;
                default:
                    LOG(LL_ERROR, "Unsupported MBC type " << static_cast<uint8_t>(mbc_));
                    ASSERT((false));
                    break;
            }
            // figure out the size of the external RAM and allocate accordingly
            uint32_t eramSize = externalRamPages();
            for (uint32_t i = 0; i < eramSize; ++i)
                eram_[i] = new uint8_t[0x2000];
            // initialize memory mapping defaults
            memMap_[0] = const_cast<uint8_t *>(gamepak_->getPage(0));
            memMap_[1] = memMap_[0] + 0x1000;
            memMap_[2] = memMap_[0] + 0x2000;
            memMap_[3] = memMap_[0] + 0x3000;
            setRomPage(1);
            setVideoRamPage(0);
            setExternalRamPage(0);
            memMap_[12] = wram_[0];
            memMap_[14] = wram_[0];
        }
        setWorkRamPage(1);
        // register initial values as if this is CGB
        // from https://gbdev.io/pandocs/Power_Up_Sequence.html
        A = 0x11;
        F = FLAG_Z;
        B = 0;
        C = 0;
        D = 0;
        E = 0x08;
        H = 0x00;
        L = 0x7c;
        PC = 0x100;
        SP = 0xfffe;
        // and IO initial values, also from CGB
        IO_JOYP = 0xcf;
        IO_SB = 0x00;
        IO_SC = 0x7f;
        // IO_DIV = 0x18; // from DMG, can't tell on CBG
        IO_TIMA = 0x00;
        IO_TMA = 0x00;
        setIORegisterOrHRAM(IO_ADDR(IO_TAC), 0xf8);
        setIORegisterOrHRAM(IO_ADDR(IO_IF), 0xe1);
        IO_NR10 = 0x80;
        IO_NR11 = 0xbf;
        IO_NR12 = 0xf3;
        IO_NR13 = 0xff;
        IO_NR14 = 0xbf;
        IO_NR21 = 0x3f;
        IO_NR22 = 0x00;
        IO_NR23 = 0xff;
        IO_NR24 = 0xbf;
        IO_NR30 = 0x7f;
        IO_NR31 = 0xff;
        IO_NR32 = 0x9f;
        IO_NR33 = 0xff;
        IO_NR34 = 0xbf;
        IO_NR41 = 0xff;
        IO_NR42 = 0x00;
        IO_NR43 = 0x00;
        IO_NR44 = 0xbf;
        IO_NR50 = 0x77;
        IO_NR51 = 0xf3;
        IO_NR52 = 0xf1;
        IO_LCDC = 0x91;
        // IO_STAT = 0; -- can't tell
        IO_STAT = 0xff;
        IO_SCY = 0;
        IO_SCX = 0;
        IO_LY = 0; // can't tell - but note this is set to 153 in initializeDisplay 
        IO_LYC = 0;
        IO_DMA = 0;
        IO_BGP = 0xfc;
        // IO_OBP0 = 0; // can't tell
        // IO_OBP1 = 0; // can't tell
        IO_WY = 0;
        IO_WY = 1;
        IO_KEY1 = 0x7e;
        IO_VBK = 0xfe;
        IO_HDMA1 = 0xff;
        IO_HDMA2 = 0xff;
        IO_HDMA3 = 0xff;
        IO_HDMA4 = 0xff;
        IO_HDMA5 = 0xff;
        IO_RP = 0x3e;
        // IO_BCPS = 0; // can't tell
        // IO_BCPD = 0; // can't tell
        // IO_OCPS = 0; // can't tell
        // IO_OCPD = 0; // can't tell
        IO_SVBK = 0xf8;
        setIORegisterOrHRAM(IO_ADDR(IO_IE), 0);
        ime_ = false;
        // and reset counters
        timerCycles_ = 0;
        // enable the APU since it is on by default
        apu_.enable(true);
        // set the initial values for the IO registers 
        IO_LY = 0; // ensure we'll start with new frame
        // load external ram from previous, if we have it
        loadExternalRam();
        LOG(LL_INFO, "Cartridge load done, free memory: " << memoryFree());
    #if (GBCEMU_INTERACTIVE_DEBUG == 1)
        resetVisited();
    #endif
    }

    void GBCEmu::setState(uint16_t pc, uint16_t sp, uint16_t af, uint16_t bc, uint16_t de, uint16_t hl, bool ime, uint8_t ie) {
        PC = pc;
        SP = sp;
        AF = af;
        BC = bc;
        DE = de;
        HL = hl;
        ime_ = ime;
        IO_IE = ie;
        IO_IF = 0;
        timerCycles_ = 0;
        IO_TIMA = 0;
    }

    void GBCEmu::writeMem(uint16_t address, std::initializer_list<uint8_t> values) {
        for (uint8_t value : values) {
            uint32_t page = address >> 12;
            uint32_t offset = address & 0xfff;
            if (page == 15)
                memWr8(address, value);
            else 
                memMap_[page][offset] = value;
            ++address;
        }
    }

    uint8_t GBCEmu::readMem(uint16_t address) {
        return memRd8(address);
    }

    uint32_t GBCEmu::step() {
        // first check if there are any interrupts to handle
        if (IO_IF != 0) {
            uint8_t interrupt = IO_IF & IO_IE;
            if (interrupt) {
                // if we are currently executing halt, move to next instruction to terminate the halting
                if (mem8(PC) == 0x76)
                    ++PC;
                // service the interrupt if enabled
                if (ime_) {
                    ime_ = false;
                    stackFramePush();
                    if (interrupt & IF_VBLANK) {
                        PC = 0x40;
                        IO_IF &= ~IF_VBLANK;
                    } else if (interrupt & IF_STAT) {
                        PC = 0x48;
                        IO_IF &= ~IF_STAT;
                    } else if (interrupt & IF_TIMER) {
                        PC = 0x50;
                        IO_IF &= ~IF_TIMER;
                    } else if (interrupt & IF_SERIAL) {
                        PC = 0x58;
                        IO_IF &= ~IF_SERIAL;
                    } else {
                        ASSERT((interrupt & IF_JOYPAD));
                        PC = 0x60;
                        IO_IF &= ~IF_JOYPAD;
                    }
                    return 20;
                }
            }
        }
        uint32_t usedCycles = 0;
#if (GBCEMU_TRACE_INSTRUCTIONS == 1)
        if (! debug_)
           disassembleInstruction(PC);
#endif        
        uint8_t opcode = mem8(PC++);
#if (GBCEMU_TRACE_PC_OPCODE == 1)
        debugWrite() << hex<uint32_t>(convertAddressToAbsolute(PC - 1), false) <<  ": " << hex(opcode, false) << '\n';
        // below code depends on system malloc likely so not working yet
        //printf("%04x: %02x\n", PC - 1, opcode);
#endif
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                usedCycles = CYCLES; \
                __VA_ARGS__ \
                if (val_ ## FLAG_Z != -1) setFlagZ(val_ ## FLAG_Z); \
                if (val_ ## FLAG_N != -1) setFlagN(val_ ## FLAG_N); \
                if (val_ ## FLAG_H != -1) setFlagH(val_ ## FLAG_H); \
                if (val_ ## FLAG_C != -1) setFlagC(val_ ## FLAG_C); \
                break;
            #include "insns.inc.h"
            default:
#if (GBCEMU_INTERACTIVE_DEBUG == 1)
            LOG(LL_ERROR, "Unknown opcode " << hex(opcode) << " at " << hex<uint16_t>(PC - 1));
            debug_ = true;
#else
            UNREACHABLE;
#endif
            break;
        };
        timerCycles_ += usedCycles;
        updateTimer();
        return usedCycles;
    }

#if (GBCEMU_INTERACTIVE_DEBUG == 1)

    void GBCEmu::logDisassembly(uint16_t start, uint16_t end) {
        debugWrite() << "===== DISASSEMBLY ===== (from " << hex(start) << " to " << hex(end) << ")\n";
        for (uint16_t i = start; i < end; ) {
            i += disassembleInstruction(i);
        }
    }

    void GBCEmu::logMemory(uint16_t start, uint16_t end) {
        debugWrite() << "===== MEMORY ===== (from " << hex(start) << " to " << hex(end) << ")\n";
        debugWrite() << "      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n";
        for (uint16_t i = start; i < end; i += 16) {
            debugWrite() << hex(i, false) << ": ";
            for (uint16_t j = 0; j < 16; ++j) {
                debugWrite() << hex(mem8(i + j), false) << ' ';
            }
            debugWrite() << '\n';
        }
    }

    void GBCEmu::logStack(uint32_t n) {
        debugWrite() << "===== STACK =====\n";
        for (uint16_t i = SP, e = SP + n; i != e; i += 2) {
            debugWrite() << hex(i, false) << ": " << hex(mem16(i), false) << '\n';
        }
    }

    void GBCEmu::logState() {
        debugWrite() << "===== CPU STATE =====\n";
        debugWrite() <<  "af:   " <<  hex(AF, false) << " bc:   " <<  hex(BC, false) << " de:   " <<  hex(DE, false) << " hl:   " <<  hex(HL, false) << " sp:   " <<  hex(SP, false) << " pc:   " <<  hex(PC, false) << '\n';

        debugWrite() << "lcdc: " << hex(IO_LCDC, false) << "   stat: " << hex(IO_STAT, false) << "   ly:   " << hex(IO_LY, false) << "   ie:   " << hex(IO_IE, false) << "   if:   " << hex(IO_IF, false) << '\n';
        debugWrite() << "TIMA: " << hex(IO_TIMA, false) << " cycles: " << timerCycles_ << '\n';
    }

    void GBCEmu::logVisited() {
        uint32_t total = 0;
        debugWrite() << "===== VISITED INSTRUCTIONS =====\n";
        for (uint32_t i = 0; i < 32; ++i) {
            for (uint32_t j = 0; j < 8; ++j) {
                if (visitedInstructions_[i] & (1 << j)) {
                    debugWrite() << hex<uint8_t>(i * 8 + j, false) << ' ';
                    ++total;
                } else {
                    debugWrite() << "   ";
                }
            }
            if (i & 1)
                debugWrite() << '\n';
        }
        debugWrite() << "Extended (0xcb) prefix:\n";
        for (uint32_t i = 0; i < 32; ++i) {
            for (uint32_t j = 0; j < 8; ++j) {
                if (visitedInstructions_[i + 32] & (1 << j)) {
                    debugWrite() << hex<uint8_t>(i * 8 + j, false) << ' ';
                    ++total;
                } else {
                    debugWrite() << "   ";
                }
            }
            if (i & 1)
                debugWrite() << '\n';
        }
        debugWrite() << "Total: " << total << '\n';
    }

    void GBCEmu::clearTilemap() {
        for (uint16_t i = 0x9800; i < 0x9fff; ++i)
            memWr8(i, 0);
    }

    void GBCEmu::setTilemap(uint32_t x, uint32_t y, uint8_t tile) {
        uint16_t tAddr = 0x9800 + y * 32 + x;
        memWr8(tAddr, tile);
    }

    void GBCEmu::clearTileset() {
        for (uint16_t i = 0x8000; i < 0x9800; ++i)
            memWr8(i, 0);
    }

    void GBCEmu::setTile(uint8_t index, uint8_t * data) {
        uint16_t tAddr = 0x8800 + index * 16;
        for (uint16_t i = 0; i < 16; ++i)
            memWr8(tAddr + i, data[i]);
    }

    uint32_t GBCEmu::instructionSize(uint8_t opcode) const {
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                return SIZE;
            #include "insns.inc.h"
            default:
                return 1;
        };
    }

    uint32_t GBCEmu::disassembleInstruction(uint16_t addr, bool state) {
        uint8_t opcode = mem8(addr);
        uint32_t size = instructionSize(opcode);
        debugWrite() << hex(addr, false) << ": " << hex(mem8(addr), false) << ' ';
        if (size == 1)
            debugWrite() << "          ";
        else if (size == 2) 
            debugWrite() << hex(mem8(addr + 1), false) << "        ";
        else if (size == 3)
            debugWrite() << hex(mem8(addr + 1), false) << ' ' << hex(mem8(addr + 2), false) << "     ";
        else 
            UNREACHABLE; // invalid instruction size
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                debugWrite() << fillRight(MNEMONIC, 15); \
                break;
            #include "insns.inc.h"
            default:
                debugWrite() << fillRight("???", 15);
        };
        if (state) {
            debugWrite() << hex(AF, false) << " " << hex(BC, false) << " " << hex(DE, false) << " " << hex(HL, false) << " " << hex(SP, false);
            debugWrite() << " " << hex(IO_TIMA, false) << " " << timerCycles_;
        }
        debugWrite() << '\n';
        return size;
    }

    void GBCEmu::markAsVisited(uint16_t pc) {
        uint32_t opcode = mem8(pc);
        if (opcode == 0xcb)
            opcode = 0xff + mem8(pc + 1);
        uint32_t addr = opcode / 8;
        uint32_t bit = opcode & 7;
        visitedInstructions_[addr] |= 1 << bit;
    }

    void GBCEmu::resetVisited() {
        for (uint32_t i = 0; i < 64; ++i)
            visitedInstructions_[i] = 0;
    }

    void GBCEmu::debugInteractive() {
        overBreakpoint_ = 0xffffff; // disable step over breakpoint
        debug_ = false;
        while (true) {
            disassembleInstruction(PC, true);
            uint8_t cmd = debugRead(false);
            switch (cmd) {
                // continue running uninterrupted
                case 'c':
                    debugWrite() << "> continue\n";
                    return;
                // execute single instruction
                case 'n':
                    step();
                    break;
                case 'o':
                    overBreakpoint_ = PC + instructionSize(mem8(PC));
                    return;
                // set breakpoint
                case 'b': {
                    debugWrite() << "? breakpoint address ";
                    breakpoint_ = debugReadHex16();
                    debugWrite() << '\n';
                    break;
                }
                // display cpu info (state & stuff)
                case 'i':
                    logState();
                    break;
                case 's':
                    logStack(10);
                    break;
                // display disassembly
                case 'd': {
                    debugWrite() << "? disassembly start address ";
                    uint16_t start = debugReadHex16();
                    debugWrite() << "\n? length (default 0x10) ";
                    uint16_t l = debugReadHex16();
                    if (l == 0)
                        l = 0x10;
                    logDisassembly(start, start + l);
                    break;
                }
                // display memory
                case 'm': {
                    debugWrite() << "? memory start address ";
                    uint16_t start = debugReadHex16();
                    debugWrite() << "\n? length (default 0x10) ";
                    uint16_t l = debugReadHex16();
                    if (l == 0)
                        l = 0x10;
                    logMemory(start, start + l);
                    break;
                }
                // shows visited instructions
                case 'v':
                    logVisited();
                    break;
                default:
                    debugWrite() << "! invalid command '" << cmd << "'\n";
            }
        }
        UNREACHABLE;
    }

#endif

    void GBCEmu::initializeDisplay() {
        displayClear();
        // set the display to row-first mode, which is what gameboy is expecting and set the resolution to 160x144
        displaySetRefreshDirection(DisplayRefreshDirection::RowFirst);
        switch (displayMode_) {
            case DisplayMode::Native:
                displaySetUpdateRegion(Rect::Centered(160, 144, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT));
                break;
            case DisplayMode::Scaled:
                displaySetUpdateRegion(Rect::Centered(267, 240, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT));
                break;
            case DisplayMode::X2:
                displaySetUpdateRegion(320, 240);
                break;
        }
        // set the render line to last of the VBLANK so that we will start drawing from the top immediately in the next scanline
        IO_LY = 153;
    }

    void GBCEmu::setPPUMode(unsigned mode) {
        // if we are in a vblank technically, don't do anything. This works well because moveToNextScanline sets vblank before updating the line
        if (IO_LY >= 144)
            return;
        // keep mode at 0 when lcd is disabled, do no interrupts
        if ((IO_LCDC & LCDC_LCD_ENABLE) == 0)
            return;
        IO_STAT &= ~ STAT_PPU_MODE; 
        IO_STAT |= (mode & STAT_PPU_MODE);
        // check the interrupts
        switch (mode) {
            case 0: // HBlank
                if (IO_STAT & STAT_INT_MODE0)
                    IO_IF |= IF_STAT;
                break;
            case 1: // VBlank
                IO_IF |= IF_VBLANK;
                if (IO_STAT & STAT_INT_MODE1)
                    IO_IF |= IF_STAT;
                break;
            case 2: // OAM Scan
                if (IO_STAT & STAT_INT_MODE2)
                    IO_IF |= IF_STAT;
                break;
            default: // no interrupt on drawing mode
                break;
        }
    }
    
    /** Moves the PPU to the next scanline.  */
    void GBCEmu::moveToNextScanline() {
        if (IO_LY == 143) {
            setPPUMode(1); // VBlank
            updateIO_JOYP();
            tick();
        }
        if (IO_LY == 153) {
            ModalApp::update();
#ifndef GBCEMU_NO_SPEED_LIMIT
            // TODO do we want a finer grained control here, or is it ok to wait after each frame only? 
            displayWaitVSync();
#endif
        }
        IO_LY = IO_LY == 153 ? 0 : IO_LY + 1;
        // check if we should generate the STAT interrupt
        if (IO_LY == IO_LYC) {
            IO_STAT |= STAT_LYC_EQ_LY;
            // check LYC LY interrupt
            if (IO_STAT & STAT_INT_LYC)
                IO_IF |= IF_STAT;
        } else {
            IO_STAT &= ~STAT_LYC_EQ_LY;
        }
    }
    
    /** TODO this is the simplest rendering possible where we just render the entire line. 
     */
    void GBCEmu::renderLine() {
        // don't do anything if disabled
        if (! (IO_LCDC & LCDC_LCD_ENABLE))
            return;
        // don't do anything if in the Vblank mode
        uint8_t ly = IO_LY;
        if (ly >= 144)
            return;
        // TODO determine which sprites to use

        // figure out the palette we will be using for the row
        uint16_t palette[4];
        uint8_t bgp = IO_BGP;
        palette[0] = palette_[bgp & 3].raw16();
        palette[1] = palette_[(bgp >> 2) & 3].raw16();
        palette[2] = palette_[(bgp >> 4) & 3].raw16();
        palette[3] = palette_[(bgp >> 6) & 3].raw16();

        uint16_t * buffer = pixels_.front();

        uint8_t * vram = memMap_[MEMMAP_VRAM_0];
        // and determine the tileset address, which could be either signed or unsigned addressing.
        uint16_t * tilesetAddress = reinterpret_cast<uint16_t *>(vram + ((IO_LCDC & LCDC_BG_WIN_TILEDATA) ? 0x0000 : 0x1000));
        // determine if we should draw window, or background. Window is visible when enabled and the current LY is greater or equal the WY register. The WX must also fit within the screen (160 pixels, the WX is +7 from the actual position )
        bool drawWindow = (IO_LCDC & LCDC_WINDOW_ENABLE) && (IO_LY >= IO_WY) && (IO_WX < 167);
        // where the window starts x-wise
        int32_t wx = drawWindow ? IO_WX - 7 : 160;
        // draw the line up to the beginning of the window (this is just optimization to save the drawing time of something we will later on cover with the window
        if (! drawWindow || wx <= 0) {
            // calculate the background position we will be drawing. This is the position to the 256x256 background map created by 32x32 tiles. Using the uint8_t values for the coordinates gives us the automatic wraparound
            uint8_t by = ly + IO_SCY;
            uint8_t bx = IO_SCX;
            // determine the row of tiles we will be using and the row of inside the tile (this stays the same for the entire line)
            uint32_t ty = by / 8;
            uint32_t tr = by % 8;
            uint8_t tx = bx / 8;
            // determine tilemap address, which too stays the same for the entire line, we can also add the line offset
            uint8_t * tilemapAddress = vram + ((IO_LCDC & LCDC_BG_TILEMAP) ? 0x1c00 : 0x1800);
            tilemapAddress += ty * 32 + tx; 
            // render the pixels now, we keep x as the current x coordinate on the screen
            int16_t x = - (bx & 0x7);
            while (x < wx) {
                // determine which tile we are using
                // TODO for CGB we also need to determine the tile attributes 
                uint8_t tileIndex = *tilemapAddress++;
                // if we are overflowing the window, we need to wrap around on the *same* line
                if (++tx == 32) {
                    tx = 0;
                    tilemapAddress = vram + ((IO_LCDC & LCDC_BG_TILEMAP) ? 0x1c00 : 0x1800);
                    tilemapAddress += ty * 32; 
                }
                uint16_t * tileAddress = (IO_LCDC & LCDC_BG_WIN_TILEDATA) ? 
                    tilesetAddress + tileIndex * 8 :
                    tilesetAddress + (int8_t)tileIndex * 8;
                uint16_t tileRow =  *(tileAddress + tr);
                // we have the tile pixels, figure out the palette indices, the tile pixels are 2 bits each in 2 panes so we need to first put them together
                uint8_t upper = tileRow >> 8;
                uint8_t lower = tileRow & 0xff;
                for (int i = 7; i >= 0; --i) {
                    uint8_t colorIndex = ((lower >> i) & 1) | (((upper >> i) & 1) << 1);
                    // TODO render the pixel
                    // displaySetPixel(x, ly, palette[colorIndex]);
                    if (x >= 0 && x < 160)
                        buffer[x] = palette[colorIndex];
                    ++x;
                }
            }
        }
        // draw the window now. We always start drawing the window from window 0, 0 and the x and y coordinates only tell us where to draw the window on the screen
        if (drawWindow) {
            uint16_t * winBuffer = buffer + ((wx < 0) ? 0 : wx); 
            int16_t winLength = 160 - wx;
            uint32_t wy = IO_LY - IO_WY;
            // determine the row and the row of tiles we will be using, window always starts from the first column
            uint32_t tr = wy % 8;
            uint32_t ty = wy / 8;
            uint32_t tx = 0;
            // determine tilemap address, which too stays the same for the entire line, we can also add the line offset
            uint8_t * tilemapAddress = vram + ((IO_LCDC & LCDC_WINDOW_TILEMAP) ? 0x1c00 : 0x1800);
            tilemapAddress += ty * 32; 

            while (wx < winLength) {
                // determine which tile we are using
                // TODO for CGB we also need to determine the tile attributes 
                uint8_t tileIndex = *tilemapAddress++;
                // if we are overflowing the window, we need to wrap around on the *same* line
                if (++tx == 32) {
                    tx = 0;
                    tilemapAddress = vram + ((IO_LCDC & LCDC_BG_TILEMAP) ? 0x1c00 : 0x1800);
                    tilemapAddress += ty * 32; 
                }
                uint16_t * tileAddress = (IO_LCDC & LCDC_BG_WIN_TILEDATA) ? 
                    tilesetAddress + tileIndex * 8 :
                    tilesetAddress + (int8_t)tileIndex * 8;
                uint16_t tileRow =  *(tileAddress + tr);
                uint8_t upper = tileRow >> 8;
                uint8_t lower = tileRow & 0xff;
                for (int i = 7; i >= 0; --i) {
                    uint8_t colorIndex = ((lower >> i) & 1) | (((upper >> i) & 1) << 1);
                    if (wx >= 0 && wx < winLength)
                        buffer[wx] = palette[colorIndex];
                    ++wx;
                }
            }
        }

        // now render the sprites. For now, we are just rendering any and all sprites that cross the line we are drawing, as opposed to scanning and prioritizing them so that only 10 will be displayed. The idea is that this is both simpler algorithm and if the sprite limit is not reached by the game also faster to draw. 
        // sprites are only rendered if their rendering is enabled in LCDC (bit 1)
        if (IO_LCDC & LCDC_OBJ_ENABLE) {
            OAMSprite * sprites = reinterpret_cast<OAMSprite *>(oam_);
            // TODO on CGB this changes and can be second bank as well
            uint16_t * tilesetAddress = reinterpret_cast<uint16_t *>(vram_[0]);
            uint32_t objectSize = IO_LCDC & LCDC_OBJ_SIZE ? 16 : 8;
            for (uint32_t i = NUM_SPRITES - 1; i < NUM_SPRITES; --i) {
                OAMSprite & s = sprites[i];
                // Calculate the row address of the sprite's tile
                int32_t sy = ly - s.y();
                // if the sprite does not intersect the current line, skip it
                if (sy < 0 || sy >= objectSize)
                    continue;
                // flip the sprite on horizontal axis
                if (s.yFlip())
                    sy = objectSize - sy;
                // for 8x16 tiles the LSB of the tile index should be ignored
                uint8_t tileIndex = (objectSize == 8) ? s.tile : (s.tile & 0b11111110);
                uint16_t tileRow = (sy >= 8) ? 
                    *(tilesetAddress + (tileIndex + 1) * 8 + (sy - 8)) :
                    *(tilesetAddress + tileIndex * 8 + sy);
                // we have the tile pixels, figure out the palette indices, the tile pixels are 2 bits each in 2 panes so we need to first put them together
                uint8_t upper = tileRow >> 8;
                uint8_t lower = tileRow & 0xff;
                uint32_t x = s.x();
                // update palette for the sprite
                uint8_t bgp = s.palette() ? IO_OBP1 : IO_OBP0;
                palette[0] = palette_[bgp & 3].raw16();
                palette[1] = palette_[(bgp >> 2) & 3].raw16();
                palette[2] = palette_[(bgp >> 4) & 3].raw16();
                palette[3] = palette_[(bgp >> 6) & 3].raw16();
                // and draw the sprite
                if (s.xFlip()) {
                    for (int i = 0; i < 8; ++i) {
                        uint8_t colorIndex = ((lower >> i) & 1) | (((upper >> i) & 1) << 1);
                        if (colorIndex != 0 && x >= 0 && x < 160) // color 0 is transparent
                            buffer[x] = palette[colorIndex];
                        ++x;
                    }
                } else {
                    for (int i = 7; i >= 0; --i) {
                        uint8_t colorIndex = ((lower >> i) & 1) | (((upper >> i) & 1) << 1);
                        if (colorIndex != 0 && x >= 0 && x < 160) // color 0 is transparent
                            buffer[x] = palette[colorIndex];
                        ++x;
                    }
                }
            }
        }
        displayWaitUpdateDone();
        switch (displayMode_) {
            case DisplayMode::Native:
                displayUpdate(buffer, 160);
                break;
            case DisplayMode::Scaled: {
                uint32_t si = SCALED_WIDTH - 1;
                for (uint32_t i = 159; i < 160; --i) {
                    buffer[si--] = buffer[i];
                    if (colScaling_[i] == 2)
                        buffer[si--] = buffer[i];
                }
                if (rowScaling_[ly] == 1) {
                    displayUpdate(buffer, 267);
                } else {
                    displayUpdate(buffer, 267, [buffer](){
                        displayUpdate(buffer, 267, nullptr);
                    });
                }
                break;
            }
            case DisplayMode::X2:
                if (ly >= displayX2Start_ && ly < displayX2Start_ + 120) {
                    displayUpdate(buffer, 160, [buffer](){
                        displayUpdate(buffer, 160, nullptr);
                    });
                }
                break;
            }
        pixels_.swap();
    }

    // memory

    void GBCEmu::setRomPage(uint32_t page) {
        LOG(LL_GBCEMU_ROMBANK, "Setting ROM page to " << page << ", from pc " << hex<uint16_t>(PC - 1));
        romPage_ = page % romPages();
        memMap_[4] = const_cast<uint8_t *>(gamepak_->getPage(romPage_));
        memMap_[5] = memMap_[4] + 0x1000;
        memMap_[6] = memMap_[4] + 0x2000;
        memMap_[7] = memMap_[4] + 0x3000;
    }

    uint32_t GBCEmu::getRomPage() const {
        return romPage_;
    }

    void GBCEmu::setVideoRamPage(uint32_t page) {
        ASSERT((page < 2));
        memMap_[MEMMAP_VRAM_0] = vram_[page];
        memMap_[MEMMAP_VRAM_1] = vram_[page] + 4096;
    }

    uint32_t GBCEmu::getVideoRamPage() const {
        return (memMap_[MEMMAP_VRAM_0] == vram_[0]) ? 0 : 1;
    }

    void GBCEmu::setWorkRamPage(uint32_t page) {
        ASSERT((page > 0 && page < 8));
        memMap_[13] = wram_[page];
        // don't forget to set the echo ram as well here
        memMap_[15] = wram_[page];
    }

    uint32_t GBCEmu::getWorkRamPage() const {
        for (uint32_t i = 0; i < 8; ++i)
            if (memMap_[13] == wram_[i])
                return i;
        UNREACHABLE;
        return 0;
    }

    void GBCEmu::setExternalRamPage(uint32_t page) {
        // wrap around the page number
        uint32_t numPages = externalRamPages();
        page = page % numPages;
        memMap_[10] = eram_[page];
        memMap_[11] = eram_[page] + 4096;
    }

    uint32_t GBCEmu::getExternalRamPage() const {
        for (uint32_t i = 0; i < 16; ++i)
            if (memMap_[10] == eram_[i])
                return i;
        UNREACHABLE;
        return 0;
    }


    // arithmetics

    uint8_t GBCEmu::inc8(uint8_t x) {
        ++x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0);
        return x;
    }
    
    uint8_t GBCEmu::dec8(uint8_t x) {
        --x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0xf);
        return x;
    }
    
    /** Adds two 8bit numbers, optinally including a carry flag and sets the Z, H and C flags accordingly. 

        The Z flag is set if the 8bit result is 0. The C flag is set if the result is greater than 256. Finally, the H flag is set if during 
     */
    uint8_t GBCEmu::add8(uint8_t a, uint8_t b, uint8_t c) {
        unsigned r = a + b + c;
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint8_t GBCEmu::sub8(uint8_t a, uint8_t b, uint8_t c) {
        unsigned r = a - (b + c);
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint16_t GBCEmu::add16(uint16_t a, uint16_t b) {
        uint32_t r = a + b;
        setFlagC(r > 0xffff);
        setFlagH(((r ^ a ^ b) & 0x1000) != 0);
        return static_cast<uint16_t>(r);
    }

    /** Rotate left, set carry
     */
    uint8_t GBCEmu::rlc8(uint8_t a) {
        uint16_t r = a << 1;
        setFlagC(r & 256);
        r = r | flagC();
        return (r & 0xff);
    }

    /** Rotate left through carry. 
     */
    uint8_t GBCEmu::rl8(uint8_t a) {
        uint16_t r = a << 1;
        r = r | flagC();
        setFlagC(r & 256);
        return (r & 0xff);
    }

    /** Rotate right, set carry. 
     */
    uint8_t GBCEmu::rrc8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (flagC() ? 128 : 0);
        return a;
    }

    /** Rotate right, through carry. 
     */
    uint8_t GBCEmu::rr8(uint8_t a) {
        bool cf = flagC();
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (cf ? 128 : 0);
        return a;
    }

    /** Shift left, overflow to carry.
     */
    uint8_t GBCEmu::sla8(uint8_t a) {
        uint16_t r = a * 2;
        setFlagC(r & 256);
        return r & 0xff;
    }

    /** Shift right, arithmetically, i.e. keep msb intact*/
    uint8_t GBCEmu::sra8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a |= (a & 64) ? 128 : 0;
        return a;
    }

    /** Shift right, logically, i.e.msb set to 0. 
     */
    uint8_t GBCEmu::srl8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        return a;
    }

    // memory 

    uint8_t GBCEmu::memRd8(uint16_t addr) {
#if (GBCEMU_INTERACTIVE_DEBUG == 1)
        if (addr >= memoryBreakpointStart_ && addr < memoryBreakpointEnd_) {
            debugWrite() << "===== MEMORY BREAKPOINT ===== (read address " << hex(addr) << ")\n";
            logMemory(memoryBreakpointStart_, memoryBreakpointEnd_);
            debug_ = true;
        }
#endif
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        switch (page) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 12:
            case 13:
            case 14:
                return memMap_[page][offset];
            case 10:
            case 11:
                return readERam(addr - ERAM_START);
            case 15:
                // if the offset is last 256 bytes, return the hram contents
                if (offset >= 0xf00)
                    return hram_[offset - 0xf00];
                // otherwise return OAM if in range
                if (offset >= 0xe00)
                    return oam_[offset - 0xe00];
                // otherwise this is the echo ram, return the wram mirror contents
                return memMap_[page][offset];
            default:
                UNREACHABLE;
        }
    }

    uint16_t GBCEmu::memRd16(uint16_t addr) {
        // TODO this is the naive implementation where we read two bytes and combine them. However, when reading from the lower pages and the read does not cross page boundary, we can optimize this into a single 16bit read
        return memRd8(addr) | (memRd8(addr + 1) << 8);
    }

    void GBCEmu::memWr8(uint16_t addr, uint8_t value) {
#if (GBCEMU_INTERACTIVE_DEBUG == 1)
        if (addr >= memoryBreakpointStart_ && addr < memoryBreakpointEnd_) {
            debugWrite() << "===== MEMORY BREAKPOINT ===== (write address " << hex(addr) << ", value " << hex(value) << ")\n";
            logMemory(memoryBreakpointStart_, memoryBreakpointEnd_);
            debug_ = true;
        }
#endif
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        switch (page) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                // ROM bank switching, depending on the MBC used
                writeRom(addr, value);
                break;
            case 8:
            case 9:
                // for VRAM we only allow writes when not in mode 3, but this does not seem to be used and so saves us the check for now
                //if ((IO_STAT & STAT_PPU_MODE) < 3)
                    memMap_[page][offset] = value;
                break;
            case 10:
            case 11:
                writeERam(addr - ERAM_START, value);
                break;
            case 12:
            case 13:
            case 14:
                // wram are always there so we can do what we want, the shadow mem is implemented having the shadow pages identical to the real ones
                memMap_[page][offset] = value;
                break;
            case 15:
                if (offset >= 0xf00) {
                    setIORegisterOrHRAM(offset - 0xf00, value);
                } else if (offset >= 0xe00) {
                    // otherwise there is the prohibited region after OAM and before HRAM, also OAM is only accessible during blank modes, again does not seem to be used in reality
                    //if ((offset < 0xea0) && ((IO_STAT & STAT_PPU_MODE) <= 1))
                    if (offset < 0xea0)
                        oam_[offset - 0xe00] = value;
                } else {
                    memMap_[page][offset] = value;
                }
                break;
        }
    }

    void GBCEmu::writeRom(uint16_t addr, uint8_t value) {
        switch (mbc_) {
            case MBC::None:
                break;
            /** MBC1 supports four registers, namely:
             
                0000-1fff - writing 0x0a to this enables the external ram, anything else disables it
                2000-3fff - lower 5 bits of the bank number
                4000-5fff - upper 2 bits of the bank number, or RAM bank number
                6000-7fff - banking mode select (0 for simple, 1 for advanced)

                The simple mode does not allow ERAM bank switching, but uses the extra bits of the second register for more ROM banks. In the advanced banking mode, this number is used for RAM switch as well as the upper 2 bits of ROM banks for both (!) pages. Finally, if the page calculated is greater than the number of pages supported, it's truncated. 
             */
            case MBC::Type1:
                if (addr < 0x2000) {
                    // enable/disable external ram by writing 0xa (enable), or anything else to disable
                    eramActive_ = (value & 0xf) == 0xa;
                } else if (addr < 0x4000) {
                    // set the lower 5 bits of the rom bank
                    value = value & 0x1f; // only use 5bits
                    // if this would be 0, treat it as one (note this ignores the extended bits, which causes problems as 0x20, 0x40, etc will be 0x21, etc.). The advanced banking helps with this, but is currently not supported by GBCEmu
                    if (value == 0)
                        value = 1;
                    uint32_t page = (romPage_ & 0xe0) | value;
                    setRomPage(page);
                } else if (addr < 0x6000) {
                    // set the upper 2 bits of the rom bank or the eram bank. This only works if the advanced banking mode is selected below in the register below
                    // TODO we ignore this for now and treat this as always choosing eram page
                    // uint32_t page = (romPage_ & 0x1f) | ((value & 3) << 5);
                    // setRomPage(page);
                    setExternalRamPage(value & 3);
                } else {
                    ASSERT((addr < 0x8000)); 
                    // we do not support advanced banking yet
                    //if (value & 1)
                    //    UNIMPLEMENTED;
                }
                break;
            /** MBC2 supports ROM Banks (0..15) and 512bytes of RAM built into the chip. The 512 ERAM bytes are echoed across the range. There is only single register 0x0000 - 0x3fff which enables / disables the eram and sets the ROM page.
             */
            case MBC::Type2:
                if (addr <= 0x3fff) {

                }
                break;
            /** MBC3 is very similar to MBC1.
             */
            case MBC::Type3:
                if (addr < 0x2000) {
                    // enable/disable external ram by writing 0xa (enable), or anything else to disable
                    eramActive_ = (value & 0xf) == 0xa;
                } else if (addr < 0x4000) {
                    // set the lower 5 bits of the rom bank
                    // if this would be 0, treat it as one (note this ignores the extended bits, which causes problems as 0x20, 0x40, etc will be 0x21, etc.). The advanced banking helps with this, but is currently not supported by GBCEmu
                    if (value == 0)
                        value = 1;
                    setRomPage(value);
                } else if (addr < 0x6000) {
                    // set the eram page, or RTC clock register
                    if (value < 0x08) {
                      setExternalRamPage(value % 4);
                      rtcMapping_ = RTC_MAPPING_NONE;
                    } else {
                        // rtc registers are mapped to the eram values
                        rtcMapping_ = value;
                    }
                } else {
                    ASSERT((addr < 0x8000)); 
                    // RTC latch
                    // TODO
                }
                break;
            /** MBC5 is the first MBC compatible with GBC speeds. It supports ROM banks of up to 0x1ff and RAM sizes of 8, 32, or 128KB. The following registers are supported:

                0000-1fff - writing 0x0a to this enables the external ram, anything else disables it
                2000-2fff - lower 8 bits of the bank number
                3000-3fff - bit 9 of the bank number
                4000-5fff - RAM bank number (if any) and rumbler
             */
            case MBC::Type5:
                if (addr < 0x2000) {
                    eramActive_ = (value & 0xf) == 0xa;
                } else if (addr < 0x3000) {
                    uint32_t page = (romPage_ & 0x00) | value;
                    setRomPage(page);
                } else if (addr < 0x4000) {
                    uint32_t page = (romPage_ & 0xff) | (value << 8);
                    setRomPage(page);
                } else if (addr < 0x6000) {
                    // TODO
                }
                break;
            case MBC::Type6:
            case MBC::Type7:
            default:
                // we should have died earlier
                UNREACHABLE;
        }
    }

    void GBCEmu::writeERam(uint16_t offset, uint8_t value) {
        switch (mbc_) {
            case MBC::None:
                break;
            case MBC::Type1:
                if (eramActive_)
                    memMap_[10][offset] = value;
                break;
            case MBC::Type2:
                UNIMPLEMENTED;
            case MBC::Type3:
                // TODO checkif we are writing the RTC clock instead
                if (eramActive_ && rtcMapping_ == RTC_MAPPING_NONE)
                    memMap_[10][offset] = value;
                break;
            case MBC::Type5:
                if (eramActive_)
                    memMap_[10][offset] = value;
                break;
            case MBC::Type6:
            case MBC::Type7:
                UNIMPLEMENTED;
            default:
                // we should have died earlier
                UNREACHABLE;
        }
    }

    uint8_t GBCEmu::readERam(uint16_t offset) {
        switch (mbc_) {
            case MBC::None:
                return 0xff;
            case MBC::Type1:
                return eramActive_ ? memMap_[10][offset] : 0xff;
            case MBC::Type2:
                UNIMPLEMENTED;
            case MBC::Type3:
                switch (rtcMapping_) {
                    case RTC_MAPPING_NONE:
                        return eramActive_ ? memMap_[10][offset] : 0xff;
                    case RTC_MAPPING_SECONDS:
                    case RTC_MAPPING_MINUTES:
                    case RTC_MAPPING_HOURS:
                    case RTC_MAPPING_DAYS_LOW:
                    case RTC_MAPPING_DAYS_HIGH:
                        // TODO
                        return 0xff;
                    default:
                        UNREACHABLE;
                }
                // TODO checkif we are reading the RTC clock instead
                return eramActive_ ? memMap_[10][offset] : 0xff;
            case MBC::Type5:
                return eramActive_ ? memMap_[10][offset] : 0xff;
            case MBC::Type6:
            case MBC::Type7:
                UNIMPLEMENTED;
            default:
                // we should have died earlier
                UNREACHABLE;
        }
    }

    void GBCEmu::setIORegisterOrHRAM(uint32_t addr, uint8_t value) {
        switch (addr) {
            case ADDR_JOYP:
                // only the upper nibble is writeable, and once written, update the lower nibble accordingly
                IO_JOYP = (IO_JOYP & 0xf) | (value & 0xf0);
                updateIO_JOYP();
                return;
            case ADDR_SB: 
                LOG(LL_GBCEMU_SERIAL, static_cast<char>(value));
                break;
            case ADDR_SC: 
                break;
            case ADDR_DIV: 
                // this is the 16374Hz timer. Any write to the register resets the value to zero
                IO_DIV = 0;
                return; // do not perform the write
            case ADDR_TIMA:
                timerCycles_ = 0;
                break;
            case ADDR_TAC: // IO_TAC
                timerCycles_ = 0;
                if ((value & TAC_ENABLE) == 0) {
                    timerTIMAModulo_ = 0;
                } else switch (value & TAC_CLOCK_SELECT_MASK) {
                    case TAC_CLOCK_SELECT_256M:
                        timerTIMAModulo_ = 1024;
                        break;
                    case TAC_CLOCK_SELECT_4M:
                        timerTIMAModulo_ = 16;
                        break;
                    case TAC_CLOCK_SELECT_16M:
                        timerTIMAModulo_ = 64;
                        break;
                    case TAC_CLOCK_SELECT_64M:
                        timerTIMAModulo_ = 256;
                        break;
                }
                break;
            case ADDR_LCDC: {
                bool alreadyEnabled = IO_LCDC & LCDC_LCD_ENABLE;
                IO_LCDC = value;
                if (alreadyEnabled) {
                    if (value & LCDC_LCD_ENABLE == 0) {
                        IO_LY = 0; // set LY to 0 and keep it there
                        IO_STAT = (IO_STAT & ~STAT_PPU_MODE) & STAT_INT_MODE0; // set stat mode to hblank
                    }
                } else {
                    if (value & LCDC_LCD_ENABLE)
                        initializeDisplay();
                }
                return;
            }
            case ADDR_STAT:
                IO_STAT = (IO_STAT & ~STAT_WRITE_MASK) | (value & STAT_WRITE_MASK);
                return;
            case ADDR_DMA: {
                // the DMA works instantenuously, because the CPU is expected to wait the dedicated number of cycles in HRAM immediately after the write so this is fine
                // TODO the waiting pattern seems to be quite straightforward and the code can be effectively skipped, might save a few cycles if needed
                uint32_t addr = value << 8;
                for (uint32_t i = 0; i < 160; ++i)
                    oam_[i] = mem8(addr + i);
                return;
            }
            default:
                // if we are changing the APU registers, let APU handle the effects of the change and the actual HRAM write)
                if (addr >= APU::ADDR_NR10 && addr <= APU::ADDR_NR52)
                    return apu_.setRegister(addr, value, hram_);
                // fallthrough to the default memory write
                break;
        }
        hram_[addr] = value;
    }

    void GBCEmu::memWr16(uint16_t addr, uint16_t value) {
        memWr8(addr, value & 0xff);
        memWr8(addr + 1, value >> 8);
    }

    uint8_t GBCEmu::mem8(uint16_t addr) {
        // turns out we can actually execute code from hram, so we need to check for that
        if (addr >= 0xff00)
            return hram_[addr - 0xff00];
        // otherwise do the fastpath to the memory pages
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        return memMap_[page][offset];
    }

    uint16_t GBCEmu::mem16(uint16_t addr) {
        // TODO this is the naive implementation where we read two bytes and combine them. However, when reading from the lower pages and the read does not cross page boundary, we can optimize this into a single 16bit read
        return mem8(addr) | (mem8(addr + 1) << 8);
    }


    void GBCEmu::updateTimer() {
        if ((timerCycles_ & timerDIVModulo_) == 0)
            ++IO_DIV;
        if (timerTIMAModulo_ != 0 && (timerCycles_ >= timerTIMAModulo_)) {
            timerCycles_ = timerCycles_ & (timerTIMAModulo_ - 1);
            if (++IO_TIMA == 0) {
                IO_TIMA = IO_TMA;
                IO_IF |= IF_TIMER;
            }
        }
    }

    void GBCEmu::updateIO_JOYP() {
        #undef A
        #undef B
        uint8_t value;
        // note that the JOYP is inverted, i.e. 0 means button is pressed or group selected
        if (IO_JOYP & 0x20) { // dpad
            value = (btnDown(Btn::Down)   ? 0 : 8) |
                    (btnDown(Btn::Up)     ? 0 : 4) |
                    (btnDown(Btn::Left)   ? 0 : 2) |
                    (btnDown(Btn::Right)  ? 0 : 1);
        } else { // buttons
            value = (btnDown(Btn::Start)  ? 0 : 8) |
                    (btnDown(Btn::Select) ? 0 : 4) |
                    (btnDown(Btn::B)      ? 0 : 2) |
                    (btnDown(Btn::A)      ? 0 : 1);
        }
        // check if we need an interrupt to be requested
        if ( ((value & 1) == 0 && (IO_JOYP & 1)) ||
             ((value & 2) == 0 && (IO_JOYP & 2)) ||
             ((value & 4) == 0 && (IO_JOYP & 4)) ||
             ((value & 8) == 0 && (IO_JOYP & 8)) )
            IO_IF |= IF_JOYPAD;
        // set the register
        IO_JOYP = (IO_JOYP & 0xf0) | value;
        // enter debug mode if home button is pressed
        // TODO this will change to showing the home menu instead from here
        //if (btnPressed(Btn::Home))
        //    debug_ = true;
    }

} // namespace rckid::gbcemu


