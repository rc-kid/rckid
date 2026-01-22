
#include <rckid/hal.h>
#include <rckid/ui/root_widget.h>

namespace rckid::ui {

    RootWidget::RootWidget(): 
        RootWidget(Rect::WH(hal::display::WIDTH, hal::display::HEIGHT)) {
    }

    void RootWidget::initializeDisplay() {
        hal::display::enable(rect(), hal::display::RefreshDirection::ColumnFirst);
    }

    void RootWidget::render() {
        if (! visible())
            return;
        // tell the widgets that we are about to render
        onRender();
        // start rendering from rightmost column
        renderCol_ = width() - 1;
        hal::display::update([&](Color::RGB565 * & buffer, uint32_t & bufferSize) {
            if (renderCol_ < 0) {
                buffer = nullptr;
                return;
            } else if (buffer == nullptr) {
                buffer = renderBuffer_.frontAndSwap();
                bufferSize = height();
            }
            renderColumn(renderCol_--, 0, buffer, height());
        });
    }

} // namespace rckid::ui