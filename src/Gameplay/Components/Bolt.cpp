#include "Gameplay/Components/Bolt.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/SimpleCameraControl.h"

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"

void Bolt::Awake()
{
}

void Bolt::RenderImGui()
{
}

nlohmann::json Bolt::ToJson() const
{
	return {

	};
}

Bolt::Bolt() :
	IComponent(),
	_impulse(10.0f)
{
}

Bolt::~Bolt() = default;

Bolt::Sptr Bolt::FromJson(const nlohmann::json & blob)
{
	Bolt::Sptr result = std::make_shared<Bolt>();

	return result;
}
extern float playerX, playerY;
extern float boltX, boltY, boltZ;
extern bool boltOut;
float fireTime = 0;

void Bolt::Update(float deltaTime)
{
	boltX = GetGameObject()->GetPosition().x;
	boltY = GetGameObject()->GetPosition().y;
	boltZ = GetGameObject()->GetPosition().z;

	if (InputEngine::IsKeyDown(GLFW_KEY_SPACE))
	{
		if (fireTime <= 0)
		{
			if (boltOut == false)
			{
				boltOut = true;
				fireTime = 1.5f;
			}
		}
	}

	if (boltOut == false)
	{
		GetGameObject()->SetScale(glm::vec3(0.01f, 0.01f, 0.01f));
		GetGameObject()->SetPostion(glm::vec3(playerX, playerY, 0.6f));
		fireTime = 0;

	}
	else if (boltOut == true)
	{
		glm::vec3 worldMovement = glm::vec3(0, 1, 0);
		worldMovement *= deltaTime * 20;

		GetGameObject()->SetPostion(GetGameObject()->GetPosition() + worldMovement);

		GetGameObject()->SetScale(glm::vec3(0.3f));
	}

	if (fireTime > 0)
	{
		fireTime -= 1 * deltaTime;
	}
	else
	{
		if (boltOut == true)
			boltOut -= 1;

		boltOut = false;
	}
}
