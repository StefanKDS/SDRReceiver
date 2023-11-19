#include <imgui.h>
#include <utils/flog.h>
#include <module.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <core.h>
#include <thread>
#include <signal_path/signal_path.h>
#include <vector>
#include <gui/tuner.h>
#include <gui/file_dialogs.h>
#include <utils/freq_formatting.h>
#include <gui/dialogs/dialog_box.h>
#include <gui/widgets/folder_select.h>
#include <fstream>
#include <filesystem>

SDRPP_MOD_INFO{
    /* Name:            */ "Logbook",
    /* Description:     */ "SWL Logbook",
    /* Author:          */ "Stefan Seyer",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ -1
};

struct LogbookEntry {
    double frequency;
    char mode[5];
    char callsign[128];
    char date[15];
    char time[15];
};

const char* demodModeList[] = {
    "NFM",
    "WFM",
    "AM",
    "DSB",
    "USB",
    "CW",
    "LSB",
    "RAW"
};

const char* demodModeListTxt = "CW\0RTTY\0SSB\0LSB\0USB\0NFM\0AM\0DSB\0RAW\0";

class Logbook : public ModuleManager::Instance 
{
public:
    Logbook(std::string name): folderSelect("%ROOT%/logbook") {
        this->name = name;
        gui::menu.registerEntry(name, menuHandler, this, NULL);
        prepared = false;
    }

    ~Logbook() {
        gui::menu.removeEntry(name);
    }

    void postInit() {}

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

private:
    bool createAdd = false;
    bool createShow = false;
    bool createExport = false;
    bool prepared;
    LogbookEntry logEntry;
    FolderSelect folderSelect;
    int selectedMode;

    int modeToIndex (char* mode)
    {
        if(strcmp(mode,"CW") == 0)
            return  0;
        else if(strcmp(mode,"RTTY") == 0)
            return 1;
        else if(strcmp(mode,"SSB") == 0)
            return 2;
        else if(strcmp(mode,"LSB") == 0)
            return 3;
        else if(strcmp(mode,"USB") == 0)
            return 4;
        else if(strcmp(mode,"NFM") == 0)
            return 5;
        else if(strcmp(mode,"AM") == 0)
            return 6;
        else if(strcmp(mode,"DSB") == 0)
            return 7;
        else
            return 8;
    }

    std::string indexToMode (int index)
    {
        if(index == 0)
            return "CW";
        else if(index == 1)
            return "RTTY";
        else if(index == 2)
            return "SSB";
        else if(index == 3)
            return "LSB";
        else if(index == 4)
            return "USB";
        else if(index == 5)
            return "NSM";
        else if(index == 6)
            return "AM";
        else if(index == 7)
            return "DSB";
        else
            return "RAW";
    }


    void prepare() {
        char nameBuf[128];
    
        if (gui::waterfall.selectedVFO == "") 
        {
            logEntry.frequency = gui::waterfall.getCenterFrequency();
        }
        else 
        {
            logEntry.frequency = gui::waterfall.getCenterFrequency() + sigpath::vfoManager.getOffset(gui::waterfall.selectedVFO);
        }

        int mode;
        core::modComManager.callInterface(gui::waterfall.selectedVFO, 0, NULL, &mode);
        sprintf(logEntry.mode, "%s", demodModeList[mode]);
        selectedMode = modeToIndex(logEntry.mode);
        
        time_t now = time(0);
        tm* ltm = localtime(&now);
        sprintf(logEntry.time, "%02d:%02d", ltm->tm_hour, ltm->tm_min);
        sprintf(logEntry.date, "%02d.%02d.%02d", ltm->tm_mday, ltm->tm_mon, ltm->tm_year + 1900);
        sprintf(logEntry.callsign, "");
    } 

    bool AddWindow() {
        bool open = true;

        gui::mainWindow.lockWaterfallControls = true;

        std::string id = "Add##logbook_add_popup_" + name;
        ImGui::OpenPopup(id.c_str());

        if(prepared == false)
        {
            prepare();
            prepared = true;
        }

        if (ImGui::BeginPopup(id.c_str(), ImGuiWindowFlags_NoResize)) {
            ImGui::BeginTable(("logbook_add_table" + name).c_str(), 2);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::LeftLabel("Date");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##Date", logEntry.date, 15);
 
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::LeftLabel("Time");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##Time", logEntry.time, 15);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::LeftLabel("Frequency");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200);
            ImGui::InputDouble("##Freq", &(logEntry.frequency),(0.0),(0.0),"%.0f");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::LeftLabel("Mode");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200);
            ImGui::Combo("##Mode",&selectedMode,demodModeListTxt);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::LeftLabel("Callsign");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##CallSign", logEntry.callsign, 128);
        
            ImGui::EndTable();

            if (ImGui::Button("Log"))
            {
                log();
                open = false;
                prepared = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                open = false;
                prepared = false;
            }
            ImGui::EndPopup();
        }

