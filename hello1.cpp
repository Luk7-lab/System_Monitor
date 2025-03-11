#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>

int WIDTH = 900;
int HEIGHT = 650;

enum SortType { PID, CPU, MEM, NAME };

struct ProcessInfo {
    int pid;
    std::string name;
    float cpu;
    float mem;
};

std::vector<ProcessInfo> getProcesses() {
    std::vector<ProcessInfo> processes;
    FILE* pipe = popen("ps aux --sort=-%cpu | awk '{print $2, $3, $4, $11}'", "r");
    if (!pipe) return processes;

    char buffer[256];
    fgets(buffer, sizeof(buffer), pipe);
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::istringstream iss(buffer);
        ProcessInfo pi;
        if (iss >> pi.pid >> pi.cpu >> pi.mem >> pi.name)
            processes.push_back(pi);
    }

    pclose(pipe);
    return processes;
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

bool isInside(int x, int y, SDL_Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

void launchGraph() {
    system("./hello &");
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("DejaVu_Sans/DejaVuSans-Bold.ttf", 16);

    SDL_Rect graphButton = {WIDTH - 150, 20, 120, 40};
    SDL_Rect logButton = {WIDTH - 150, 70, 120, 40};
    SDL_Rect headerRects[4] = {{20, 50, 80, 25}, {120, 50, 80, 25}, {220, 50, 80, 25}, {320, 50, 200, 25}};

    SortType currentSort = CPU;

    bool running = true;
    SDL_Event event;
    std::string searchQuery;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;

                if (isInside(x, y, graphButton)) launchGraph();
                if (isInside(x, y, logButton)) system("./logger &");

                for (int i = 0; i < 4; ++i)
                    if (isInside(x, y, headerRects[i]))
                        currentSort = static_cast<SortType>(i);
            }

            if (event.type == SDL_TEXTINPUT) searchQuery += event.text.text;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE && !searchQuery.empty()) searchQuery.pop_back();
        }

        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
        SDL_RenderClear(renderer);

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        // Graph Button
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        if (isInside(mouseX, mouseY, graphButton)) SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(renderer, &graphButton);
        SDL_RenderDrawRect(renderer, &graphButton);
        renderText(renderer, font, "Show Graph", graphButton.x + 15, graphButton.y + 10, {255,255,255});

        // Log Button
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        if (isInside(mouseX, mouseY, logButton)) SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        SDL_RenderFillRect(renderer, &logButton);
        SDL_RenderDrawRect(renderer, &logButton);
        renderText(renderer, font, "Save Log", logButton.x + 20, logButton.y + 10, {255,255,255});

        // Headers
        const char* headers[] = {"PID", "CPU%", "MEM%", "NAME"};
        for (int i = 0; i < 4; ++i) {
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &headerRects[i]);
            renderText(renderer, font, headers[i], headerRects[i].x + 5, headerRects[i].y + 5, {255, 255, 255});
        }

        auto processes = getProcesses();
        if (currentSort == PID)
            std::sort(processes.begin(), processes.end(), [](auto &a, auto &b){return a.pid < b.pid;});
        else if (currentSort == CPU)
            std::sort(processes.begin(), processes.end(), [](auto &a, auto &b){return a.cpu > b.cpu;});
        else if (currentSort == MEM)
            std::sort(processes.begin(), processes.end(), [](auto &a, auto &b){return a.mem > b.mem;});
        else if (currentSort == NAME)
            std::sort(processes.begin(), processes.end(), [](auto &a, auto &b){return a.name < b.name;});

        int y_offset = 80;
        for (auto& p : processes) {
            std::ostringstream oss;
            oss << std::setw(10)<< p.pid << std::setw(10) << p.cpu << "%" << std::setw(10) << p.mem << "%" << "  " << p.name;
            renderText(renderer, font, oss.str(), 20, y_offset, {150,255,150});
            y_offset += 20;
            if (y_offset > HEIGHT - 40) break;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(500);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit();

    return 0;
}