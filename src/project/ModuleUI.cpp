﻿#include "ModuleUI.h"
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include "ModuleInput.h"
#include "Application.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <filesystem>
#include "ModuleProfile.h"
#include "imgui/imgui_impl_opengl3.h"
#include <gl/GL.h>
#include "Time.h"
#include "SDL/include/SDL_assert.h"
#include "PanelDetailedFrame.h"
#include "PanelSectionTree.h"
#include "ModuleWindow.h"
#include "JSONfilepack.h"
#include "ModuleRenderer3D.h"
#include <string>
#include "PanelSectionFunctions.h"
#include "PanelFrames.h"
#include <fstream>
#include "IconFontCppHeaders/IconsFontAwesome5.h"
#include <ctime>

#pragma comment (lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */
#pragma comment (lib, "glew/glew32.lib") 

#define ASTEROID_NO_DOT_EXTENSION "asteroid"
#define ASTEROID_EXTENSION ".asteroid"

ModuleUI::ModuleUI(bool start_enabled) : Module(start_enabled)
{
	name.assign("UI");
}

ModuleUI::~ModuleUI()
{
}

bool ModuleUI::Init()
{
	return true;
}

bool ModuleUI::Start()
{
	bool ret = true;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

	static ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

	// load the default the first to set it as the main font, so no need to use PushFont and Pop font
	LoadFontDefaultFontWithIcons(13, "fa-solid-900.ttf", 30, icons_ranges, Font::DEFAULT);
	LoadFontDefaultFontWithIcons(16.9F, "fa-solid-900.ttf", 15.6F, icons_ranges, Font::DEFAULT_X0F3);
	LoadIconsFont("fa-solid-900.ttf", 17, icons_ranges, Font::ICONS_17);

	ImGui::StyleColorsDark();

	ImGui_ImplOpenGL3_Init();
	ImGui_ImplSDL2_InitForOpenGL(App->window->window, App->renderer->context);

	panels.push_back(panel_frames = new PanelFrames("Frames"));
	panels.push_back(panel_detailed_frame = new PanelDetailedFrame("Detailed Frame"));
	panels.push_back(panel_section_functions = new PanelSectionFunctions("Section Functions"));
	panels.push_back(panel_section_tree = new PanelSectionTree("Section Tree"));

	return ret;
}

bool ModuleUI::CleanUp()
{
	std::vector<Panel*>::iterator item = panels.begin();
	for (; item != panels.end(); ++item) {
		delete* item;
		*item = nullptr;
	}
	panels.clear();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return true;
}

update_status ModuleUI::PreUpdate(float dt)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(App->window->window);
	ImGui::NewFrame();

	return UPDATE_CONTINUE;
}

bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	// Render
	window->DrawList->PathClear();

	int num_segments = 30;
	int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

	const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
	const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < num_segments; i++) {
		const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
		window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
			centre.y + ImSin(a + g.Time * 8) * radius));
	}

	window->DrawList->PathStroke(color, false, thickness);
}

