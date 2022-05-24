#pragma once

class CommonData;
class BaseScene
{
protected:
	// コンストラクタ
	BaseScene(){}

public:
	// コピー禁止
	BaseScene(const BaseScene&) = delete;
	// コピー禁止
	BaseScene& operator=(const BaseScene&) = delete;

	// 仮想関数化したデストラクタ
	virtual ~BaseScene() = default;

protected:
	// 共有データ
	// CommonData* const m_CommonData;

public:
	virtual BaseScene* Update() = 0;
	virtual void Draw() const = 0;

protected:
	template<typename T> BaseScene* makeScene();
};
