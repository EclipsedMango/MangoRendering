
#ifndef MANGORENDERING_CONTENTBROWSERWINDOW_H
#define MANGORENDERING_CONTENTBROWSERWINDOW_H

#include <filesystem>
#include <vector>

#include "imgui.h"

class Editor;

namespace fs = std::filesystem;

class ContentBrowserWindow {
public:
    explicit ContentBrowserWindow(Editor* editor);
    ~ContentBrowserWindow() = default;

    void DrawContentBrowser();

private:
    void DisplayPath();
    void DrawEntry(const fs::directory_entry& entry, int index);
    static void DrawEntryFile(const ImVec2& startPos, float itemSize);
    static void DrawEntryFolder(const ImVec2& startPos, float itemSize);
    void RefreshDirectory();

    void QueuePathChange(const fs::path& newPath);
    void FlushPendingPathChange();

    static void NewFolder(const fs::path& path);
    static void NewFile(fs::path& path);

    static void StartDrag(const std::string& fullPath, const std::string& filename);
    bool AcceptDrop(const fs::path& destDir);

    Editor* m_editor = nullptr;
    fs::path m_currentPath;
    fs::path m_contextPayloadPath;

    std::vector<fs::directory_entry> m_currentDirEntries;
    ImGuiSelectionBasicStorage m_selection;

    double m_backHoverStart = 0.0;
    const double m_backHoverDelay = 0.4;

    fs::path m_pendingPathChange;
    bool m_hasPendingPathChange = false;
};


#endif //MANGORENDERING_CONTENTBROWSERWINDOW_H