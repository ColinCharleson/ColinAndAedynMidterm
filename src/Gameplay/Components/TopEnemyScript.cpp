#include "Gameplay/Components/TopEnemyScript.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"

TopEnemyScript::TopEnemyScript() :
	IComponent(),
	_mouseSensitivity({ 0.5f, 0.3f }),
	_moveSpeeds(glm::vec3(4.0f)),
	_shiftMultipler(1.5f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false)
{
}

extern float playerX, playerY;
extern float boltX, boltY, boltZ;
extern int playerScore;
extern bool boltOut;
extern bool canShoot;

float topMoveTimer;
bool topMoveLeft = true;
TopEnemyScript::~TopEnemyScript() = default;

void TopEnemyScript::Update(float deltaTime)
{
	if ((sqrt(pow(GetGameObject()->GetPosition().x - boltX, 2) + pow(GetGameObject()->GetPosition().y - boltY, 2) + pow(GetGameObject()->GetPosition().z - boltZ, 2) * 2)) <= 1.0f)
	{
		playerScore += 1;
		GetGameObject()->GetScene()->RemoveGameObject(GetGameObject()->SelfRef());
		boltOut = false;
	}


	topMoveTimer += 1 * deltaTime;

	if (topMoveTimer >= 5)
	{
		topMoveLeft = !topMoveLeft;
		topMoveTimer = 0;
	}
	if (topMoveLeft == true)
	{
		GetGameObject()->SetPostion(GetGameObject()->GetPosition() + glm::vec3(-3.8 * deltaTime, 0,0));
	}
	else if (topMoveLeft == false)
	{
		GetGameObject()->SetPostion(GetGameObject()->GetPosition() + glm::vec3(3.8 * deltaTime, 0, 0));
	}

	if (GetGameObject()->GetPosition().y <= -9)
	{
		std::cout << "GAME OVER" << std::endl;
	}
}

void TopEnemyScript::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat2, "Mouse Sensitivity", &_mouseSensitivity.x, 0.01f);
	LABEL_LEFT(ImGui::DragFloat3, "Move Speed       ", &_moveSpeeds.x, 0.01f, 0.01f);
	LABEL_LEFT(ImGui::DragFloat, "Shift Multiplier ", &_shiftMultipler, 0.01f, 1.0f);
}

nlohmann::json TopEnemyScript::ToJson() const
{
	return {
		{ "mouse_sensitivity", _mouseSensitivity },
		{ "move_speed", _moveSpeeds },
		{ "shift_mult", _shiftMultipler }
	};
}

TopEnemyScript::Sptr TopEnemyScript::FromJson(const nlohmann::json & blob)
{
	TopEnemyScript::Sptr result = std::make_shared<TopEnemyScript>();
	result->_mouseSensitivity = JsonGet(blob, "mouse_sensitivity", result->_mouseSensitivity);
	result->_moveSpeeds = JsonGet(blob, "move_speed", result->_moveSpeeds);
	result->_shiftMultipler = JsonGet(blob, "shift_mult", 2.0f);
	return result;
}
