#include "ContentBrowserWindow.h"

#include <algorithm>
#include <fstream>

#include "imgui.h"
#include "Core/Editor/EditorStyle.h"

ContentBrowserWindow::ContentBrowserWindow(Editor *editor) : m_editor(editor), m_currentPath(fs::current_path() / "Root") {
    RefreshDirectory();
}

void ContentBrowserWindow::DrawContentBrowser() {
    ImGui::Begin("Content Browser");

    const auto& style = EditorStyle::Get();

    style.PushLabel();
    ImGui::Text("Current Directory: %s", m_currentPath.string().c_str());
    EditorStyle::PopLabel();

    if (ImGui::Button("Back", ImVec2(64, 32))) {
        if (m_currentPath.filename() != "Root") {
            m_currentPath = m_currentPath.parent_path();
            RefreshDirectory();
            m_selection.Clear();
        }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::GetDragDropPayload() != nullptr) {
        if (m_backHoverStart == 0.0) {
            m_backHoverStart = ImGui::GetTime();
        } else if (ImGui::GetTime() - m_backHoverStart >= m_backHoverDelay) {
            if (m_currentPath.filename() != "Root") {
                m_currentPath = m_currentPath.parent_path();
                RefreshDirectory();
                m_selection.Clear();
            }
            m_backHoverStart = 0.0;
        }
    } else {
        m_backHoverStart = 0.0;
    }

    ImGui::Separator();

    ImGui::BeginChild("##content_browser_region", ImVec2(0, 0), false);

    DisplayPath();

    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_selection.Clear();
    }

    if (ImGui::BeginPopupContextWindow("##bg_context",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("New Folder")) {
            NewFolder(m_currentPath);
            RefreshDirectory();
        }

        if (ImGui::MenuItem("New File")) {
            NewFile(m_currentPath);
            RefreshDirectory();
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();

    AcceptDrop(m_currentPath);
    FlushPendingPathChange();

    ImGui::End();
}

void ContentBrowserWindow::DisplayPath() {
    constexpr float itemSize = 80.0f;
    constexpr float padding = 10.0f;

    const float windowWidth = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, static_cast<int>(windowWidth / (itemSize + padding)));

    ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape;

    if (ImGui::GetIO().KeyShift) {
        ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;
    }

    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, m_selection.Size, static_cast<int>(m_currentDirEntries.size()));
    m_selection.ApplyRequests(ms_io);

    for (int n = 0; n < static_cast<int>(m_currentDirEntries.size()); ++n) {
        if (n % columns != 0) {
            ImGui::SameLine(0.0f, padding);
        }

        ImGui::SetNextItemSelectionUserData(n);
        DrawEntry(m_currentDirEntries[n], n);
    }

    ms_io = ImGui::EndMultiSelect();
    m_selection.ApplyRequests(ms_io);
}

void ContentBrowserWindow::DrawEntry(const fs::directory_entry& entry, const int index) {
    constexpr float itemSize = 80.0f;
    const std::string filename = entry.path().filename().string();

    const EditorStyle& style = EditorStyle::Get();

    ImGui::PushID(index);
    ImGui::BeginGroup();

    const ImVec2 startPos = ImGui::GetCursorScreenPos();
    const bool isSelected = m_selection.Contains(static_cast<ImGuiID>(index));

    constexpr ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap;

    style.PushInvisibleSelectable();
    ImGui::Selectable("##item", isSelected, flags, ImVec2(itemSize, itemSize));
    EditorStyle::PopInvisibleSelectable();

    const bool hovered = ImGui::IsItemHovered();
    const bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

    StartDrag(entry.path().string(), filename);
    if (entry.is_directory()) {
        AcceptDrop(entry.path());
    }

    if (rightClicked && !isSelected) {
        m_selection.Clear();
        m_selection.SetItemSelected(static_cast<ImGuiID>(index), true);
    }

    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (entry.is_directory()) {
            QueuePathChange(entry.path());
        }
    }

    const bool drawSelected = isSelected || (rightClicked && !isSelected);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawSelected) {
        drawList->AddRectFilled(startPos, ImVec2(startPos.x + itemSize, startPos.y + itemSize), style.colSelectionFillU32, 4.0f);
        drawList->AddRect(startPos, ImVec2(startPos.x + itemSize, startPos.y + itemSize), style.colSelectionOutlineU32, 4.0f, 0, 2.0f);
    } else if (hovered) {
        drawList->AddRect(startPos, ImVec2(startPos.x + itemSize, startPos.y + itemSize), style.colHoverOutlineU32, 4.0f, 0, 1.0f);
    }

    if (entry.is_directory()) {
        DrawEntryFolder(startPos, itemSize);
    } else {
        DrawEntryFile(startPos, itemSize);
    }

    const ImVec2 textSize = ImGui::CalcTextSize(filename.c_str());
    float textX = startPos.x + (itemSize - textSize.x) * 0.5f;
    if (textSize.x > itemSize) {
        textX = startPos.x;
    }

    drawList->PushClipRect(startPos, ImVec2(startPos.x + itemSize, startPos.y + itemSize), true);
    drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textX, startPos.y + itemSize - textSize.y - 2.0f), style.colContentTextU32, filename.c_str());
    drawList->PopClipRect();

    if (ImGui::BeginPopupContextItem("##entry_context")) {
        if (entry.is_directory() && ImGui::MenuItem("Open Folder")) {
            QueuePathChange(entry.path());
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Copy Path")) {
            const std::string pathStr = entry.path().string();
            ImGui::SetClipboardText(pathStr.c_str());
        }

        if (ImGui::MenuItem("Copy Absolute Path")) {
            const std::string absPathStr = fs::absolute(entry.path()).string();
            ImGui::SetClipboardText(absPathStr.c_str());
        }

        ImGui::Separator();

        if (entry.is_directory() && ImGui::MenuItem("Duplicate Folder")) {
            // TODO
        }
        if (ImGui::MenuItem("Rename")) {
            // TODO
        }
        if (ImGui::MenuItem("Delete")) {
            // TODO
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Open in Terminal")) {
            // TODO
        }
        if (ImGui::MenuItem("Open in File Manager")) {
            // TODO
        }

        ImGui::EndPopup();
    }

    ImGui::EndGroup();
    ImGui::PopID();
}

