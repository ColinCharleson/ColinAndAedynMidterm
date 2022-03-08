#include "Gameplay/Components/WinScreenBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"

#include "Gameplay/GameObject.h"
#include "Utils/JsonGlmHelpers.h"
#include "Gameplay/Components/SimpleCameraControl.h"

void WinScreen::Awake()
{
}

void WinScreen::RenderImGui() {
}

nlohmann::json WinScreen::ToJson() const {
	return {
		
	};
}

WinScreen::WinScreen() :
	IComponent(),
	_impulse(10.0f)
{ }

WinScreen::~WinScreen() = default;

WinScreen::Sptr WinScreen::FromJson(const nlohmann::json& blob) {
	WinScreen::Sptr result = std::make_shared<WinScreen>();

	return result;
}

extern int gameState;
void WinScreen::Update(float deltaTime) {
	Gameplay::IComponent::Sptr ptr = testPanel.lock();
	if (gameState == 2)
	{
		ptr->IsEnabled = true;
	}
}

