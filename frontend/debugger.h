#ifndef DEBUGGER_UI_H
#define DEBUGGER_UI_H

#include "gbemu.h"

// Build one frame of debugger panels (registers, disassembly, memory,
// breakpoints). Call between gb_imgui_new_frame() and gb_imgui_render()
// while the debugger window is visible. Safe to call with dbg == NULL.
void debug_ui_panels(gb_debugger* dbg);

#endif
