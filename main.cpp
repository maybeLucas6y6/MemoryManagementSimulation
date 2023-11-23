#include "raylib.h"
#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include <string_view>

struct Address {
    int pos, size;
};
struct Memory {
    std::list<Address> blocks;
    int capacity;
    Color cellColor, gridColor;

    void DrawGrid(int screenWidth, int screenHeight, float gridOffsetX, float gridOffsetY) {
        float cellWidth = (screenWidth - 2 * gridOffsetX) / capacity, cellHeight = cellWidth;
        DrawLineV({ gridOffsetX, gridOffsetY }, { screenWidth - gridOffsetX, gridOffsetY }, gridColor);
        DrawLineV({ gridOffsetX, cellHeight + gridOffsetY }, { screenWidth - gridOffsetX, cellHeight + gridOffsetY }, gridColor);
        for (int i = 0; i < capacity; i++) {
            DrawLineV({ i * cellWidth + gridOffsetX, gridOffsetY }, { i * cellWidth + gridOffsetX, cellHeight + gridOffsetY }, gridColor);
            std::string s = std::to_string(i);
            const char* text = s.c_str();
            int textWidth = MeasureText(text, (int)cellWidth / 2);
            DrawTextEx(GetFontDefault(), text, { i * cellWidth + gridOffsetX + cellWidth / 2 - textWidth / 2, cellHeight + gridOffsetY + 5 }, cellWidth / 2.0f, 1, WHITE);
        }
        DrawLineV({ screenWidth - gridOffsetX, gridOffsetY }, { screenWidth - gridOffsetX, cellHeight + gridOffsetY }, gridColor);
    }
    void DrawBlocks(int screenWidth, int screenHeight, float gridOffsetX, float gridOffsetY) {
        float cellWidth = (screenWidth - 2 * gridOffsetX) / capacity, cellHeight = cellWidth;
        for (auto& e : blocks) {
            DrawRectangleV({ e.pos * cellWidth + gridOffsetX, gridOffsetY }, { cellWidth * e.size, cellHeight }, cellColor);
            DrawRectangleLinesEx({ e.pos * cellWidth + gridOffsetX, gridOffsetY, cellWidth * e.size, cellHeight }, 1.0f, DARKGREEN);
        }
    }
};

struct Button { // TODO: might derive from this to create hollow buttons
    std::string label;
    int fontSize;
    int textWidth;

    struct Rectangle {
        int x, y, width, height;
    } rec;
    int padding;

    struct Colors { // TODO: clicked color
        Color background, backgroundHovered;
        Color text, textHovered;
    } colors;

    Button(std::string_view s, int posx, int posy, Colors _colors = { BLACK, BLACK, WHITE, WHITE }, int _padding = 0.0f, int _fontSize = 20.0f) {
        label = s;
        fontSize = _fontSize;
        textWidth = MeasureText(label.data(), fontSize);

        padding = _padding;
        rec.x = posx;
        rec.y = posy;
        rec.width = textWidth + 2 * padding;
        rec.height = fontSize + 2 * padding;

        colors = _colors;
    }
    void SetLabel(std::string_view s) {
        label = s;
        textWidth = MeasureText(label.data(), fontSize);
        rec.width = textWidth + 2 * padding;
    }
    void SetPadding(int newPadding) {
        padding = newPadding;
        rec.width = textWidth + 2 * padding;
        rec.height = fontSize + 2 * padding;
    }
    void SetFontSize(int newFontSize) {
        fontSize = newFontSize;
        textWidth = MeasureText(label.data(), fontSize);
        rec.width = textWidth + 2 * padding;
        rec.height = fontSize + 2 * padding;
    }
    void SetPosition(int posx, int posy) {
        rec.x = posx;
        rec.y = posy;
    }
    void Draw() {
        if (IsHovered()) {
            DrawRectangle(rec.x, rec.y, rec.width, rec.height, colors.backgroundHovered);
            DrawText(label.data(), rec.x + padding, rec.y + padding, fontSize, colors.textHovered);
        }
        else {
            DrawRectangle(rec.x, rec.y, rec.width, rec.height, colors.background);
            DrawText(label.data(), rec.x + padding, rec.y + padding, fontSize, colors.text);
        }
    }
    bool IsHovered() {
        Vector2 mouse = GetMousePosition();
        return mouse.x >= rec.x && mouse.x <= rec.x + rec.width && mouse.y >= rec.y && mouse.y <= rec.y + rec.height;
    }
    bool IsClicked() { // TODO: this only returns true once when clicked, even if the mouse button is still down
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            return mouse.x >= rec.x && mouse.x <= rec.x + rec.width && mouse.y >= rec.y && mouse.y <= rec.y + rec.height;
        }
        else {
            return false;
        }
    }
};

