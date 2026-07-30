#pragma once

// ────────────────────────────────────────────────────
//  GL Loader — platform-dispatch GL header
// ────────────────────────────────────────────────────
//  Desktop (Linux/macOS/Windows): GLAD loader for OpenGL 4.6 Core
//  Emscripten (WASM):          native GLES3 for WebGL 2.0
//
//  Include this header instead of <glad/gl.h> or <GLES3/gl3.h>
//  directly. It must be included before <exd/core/macros.hpp>
//  so that GL_CALL can find glGetError / GLenum / GL_NO_ERROR.

#ifdef __EMSCRIPTEN__
  #include <GLES3/gl3.h>
  // On Emscripten, glXxx are real functions — no GLAD function-pointer
  // indirection.  The GL_CALL debug wrapper from exd/core/macros.hpp
  // works correctly because GLES3 provides glGetError / GLenum / GL_NO_ERROR.
#else
  #include <glad/gl.h>
  // GLAD redefines glXxx as glad_glXxx function pointers, populated at
  // runtime by gladLoadGL() in external/src/gl.c.
#endif
