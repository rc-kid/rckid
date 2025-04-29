#include <rckid/rckid.h>
#include <gbcemu/gbcemu.h>
#include <gbcemu/../tests/bootloader.h>
#include <gbcemu/../tests/roms/blargg_cpu_instrs.h>
#include <gbcemu/gamepak.h>

#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/ui/label.h>
#include <rckid/ui/form.h>

using namespace rckid;

/** Comments
 
    NumCycles:              2,191,855

    Initial run:            3,220,456 [uS]
    Nodraw:                 1,497,884
    Direct vram addressing: 2,966,504
 */


class GBCStats : public ui::App<void> {
public:

    GBCStats(uint32_t t, uint32_t cycles): ui::App<void>{320, 240} {

        ui::Label * l = new ui::Label{0, 50, STR(t)};
        l->setFont(Font::fromROM<assets::OpenDyslexic64>());
        l->setColor(ColorRGB{255, 255, 255});
        l->setWidth(320);
        l->setHeight(50);
        g_.add(l);
        ui::Label * l2 = new ui::Label{0, 100, STR(cycles)};
        l2->setFont(Font::fromROM<assets::OpenDyslexic64>());
        l2->setColor(ColorRGB{128, 128, 128});
        l2->setWidth(320);
        l2->setHeight(50);
        g_.add(l2);
        g_.setRect(Rect::WH(320, 240));
    }

    void update() override {
        ui::App<void>::update();
        if (btnPressed(Btn::B))
            exit();
    }
}; // GBCStats

int main() {
    initialize();
    while (true) {
        Arena::enter();
        auto app = gbcemu::GBCEmu{Arena::allocator()};
        //app.loadCartridge(new gbcemu::FlashGamePak{});
        if (false)
        app.loadCartridge(new gbcemu::FlashGamePak{
            gbcemu::DMGBootloader
            //rckid::gbcemu::rom::blargg::instrs::special
            //rckid::gbcemu::rom::blargg::instrs::interrupts
            //rckid::gbcemu::rom::blargg::instrs::op_sp_hl
            //rckid::gbcemu::rom::blargg::instrs::op_r_imm
            //rckid::gbcemu::rom::blargg::instrs::op_rp
            //rckid::gbcemu::rom::blargg::instrs::ld_r_r
            //rckid::gbcemu::rom::blargg::instrs::jr_jp_call_ret_rst
            //rckid::gbcemu::rom::blargg::instrs::misc_instrs
            //rckid::gbcemu::rom::blargg::instrs::op_r_r
            //rckid::gbcemu::rom::blargg::instrs::bit_ops
            //rckid::gbcemu::rom::blargg::instrs::op_a__hl_
        });
        app.loadCartridge(new gbcemu::FileGamePak("/mnt/c/delete/testrom2.gb"));
        //app.loadCartridge(new gbcemu::FileGamePak("gbcemu/roms/blargg-test-roms/instr_timing/instr_timing.gb"));
        //app.setTerminateAfterStop(true);
        uint32_t t = uptimeUs();
        app.run();
        t = uptimeUs() - t;
        auto stats = GBCStats{t, app.elapsedCycles()};
        stats.run();
        Arena::leave();
    }
}