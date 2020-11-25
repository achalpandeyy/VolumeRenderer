#ifndef IMGUI_FILE_BROWSER_H

#include <imgui.h>
#include <string>

// Distribute the file browser dimensions (in ratios) for different parts of the file browser
// 10% for the top bar showing the current directory and has the up button
// 62% for the enclosure which lists all the files
// 10% for the text field
// 10% for the Show Hidden Items checkbox and "Open" and "Cancel" buttons
struct ImGuiFileBrowser
{
    void Open();

    inline bool IsVisible() const { return visible; }
    inline void SetVisible(bool v) { visible = v; }

private:
    void ListFilesAndDirectories();
    void ListLogicalDrives();

    bool visible = false;
    bool show_hidden_items = false;
    std::string current_dir = "../";
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
};

#define IMGUI_FILE_BROWSER_H
#endif