class InputField { // TODO: make it accept more than numbers
protected:
    bool isWriting = false;
    std::string s;
    bool cursorIsShown = false;
    float cursorTime = 1.0f;
public:
    bool resetAfterEnter = false;
    // might rename it 'Update'
    bool CanGetInput(/*float deltaTime TODO: maybe pass deltaTime as param for better performance*/) {
        if (!isWriting) return false;

        float deltaTime = GetFrameTime();
        cursorTime -= deltaTime;
        if (cursorTime < 0.0f) {
            if (cursorIsShown) {
                if (!s.empty()) s.erase(s.end() - 1);
            }
            else {
                s += "|";
            }
            cursorIsShown = !cursorIsShown;
            cursorTime = 1.0f;
        }

        auto k = GetKeyPressed();
        if (k == KEY_ENTER) {
            return true;
        }
        else if (k == KEY_BACKSPACE) {
            if (cursorIsShown && s.size() > 1) s.erase(s.end() - 2, s.end() - 1);
            else if (!s.empty()) s.erase(s.end() - 1);
        }
        else if (k >= KEY_ZERO && k <= KEY_NINE || k == KEY_SPACE) {
            s += (char)k;
            if (cursorIsShown) std::swap(s[s.size() - 1], s[s.size() - 2]);
        }
        return false;
    }
    bool IsWriting() const { return isWriting; }
    void Activate() {
        isWriting = true;
    }
    void Deactivate() {
        isWriting = false;
        s.clear();
        if (cursorIsShown) s += "|";
    }
    void Reset() {
        s.clear();
        if (cursorIsShown) s += "|";
    }
    std::string GetInput() {
        std::string input;
        input = s.substr(0, s.size() - cursorIsShown);

        if (resetAfterEnter) {
            s.clear();
            if (cursorIsShown) s += "|";
        }

        return input;
    }
    char* Data() {
        return s.data();
    }
};

void tryAllocMem(Memory& mem, std::string s) {
    std::stringstream ss; ss << s;
    int size; ss >> size;

    bool inserted = false;
    if (size > 0 && size <= mem.capacity) {
        Address prev{ 0,0 };
        for (auto e = mem.blocks.begin(); e != mem.blocks.end(); e++) {
            if (e->pos - (prev.pos + prev.size) >= size) {
                mem.blocks.insert(e, { prev.pos + prev.size, size });
                inserted = true;
                break;
            }
            prev = *e;
        }
        if (!inserted && (mem.capacity - (prev.pos + prev.size) >= size)) {
            mem.blocks.push_back({ prev.pos + prev.size, size });
            inserted = true;
        }
    }
}

void tryFreeMem(Memory& mem, std::string s) {
    std::stringstream ss; ss << s;
    int pos, size; ss >> pos >> size;
    int end = pos + size;

    if (pos >= 0 && pos < mem.capacity && end > 0 && end <= mem.capacity) {
        for (auto b = mem.blocks.begin(); b != mem.blocks.end(); b++) {
            int bpos = b->pos, bend = b->pos + b->size;

            if (pos <= bpos && end >= bend) {
                b->size = 0;
                continue;
            }
            if (pos < bpos && end >= bpos) {
                b->pos = end;
                b->size = bend - end;
                continue;
            }
            if (pos < bend && end >= bend) {
                b->size = pos - bpos;
                continue;
            }
            if (pos >= bpos && end <= bend) {
                b->size = pos - bpos;
                mem.blocks.insert(b, { end, bend - end });
                continue;
            }
        }
        bool cleaned;
        do {
            cleaned = true;
            for (auto e = mem.blocks.begin(); e != mem.blocks.end(); e++) {
                if (e->size <= 0) {
                    mem.blocks.erase(e);
                    cleaned = false;
                    break;
                }
            }
        } while (!cleaned);
    }
}

