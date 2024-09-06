#pragma once

/** \defgroup graphics Graphics
 
    Graphics suppport in the SDK revolves around two basic engines - bitmaps and tile engines. Bitmaps offer the traditional frame-buffer approach to displaying graphics data, which offers great flexibility, but is CPU and memory intensive. Tile engines on the other hand split the screen into smaller rectangles (tiles), where each tile can be thought of a small simple image and a tilemap that tells the engine what tiles to render at what position, similarly to how GPUs worked in the olden days of Gameboys. Tile engines are more restrictive in their use and require more planning ahead, but they are less computationally intensive and consume much less RAM. 

    The SDK is heavily templated, mostly based on color, of which three variants are supported - ColorRGB which corresponds to the 5-6-5 display native format, capable of displaying 65k colors, Color256 where colors are selected from a palette of 256 colors and Color16 where the palette only supports 16 values. 

    ## Bitmap Drawing

    The simplest idea is a pixel buffer, which is simply a buffer containing pixel information in right to left, top to bottom native display format. Functions (often optimized for RCKid architecture) that manipulate pixel buffers are defined in drawing.h. 

    Drawable objects then follow a simple hierarchy:

    - Surface is simply a pixel buffer paired with information about its size. It has minimal overhead, but suppprts all drawing methods and blitting of raw data. 
    - Bitmap is a surface and a palette, and as such can actually be rendered. 
    - Canvas is bitmap and drawing state, such as foreground and background color, font, etc. 

    ## Tile Engines



    ## Rendering 

    Rendering is decouple from the drawing objects. A Renderer class is specialized for all drawable class that can render itself and provides the basic rendering API functions (initialization, finalization and actual rendering) and contains all rendering related data. 

  */
    