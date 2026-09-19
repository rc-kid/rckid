
#include <rckid/ui/style.h>

#include <rckid/apps/music_player.h>
#include <rckid/apps/friends.h>
#include <rckid/apps/messages.h>
#include <rckid/apps/drawing.h>

#include <rckid/apps/utils/about.h>
#include <rckid/apps/utils/steps.h>
#include <rckid/apps/utils/data_sync.h>
#include <rckid/apps/utils/file_browser.h>
#include <rckid/apps/utils/clock.h>
#include <rckid/apps/utils/stopwatch.h>
#include <rckid/apps/utils/flashlight.h>
#include <rckid/apps/utils/piggy_bank.h>
#include <rckid/apps/utils/calculator.h>
#include <rckid/apps/utils/level.h>

#include <rckid/apps/debug/hwstatus.h>
#include <rckid/apps/debug/bootloader.h>
#include <rckid/apps/debug/serial_monitor.h>
#include <rckid/apps/debug/jacdac_monitor.h>
#include <rckid/apps/debug/jacdac_service.h>

#include <rckid/apps/dialogs/file_dialog.h>
#include <rckid/apps/dialogs/color_dialog.h>

#include <rckid/apps/games/blocks.h>


#include <rckid/apps/audio/recorder.h>

#include <rckid/apps/launcher.h>


// TODO this will not be here eventually
#include <rckid/game/engine.h>
#include <rckid/apps/games/cat_chase.h>

namespace rckid {

