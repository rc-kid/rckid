
#include <rckid/ui/animation.h>
#include <rckid/ui/style.h>
#include <rckid/ui/root_widget.h>

namespace rckid::ui {

    RootWidget::RootWidget(): 
        RootWidget(Rect::WH(hal::display::WIDTH, hal::display::HEIGHT)) {
    }

    void RootWidget::initializeDisplay() {
        display::enable(rect(), hal::display::RefreshDirection::ColumnFirst);
        if (useBackgroundImage_ && background_ == nullptr)
            setBackgroundImage(Style::defaultStyle());
    }

    void RootWidget::render() {
        if (! visible())
            return;
        // update all animations & render essentials
        Widget::renderEssentials();
        // tell the widgets that we are about to render
        onRender();
        // start rendering from rightmost column
        hal::display::update([this, renderCol = width() - 1](Color::RGB565 * & buffer, uint32_t & bufferSize) mutable {
            ASSERT(renderCol >= 0);
            if (buffer == nullptr) {
                buffer = renderBuffer_.front().data();
                bufferSize = height();
                ASSERT(bufferSize <= renderBuffer_.size());
                renderBuffer_.swap();
            }
            renderColumn(renderCol--, 0, buffer, height());
        });
    }

} // namespace rckid::ui