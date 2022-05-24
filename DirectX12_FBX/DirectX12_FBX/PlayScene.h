#pragma once
#include "BaseScene.h"

class PlayScene : public BaseScene
{
public:
	PlayScene();

	~PlayScene();
	bool Init(); // ‰Šú‰»

	virtual BaseScene* Update() override;
	virtual void Draw() const override;

private:
	float rotateZ;
};