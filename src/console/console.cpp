#include "console.hpp"
#include "renderer.hpp"

void initializeRendering(BootInfo& bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);
}