int WinMain() {
    constexpr int screenWidth = 800, screenHeight = 280;

    Memory mem;
    mem.capacity = 25;
    mem.gridColor = GRAY;
    mem.cellColor = GREEN;
    mem.blocks.push_back({ 1, 3 });
    mem.blocks.push_back({ 8, 5 });
    mem.blocks.push_back({ 22, 1 });

    InitWindow(screenWidth, screenHeight, "Tree visualizer");
    SetTargetFPS(60);

    InputField allocMemField;
    allocMemField.resetAfterEnter = true;
    InputField freeMemField;
    freeMemField.resetAfterEnter = true;

    Button allocMem("New", 10, 120, { DARKGREEN, GREEN, WHITE, WHITE }, 10);
    Button freeMem("Free", 10, 170, { DARKGREEN, GREEN, WHITE, WHITE }, 10);
    Button defragMem("Defragmentate", 10, 220, { DARKGREEN, GREEN, WHITE, WHITE }, 10);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        if (allocMemField.CanGetInput()) {
            tryAllocMem(mem, allocMemField.GetInput());
        }
        if (freeMemField.CanGetInput()) {
            tryFreeMem(mem, freeMemField.GetInput());
        }

        if (allocMem.IsClicked()) {
            if (freeMemField.IsWriting()) {
                freeMem.SetLabel("Free");
                freeMem.colors.background = DARKGREEN;
                freeMem.colors.backgroundHovered = GREEN;

                freeMemField.Deactivate();
            }
            if (!allocMemField.IsWriting()) {
                allocMem.SetLabel("Cancel");
                allocMem.colors.background = RED;
                allocMem.colors.backgroundHovered = RED;

                allocMemField.Activate();
            }
            else {
                allocMem.SetLabel("New");
                allocMem.colors.background = DARKGREEN;
                allocMem.colors.backgroundHovered = GREEN;

                allocMemField.Deactivate();
            }
        }
        if (freeMem.IsClicked()) {
            if (allocMemField.IsWriting()) {
                allocMem.SetLabel("New");
                allocMem.colors.background = DARKGREEN;
                allocMem.colors.backgroundHovered = GREEN;

                allocMemField.Deactivate();
            }
            if (!freeMemField.IsWriting()) {
                freeMem.SetLabel("Cancel");
                freeMem.colors.background = RED;
                freeMem.colors.backgroundHovered = RED;

                freeMemField.Activate();
            }
            else {
                freeMem.SetLabel("Free");
                freeMem.colors.background = DARKGREEN;
                freeMem.colors.backgroundHovered = GREEN;

                freeMemField.Deactivate();
            }
        }
        if (defragMem.IsClicked()) {
            allocMem.SetLabel("New");
            allocMem.colors.background = DARKGREEN;
            allocMem.colors.backgroundHovered = GREEN;

            freeMem.SetLabel("Free");
            freeMem.colors.background = DARKGREEN;
            freeMem.colors.backgroundHovered = GREEN;

            allocMemField.Deactivate();
            freeMemField.Deactivate();

            // defrag memory
            int size = 0;
            for (auto& e : mem.blocks) {
                e.pos = size;
                size += e.size;
            }
        }

        BeginDrawing();

        ClearBackground(BLACK);
        //DrawFPS(0, 0);
        std::string memSize = "No. of blocks: " + std::to_string(mem.blocks.size());
        DrawText(memSize.data(), 10, 20, 20, WHITE);

        if (allocMemField.IsWriting()) {
            std::string input = "Allocate memory of size _ : "; input += allocMemField.Data();
            DrawText(input.data(), allocMem.rec.width + 20, allocMem.rec.y + allocMem.padding, 20, WHITE);
        }
        if (freeMemField.IsWriting()) {
            std::string input = "Free memory at position _ of size _ : "; input += freeMemField.Data();
            DrawText(input.data(), freeMem.rec.width + 20, freeMem.rec.y + freeMem.padding, 20, WHITE);
        }

        mem.DrawGrid(screenWidth, screenHeight, 10, 50);
        mem.DrawBlocks(screenWidth, screenHeight, 10, 50);

        if (!freeMemField.IsWriting()) allocMem.Draw();
        if (!allocMemField.IsWriting()) freeMem.Draw();
        if (!freeMemField.IsWriting() && !allocMemField.IsWriting()) defragMem.Draw();

        EndDrawing();
    }
    CloseWindow();

    return 0;
}