#include "Gameplay/Components/PlayerController.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"

PlayerController::PlayerController() :
	IComponent(),
	_mouseSensitivity({ 0.5f, 0.3f }),
	_moveSpeeds(glm::vec3(2.0f)),
	_shiftMultipler(1.5f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false)
{ }

PlayerController::~PlayerController() = default;

void PlayerController::Update(float deltaTime)
{
	if (Application::Get().IsFocused) {
		if (InputEngine::GetMouseState(GLFW_MOUSE_BUTTON_LEFT) == ButtonState::Pressed) {
			_prevMousePos = InputEngine::GetMousePos();
			LOG_INFO("doot");
		}
			
			glm::vec3 input = glm::vec3(0.0f);
			if (InputEngine::IsKeyDown(GLFW_KEY_W))
			{
				input.y += _moveSpeeds.x;
				GetGameObject()->SetRotation(glm::vec3(0, 0, -90));
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_S))
			{
				input.y -= _moveSpeeds.x;
				GetGameObject()->SetRotation(glm::vec3(0, 0, 90));
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_A))
			{
				input.x -= _moveSpeeds.y;
				GetGameObject()->SetRotation(glm::vec3(0, 0, 0));
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_D))
			{
				input.x += _moveSpeeds.y;
				GetGameObject()->SetRotation(glm::vec3(0, 0, 180));
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
			{
				input *= _shiftMultipler;
			}

			input *= deltaTime;

			glm::vec3 worldMovement = glm::vec4(input, 1.0f);
			GetGameObject()->SetPostion(GetGameObject()->GetPosition() + worldMovement);
	}
	_prevMousePos = InputEngine::GetMousePos();
}

void PlayerController::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat2, "Mouse Sensitivity", &_mouseSensitivity.x, 0.01f);
	LABEL_LEFT(ImGui::DragFloat3, "Move Speed       ", &_moveSpeeds.x, 0.01f, 0.01f);
	LABEL_LEFT(ImGui::DragFloat , "Shift Multiplier ", &_shiftMultipler, 0.01f, 1.0f);
}

nlohmann::json PlayerController::ToJson() const {
	return {
		{ "mouse_sensitivity", _mouseSensitivity },
		{ "move_speed", _moveSpeeds },
		{ "shift_mult", _shiftMultipler }
	};
}

PlayerController::Sptr PlayerController::FromJson(const nlohmann::json& blob) {
	PlayerController::Sptr result = std::make_shared<PlayerController>();
	result->_mouseSensitivity = JsonGet(blob, "mouse_sensitivity", result->_mouseSensitivity);
	result->_moveSpeeds       = JsonGet(blob, "move_speed", result->_moveSpeeds);
	result->_shiftMultipler   = JsonGet(blob, "shift_mult", 2.0f);
	return result;
}
