#include "Gameplay/Components/LoseScreenBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"

#include "Gameplay/GameObject.h"
#include "Utils/JsonGlmHelpers.h"
#include "Gameplay/Components/SimpleCameraControl.h"

void LoseScreen::Awake()
{
}

void LoseScreen::RenderImGui() {
}

nlohmann::json LoseScreen::ToJson() const {
	return {
		
	};
}

LoseScreen::LoseScreen() :
	IComponent(),
	_impulse(10.0f)
{ }

LoseScreen::~LoseScreen() = default;

LoseScreen::Sptr LoseScreen::FromJson(const nlohmann::json& blob) {
	LoseScreen::Sptr result = std::make_shared<LoseScreen>();

	return result;
}

extern int gameState;
void LoseScreen::Update(float deltaTime) {
	Gameplay::IComponent::Sptr ptr = imageLose.lock();
	if (gameState == 1)
	{
		ptr->IsEnabled = true;
	}
}