void ContentBrowserWindow::DrawEntryFile(const ImVec2& startPos, const float itemSize) {
    const auto& style = EditorStyle::Get();

    const char* icon = "\uf016";
    constexpr ImVec2 iconOffset = ImVec2(-2.5f, 0.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushFont(style.mainFontExLg);
    const ImVec2 iconSize = ImGui::CalcTextSize(icon);
    drawList->AddText(ImVec2(startPos.x + (itemSize - iconSize.x) * 0.5f + iconOffset.x, startPos.y + iconOffset.y), style.colFileIconU32, icon);
    ImGui::PopFont();
}

void ContentBrowserWindow::DrawEntryFolder(const ImVec2& startPos, const float itemSize) {
    const auto& style = EditorStyle::Get();

    const char* icon = "\uf114";
    constexpr ImVec2 iconOffset = ImVec2(-7.5f, 0.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushFont(style.mainFontExLg);
    const ImVec2 iconSize = ImGui::CalcTextSize(icon);
    drawList->AddText(ImVec2(startPos.x + (itemSize - iconSize.x) * 0.5f + iconOffset.x, startPos.y + iconOffset.y), style.colFolderIconU32, icon);
    ImGui::PopFont();
}

void ContentBrowserWindow::RefreshDirectory() {
    m_currentDirEntries.clear();
    for (const auto& entry : fs::directory_iterator(m_currentPath)) {
        m_currentDirEntries.push_back(entry);
    }

    // TODO: sort m_currentDirEntries so folders are first or something
}

void ContentBrowserWindow::QueuePathChange(const fs::path& newPath) {
    m_pendingPathChange = newPath;
    m_hasPendingPathChange = true;
}

void ContentBrowserWindow::FlushPendingPathChange() {
    if (!m_hasPendingPathChange) {
        return;
    }

    m_currentPath = m_pendingPathChange;
    RefreshDirectory();
    m_selection.Clear();

    m_hasPendingPathChange = false;
    m_pendingPathChange.clear();
}

void ContentBrowserWindow::NewFolder(const fs::path& path) {
    const std::string baseName = "NewFolder";
    fs::path newFolderPath = path / baseName;

    int counter = 1;
    while (fs::exists(newFolderPath)) {
        newFolderPath = path / (baseName + std::to_string(counter));
        counter++;
    }

    try {
        fs::create_directory(newFolderPath);
    } catch (const fs::filesystem_error& e) {
        throw e;
    }
}

void ContentBrowserWindow::NewFile(fs::path& path) {
    const std::string baseName = "NewFile";
    const std::string extension = ".txt";

    fs::path newFilePath = path / (baseName + extension);

    int counter = 1;
    while (fs::exists(newFilePath)) {
        newFilePath = path / (baseName + std::to_string(counter) + extension);
        counter++;
    }

    try {
        std::ofstream newFile(newFilePath);

        if (!newFile.is_open()) {
            throw std::runtime_error("Failed to create file at: " + newFilePath.string());
        }

        newFile.close();
    } catch (const std::exception& e) {
        throw e;
    }
}

void ContentBrowserWindow::StartDrag(const std::string& fullPath, const std::string& filename) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
        ImGui::SetDragDropPayload("FILE_PATH", fullPath.c_str(), fullPath.size() + 1);
        ImGui::Text("Dragging: %s", filename.c_str());
        ImGui::EndDragDropSource();
    }
}

bool ContentBrowserWindow::AcceptDrop(const fs::path& destDir) {
    bool moved = false;
    fs::path sourcePath;

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
            sourcePath = static_cast<const char*>(payload->Data);
            try {
                const fs::path targetPath = destDir / sourcePath.filename();
                if (sourcePath != targetPath) {
                    fs::rename(sourcePath, targetPath);
                    moved = true;
                }
            } catch (const fs::filesystem_error&) {}
        }
        ImGui::EndDragDropTarget();
    }

    if (moved) {
        const bool sourceWasVisible = (sourcePath.parent_path() == m_currentPath);
        const bool destIsVisible = (destDir == m_currentPath);

        if (sourceWasVisible || destIsVisible) {
            RefreshDirectory();
            m_selection.Clear();
        }
    }

    return moved;
}