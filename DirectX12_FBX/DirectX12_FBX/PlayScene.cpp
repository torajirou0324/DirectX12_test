#include "PlayScene.h"
#include "Engine.h"
#include "App.h"
#include <d3dx12.h>
#include "SheredStruct.h"
#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"

using namespace DirectX;

VertexBuffer* vertexBuffer;
ConstantBuffer* constantBuffer[Engine::FRAME_BUFFER_COUNT];
RootSignature* rootSignature;
PipelineState* pipelineState;

PlayScene::PlayScene()
	: rotateZ(0.0f)
{
	
}

PlayScene::~PlayScene()
{
}

bool PlayScene::Init()
{
	Vertex vertices[3] = {};
	vertices[0].Position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertices[0].Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	vertices[1].Position = XMFLOAT3(1.0f, -1.0f, 0.0f);
	vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[2].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	vertices[2].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	auto vertexSize = sizeof(Vertex) * std::size(vertices);
	auto vertexStride = sizeof(Vertex);
	vertexBuffer = new VertexBuffer(vertexSize, vertexStride, vertices);

	if (!vertexBuffer->IsValid())
	{
		printf("���_�o�b�t�@�̐����Ɏ��s\n");
		return false;
	}

	auto eyePos = XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f); // ���_�̈ʒu
	auto targetPos = XMVectorZero(); // ���_����������W
	auto upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // �������\���x�N�g��
	auto fov = XMConvertToRadians(37.5); // ����p
	auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT); // �A�X�y�N�g��

	for (size_t i = 0; i < Engine::FRAME_BUFFER_COUNT; i++)
	{
		constantBuffer[i] = new ConstantBuffer(sizeof(Transform));
		if (!constantBuffer[i]->IsValid())
		{
			printf("�ϊ��s��p�萔�o�b�t�@�̐����Ɏ��s\n");
			return false;
		}

		// �ϊ��s��̓o�^
		auto ptr = constantBuffer[i]->GetPtr<Transform>();
		ptr->World = XMMatrixIdentity();
		ptr->View = XMMatrixLookAtRH(eyePos, targetPos, upward);
		ptr->Proj = XMMatrixPerspectiveFovRH(fov, aspect, 0.3f, 1000.0f);
	}

	rootSignature = new RootSignature();
	if (!rootSignature->IsValid())
	{
		return false; // ���[�g�V�O�l�`���̐����Ɏ��s
	}

	pipelineState = new PipelineState();
	pipelineState->SetInputLayout(Vertex::InputLayout);
	pipelineState->SetRootSignature(rootSignature->Get());
	pipelineState->SetVS(L"../x64/Debug/BasicVertexShader.cso");
	pipelineState->SetPS(L"../x64/Debug/BasicPixelShader.cso");
	pipelineState->Create();
	if (!pipelineState->IsValid())
	{
		return false; // �p�C�v���C���X�e�[�g�̐����Ɏ��s
	}

	printf("�V�[���̏������ɐ���\n");
	return true;
}

BaseScene* PlayScene::Update()
{
	rotateZ += 0.02f;
	auto currentIndex = g_Engine->CurrentBackBufferIndex(); // ���݂̃t���[���ԍ����擾
	auto currentTransform = constantBuffer[currentIndex]->GetPtr<Transform>(); // ���݂̃t���[���ԍ��ɑΉ�����萔�o�b�t�@���擾
	currentTransform->World = XMMatrixRotationZ(rotateZ);
	return this;
}

void PlayScene::Draw() const
{
	auto currentIndex = g_Engine->CurrentBackBufferIndex(); // ���݂̃t���[���ԍ����擾����
	auto commandList = g_Engine->CommandList(); // �R�}���h���X�g
	auto vbView = vertexBuffer->View(); // ���_�o�b�t�@�r���[

	commandList->SetGraphicsRootSignature(rootSignature->Get()); // ���[�g�V�O�l�`�����Z�b�g
	commandList->SetPipelineState(pipelineState->Get()); // �p�C�v���C���X�e�[�g���Z�b�g
	commandList->SetGraphicsRootConstantBufferView(0, constantBuffer[currentIndex]->GetAddress()); // �萔�o�b�t�@���Z�b�g

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // �O�p�`��`�悷��ݒ�ɂ���
	commandList->IASetVertexBuffers(0, 1, &vbView); // ���_�o�b�t�@���X���b�g��0�Ԃ��g����1�����ݒ肷��

	commandList->DrawInstanced(3, 1, 0, 0); // 3�̒��_��`�悷��
}

template<> BaseScene* BaseScene::makeScene<PlayScene>()
{
	return new PlayScene();
}
