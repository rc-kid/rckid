#pragma once

namespace rckid::ui {

    /** Style for ui widgets. 
     
        Styles are object that define general widget visualization properties, such as colors, accents, fonts, etc. Widgets then support applying styles to themselves. The styling is simple, as the style merely holds the properties and the widgets determines what properties from the style to apply and how. 

        There is no connection between styles and widgets, i.e. applying style to a widget, then changing the style does not change the widget. Applying a style to a widget is always explicit. 
     */
    class Style {

    }; // rckid::ui::Style



} // namespace rckid