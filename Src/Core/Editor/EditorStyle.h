
#ifndef MANGORENDERING_EDITORSTYLE_H
#define MANGORENDERING_EDITORSTYLE_H
#include "imgui.h"
#include "Core/ResourceManager.h"

class EditorStyle {
public:
    static EditorStyle& Get() {
        static EditorStyle instance;
        return instance;
    }

    EditorStyle(const EditorStyle&) = delete;
    EditorStyle& operator=(const EditorStyle&) = delete;

    void Init(ImGuiIO& io) {
        LoadFonts(io);
        ApplyStyle();
    }

    // ----------------------------------------------------------------
    // Fonts
    // ----------------------------------------------------------------
    ImFont* mainFontSm   = nullptr;  // 16px - labels, secondary text
    ImFont* mainFont     = nullptr;  // 18px - general UI
    ImFont* mainFontLg   = nullptr;  // 22px - headers
    ImFont* mainFontExLg = nullptr;  // 64px - giant things / icons
    ImFont* monoFont     = nullptr;  // 17px - values, stats, debug

    // ----------------------------------------------------------------
    // Semantic colors
    // ----------------------------------------------------------------
    ImVec4 colHeader      = { 0.80f, 0.66f, 0.97f, 1.00f }; // lavender accent
    ImVec4 colLabel       = { 0.75f, 0.75f, 0.78f, 1.00f }; // muted grey
    ImVec4 colSeparator   = { 0.30f, 0.30f, 0.35f, 1.00f }; // table / section lines
    ImVec4 colSuccess     = { 0.46f, 0.90f, 0.58f, 1.00f }; // linked / valid state
    ImVec4 colTransparent = { 0.00f, 0.00f, 0.00f, 0.00f };

    ImU32 colTreeLine = IM_COL32(80, 80, 90, 200);

    ImVec4 colSelection        = { 0.63f, 0.36f, 0.90f, 0.50f };
    ImVec4 colSelectionBorder  = { 0.78f, 0.37f, 1.00f, 1.00f };
    ImVec4 colSelectionHovered = { 0.72f, 0.46f, 0.98f, 0.50f };
    ImVec4 colSelectionActive  = { 0.54f, 0.28f, 0.82f, 1.00f };
    ImVec4 colSelectionSoft    = { 0.63f, 0.36f, 0.90f, 0.30f };

    ImU32 colSelectionOutlineU32 = IM_COL32(161, 92, 232, 255);
    ImU32 colSelectionFillU32    = IM_COL32(161, 92, 232, 64);
    ImU32 colHoverOutlineU32     = IM_COL32(210, 180, 255, 120);

    // Content browser / texture preview colors
    ImU32 colContentTextU32      = IM_COL32(230, 230, 235, 255);
    ImU32 colFileIconU32         = IM_COL32(188, 194, 206, 255);
    ImU32 colFolderIconU32       = IM_COL32(206, 164, 252, 230);
    ImU32 colCheckerLightU32     = IM_COL32(82, 82, 88, 255);
    ImU32 colCheckerDarkU32      = IM_COL32(56, 56, 62, 255);

    // ----------------------------------------------------------------
    // Scoped helpers
    // ----------------------------------------------------------------

    void PushMono() const { ImGui::PushFont(monoFont); }
    static void PopMono() { ImGui::PopFont(); }

