#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "debugger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISASM_LINES 42
#define MEM_ROWS 48
#define MEM_COLS 16

static const ImVec4 c_title = {0.62f, 0.78f, 1.00f, 1.00f};
static const ImVec4 c_pc = {1.00f, 0.85f, 0.25f, 1.00f};
static const ImVec4 c_run = {0.35f, 0.90f, 0.45f, 1.00f};
static const ImVec4 c_pause = {1.00f, 0.55f, 0.35f, 1.00f};
static const ImVec4 c_dim = {0.55f, 0.55f, 0.62f, 1.00f};

static const char* mem_regions[] = {
    "ROM (bank 0)", "ROM (bank N)", "VRAM", "Cart RAM",
    "WRAM", "Echo RAM", "OAM", "IO registers", "HRAM", "IE"
};
static const uint16_t mem_bases[] = {
    0x0000, 0x4000, 0x8000, 0xA000, 0xC000, 0xE000, 0xFE00, 0xFF00, 0xFF80, 0xFFFF
};

static void add_bp_from_text(gb_debugger* dbg, const char* text)
{
    if (!text || !text[0]) return;
    char* end = NULL;
    unsigned long v = strtoul(text, &end, 16);
    if (end == text) return;
    gb_debugger_add_breakpoint(dbg, (uint16_t)(v & 0xFFFF));
}

static bool parse_seek(const char* text, int* base_out)
{
    if (!text || !text[0]) return false;
    char* end = NULL;
    unsigned long v = strtoul(text, &end, 16);
    if (end == text) return false;
    int b = (int)(v & 0xFFFF) & ~0xF;
    if (b > 0xFC00) b = 0xFC00;
    *base_out = b;
    return true;
}

static void draw_toolbar(gb_debugger* dbg)
{
    if (dbg->paused)
        igTextColored(c_pause, "PAUSED");
    else
        igTextColored(c_run, "RUNNING");
    igSameLine(0.0f, 12.0f);
    igText("PC $%04X", dbg->cpu->pc);
    igSameLine(0.0f, 12.0f);

    if (!dbg->paused) {
        if (igButton("Pause", (ImVec2){0.0f, 0.0f}))
            gb_debugger_pause(dbg);
    } else {
        if (igButton("Continue", (ImVec2){0.0f, 0.0f}))
            gb_debugger_continue(dbg);
        igSameLine(0.0f, 4.0f);
        if (igButton("Step", (ImVec2){0.0f, 0.0f}))
            gb_debugger_step(dbg, 1);
        igSameLine(0.0f, 4.0f);
        if (igButton("Step Over", (ImVec2){0.0f, 0.0f}))
            gb_debugger_step_over(dbg);
    }
    igSameLine(0.0f, 12.0f);
    igText("%d BP%s", dbg->num_breakpoints, dbg->num_breakpoints == 1 ? "" : "s");
}

static void flag_tag(bool first, const char* name, bool on)
{
    if (!first) igSameLine(0.0f, 6.0f);
    igTextColored(on ? c_run : c_dim, "%s%d", name, on ? 1 : 0);
}

static void reg_row(const char* name, uint16_t value)
{
    igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
    igTableNextColumn();
    igText("%s", name);
    igTableNextColumn();
    igText("$%04X", value);
}

static void draw_registers(gb_debugger* dbg)
{
    gb_cpu* cpu = dbg->cpu;

    igTextColored(c_title, "Registers");
    if (igBeginTable("##regs", 2,
                     ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings,
                     (ImVec2){0.0f, 0.0f}, 0.0f)) {
        igTableSetupColumn("##n", ImGuiTableColumnFlags_WidthFixed, 24.0f, 0);
        igTableSetupColumn("##v", ImGuiTableColumnFlags_WidthFixed, 62.0f, 0);
        reg_row("AF", cpu->af);
        reg_row("BC", cpu->bc);
        reg_row("DE", cpu->de);
        reg_row("HL", cpu->hl);
        reg_row("SP", cpu->sp);
        reg_row("PC", cpu->pc);
        igEndTable();
    }

    igTextColored(c_dim, "Flags");
    igSameLine(0.0f, 8.0f);
    flag_tag(true, "Z", (cpu->f & GB_FLAG_Z) != 0);
    flag_tag(false, "N", (cpu->f & GB_FLAG_N) != 0);
    flag_tag(false, "H", (cpu->f & GB_FLAG_H) != 0);
    flag_tag(false, "C", (cpu->f & GB_FLAG_C) != 0);

    igText("IME: %s%s", cpu->ime ? "1" : "0", cpu->ime_scheduled ? " (sched)" : "");
    igSameLine(0.0f, 10.0f);
    igText("HLT: %s", cpu->halted ? "yes" : "no");
}

