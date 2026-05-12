#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map> 
#include <unordered_set>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "SocialNetwork.h"
#include "RecommendationEngine.h"

using namespace std;

// ---- Graph layout state ----
static unordered_map<string, ImVec2> g_nodePos;
static int   g_cachedUserCount = 0;
static float g_cachedCx = 0.f, g_cachedCy = 0.f;

// Recomputes circular node layout only when the user count or canvas centre changes.
static void refreshLayout(const SocialNetwork& net, float cx, float cy, float radius)
{
    const auto& users = net.getAllUsers();
    int n = (int)users.size();

    bool centerMoved = (fabsf(cx - g_cachedCx) > 2.f || fabsf(cy - g_cachedCy) > 2.f);
    if (n == g_cachedUserCount && !centerMoved) return;

    g_cachedUserCount = n;
    g_cachedCx = cx;
    g_cachedCy = cy;

    // Sort names for a stable, deterministic layout
    vector<string> names;
    names.reserve(n);
    for (const auto& [name, _] : users) names.push_back(name);
    sort(names.begin(), names.end());

    for (int i = 0; i < n; i++) {
        float angle = 2.f * (float)M_PI * i / max(n, 1) - (float)M_PI / 2.f;
        g_nodePos[names[i]] = { cx + radius * cosf(angle), cy + radius * sinf(angle) };
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Friend Suggestion Network", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.f;
    style.FrameRounding  = 3.f;
    style.ItemSpacing    = { 8.f, 6.f };

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    SocialNetwork network;

    // ---- UI state ----
    char userName[128]      = {};
    char interestInput[128] = {};
    vector<string> pendingInterests;

    char friend1[128] = {};
    char friend2[128] = {};

    char suggestTarget[128] = {};
    vector<ComprehensiveSuggestion> suggestions;
    unordered_set<string> suggestedSet;
    string queryUser;
    string statusMsg;

    const float NODE_R     = 24.f;
    const float PANEL_W    = 400.f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        // Full-screen root window
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ (float)fbW, (float)fbH });
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar);

        // Title bar row
        ImGui::TextColored({ 0.45f, 0.85f, 1.f, 1.f }, "Friend Suggestion Network");
        ImGui::SameLine();
        const auto& allUsers = network.getAllUsers();
        ImGui::TextDisabled("| %d users", (int)allUsers.size());
        ImGui::Separator();

        // ================================================================
        // LEFT PANEL — controls
        // ================================================================
        ImGui::BeginChild("controls", { PANEL_W, 0 }, true);

        // ---- Add User ----
        static bool s_focusInterest = false;

        ImGui::TextColored({ 0.5f, 0.95f, 0.55f, 1.f }, "Add User");

        // Name field — Enter triggers Create User
        ImGui::SetNextItemWidth(-1.f);
        bool nameEnter = ImGui::InputTextWithHint("##uname", "User name...", userName, sizeof(userName),
                                                  ImGuiInputTextFlags_EnterReturnsTrue);

        // Interest row: [input box          ][ + ]
        ImGui::SetNextItemWidth(-52.f);
        if (s_focusInterest) { ImGui::SetKeyboardFocusHere(); s_focusInterest = false; }
        bool addInterest = ImGui::InputTextWithHint("##interest", "Add an interest...", interestInput, sizeof(interestInput),
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        addInterest |= ImGui::Button("+##add", { -1.f, 0 });
        if (addInterest) {
            string s(interestInput);
            if (!s.empty()) {
                for (auto& c : s) c = (char)tolower((unsigned char)c);
                pendingInterests.push_back(s);
                interestInput[0] = '\0';
            }
            s_focusInterest = true;
        }

        if (!pendingInterests.empty()) {
            ImGui::TextDisabled("Interests queued:");
            for (auto& p : pendingInterests) ImGui::BulletText("%s", p.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) pendingInterests.clear();
        }

        bool noName = (userName[0] == '\0');
        if (noName) ImGui::BeginDisabled();
        bool doCreate = ImGui::Button("Create User", { -1.f, 0 }) || (nameEnter && !noName);
        if (noName) ImGui::EndDisabled();
        if (doCreate && !noName) {
            unordered_set<string> ints(pendingInterests.begin(), pendingInterests.end());
            if (network.addUser(string(userName), ints)) {
                statusMsg = "User added: " + string(userName);
                userName[0] = '\0';
                pendingInterests.clear();
            } else {
                statusMsg = "\"" + string(userName) + "\" already exists!";
            }
        }

        ImGui::Separator();

        // ---- Add Friendship ----
        ImGui::TextColored({ 1.f, 0.80f, 0.35f, 1.f }, "Add Friendship");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputTextWithHint("##f1", "User A...", friend1, sizeof(friend1));
        ImGui::SetNextItemWidth(-1.f);
        bool f2Enter = ImGui::InputTextWithHint("##f2", "User B...", friend2, sizeof(friend2),
                                                ImGuiInputTextFlags_EnterReturnsTrue);

        bool bothFilled = (friend1[0] != '\0' && friend2[0] != '\0');
        if (!bothFilled) ImGui::BeginDisabled();
        bool doConnect = ImGui::Button("Connect", { -1.f, 0 }) || (f2Enter && bothFilled);
        if (!bothFilled) ImGui::EndDisabled();
        if (doConnect && bothFilled) {
            network.addFriendship(string(friend1), string(friend2));
            statusMsg = string(friend1) + " <-> " + string(friend2);
            friend1[0] = '\0';
            friend2[0] = '\0';
        }

        ImGui::Separator();

        // ---- Get Suggestions ----
        ImGui::TextColored({ 1.f, 0.60f, 0.80f, 1.f }, "Get Suggestions");
        ImGui::SetNextItemWidth(-1.f);
        bool suggestEnter = ImGui::InputTextWithHint("##suggest", "User name...", suggestTarget, sizeof(suggestTarget),
                                                     ImGuiInputTextFlags_EnterReturnsTrue);

        bool noTarget = (suggestTarget[0] == '\0');
        if (noTarget) ImGui::BeginDisabled();
        bool doSuggest = ImGui::Button("Find Matches", { -1.f, 0 }) || (suggestEnter && !noTarget);
        if (noTarget) ImGui::EndDisabled();
        if (doSuggest && !noTarget) {
            queryUser   = string(suggestTarget);
            suggestions = RecommendationEngine::getSmartSuggestions(network, queryUser);
            suggestedSet.clear();
            for (auto& s : suggestions) suggestedSet.insert(s.suggestedUserName);
            statusMsg = suggestions.empty()
                ? "No suggestions found for \"" + queryUser + "\""
                : "Found " + to_string(suggestions.size()) + " suggestion(s) for \"" + queryUser + "\"";
        }

        // Suggestion results list
        if (!suggestions.empty()) {
            ImGui::Separator();
            for (const auto& s : suggestions) {
                ImGui::TextColored({ 1.f, 0.85f, 0.3f, 1.f },
                    "-> %s  (score: %d)", s.suggestedUserName.c_str(), s.getScore());

                if (!s.mutualFriends.empty()) {
                    string line = "   mutual: ";
                    for (size_t i = 0; i < s.mutualFriends.size(); i++) {
                        if (i) line += ", ";
                        line += s.mutualFriends[i];
                    }
                    ImGui::TextDisabled("%s", line.c_str());
                }
                if (!s.sharedInterests.empty()) {
                    string line = "   likes: ";
                    for (size_t i = 0; i < s.sharedInterests.size(); i++) {
                        if (i) line += ", ";
                        line += s.sharedInterests[i];
                    }
                    ImGui::TextDisabled("%s", line.c_str());
                }
            }
        }

        // Status bar
        if (!statusMsg.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", statusMsg.c_str());
        }

        ImGui::Separator();
        ImGui::TextDisabled("Legend:");
        ImGui::ColorButton("##c1", { 0.31f, 0.71f, 0.39f, 1.f }, 0, { 12, 12 });
        ImGui::SameLine(); ImGui::TextDisabled("User");
        ImGui::SameLine(0, 16);
        ImGui::ColorButton("##c2", { 0.20f, 0.78f, 1.f, 1.f }, 0, { 12, 12 });
        ImGui::SameLine(); ImGui::TextDisabled("Selected");
        ImGui::SameLine(0, 16);
        ImGui::ColorButton("##c3", { 1.f, 0.82f, 0.20f, 1.f }, 0, { 12, 12 });
        ImGui::SameLine(); ImGui::TextDisabled("Suggested");

        ImGui::EndChild();

        ImGui::SameLine();

        // ================================================================
        // RIGHT PANEL — graph canvas
        // ================================================================
        ImGui::BeginChild("graph", { 0, 0 }, true);
        ImGui::TextDisabled("Social Graph  (hover a node to see interests)");

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 avail  = ImGui::GetContentRegionAvail();
        float  cx     = origin.x + avail.x * 0.5f;
        float  cy     = origin.y + avail.y * 0.5f;
        float  radius = min(avail.x, avail.y) * 0.36f;

        refreshLayout(network, cx, cy, radius);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Draw edges
        for (const auto& [name, user] : allUsers) {
            if (!g_nodePos.count(name)) continue;
            ImVec2 a = g_nodePos.at(name);
            for (const auto& fn : network.getFriends(name)) {
                if (name >= fn || !g_nodePos.count(fn)) continue;
                ImVec2 b = g_nodePos.at(fn);
                dl->AddLine(a, b, IM_COL32(160, 160, 180, 160), 2.f);
            }
        }

        // Draw nodes + labels
        ImVec2 mousePos = ImGui::GetMousePos();
        for (const auto& [name, user] : allUsers) {
            if (!g_nodePos.count(name)) continue;
            ImVec2 p = g_nodePos.at(name);

            ImU32 fill;
            if (!queryUser.empty() && name == queryUser)
                fill = IM_COL32(50,  200, 255, 255);   // cyan — queried user
            else if (suggestedSet.count(name))
                fill = IM_COL32(255, 210,  50, 255);   // gold — suggested
            else
                fill = IM_COL32(80,  180, 100, 255);   // green — normal

            dl->AddCircleFilled(p, NODE_R, fill, 32);
            dl->AddCircle(p, NODE_R, IM_COL32(220, 220, 220, 180), 32, 2.f);

            ImVec2 sz = ImGui::CalcTextSize(name.c_str());
            dl->AddText({ p.x - sz.x * 0.5f, p.y + NODE_R + 4.f },
                        IM_COL32(230, 230, 230, 255), name.c_str());
        }

        // Hover tooltip: show user interests
        for (const auto& [name, user] : allUsers) {
            if (!g_nodePos.count(name)) continue;
            ImVec2 p  = g_nodePos.at(name);
            float  dx = mousePos.x - p.x, dy = mousePos.y - p.y;
            if (dx * dx + dy * dy <= NODE_R * NODE_R) {
                string istr;
                for (const auto& interest : user.interests) {
                    if (!istr.empty()) istr += ", ";
                    istr += interest;
                }
                ImGui::SetTooltip("%s\nInterests: %s",
                    name.c_str(),
                    istr.empty() ? "(none)" : istr.c_str());
                break;
            }
        }

        ImGui::EndChild();
        ImGui::End();

        // Render
        ImGui::Render();
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.08f, 0.08f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
