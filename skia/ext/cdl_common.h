/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKIA_EXT_CDL_CONFIG_H_
#define SKIA_EXT_CDL_CONFIG_H_

#define CDL_ENABLED 1

#if CDL_ENABLED

class CdlAutoCanvasRestore;
class CdlCanvas;
class CdlNoDrawCanvas;
class CdlPassThroughCanvas;
class CdlPaint;
class CdlPicture;
class CdlPictureRecorder;
class CdlShader;
class CdlSurface;

#else

class SkAutoCanvasRestore;
typedef SkAutoCanvasRestore CdlAutoCanvasRestore;

class SkCanvas;
typedef SkCanvas CdlCanvas;

class SkNoDrawCanvas;
typedef SkNoDrawCanvas CdlNoDrawCanvas;

class CdlPassThroughCanvas;

class SkPaint;
typedef SkPaint CdlPaint;

class SkPicture;
typedef SkPicture CdlPicture;

class SkPictureRecorder;
typedef SkPictureRecorder CdlPictureRecorder;

class SkShader;
typedef SkShader CdlShader;

class SkSurface;
typedef SkSurface CdlSurface;

#endif

#endif  // SKIA_EXT_CDL_CONFIG_H_
