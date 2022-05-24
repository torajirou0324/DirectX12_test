#include "SceneManager.h"
#include "PlayScene.h"
#include "CommonData.h"

SceneManager::SceneManager()
	: m_Scene(new PlayScene())
{
}

SceneManager::~SceneManager()
{
	if(m_Scene != nullptr)delete m_Scene;
	//delete m_CommonData;
}

void SceneManager::Update()
{
	BaseScene* p = m_Scene->Update();
	if (p != m_Scene)
	{
		delete m_Scene;
		m_Scene = p;
	}
}

void SceneManager::Draw() const
{
	m_Scene->Draw();
}
