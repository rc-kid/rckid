#include "widget.h"
#include "form.h"

namespace rckid::ui {


    bool Widget::focused() const { 
        return form_ != nullptr && form_->focused() == this;
    }

    /** Focuses the widget on which the method is called. 
     */
    void Widget::focus() { 
        if (form_ != nullptr)
            form_->setFocused(this);
    }


}