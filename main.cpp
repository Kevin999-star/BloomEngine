#include "Core/Application.h"

int main() {
    Application app(RenderBackend::OpenGL);
    app.Run();
    return 0;
}
