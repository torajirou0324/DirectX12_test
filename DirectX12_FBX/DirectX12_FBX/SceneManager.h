#pragma once

class BaseScene;
class SceneManager
{
public:
	SceneManager();
	~SceneManager();

private:
	BaseScene *m_Scene;
public:
	void Update();
	void Draw() const;
};