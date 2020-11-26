#include "ImGuiFileBrowser.h"
#include "Core/Win32.h"

#include <imgui_internal.h>
#include <filesystem>
#include <sstream>
#include <vector>

std::vector<std::string> ParsePath(std::string path)
{
    // Add a trailing slash
    if (path.back() != '\\')
        path += "\\";

    std::vector<std::string> result;
    std::size_t prev_pos = 0;
    std::size_t pos = path.find("\\", prev_pos);

    while (pos != std::string::npos)
    {
        result.push_back(path.substr(prev_pos, pos - prev_pos));
        prev_pos = pos + 1;
        pos = path.find("\\", prev_pos);
    }
    return result;
}

void DrawFakeArrowButton()
{
    ImGui::SameLine(0.f, 0.f);
    float frame_height = ImGui::GetFrameHeight();
    ImGui::ArrowButtonEx("FakeButton", ImGuiDir_Right, ImVec2(frame_height, frame_height), ImGuiButtonFlags_Disabled);
}

void ImGuiFileBrowser::Open()
{
    // Set the file browser window to be appear at the center
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Set file browser's dimensions
    ImVec2 file_browser_dimensions = { 0.5f * ImGui::GetIO().DisplaySize.x, 0.5f * ImGui::GetIO().DisplaySize.y };
    ImGui::SetNextWindowSizeConstraints(file_browser_dimensions, file_browser_dimensions);

    ImGui::OpenPopup("File Browser");
    if (ImGui::BeginPopupModal("File Browser", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
    {
        DrawFileTab();
        DrawFileList();
        DrawButtonGroup();

        ImGui::EndPopup();
    }
}

void ImGuiFileBrowser::DrawFileTab()
{
    std::string absolute_filepath = std::filesystem::absolute(current_dir).string();

    const float file_browser_height = 0.5f * ImGui::GetIO().DisplaySize.y;
    const float file_tab_height = 0.1f * file_browser_height;

    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 2.f);
    ImGui::BeginChild("FileTab", ImVec2(0, file_tab_height), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Up button
    if (ImGui::ArrowButton("ButtonUp", ImGuiDir_Up))
    {
        // You cannot go up a directory if you're already in the root directory of a logical
        // drive. From there just empty out the current directory and start building it again once
        // the user selects the logical drive.
        bool cannot_go_up = absolute_filepath.empty() || absolute_filepath.length() == 3;
        if (cannot_go_up)
        {
            current_dir = "";
        }
        else
        {
            current_dir += "/..";
            absolute_filepath = std::filesystem::absolute(current_dir).string();
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.f));

    ImGui::SameLine();
    if (ImGui::Button("Computer"))
    {
        current_dir = "";
    }

    if (!absolute_filepath.empty())
    {
        std::vector<std::string> directories = ParsePath(absolute_filepath);
        for (size_t i = 0; i < directories.size(); ++i)
        {
            DrawFakeArrowButton();

            ImGui::SameLine(0.f, 0.f);
            if (ImGui::Button(directories[i].c_str()))
            {
                current_dir = "";
                for (size_t k = 0; k <= i; ++k)
                {
                    current_dir += directories[k];
                    current_dir += "\\";
                }
            }
        }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::EndChild();
}

void ImGuiFileBrowser::DrawFileList()
{
    const float file_browser_height = 0.5f * ImGui::GetIO().DisplaySize.y;
    const float file_list_height = 0.72f * file_browser_height;
    ImGui::BeginChild("FileList", ImVec2(0, file_list_height), true);
    current_dir.empty() ? ListLogicalDrives() : ListFilesAndDirectories();
    ImGui::EndChild();
}

void ImGuiFileBrowser::DrawButtonGroup()
{
    const ImVec2 file_browser_dimensions = { 0.5f * ImGui::GetIO().DisplaySize.x, 0.5f * ImGui::GetIO().DisplaySize.y };
    const float button_group_height = 0.1f * file_browser_dimensions.y;
    ImGui::BeginChild("ButtonGroup", ImVec2(0, button_group_height));
    ImGui::SetCursorPosY(button_group_height * 0.2f);

    ImGui::Checkbox("Show Hidden Files", &show_hidden_items);

    ImVec2 button_size = { button_group_height * 2.f, button_group_height * 0.75f };
    ImGui::SameLine();
    ImGui::SetCursorPosX(file_browser_dimensions.x - 2.25f * button_size.x);

    // Open Button
    if (ImGui::Button("Open", button_size))
    {

    }

    // Cancel Button
    ImGui::SameLine();
    if (ImGui::Button("Cancel", button_size))
    {
        visible = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndChild();
}

void ImGuiFileBrowser::ListFilesAndDirectories()
{
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    for (const auto& entry : std::filesystem::directory_iterator(current_dir))
    {
        const std::filesystem::path& filepath = entry.path();
        std::string extension = filepath.extension().string();

        bool is_item_hidden = FILE_ATTRIBUTE_HIDDEN & GetFileAttributesW(filepath.wstring().c_str());
        bool is_item_displayable = (!is_item_hidden || (is_item_hidden && show_hidden_items))
            && (entry.is_directory() || extension == ".raw" || extension == ".pvm");
        if (is_item_displayable)
        {
            const std::string& path = filepath.filename().string();

            ImVec4 text_color = GetListItemTextColor(entry.is_directory(), is_item_hidden);
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            if (ImGui::TreeNodeEx(path.c_str(), flags))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                {
                    if (entry.is_directory())
                    {
                        current_dir.append("/" + path);
                    }
                    else
                    {
                        const std::string& absolute_filepath = std::filesystem::absolute(filepath).string();
                        // Start up a pop up which would ask the user for the following
                        // - dimensions
                        // - data type
                        // - spacing
                        // Somehow package this data and send it to the thing which would render the volume
                        OutputDebugStringA(absolute_filepath.c_str());
                    }
                }
            }
            ImGui::PopStyleColor();
        }
    }
}

// Todo: Visually separate Removable drives
void ImGuiFileBrowser::ListLogicalDrives()
{
    char buffer[64];
    DWORD length = GetLogicalDriveStringsA(64, buffer);

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    for (unsigned long i = 0; i < length; ++i)
    {
        bool is_drive_name = buffer[i] != ':' && buffer[i] != '\\' && buffer[i] != '\0';
        if (is_drive_name)
        {
            if (ImGui::TreeNodeEx(&buffer[i], flags))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                {
                    current_dir += buffer[i];
                    current_dir += ":/";
                }
            }
        }
    }
}

ImVec4 ImGuiFileBrowser::GetListItemTextColor(bool is_directory, bool is_hidden)
{
    if (is_directory && is_hidden)
    {
        // Hidden directory
        return { 0.882f, 0.745f, 0.078f, 0.5f};
    }
    else if (is_directory && !is_hidden)
    {
        // Visible directory
        return { 0.882f, 0.745f, 0.078f, 1.0f };
    }
    else if (!is_directory && is_hidden)
    {
        // Hidden file
        return { 1.f, 1.f, 1.f, 0.5f };
    }
    else
    {
        // Visible file
        return { 1.f, 1.f, 1.f, 1.f };
    }
}