void ModuleUI::Draw() 
{
	if (show_demo_window) {
		ImGui::ShowDemoWindow(&show_demo_window);
	}
	MainMenuBar();
	
	// Create Docking
	// Seeting Docking to fit the window and preferences
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace BackGround", (bool*)0, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
	ImGui::PopStyleVar(3);

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		ImGuiID dockspace_id = ImGui::GetID("Dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}
	ImGui::End();
	// Create Docking

	switch (App->profile->state) {
	case ProfileState::NONE: {
		ImGui::SetWindowSize(ImVec2(30, 30), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * 0.5f, ImGuiCond_Always, { 0.5f, 0.5f });
		ImGui::Begin("Play", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.9));
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(0, 35))) {
			App->profile->ConnectClient();
		}
		ImGui::PopStyleVar();
		ImGui::End();
		break; }	
	case ProfileState::CONNECTING: {
		ImGui::SetWindowSize(ImVec2(30, 30), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * 0.5f, ImGuiCond_Always, { 0.5f, 0.5f });
		ImGui::Begin("Connect", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		ImGui::Indent();
		Spinner("##Conecting", 15.0f, 6, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
		ImGui::Unindent();
		ImGui::Text("Connecting...");
		ImGui::End();
		break; }
	case ProfileState::WAITING_INFO: {
		ImGui::SetWindowSize(ImVec2(50, 30), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * 0.5f, ImGuiCond_Always, { 0.5f, 0.5f });
		ImGui::Begin("Pause", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.9));
		ImGui::SetCursorPosX(27);
		if (ImGui::Button(ICON_FA_PAUSE, ImVec2(0, 35))) {
			App->profile->DisconnectClient();
		}
		ImGui::PopStyleVar();
		ImGui::Spacing();
		ImGui::SetCursorPosX(13);
		ImGui::Text("Frames: %i", App->profile->framesCount);
		ImGui::End();
		break; }
	case ProfileState::INFO: {
		UpdatePanels();
		break; }
	}

	static bool show_demo = false;
	if (App->input->GetKey(SDL_SCANCODE_F1) == KEY_REPEAT) {
		show_demo = true;
	}

	if (show_demo) {
		ImGui::ShowDemoWindow(&show_demo);
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ModuleUI::NormalizePath(std::string& path)
{
	for (auto it = path.begin(); it != path.end(); ++it) {
		if (*it == '\\') {
			*it = '/';
		}
	}
}

void ModuleUI::MainMenuBar()
{
	ImGui::BeginMainMenuBar();
	
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Load", nullptr, nullptr, App->profile->state == ProfileState::INFO || App->profile->state == ProfileState::NONE)) {
			LoadAsteroid();
		}
		if (ImGui::MenuItem("Save", nullptr, nullptr, App->profile->state == ProfileState::INFO)) {
			SaveAsteroid();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Clear", nullptr, nullptr, App->profile->state == ProfileState::INFO)) {
			App->profile->ResetInfo();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("About")) {

		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void ModuleUI::ChangeEnableDemo()
{
	show_demo_window = !show_demo_window;
}

void ModuleUI::LoadAsteroid()
{
	OPENFILENAME to_load;

	static char filename[MAX_PATH];

	static char curr_dir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, curr_dir);

	std::string filter = std::string(std::string(ASTEROID_NO_DOT_EXTENSION) + '\0' + "*" + ASTEROID_EXTENSION + '\0');
	std::string title = std::string("Load " + std::string(ASTEROID_EXTENSION) + '\0');

	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&to_load, sizeof(to_load));

	to_load.lStructSize = sizeof(to_load);
	to_load.hwndOwner = NULL;
	to_load.lpstrFilter = filter.data();
	to_load.lpstrFile = filename;
	to_load.nMaxFile = MAX_PATH;
	to_load.lpstrTitle = title.data();
	to_load.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	if (GetOpenFileNameA(&to_load)) {
		SetCurrentDirectoryA(curr_dir);
		std::string name = filename;
		NormalizePath(name);

		App->profile->LoadFile(name);
	}
	else {
		SetCurrentDirectoryA(curr_dir);
	}
}

void ModuleUI::SaveAsteroid()
{
	OPENFILENAME to_save;

	static char filename[MAX_PATH];

	static char curr_dir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, curr_dir);

	std::string filter = std::string(std::string(ASTEROID_NO_DOT_EXTENSION) + '\0' + "*" + ASTEROID_EXTENSION + '\0');
	std::string title = std::string("Save a" + std::string(ASTEROID_EXTENSION) + '\0');

	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&to_save, sizeof(to_save));

	to_save.lStructSize = sizeof(to_save);
	to_save.hwndOwner = NULL;
	to_save.lpstrFilter = filter.data();
	to_save.lpstrFile = filename;
	to_save.nMaxFile = MAX_PATH;
	to_save.lpstrTitle = title.data();
	to_save.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	if (GetSaveFileNameA(&to_save)) {
		SetCurrentDirectoryA(curr_dir);

		std::string extension = std::filesystem::path(filename).extension().string();
		extension = extension.size() > 1 ? extension.substr(1) : extension;

		std::string path;
		if (strcmp(ASTEROID_NO_DOT_EXTENSION, extension.data()) != 0) {
			path = std::string(filename + std::string(ASTEROID_EXTENSION)).data();
		}
		else {
			path = filename;
		}

		NormalizePath(path);

		remove(path.data());

		App->profile->SaveCurrentDataToFile(path);
	}
	else {
		SetCurrentDirectoryA(curr_dir);
	}
}

