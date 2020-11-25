#include "ImGuiFileBrowser.h"
#include "Core/Win32.h"

#include <imgui_internal.h>
#include <filesystem>

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
        std::string absolute_filepath = std::filesystem::absolute(current_dir).string();

        // Todo: I need to make buttons for different parts of the filepath, instead of just a
        // simple text string.

        const float file_tab_height = 0.1f * file_browser_dimensions.y;
        ImGui::BeginChild("FileTab", ImVec2(0, file_tab_height));
        if (ImGui::ArrowButton("ButtonUp", ImGuiDir_Up))
        {
            // You cannot go back (up) a directory if you're already in the root directory of a logical
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

        // Computer => C: => Projects => VolumeRenderer
        ImGui::SameLine();
        if (ImGui::Button("Computer"))
        {

        }

        ImGui::SameLine();
        ImGui::ArrowButtonEx("FakeButton", ImGuiDir_Right, ImVec2(10, 10), ImGuiButtonFlags_Disabled);

        ImGui::SameLine();
        if (ImGui::Button("C:"))
        {
        }

        ImGui::SameLine();
        if (ImGui::Button("Projects"))
        {

        }

        ImGui::SameLine();
        if (ImGui::Button("VolumeRenderer"))
        {

        }
        ImGui::EndChild();

        const float file_list_height = 0.62f * file_browser_dimensions.y;
        ImGui::BeginChild("FileList", ImVec2(0, file_list_height), true);
        if (!current_dir.empty())
        {
            ListFilesAndDirectories();
        }
        else
        {
            ListLogicalDrives();
        }
        ImGui::EndChild();

        const float search_field_height = 0.1f * file_browser_dimensions.y;
        ImGui::BeginChild("SearchField", ImVec2(0, search_field_height), false);
        char buf[64];
        ImGui::InputText("Label", buf, 64);
        ImGui::EndChild();

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

        ImGui::SameLine();

        // Cancel Button
        if (ImGui::Button("Cancel", button_size))
        {
            visible = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }
}

void ImGuiFileBrowser::ListFilesAndDirectories()
{
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    for (const auto& entry : std::filesystem::directory_iterator(current_dir))
    {
        const std::filesystem::path& filepath = entry.path();
        std::string extension = filepath.extension().string();

        bool is_item_hidden = FILE_ATTRIBUTE_HIDDEN & GetFileAttributesA(filepath.string().c_str());
        bool is_item_displayable = (!is_item_hidden || (is_item_hidden && show_hidden_items))
            && (entry.is_directory() || extension == ".raw" || extension == ".pvm");
        if (is_item_displayable)
        {
            const std::string& path = filepath.filename().string();

            // Todo: Display directory names in other color (say yellow) than the file names, and display
            // hidden items in other colors as well
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
                        // Todo: Open the damn volume
                    }
                }
            }
        }
    }
}


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