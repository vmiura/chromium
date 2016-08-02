// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CLIENT_CLIENT_EXPORT_H_
#define CC_CLIENT_CLIENT_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(CC_CLIENT_IMPLEMENTATION)
#define CC_CLIENT_EXPORT __declspec(dllexport)
#else
#define CC_CLIENT_EXPORT __declspec(dllimport)
#endif  // defined(CC_CLIENT_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(CC_CLIENT_IMPLEMENTATION)
#define CC_CLIENT_EXPORT __attribute__((visibility("default")))
#else
#define CC_CLIENT_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define CC_CLIENT_EXPORT
#endif

#endif  // CC_CLIENT_CLIENT_EXPORT_H_