static void draw_breakpoints(gb_debugger* dbg)
{
    static char add_buf[8] = "";

    igTextColored(c_title, "Breakpoints");
    if (igInputText("##bpaddr", add_buf, sizeof(add_buf),
                    ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue,
                    NULL, NULL)) {
        add_bp_from_text(dbg, add_buf);
        add_buf[0] = '\0';
    }
    igSameLine(0.0f, 4.0f);
    if (igButton("+##bpadd", (ImVec2){0.0f, 0.0f})) {
        add_bp_from_text(dbg, add_buf);
        add_buf[0] = '\0';
    }
    igSameLine(0.0f, 8.0f);
    igTextColored(c_dim, "hex");

    if (dbg->num_breakpoints == 0) {
        igTextColored(c_dim, "(none)");
        return;
    }

    for (int i = 0; i < dbg->num_breakpoints; i++) {
        uint16_t addr = dbg->breakpoints[i];
        igText("$%04X", addr);
        igSameLine(0.0f, 6.0f);
        char lbl[16];
        snprintf(lbl, sizeof(lbl), "x##bp%04X", addr);
        if (igSmallButton(lbl)) {
            gb_debugger_remove_breakpoint(dbg, addr);
            break;
        }
    }
}

// Find a decode start <= pc so instruction boundaries line up exactly on pc.
static uint16_t disasm_window_start(gb_bus* bus, uint16_t pc)
{
    uint16_t best = pc;
    for (int k = 1; k <= 24; k++) {
        uint16_t start = (uint16_t)(pc - k);
        uint16_t a = start;
        bool ok = true;
        while (a < pc) {
            int len = gb_instruction_length(bus, a);
            if (len <= 0 || a + len > pc) {
                ok = false;
                break;
            }
            a = (uint16_t)(a + len);
        }
        if (ok && a == pc) best = start;
    }
    return best;
}