    ui::MenuItem::GeneratorEvent mainMenuGenerator(MainMenuOptions options) {
        return [options]() {
            auto result = std::make_unique<LauncherMenu>();
            (*result)
                << ui::MenuItem::Generator("Games", assets::icons_64::game_controller, [extend = options.gamesExtender]() {
                    auto gamesMenu = gamesMenuGenerator();
                    if (extend != nullptr)
                        return extend(std::move(gamesMenu));
                    else 
                        return gamesMenu;
                })
                << ui::MenuItem::Generator("Music", assets::icons_64::music, 
                    MusicPlayer::generateLauncherMenu
                )
                << ui::MenuItem::Generator("Friends", assets::icons_64::birthday_cake,
                    Friends::generateLauncherMenu
                )
                << ui::MenuItem{"Messages", assets::icons_64::chat, []() {
                    App::run<Messages>();
                }}
                << ui::MenuItem{"Drawing", assets::icons_64::paint_palette, []() {
                    auto canvas = std::make_unique<Canvas>(32, 32);
                    App::run<Drawing>(canvas.get());
                }}
                << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator)
                // TODO change this to proper menu, using the options ideally
                //<< ui::MenuItem::Generator("Jacdac", assets::icons_64::jacdac, utilitiesMenuGenerator)
                << ui::MenuItem("Jacdac", assets::icons_64::jacdac, []() {
                    App::run<JacdacMonitor>();
                })
                << ui::MenuItem("Jacdac IMU", assets::icons_64::microchip, []() {
                    App::run<JacdacService>();
                })
                << ui::MenuItem::Generator("Settings", assets::icons_64::settings, settingsMenuGenerator);
            if (debug::debugMode())
                (*result)
                    << ui::MenuItem::Generator("Debug", assets::icons_64::ladybug, debugMenuGenerator);
            // extend the menu with any cartridge specific details
            if (options.cartridgeExtender)
                result = options.cartridgeExtender(std::move(result));
            return result;
        };
    }

    unique_ptr<LauncherMenu> gamesMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
            (*result)
                << ui::MenuItem{"Game Engine", assets::icons_64::gameboy, []() {
                    App::run<CatChase>();
                }}
                << ui::MenuItem{"Blocks", assets::icons_64::tetris, []() {
                    App::run<Blocks>();
                }};
        return result;
    }

    unique_ptr<LauncherMenu> utilitiesMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem{"Clock", assets::icons_64::alarm_clock, []() {
                App::run<Clock>();
            }}
            << ui::MenuItem{"Stopwatch", assets::icons_64::chronometer, []() {
                App::run<Stopwatch>();
            }}
            << ui::MenuItem{"Flashlight", assets::icons_64::flashlight, []() {
                App::run<Flashlight>();
            }}
            << ui::MenuItem{"Calculator", assets::icons_64::calculator, []() {
                App::run<Calculator>();
            }}
            << ui::MenuItem{"Level", assets::icons_64::level, []() {
                App::run<Level>();
            }}
            << ui::MenuItem{"Steps", assets::icons_64::footprint, []() {
                App::run<Steps>();
            }}
            << ui::MenuItem{"Piggy Bank", assets::icons_64::piggy_bank, []() {
                App::run<PiggyBank>();
            }}
            << ui::MenuItem{"Data Sync", assets::icons_64::pen_drive, []() {
                App::run<DataSync>();
            }}
            << ui::MenuItem::Generator("File Browser", assets::icons_64::folder,
                FileBrowser::rootMenuGenerator
            );

        return result;
    }

    unique_ptr<LauncherMenu> styleSettingsMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem::Generator("Background", assets::icons_64::picture, [](){
                auto result = std::make_unique<LauncherMenu>();
                (*result)
                    << ui::MenuItem{"Image", assets::icons_64::picture, []() {
                        auto path = App::run<FileDialog>("/files/images/backgrounds");
                        if (path) {
                            ui::Style::setBackgroundImage(ImageSource{path.value()});
                            // force repaint
                            App::refreshStyle();
                            ui::Style::saveDefaultStyle();
                        }
                    }};
                return result;
            })
            << ui::MenuItem::Generator("Colors", assets::icons_64::light, [](){
                auto result = std::make_unique<LauncherMenu>();
                (*result)
                    << ui::MenuItem{"Text", assets::icons_64::light, []() {
                        auto color = App::run<ColorDialog>(ui::Style::defaultFg());
                        if (color) {
                            ui::Style::setDefaultFg(color.value());
                            App::refreshStyle();
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Background", assets::icons_64::light, []() {
                        auto color = App::run<ColorDialog>(ui::Style::defaultBg());
                        if (color) {
                            ui::Style::setDefaultBg(color.value());
                            App::refreshStyle();
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Accent Text", assets::icons_64::light, []() {
                        auto color = App::run<ColorDialog>(ui::Style::accentFg());
                        if (color) {
                            ui::Style::setAccentFg(color.value());
                            App::refreshStyle();
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Accent Bg", assets::icons_64::light, []() {
                        auto color = App::run<ColorDialog>(ui::Style::accentBg());
                        if (color) {
                            ui::Style::setAccentBg(color.value());
                            App::refreshStyle();
                            ui::Style::saveDefaultStyle();
                        }
                    }};
                return result;
            })
            // key settings (autorepeat speed, accel as joystick, etc)
            << ui::MenuItem::Generator("Keys", assets::icons_64::numpad, [](){
                auto result = std::make_unique<ui::Menu>();
                /* TODO should this really be user controllable
                (*result)
                    << ui::MenuItem{"Repeat", assets::icons_64::picture, []() {
                        UNIMPLEMENTED;
                    }};
                */
                return result;
            });

        return result;
    }

    void updateRGBEffectStyle(rgb::KeyboardEffect effect) {
        rgb::setKeyboardEffect(effect, ui::Style::keyboardRGBColor());
        ui::Style::setKeyboardEffect(effect);
        ui::Style::saveDefaultStyle();
    }

    unique_ptr<LauncherMenu> rgbEffectSettingsMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem{"Press", assets::icons_64::letter_a_1, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::Press);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::Press;
            })
            << ui::MenuItem{"Rainbow Press", assets::icons_64::rainbow, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::RainbowPress);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::RainbowPress;
            })
            << ui::MenuItem{"Solid", assets::icons_64::light, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::Solid);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::Solid;
            })
            << ui::MenuItem{"Breathe", assets::icons_64::star, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::Breathe);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::Breathe;
            })
            << ui::MenuItem{"Rainbow", assets::icons_64::rainbow, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::Rainbow);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::Rainbow;
            })
            << ui::MenuItem{"Rainbow Wave", assets::icons_64::rainbow, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::RainbowWave);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::RainbowWave;
            })
            << ui::MenuItem{"Off", assets::icons_64::turn_off, []() {
                updateRGBEffectStyle(rgb::KeyboardEffect::Off);
            }}.withCheckDecorator([]() {
                return ui::Style::keyboardEffect() == rgb::KeyboardEffect::Off;
            });
        return result;
    }

    unique_ptr<LauncherMenu> rgbSettingsMenuGenerator() {
        using namespace ui;
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem::Generator("Effect", assets::icons_64::numpad, rgbEffectSettingsMenuGenerator)
            << ui::MenuItem{"Color", assets::icons_64::light, []() {
                auto color = rckid::App::run<ColorDialog>(ui::Style::accentBg());
                if (color) {
                    ui::Style::setKeyboardRGBColor(color.value());
                    rgb::setKeyboardEffect(ui::Style::keyboardEffect(), ui::Style::keyboardRGBColor());
                    ui::Style::saveDefaultStyle();
                }
            }}
            << ui::MenuItem{"Brightness", assets::icons_64::brightness, [](){
                CarouselMenu * c = Launcher::instance()->carousel();
                c->showSubwidget(std::unique_ptr<Widget>{
                    new ProgressBarSubWidget{c, 0, 15, rckid::rgb::brightness(), [](int32_t value) {
                        rckid::rgb::setBrightness(static_cast<uint8_t>(value));
                        rgb::setKeyboardEffect(ui::Style::keyboardEffect(), ui::Style::keyboardRGBColor());
                    }}
                });
            }};
        return result;
    }

    unique_ptr<LauncherMenu> rumblerSettingsMenuGenerator() {
        using namespace ui;
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << MenuItem{"Strength", assets::icons_64::vibration, []() {
                CarouselMenu * c = Launcher::instance()->carousel();
                c->showSubwidget(std::unique_ptr<Widget>{
                    new ProgressBarSubWidget{c, 0, 15, rckid::rumbler::strength(), [](int32_t value) {
                        rckid::rumbler::setStrength(static_cast<uint8_t>(value));
                    }}
                });
            }}
            << ui::MenuItem{"Key Press", assets::icons_64::down_arrow, []() {
                // TODO on/off subwidget
            }};
        return result;
    }

    unique_ptr<LauncherMenu> parentModeMenuGenerator() {
        using namespace ui;
        auto result = std::make_unique<LauncherMenu>();
        if (pim::parentMode()) {
            (*result)
                // TODO add parent mode options
                << MenuItem{"Clear Device Password", assets::icons_64::unlock, []() {
                    pim::setPassword("");
                    InfoDialog::info("Parent Mode", "Device password cleared", assets::icons_64::unlock);
                }}
                << MenuItem{"Change Parent password", assets::icons_64::key, []() {
                    auto pwd = rckid::App::run<TextDialog>("");
                    if (pwd) {
                        bool empty = pwd.value().empty();
                        pim::setParentPassword(std::move(pwd.value()));
                        if (empty)
                            InfoDialog::info("Password", "Password cleared", assets::icons_64::unlock);
                        else
                            InfoDialog::info("Password", "Password updated", assets::icons_64::lock);
                    }
                }}
                << MenuItem("Mute Speaker", assets::icons_64::silent, []() {
                    audio::setMuteSpeaker(!audio::muteSpeaker());
                }).withToggleDecorator([]() {
                    return audio::muteSpeaker();
                })
                << MenuItem{"Leave", assets::icons_64::logout, []() {
                    pim::leaveParentMode();
                    InfoDialog::info("Parent Mode", "Parent mode disabled", assets::icons_64::family);
                }};
        } else {
            (*result)
                << MenuItem{"Enter", assets::icons_64::family, []() {
                    pim::enterParentMode();
                    InfoDialog::info("Parent Mode", "Parent mode enabled", assets::icons_64::family);
                }};
        }
        return result;
    }

    unique_ptr<LauncherMenu> settingsMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem::Generator("Style", assets::icons_64::paint_palette, styleSettingsMenuGenerator)
            << ui::MenuItem::Generator("Lights", assets::icons_64::brightness_1, rgbSettingsMenuGenerator)
            << ui::MenuItem::Generator("Rumbler", assets::icons_64::vibration, rumblerSettingsMenuGenerator)
            << ui::MenuItem{"Password", assets::icons_64::key, []() {
                auto pwd = App::run<TextDialog>("");
                if (pwd) {
                    pim::setPassword(std::move(pwd.value()));
                    InfoDialog::info("Password", "Password updated");
                }
            }}
            << ui::MenuItem::Generator("Parent Mode", assets::icons_64::family, parentModeMenuGenerator)
            << ui::MenuItem{"About", assets::icons_64::info, []() {
                App::run<About>();
            }};
        return result;
    }

    unique_ptr<LauncherMenu> debugMenuGenerator() {
        auto result = std::make_unique<LauncherMenu>();
        (*result)
            << ui::MenuItem{"Recorder", assets::icons_64::microphone, []() {
                App::run<Recorder>();
            }}
            << ui::MenuItem{"HW Status", assets::icons_64::gameboy, []() {
                App::run<HWStatus>();
            }}
            << ui::MenuItem{"Bootloader", assets::icons_64::microchip, []() {
                App::run<Bootloader>();
            }}
            << ui::MenuItem{"Serial", assets::icons_64::microchip, []() {
                App::run<SerialMonitor>();
            }}
            << ui::MenuItem{"Leave", assets::icons_64::logout, []() {
                debug::setDebugMode(false);  
                // TODO and exit the debug menu
            }}.withPayload(Launcher::PAYLOAD_MOVE_DOWN);

        return result;
    }

} // namespace rckid