        return open;
    }

    bool ExportWindow() {
       bool open = true;
       std::string newPath;

       gui::mainWindow.lockWaterfallControls = true;

        std::string id = "Export##logbook_export_popup_" + name;
        ImGui::OpenPopup(id.c_str());

        if (ImGui::BeginPopup(id.c_str(), ImGuiWindowFlags_NoResize)) 
        {
            folderSelect.render("##_logbook_fold_" + name);

            if (ImGui::Button("Export", ImVec2(150,0))) 
            {
                if (folderSelect.pathIsValid())
                {
                    //Export
                    std::string root = (std::string)core::args["root"];
                    std::ifstream myFile;
                    myFile.open (root+"/logbook/logbook.csv", std::ios::in);

                    std::ifstream src(root+"/logbook/logbook.csv", std::ios::binary);
                    std::ofstream dest(folderSelect.path+"/logbook/logbook.csv", std::ios::binary);
                    dest << src.rdbuf();

                    open = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                open = false;
                prepared = false;
            }
            ImGui::EndPopup();
        }

        return open;
    }

    bool ShowLogWindow() {
        bool open = true;

        gui::mainWindow.lockWaterfallControls = true;

        std::string id = "Logbook##logbook_show_popup_" + name;
        ImGui::OpenPopup(id.c_str());

        if (ImGui::BeginPopup(id.c_str(), ImGuiWindowFlags_NoResize)) 
        {
    	    if (ImGui::BeginTable(("logbook_show_table" + name).c_str(), 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) 
            {
                ImGui::TableSetupColumn("Date");
                ImGui::TableSetupColumn("Time");
                ImGui::TableSetupColumn("Callsign");
                ImGui::TableSetupColumn("Frequency");
                ImGui::TableSetupColumn("Mode");
                ImGui::TableHeadersRow();

 
                std::string root = (std::string)core::args["root"];
                std::ifstream myFile;
                myFile.open (root+"/logbook/logbook.csv", std::ios::in);

    
                    std::string line;
                    while (std::getline(myFile, line))
                    {
                        ImGui::TableNextRow();
                        std::istringstream s(line);
                        std::string field;
                        int colCounter = 0;

                        while (getline(s, field,';'))
                        {
                            ImGui::TableSetColumnIndex(colCounter);
                            ImGui::TextUnformatted(field.c_str());
                            colCounter++;
                        }
                    }
    
                    // Close the file
                    myFile.close();

                ImGui::EndTable();
            }

            if (ImGui::Button("Close")) {
                open = false;
            }
            ImGui::EndPopup();
        }

        return open;
    }

    void log() {
        // Write into csv file
        std::string root = (std::string)core::args["root"];
        std::ofstream myFile;
        myFile.open (root+"/logbook/logbook.csv", std::ios::out | std::ios::app );

        char dataLine[1024];
        sprintf(dataLine, "%s;%s;%s;%i;%s\n", logEntry.date, logEntry.time, logEntry.callsign, (int)logEntry.frequency, indexToMode(selectedMode).c_str());
        // Send data to the stream
        myFile << dataLine;
    
        // Close the file
        myFile.close();
    }

    static void menuHandler(void* ctx) {
        Logbook* _this = (Logbook*)ctx;
        float menuWidth = ImGui::GetContentRegionAvail().x;
        
        ImGui::FillWidth();
        if (ImGui::Button("Add Log", ImVec2(menuWidth, 0)))
        {
            _this->createAdd = true;
        }

        if (ImGui::Button("Show Logbook", ImVec2(menuWidth, 0)))
        {
            _this->createShow = true;
        }

        if (ImGui::Button("Export Logbook", ImVec2(menuWidth, 0)))
        {
            _this->createExport = true;
        }

        if (_this->createAdd) {
            _this->createAdd = _this->AddWindow();
        }

        if (_this->createShow) {
            _this->createShow = _this->ShowLogWindow();
        }

        if (_this->createExport) {
            _this->createExport = _this->ExportWindow();
        }
    }

    std::string name;
    bool enabled = true;
};

MOD_EXPORT void _INIT_() {
     // Create default log directory
    std::string root = (std::string)core::args["root"];
    if (!std::filesystem::exists(root + "/logbook")) {
        flog::warn("Logbook directory does not exist, creating it");
        if (!std::filesystem::create_directory(root + "/logbook")) {
            flog::error("Could not create Logbook directory");
        }
    }

}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    return new Logbook(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
    delete (Logbook*)instance;
}

MOD_EXPORT void _END_() {
    // Nothing here
}