static void draw_disasm(gb_debugger* dbg)
{
    gb_bus* bus = dbg->bus;
    uint16_t pc = dbg->cpu->pc;
    static uint16_t last_pc = 0xFFFF;

    bool follow = (!dbg->paused) || (pc != last_pc);

    uint16_t addr = disasm_window_start(bus, pc);

    if (igBeginTable("##disasm", 4,
                     ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings,
                     (ImVec2){0.0f, 0.0f}, 0.0f)) {
        igTableSetupColumn("##m", ImGuiTableColumnFlags_WidthFixed, 16.0f, 0);
        igTableSetupColumn("##a", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
        igTableSetupColumn("##b", ImGuiTableColumnFlags_WidthFixed, 74.0f, 0);
        igTableSetupColumn("##d", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

        for (int i = 0; i < DISASM_LINES; i++) {
            char text[64];
            int len = gb_disasm(bus, addr, text, sizeof(text));
            if (len <= 0) {
                igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
                igTableNextColumn();
                igText(" ");
                igTableNextColumn();
                igText("$%04X", addr);
                igTableNextColumn();
                igText("??");
                igTableNextColumn();
                igTextColored(c_dim, "(bad opcode)");
                break;
            }

            bool at_pc = (addr == pc);
            bool has_bp = gb_debugger_has_breakpoint(dbg, addr);

            char bytes[10] = "";
            for (int b = 0; b < len && b < 3; b++) {
                size_t used = strlen(bytes);
                snprintf(bytes + used, sizeof(bytes) - used, "%02X ",
                         gb_bus_read(bus, (uint16_t)(addr + b)));
            }

            igTableNextRow(ImGuiTableRowFlags_None, 0.0f);

            igTableNextColumn();
            if (at_pc && has_bp)
                igText(">*");
            else if (at_pc)
                igText(">");
            else if (has_bp)
                igText("*");
            else
                igText(" ");

            igTableNextColumn();
            igText("$%04X", addr);

            igTableNextColumn();
            igText("%-8s", bytes);

            igTableNextColumn();
            char label[96];
            snprintf(label, sizeof(label), "%s##d%04X", text, addr);
            if (at_pc) igPushStyleColor_Vec4(ImGuiCol_Text, c_pc);
            bool clicked = igSelectable_Bool(label, at_pc, ImGuiSelectableFlags_None,
                                             (ImVec2){0.0f, 0.0f});
            if (at_pc) igPopStyleColor(1);
            if (clicked) gb_debugger_toggle_breakpoint(dbg, addr);

            if (follow && at_pc)
                igSetScrollHereY(0.5f);

            addr = (uint16_t)(addr + len);
        }
        igEndTable();
    }

    last_pc = pc;
}

static void draw_memory(gb_debugger* dbg)
{
    gb_bus* bus = dbg->bus;
    static int region = 0;
    static int base = 0x0000;
    static char goto_buf[8] = "";
    static bool seek_request = false;

    igTextColored(c_title, "Memory");
    igSameLine(0.0f, 10.0f);
    if (igCombo_Str_arr("##region", &region, mem_regions, 10, 10.0f)) {
        base = (int)mem_bases[region] & ~0xF;
        if (base > 0xFC00) base = 0xFC00;
        seek_request = true;
    }
    igSameLine(0.0f, 10.0f);
    if (igButton("PC##mem", (ImVec2){0.0f, 0.0f})) {
        base = dbg->cpu->pc & 0xFFF0;
        seek_request = true;
    }
    igSameLine(0.0f, 4.0f);
    if (igButton("SP##mem", (ImVec2){0.0f, 0.0f})) {
        base = dbg->cpu->sp & 0xFFF0;
        seek_request = true;
    }
    igSameLine(0.0f, 10.0f);
    bool go = igInputText("##goto", goto_buf, sizeof(goto_buf),
                          ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue,
                          NULL, NULL);
    if (igButton("Go##mem", (ImVec2){0.0f, 0.0f}))
        go = true;
    if (go) {
        if (parse_seek(goto_buf, &base))
            seek_request = true;
        goto_buf[0] = '\0';
    }

    if (igBeginChild_Str("##hex", (ImVec2){0.0f, 0.0f}, ImGuiChildFlags_Borders, 0)) {
        if (seek_request) {
            igSetScrollY_Float(0.0f);
            seek_request = false;
        }
        if (igBeginTable("##hextbl", 18,
                         ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings,
                         (ImVec2){0.0f, 0.0f}, 0.0f)) {
            igTableSetupColumn("##a", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
            for (int b = 0; b < MEM_COLS; b++) {
                char h[8];
                snprintf(h, sizeof(h), "##b%d", b);
                igTableSetupColumn(h, ImGuiTableColumnFlags_WidthFixed, 21.0f, 0);
            }
            igTableSetupColumn("##asc", ImGuiTableColumnFlags_WidthFixed, 115.0f, 0);

            for (int r = 0; r < MEM_ROWS; r++) {
                uint16_t row = (uint16_t)(base + r * MEM_COLS);
                char ascii[MEM_COLS + 1];

                igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
                igTableNextColumn();
                igText("$%04X", row);

                for (int i = 0; i < MEM_COLS; i++) {
                    uint8_t v = gb_bus_read(bus, (uint16_t)(row + i));
                    igTableNextColumn();
                    igText("%02X", v);
                    ascii[i] = (v >= 0x20 && v <= 0x7E) ? (char)v : '.';
                }
                ascii[MEM_COLS] = '\0';

                igTableNextColumn();
                igTextUnformatted(ascii, ascii + MEM_COLS);
            }
            igEndTable();
        }
    }
    igEndChild();
}

void debug_ui_panels(gb_debugger* dbg)
{
    if (!dbg || !dbg->cpu || !dbg->bus) return;

    igSetNextWindowSize((ImVec2){780.0f, 660.0f}, ImGuiCond_FirstUseEver);
    if (!igBegin("GB Debugger", NULL, 0)) {
        igEnd();
        return;
    }

    draw_toolbar(dbg);
    igSeparator();

    ImVec2 avail = igGetContentRegionAvail();
    float top_h = avail.y - 240.0f;
    if (top_h < 150.0f) top_h = 150.0f;

    if (igBeginChild_Str("##left", (ImVec2){240.0f, top_h}, ImGuiChildFlags_Borders, 0)) {
        draw_registers(dbg);
        igSeparator();
        draw_breakpoints(dbg);
    }
    igEndChild();

    igSameLine(0.0f, 6.0f);

    if (igBeginChild_Str("##right", (ImVec2){0.0f, top_h}, ImGuiChildFlags_Borders, 0)) {
        draw_disasm(dbg);
    }
    igEndChild();

    draw_memory(dbg);

    igEnd();
}
