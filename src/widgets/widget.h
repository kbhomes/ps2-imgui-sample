#pragma once

#include "../drawing/drawing.h"

#include <libpad.h>
#include <imgui.h>

namespace ImGui {
    void GamePadVisualizer(padButtonStatus *pad, float width = 300.f, float height = 150.f);
}