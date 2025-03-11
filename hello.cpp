#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>

int WIDTH = 900;
int HEIGHT = 650;

struct MemoryUsage {
    float used_memory_percentage;
    float cpu_load_percentage;
};

MemoryUsage getSystemUsage() {
    std::ifstream memfile("/proc/meminfo");
    std::string line;
    long total_memory = 0, free_memory = 0;

    while (std::getline(memfile, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;
        iss >> key >> value;

        if (key == "MemTotal:") total_memory = value;
        if (key == "MemAvailable:") {
            free_memory = value;
            break;
        }
    }

    float used_memory = 100.0f * (1.0f - (float)free_memory / total_memory);

    static long prevIdle = 0, prevTotal = 0;
    std::ifstream cpufile("/proc/stat");
    std::string cpuline;
    std::getline(cpufile, cpuline);
    std::istringstream ss(cpuline);
    std::string cpu;
    long user, nice, system, idle;
    ss >> cpu >> user >> nice >> system >> idle;

    long total = user + nice + system + idle;
    long totalDiff = total - prevTotal;
    long idleDiff = idle - prevIdle;

    float cpu_load = totalDiff > 0 ? 100.0f * (1.0f - idleDiff / (float)totalDiff) : 0;

    prevTotal = total;
    prevIdle = idle;

    return {used_memory, cpu_load};
}

void renderGrid(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);

    for (int i = 0; i <= WIDTH; i += 50)
        SDL_RenderDrawLine(renderer, i, 0, i, HEIGHT);
    for (int j = 0; j <= HEIGHT; j += 50)
        SDL_RenderDrawLine(renderer, 0, j, WIDTH, j);
}

void renderGraph(SDL_Renderer* renderer, const std::vector<float>& history, int offsetY, SDL_Color color, float scale) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    int x_step = WIDTH / history.size();
    for (size_t i = 1; i < history.size(); ++i) {
        SDL_RenderDrawLine(renderer,
                           (i - 1) * x_step, offsetY - (history[i - 1] * scale),
                           i * x_step, offsetY - (history[i] * scale));
    }

    // Нижняя граница графика
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 100);
    SDL_RenderDrawLine(renderer, 0, offsetY, WIDTH, offsetY);
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, float mem_usage, float cpu_usage) {
    char text[256];
    snprintf(text, sizeof(text), "RAM Usage: %.2f%% | CPU Usage: %.2f%%", mem_usage, cpu_usage);

    SDL_Color color = {255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect = {20, 20, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("🖥️ System Resource Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("DejaVu_Sans/DejaVuSans-Bold.ttf", 18);

    std::vector<float> memory_history(100, 0.0f);
    std::vector<float> cpu_history(100, 0.0f);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                WIDTH = event.window.data1;
                HEIGHT = event.window.data2;
            }
        }

        MemoryUsage usage = getSystemUsage();

        memory_history.push_back(usage.used_memory_percentage);
        cpu_history.push_back(usage.cpu_load_percentage);

        if (memory_history.size() > 100) memory_history.erase(memory_history.begin());
        if (cpu_history.size() > 100) cpu_history.erase(cpu_history.begin());

        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
        SDL_RenderClear(renderer);

        renderGrid(renderer);

        renderGraph(renderer, memory_history, HEIGHT / 1.7, {0, 150, 255}, 2.5f);
        renderGraph(renderer, cpu_history, HEIGHT - 50, {0, 255, 100}, 2.5f);

        renderText(renderer, font, usage.used_memory_percentage, usage.cpu_load_percentage);

        SDL_RenderPresent(renderer);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}