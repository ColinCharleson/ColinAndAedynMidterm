#pragma once
#include "IComponent.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class LoseScreen : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<LoseScreen> Sptr;

	LoseScreen();
	virtual ~LoseScreen();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	std::weak_ptr<Gameplay::IComponent> imageLose;
public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(LoseScreen);
	virtual nlohmann::json ToJson() const override;
	static LoseScreen::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _impulse;

	bool _isPressed = false;
};