    void PushHeader() const {
        ImGui::PushFont(mainFontLg);
        ImGui::PushStyleColor(ImGuiCol_Text, colHeader);
    }
    static void PopHeader() {
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void PushLabel() const {
        ImGui::PushFont(mainFontSm);
        ImGui::PushStyleColor(ImGuiCol_Text, colLabel);
    }
    static void PopLabel() {
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void PushAccentText() const {
        ImGui::PushStyleColor(ImGuiCol_Text, colHeader);
    }
    static void PopAccentText() {
        ImGui::PopStyleColor();
    }

    void PushSuccessText() const {
        ImGui::PushStyleColor(ImGuiCol_Text, colSuccess);
    }
    static void PopSuccessText() {
        ImGui::PopStyleColor();
    }

    void PushInvisibleSelectable() const {
        ImGui::PushStyleColor(ImGuiCol_Header, colTransparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colTransparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, colTransparent);
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, colTransparent);
    }
    static void PopInvisibleSelectable() {
        ImGui::PopStyleColor(4);
    }

private:
    EditorStyle() = default;

    void LoadFonts(ImGuiIO& io) {
        const std::string lexend = ResourceManager::Get().ResolveAssetPath("Lexend-VariableFont_wght.ttf");
        const std::string nerdFontPath = ResourceManager::Get().ResolveAssetPath("JetBrainsMonoNerdFont-Regular.ttf");

        static constexpr ImWchar icon_ranges[] = { 0xe000, 0xf8ff, 0 };

        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;

        mainFontSm = io.Fonts->AddFontFromFileTTF(lexend.c_str(), 16.0f);
        io.Fonts->AddFontFromFileTTF(nerdFontPath.c_str(), 16.0f, &iconConfig, icon_ranges);

        mainFont = io.Fonts->AddFontFromFileTTF(lexend.c_str(), 18.0f);
        io.Fonts->AddFontFromFileTTF(nerdFontPath.c_str(), 18.0f, &iconConfig, icon_ranges);

        mainFontLg = io.Fonts->AddFontFromFileTTF(lexend.c_str(), 22.0f);
        io.Fonts->AddFontFromFileTTF(nerdFontPath.c_str(), 22.0f, &iconConfig, icon_ranges);

        mainFontExLg = io.Fonts->AddFontFromFileTTF(lexend.c_str(), 64.0f);
        io.Fonts->AddFontFromFileTTF(nerdFontPath.c_str(), 64.0f, &iconConfig, icon_ranges);

        monoFont = io.Fonts->AddFontFromFileTTF(nerdFontPath.c_str(), 17.0f);
    }

    void ApplyStyle() const {
        ImGuiStyle& s = ImGui::GetStyle();

        // ----- rounding -----
        s.WindowRounding = 4.0f;
        s.FrameRounding = 3.0f;
        s.PopupRounding = 3.0f;
        s.ScrollbarRounding = 3.0f;
        s.GrabRounding = 3.0f;
        s.TabRounding = 3.0f;

        // ----- spacing -----
        s.WindowPadding = ImVec2(10.0f, 10.0f);
        s.FramePadding = ImVec2(6.0f, 3.0f);
        s.CellPadding = ImVec2(4.0f, 3.0f);
        s.ItemSpacing = ImVec2(8.0f, 5.0f);
        s.IndentSpacing = 16.0f;
        s.ScrollbarSize = 10.0f;

        // ----- colors -----
         ImVec4* c = s.Colors;

        // base surfaces
        c[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
        c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
        c[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.16f, 1.00f);

        // borders
        c[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
        c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        c[ImGuiCol_TableBorderLight] = colSeparator;
        c[ImGuiCol_TableBorderStrong] = ImVec4(0.40f, 0.40f, 0.46f, 1.00f);

        // title bars
        c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.16f, 1.00f);
        c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

        // menu bar
        c[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

        // frames
        c[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

        // buttons
        c[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.00f);

        // headers / selected items
        c[ImGuiCol_Header] = colSelection;
        c[ImGuiCol_HeaderHovered] = colSelectionHovered;
        c[ImGuiCol_HeaderActive] = colSelectionActive;

        // tabs
        c[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.16f, 1.00f);
        c[ImGuiCol_TabHovered] = colSelectionHovered;
        c[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
        c[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        c[ImGuiCol_TabDimmedSelected] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);

        // docking
        c[ImGuiCol_DockingPreview] = colSelectionSoft;
        c[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

        // scrollbar
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.42f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.42f, 0.42f, 0.50f, 1.00f);

        // separator
        c[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
        c[ImGuiCol_SeparatorHovered] = colSelectionHovered;
        c[ImGuiCol_SeparatorActive] = colSelection;

        // slider / grab
        c[ImGuiCol_SliderGrab] = colSelection;
        c[ImGuiCol_SliderGrabActive] = colSelectionHovered;

        // check mark
        c[ImGuiCol_CheckMark] = colSelection;

        // text
        c[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
        c[ImGuiCol_TextDisabled] = colLabel;
        c[ImGuiCol_TextSelectedBg] = colSelectionSoft;

        // drag drop target
        c[ImGuiCol_DragDropTarget] = colSelectionHovered;

        // navigation
        c[ImGuiCol_NavHighlight] = colSelectionBorder;
    }
};


#endif //MANGORENDERING_EDITORSTYLE_H