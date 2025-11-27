#pragma once

#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../ui/tilemap.h"

#include "../../wifi.h"

namespace rckid {

    class WiFiScan  : public ui::Form<void> {
    public:

        String name() const override { return "WiFiScan"; }

        WiFiScan():
            ui::Form<void>{},
            text_{40,15, assets::System16, palette_} {
            g_.addChild(text_);
            text_.setPos(0, 16);
            text_.text(0, 0) << "WiFi Scan";
            wifi_ = WiFi::getOrCreateInstance();
            if (wifi_ != nullptr) {
                wifi_->enable();
            }
        } 

        ~WiFiScan() override {
            wifi_->enable(false);
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::A)) {
                wifi_->connect();

/*                
                line_ = 0;
                println("Scanning...         ");
                wifi_->scan([this](String && ssid, int16_t rssi, WiFi::AuthMode authMode) {
                    if (ssid.empty()) {
                        println("Scan complete      ");
                    } else {
                        println(STR(ssid << " " << rssi << "dBm " << authMode));
                    }
                });
*/
            }
            if (btnPressed(Btn::Start)) {
                wifi_->http_get("api.telegram.org", "/botFoobar/getMe", [this](uint32_t status, uint32_t size, uint8_t const * data) {
                    LOG(LL_INFO, "HTTPS GET complete, status: " <<  status << ", size: " << size);
                    for (uint32_t i = 0; i < size; ++i)
                        debugWrite() << (char)(data[i]);
                });
            }
        }

    private:

        void println(String const & str) {
            text_.text(0, line_) << str;
            line_++;
            if (line_ >= 14) {
                line_ = 0;
            }
        }

        ui::Tilemap<Tile<8, 16, Color16>> text_;
        Coord line_ = 0;

        WiFi * wifi_;
/*

        WiFiScan():
            ui::Form<void>{},
            text_{40,15, assets::System16, palette_} {
            g_.addChild(text_);
            text_.setPos(0, 16);
            if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE)) {
                text_.text(0, 0) << "failed to initialise";
            } else {
                cpu::delayMs(100);
                cyw43_arch_gpio_put(0, 1);
                cyw43_arch_enable_sta_mode();
            }
        }

        ~WiFiScan() override {
            cyw43_arch_deinit();
        }

        void update() override {
            cyw43_arch_poll();
            ui::Form<void>::update();
            if (btnPressed(Btn::A)) {
                uint8_t mac[6];
                if (cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac) == 0) {
                    text_.text(0, 0) <<  "MAC: " << hex(mac[0]) << ":" << hex(mac[1]) << ":" << hex(mac[2]) << ":" << hex(mac[3]) << ":" << hex(mac[4]) << ":" << hex(mac[5]);
                } else {
                    text_.text(0, 0) << "No MAC address (chip not responding)";
                }
                cyw43_arch_gpio_put(0, 1);
                cyw43_wifi_scan_options_t options = {};
                int32_t res = cyw43_wifi_scan(&cyw43_state, & options, this, scan_result);
                cyw43_arch_poll();
                text_.text(0, 1) << "Scan result: " << res;
                cyw43_arch_poll();
            }
            //text_.text(0, 13) << "Callbacks: " << numCallbacks << "      " << cyw43_wifi_scan_active(&cyw43_state);
            cyw43_arch_poll();
        }

    private:
       ui::Tilemap<Tile<8, 16, Color16>> text_;


        static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
            WiFiScan *self = static_cast<WiFiScan *>(env);
            ++self->numCallbacks;
            if (result != nullptr)
                return self->showNetwork(result->ssid, result->rssi);
            return 0;
        }

        uint32_t numCallbacks = 0;

        Coord line = 2;

        int showNetwork(const uint8_t *ssid, int32_t rssi) {
            String ssidStr;
            for (int i = 0; i < 32; ++i) {
                if (ssid[i] == 0)
                    break;
                ssidStr.append(char(ssid[i]));
            }
            text_.text(0, line) << ssidStr << " " << rssi << "dBm      ";
            line++;
            if (line >= 14) {
                line = 2;
                return 1;
            } else {
                return 0;
            }
        }

        */


        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0, 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            ColorRGB{0x00, 0xff, 0x00}.toRaw(),
        };


    }; // rckid::WiFiScan

} // namespace rckid