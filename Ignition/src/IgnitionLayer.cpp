#include "IgnitionLayer.h"

#include "RAPIER/Utilities/FileSystem.h"
#include "RAPIER/Renderer/Texture.h"

#include "RAPIER/ImGui/ImGui.h"
#include "RAPIER/Utilities/StringUtils.h"

#include <ctime>

namespace RAPIER
{
#define MAX_PROJECT_NAME_LENGTH 225
#define MAX_PROJECT_FILEPATH_LENGTH 512

	static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
	static char* s_ProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];

	static std::string GetDateTimeString(const time_t& input_time, const std::locale& loc, char fmt)
	{
		const std::time_put<char>& tmput = std::use_facet <std::time_put<char>>(loc);

		std::stringstream s;
		s.imbue(loc);

		tm time;
		localtime_s(&time, &input_time);
		tmput.put(s, s, ' ', &time, fmt);

		return s.str();
	}

	static std::string FormatDateAndTime(time_t dateTime) { return GetDateTimeString(dateTime, std::locale(""), 'R') + " " + GetDateTimeString(dateTime, std::locale(""), 'x'); }

	IgnitionLayer::IgnitionLayer(const IgnitionProperties& properties)
		: m_Properties(properties), m_HoveredProjectID(0)
	{
		memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
		memset(s_ProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
	}

	IgnitionLayer::~IgnitionLayer()
	{}

	void IgnitionLayer::OnAttach()
	{
		m_RapierLogo = Texture2D::Create("Resources/Editor/RAPIER.png");
	}

	void IgnitionLayer::OnDetach()
	{
		delete[] s_ProjectNameBuffer;
		delete[] s_ProjectFilePathBuffer;
	}

	void IgnitionLayer::OnImGuiRender()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin("Ignition", 0, window_flags);

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];
		auto largefont = io.Fonts->Fonts[1];

		ImGui::PopStyleVar(2);

		{	//	RAPIER install folder prompt
			if (m_Properties.InstallPath.empty() && !ImGui::IsPopupOpen("Select RAPIER Install"))
			{
				ImGui::OpenPopup("Select RAPIER Install");
				m_Properties.InstallPath.reserve(MAX_PROJECT_FILEPATH_LENGTH);
			}

			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(700, 0));
			if (ImGui::BeginPopupModal("Select RAPIER install", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
			{
				ImGui::PushFont(boldFont);
				ImGui::TextUnformatted("Failed to find an appropiate Hazel installation!");
				ImGui::PopFont();

				ImGui::TextWrapped("Please select the root folder for the RAPIER version you want to use (Ex: C:/GitHub/RAPIER)");

				ImGui::Dummy(ImVec2(0, 8));

				ImVec2 label_size = ImGui::CalcTextSize("...", NULL, true);
				auto& style = ImGui::GetStyle();
				ImVec2 button_size = ImGui::CalcItemSize(ImVec2(0, 0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 10));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
				ImGui::SetNextItemWidth(700 - button_size.x - style.FramePadding.x * 2.0f - style.ItemInnerSpacing.x - 1);
				ImGui::InputTextWithHint("##rapier_install_location", "C:/GitHub/RAPIER", m_Properties.InstallPath.data(), MAX_PROJECT_FILEPATH_LENGTH, ImGuiInputTextFlags_ReadOnly);
				ImGui::SameLine();
				if (ImGui::Button("..."))
				{
					std::string result = Application::Get().OpenFolder();
					m_Properties.InstallPath.assign(result);
				}

				if (ImGui::Button("Confirm"))
				{
					bool success = FileSystem::SetEnvironmentVariable("RAPIER_DIR", m_Properties.InstallPath);
					RP_CORE_ASSERT(success, "Failed to set Environment Variable!");
					ImGui::CloseCurrentPopup();
				}

				ImGui::PopStyleVar(2);

				ImGui::EndPopup();

			}
		}

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, viewport->WorkSize.x / 1.5f);

		static std::string s_ProjectToOpen = "";

		bool showNewProjectPopup = false;
		bool serializePreferences = false;

		// Info Area
		ImGui::BeginChild("info_area");
		{
			float columnWidth = ImGui::GetColumnWidth();
			float columnCenterX = columnWidth / 2.0f;
			float imageSize = 160.0f;

			ImGui::SetCursorPosY(-40.0f);
			UI::Image(m_RapierLogo, ImVec2(imageSize, imageSize));

			ImGui::Separator();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + imageSize / 3.0f);

			ImGui::BeginChild("RecentProjects");

			float projectButtonWidth = columnWidth - 60.0f;
			ImGui::SetCursorPosX(20.0f);
			ImGui::BeginGroup();

			bool anyFrameHovered = false;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 5));
			auto& recentProjects = m_Properties.UserPreferences->RecentProjects;
			for (auto it = recentProjects.begin(); it != recentProjects.end(); it++)
			{
				time_t lastOpened = it->first;
				auto& recentProject = it->second;

				// Custom button rendering to allow for multiple text elements inside a button
				std::string fullID = "Project_" + recentProject.FilePath;
				ImGuiID id = ImGui::GetID(fullID.c_str());

				bool changedColor = false;
				if (id == m_HoveredProjectID)
				{
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
					changedColor = true;
				}

				ImGui::BeginChildFrame(id, ImVec2(projectButtonWidth, 50));
				{
					float leftEdge = ImGui::GetCursorPosX();

					ImGui::PushFont(boldFont);
					ImGui::TextUnformatted(recentProject.Name.c_str());
					ImGui::PopFont();

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
					ImGui::TextUnformatted(recentProject.FilePath.c_str());

					float prevX = ImGui::GetCursorPosX();
					ImGui::SameLine();
					std::string lastOpenedString = FormatDateAndTime(lastOpened);
					ImGui::SetCursorPosX(leftEdge + projectButtonWidth - ImGui::CalcTextSize(lastOpenedString.c_str()).x - ImGui::GetStyle().FramePadding.x * 1.5f);
					ImGui::TextUnformatted(lastOpenedString.c_str());
					ImGui::PopStyleColor();

					ImGui::SetCursorPosX(prevX);

					if (ImGui::IsWindowHovered())
					{
						anyFrameHovered = true;
						m_HoveredProjectID = id;

						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						{
							s_ProjectToOpen = recentProject.FilePath;
							recentProject.LastOpened = time(NULL);
						}
					}

					if (ImGui::BeginPopupContextWindow("project_context_window"))
					{
						bool isStartupProject = m_Properties.UserPreferences->StartupProject == recentProject.FilePath;
						if (ImGui::MenuItem("Set Startup Project", nullptr, &isStartupProject))
						{
							m_Properties.UserPreferences->StartupProject = isStartupProject ? recentProject.FilePath : "";
							serializePreferences = true;
						}

						if (ImGui::MenuItem("Remove From List"))
						{
							if (isStartupProject)
								m_Properties.UserPreferences->StartupProject = "";
							it = recentProjects.erase(it);
						}

						ImGui::EndPopup();
					}
				}
				ImGui::EndChildFrame();

				if (changedColor)
					ImGui::PopStyleColor();
			}
			ImGui::PopStyleVar();

			if (!anyFrameHovered)
				m_HoveredProjectID = 0;

			ImGui::EndGroup();
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("general_area");
		{
			float columnWidth = ImGui::GetColumnWidth();
			float buttonWidth = columnWidth / 1.5f;
			float columnCenterX = columnWidth / 2.0f;

			ImGui::SetCursorPosX(columnCenterX - buttonWidth / 2.0f);
			ImGui::BeginGroup();

			if (ImGui::Button("New Project", ImVec2(buttonWidth, 50)))
				showNewProjectPopup = true;

			if (ImGui::Button("Open Project...", ImVec2(buttonWidth, 50)))
			{
				std::string result = Application::Get().OpenFile("Hazel Project (*.hproj)\0*.hproj\0");
				std::replace(result.begin(), result.end(), '\\', '/');
				AddProjectToRecents(result);
				s_ProjectToOpen = result;
			}

			ImGui::EndGroup();
		}
		ImGui::EndChild();

		if (showNewProjectPopup)
		{
			ImGui::OpenPopup("New Project");
			memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
			memset(s_ProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
			showNewProjectPopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 700, 325 });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 10));
		if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 325 / 8);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));

			ImGui::PushFont(boldFont);
			std::string fullProjectPath = strlen(s_ProjectFilePathBuffer) > 0 ? std::string(s_ProjectFilePathBuffer) + "/" + std::string(s_ProjectNameBuffer) : "";
			ImGui::Text("Full Project Path: %s", fullProjectPath.c_str());
			ImGui::PopFont();

			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##new_project_name", "Project Name", s_ProjectNameBuffer, MAX_PROJECT_NAME_LENGTH);

			ImVec2 label_size = ImGui::CalcTextSize("...", NULL, true);
			auto& style = ImGui::GetStyle();
			ImVec2 button_size = ImGui::CalcItemSize(ImVec2(0, 0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

			ImGui::SetNextItemWidth(600 - button_size.x - style.FramePadding.x * 2.0f - style.ItemInnerSpacing.x - 1);
			ImGui::InputTextWithHint("##new_project_location", "Project Location", s_ProjectFilePathBuffer, MAX_PROJECT_FILEPATH_LENGTH, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				std::string result = Application::Get().OpenFolder();
				std::replace(result.begin(), result.end(), '\\', '/');
				memcpy(s_ProjectFilePathBuffer, result.data(), result.length());
			}

			ImGui::Separator();

			ImGui::PushFont(boldFont);
			if (ImGui::Button("Create"))
			{
				CreateProject(fullProjectPath);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::PopFont();

			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(2);

		ImGui::End();

		if (!s_ProjectToOpen.empty())
		{
			m_Properties.ProjectOpenedCallback(s_ProjectToOpen);
			s_ProjectToOpen = "";
			serializePreferences = true;
		}

		if (serializePreferences)
		{
			UserPreferencesSerializer serializer(m_Properties.UserPreferences);
			serializer.Serialize(m_Properties.UserPreferences->FilePath);
		}

	}

	static void ReplaceProjectName(std::string& str, const std::string& projectName)
	{
		static const char* s_ProjectNameToken = "$PROJECT_NAME$";
		size_t pos = 0;
		while ((pos = str.find(s_ProjectNameToken, pos)) != std::string::npos)
		{
			str.replace(pos, strlen(s_ProjectNameToken), projectName);
			pos += strlen(s_ProjectNameToken);
		}
	}

	void IgnitionLayer::CreateProject(std::filesystem::path projectPath)
	{
		if (!std::filesystem::exists(projectPath))
			std::filesystem::create_directories(projectPath);

		std::filesystem::copy(m_Properties.InstallPath + "/FORGE/Resources/NewProjectTemplate", projectPath, std::filesystem::copy_options::recursive);

		{
			// premake5.lua
			std::ifstream stream(projectPath / "premake5.lua");
			RP_CORE_VERIFY(stream.is_open());
			std::stringstream ss;
			ss << stream.rdbuf();
			stream.close();

			std::string str = ss.str();
			ReplaceProjectName(str, s_ProjectNameBuffer);

			std::ofstream ostream(projectPath / "premake5.lua");
			ostream << str;
			ostream.close();
		}

		{
			// Project File
			std::ifstream stream(projectPath / "Project.rproj");
			RP_CORE_VERIFY(stream.is_open());
			std::stringstream ss;
			ss << stream.rdbuf();
			stream.close();

			std::string str = ss.str();
			ReplaceProjectName(str, s_ProjectNameBuffer);

			std::ofstream ostream(projectPath / "Project.rproj");
			ostream << str;
			ostream.close();

			std::string newProjectFileName = std::string(s_ProjectNameBuffer) + ".rproj";
			std::filesystem::rename(projectPath / "Project.rproj", projectPath / newProjectFileName);
		}

		std::filesystem::create_directories(projectPath / "Assets" / "Audio");
		std::filesystem::create_directories(projectPath / "Assets" / "Materials");
		std::filesystem::create_directories(projectPath / "Assets" / "Meshes" / "Source");
		std::filesystem::create_directories(projectPath / "Assets" / "Scenes");
		std::filesystem::create_directories(projectPath / "Assets" / "Scripts" / "Source");
		std::filesystem::create_directories(projectPath / "Assets" / "Textures");

		std::string batchFilePath = projectPath.string();
		std::replace(batchFilePath.begin(), batchFilePath.end(), '/', '\\'); // Only windows
		batchFilePath += "\\Win-CreateScriptProjects.bat";
		system(batchFilePath.c_str());

		AddProjectToRecents(projectPath.string() + "/" + std::string(s_ProjectNameBuffer) + ".rproj");
	}

	void IgnitionLayer::AddProjectToRecents(const std::filesystem::path& projectFile)
	{
		RecentProject projectEntry;
		projectEntry.Name = Utils::RemoveExtension(projectFile.filename().string());
		projectEntry.FilePath = projectFile.string();
		projectEntry.LastOpened = time(NULL);
		m_Properties.UserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;

		UserPreferencesSerializer serializer(m_Properties.UserPreferences);
		serializer.Serialize(m_Properties.UserPreferences->FilePath);
	}


}	//	END namespace RAPIER
