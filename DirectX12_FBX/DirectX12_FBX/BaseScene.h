#pragma once

class CommonData;
class BaseScene
{
protected:
	// �R���X�g���N�^
	BaseScene(){}

public:
	// �R�s�[�֎~
	BaseScene(const BaseScene&) = delete;
	// �R�s�[�֎~
	BaseScene& operator=(const BaseScene&) = delete;

	// ���z�֐��������f�X�g���N�^
	virtual ~BaseScene() = default;

protected:
	// ���L�f�[�^
	// CommonData* const m_CommonData;

public:
	virtual BaseScene* Update() = 0;
	virtual void Draw() const = 0;

protected:
	template<typename T> BaseScene* makeScene();
};
