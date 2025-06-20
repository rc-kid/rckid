//
// PNG Decoder
//
// written by Larry Bank
// bitbank@pobox.com
// Arduino port started 5/3/2021
// Original PNG code written 20+ years ago :)
// The goal of this code is to decode PNG images on embedded systems
//
// Copyright 2021 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================
//
#include "PNGdec.h"

// forward references
PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
PNG_STATIC int DecodePNG(PNGIMAGE *pImage, void *pUser, int iOptions);
PNG_STATIC uint8_t PNGMakeMask(PNGDRAW *pDraw, uint8_t *pMask, uint8_t ucThreshold);
// Include the C code which does the actual work
#include "png.inl"