void ModuleUI::UpdatePanels()
{
	std::vector<Panel*>::iterator item = panels.begin();
	for (; item != panels.end(); ++item) {
		if (*item != nullptr) {
			if ((*item)->IsEnabled()) {
				(*item)->PanelLogic();
			}
			else if (!(*item)->desactive_called) {
				(*item)->desactive_called = true;
				(*item)->OnPanelDesactive();
			}
		}
	}

	if (pauseImGui) {
		ImGui::OpenPopup("PauseImGUi");
		ImGui::SetNextWindowSize({ 1,1 });
		ImGui::PushStyleColor(ImGuiCol_PopupBg, 0);
		ImGui::PushStyleColor(ImGuiCol_Border, 0);
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDarkening, ImVec4(0.8F, 0.8F, 0.8F, 0.15F));
		ImGui::BeginPopupModal("PauseImGUi", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);
		ImGui::EndPopup();
		ImGui::PopStyleColor(3);
	}
}

ImFont* ModuleUI::GetFont(Font font)
{
	return imFonts[font];
}

void ModuleUI::OnFrameDeselected()
{
	panel_frames->frame = nullptr;
	panel_detailed_frame->function_selected = nullptr;
}

void ModuleUI::OnFrameSelected(Frame* frame)
{
	panel_frames->frame = frame;
	panel_detailed_frame->function_selected = nullptr;
}

void ModuleUI::OnFunctionSelected(Function* function)
{
	panel_detailed_frame->function_selected = function;
}

void ModuleUI::LoadFontDefaultFontWithIcons(float fontSize, const char* iconsPath, float iconsSize, ImWchar* fontRanges, Font font)
{
	ImFontConfig default_config;
	default_config.OversampleH = default_config.OversampleV = 1;
	default_config.PixelSnapH = true;
	default_config.SizePixels = fontSize * 1.0f;
	ImFormatString(default_config.Name, IM_ARRAYSIZE(default_config.Name), "ProggyClean.ttf, %dpx", (int)default_config.SizePixels);
	default_config.EllipsisChar = (ImWchar)0x0085;
	
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	imFonts.emplace(font, io.Fonts->AddFontDefault(&default_config));

	ImFontConfig icons_config; 
	icons_config.MergeMode = true; 
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(iconsPath, iconsSize, &icons_config, fontRanges);
}
void ModuleUI::LoadFont(const char* path, float fontSize, Font font, const char* iconsPath, float iconsSize, ImWchar* fontRanges)
{
	ImFontConfig font_config;
	font_config.OversampleH = font_config.OversampleV = 1;
	font_config.PixelSnapH = true;
	font_config.SizePixels = fontSize * 1.0f;
	strcpy(font_config.Name, path);
	font_config.EllipsisChar = (ImWchar)0x0085;

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	imFonts.emplace(font, io.Fonts->AddFontFromFileTTF(path, fontSize, &font_config));

	if (iconsPath != nullptr) {
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		io.Fonts->AddFontFromFileTTF(iconsPath, iconsSize, &icons_config, fontRanges);
	}

}

void ModuleUI::LoadIconsFont(const char* path, float iconsSIze, ImWchar* fontRanges, Font font)
{
	ImFontConfig icons_config;
	icons_config.PixelSnapH = true;

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	imFonts.emplace(font, io.Fonts->AddFontFromFileTTF(path, iconsSIze, &icons_config, fontRanges));
}