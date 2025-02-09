#pragma once

/** \page ui UI Framework 
 
    The SDK provides a simple UI framework and assorted widgets. The UI does not need any framebuffer and all the widgets render themselves in a column-wise manner. This makes the UI's memory footprint very small as only a double buffer of 240 pixels and the actual widget data are required, often fitting in afew kilobytes. 
    
    TODO the UI should support basic styling
 */
