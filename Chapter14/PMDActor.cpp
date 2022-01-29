#include "PMDActor.h"
#include"PMDRenderer.h"
#include"Dx12Wrapper.h"
#include <d3dx12.h>
#include <mmsystem.h>
#include <algorithm>

#pragma comment(lib,"winmm.lib")

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace {
	///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	///@param path �Ώۂ̃p�X������
	///@param splitter ��؂蕶��
	///@return �����O��̕�����y�A
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}
	///�t�@�C��������g���q���擾����
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}
	///���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	///@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	///@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	///@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//�t�@�C���̃t�H���_��؂��\��/�̓��ނ��g�p�����\��������
		//�Ƃ�����������\��/�𓾂���΂����̂ŁA�o����rfind���Ƃ��r����
		//int�^�ɑ�����Ă���̂͌�����Ȃ������ꍇ��rfind��epos(-1��0xffffffff)��Ԃ�����
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}
}

void*
PMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

float PMDActor::GetYFromXOnBazier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y) return x;

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	//t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;	//t^2�̌W��
	const float k2 = 3 * a.x;

	//�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	//t���ߎ��ŋ��߂�
	for (int i = 0; i < n; i++)
	{
		//f(t)�����߂�
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		//�������ʂ�0�ɋ߂�(�덷�͈͓̔�)�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon) break;

		t -= ft / 2; //����
	}

	//���߂���t�͂��łɋ��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

PMDActor::PMDActor(const char* filepath, PMDRenderer& renderer) :
	_renderer(renderer),
	_dx12(renderer._dx12),
	_angle(0.0f)
{
	_transform.world = XMMatrixIdentity();
	LoadPMDFile(filepath);
	LoadMotionData("motion/�}�t�e�B�[�_���X.vmd");
	CreateTransformView();
	CreateMaterialData();
	CreateMaterialAndTextureView();
}

PMDActor::~PMDActor()
{
}

HRESULT
PMDActor::LoadPMDFile(const char* path) {
	//PMD�w�b�_�\����
	struct PMDHeader {
		float version; //��F00 00 80 3F == 1.00
		char model_name[20];//���f����
		char comment[256];//���f���R�����g
	};
	char signature[3];
	PMDHeader pmdheader = {};

	string strModelPath = path;
	FILE* fp;

	fopen_s(&fp, strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;//���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);


#pragma pack(1)//��������1�o�C�g�p�b�L���O�c�A���C�����g�͔������Ȃ�
	//PMD�}�e���A���\����
	struct PMDMaterial {
		XMFLOAT3 diffuse; //�f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y��
		float specularity;//�X�y�L�����̋���(��Z�l)
		XMFLOAT3 specular; //�X�y�L�����F
		XMFLOAT3 ambient; //�A���r�G���g�F
		unsigned char toonIdx; //�g�D�[���ԍ�(��q)
		unsigned char edgeFlg;//�}�e���A�����̗֊s���t���O
		//2�o�C�g�̃p�f�B���O�������I�I
		unsigned int indicesNum; //���̃}�e���A�������蓖����C���f�b�N�X��
		char texFilePath[20]; //�e�N�X�`���t�@�C����(�v���X�A���t�@�c��q)
	};//70�o�C�g�̂͂��c�ł��p�f�B���O���������邽��72�o�C�g
#pragma pack()//1�o�C�g�p�b�L���O����

	constexpr unsigned int pmdvertex_size = 38;//���_1������̃T�C�Y
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);//�o�b�t�@�m��
	fread(vertices.data(), vertices.size(), 1, fp);//��C�ɓǂݍ���

	fread(&indicesNum, sizeof(indicesNum), 1, fp);//

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));

	//UPLOAD(�m�ۂ͉\)
	auto result = _dx12.Device()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_vb.ReleaseAndGetAddressOf()));

	unsigned char* vertMap = nullptr;
	result = _vb->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	_vb->Unmap(0, nullptr);


	_vbView.BufferLocation = _vb->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	_vbView.SizeInBytes = vertices.size();//�S�o�C�g��
	_vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);//��C�ɓǂݍ���


	auto resDescBuf = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));


	//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
	//OK���Ǝv���܂��B
	result = _dx12.Device()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDescBuf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_ib.ReleaseAndGetAddressOf()));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	_ib->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	_ib->Unmap(0, nullptr);


	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	_ibView.BufferLocation = _ib->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	_materials.resize(materialNum);
	_textureResources.resize(materialNum);
	_sphResources.resize(materialNum);
	_spaResources.resize(materialNum);
	_toonResources.resize(materialNum);

	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
	//�R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		_materials[i].indicesNum = pmdMaterials[i].indicesNum;
		_materials[i].material.diffuse = pmdMaterials[i].diffuse;
		_materials[i].material.alpha = pmdMaterials[i].alpha;
		_materials[i].material.specular = pmdMaterials[i].specular;
		_materials[i].material.specularity = pmdMaterials[i].specularity;
		_materials[i].material.ambient = pmdMaterials[i].ambient;
		_materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i) {
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFilePath[32];
		sprintf_s(toonFilePath, "toon/toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		_toonResources[i] = _dx12.GetTextureByPath(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			_textureResources[i] = nullptr;
			continue;
		}

		string texFileName = pmdMaterials[i].texFilePath;
		string sphFileName = "";
		string spaFileName = "";
		if (count(texFileName.begin(), texFileName.end(), '*') > 0) {//�X�v���b�^������
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph") {
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else {
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					spaFileName = namepair.second;
				}
			}
		}
		else {
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph") {
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa") {
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else {
				texFileName = pmdMaterials[i].texFilePath;
			}
		}
		//���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
		if (texFileName != "") {
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
			_textureResources[i] = _dx12.GetTextureByPath(texFilePath.c_str());
		}
		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			_sphResources[i] = _dx12.GetTextureByPath(sphFilePath.c_str());
		}
		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			_spaResources[i] = _dx12.GetTextureByPath(spaFilePath.c_str());
		}
	}

	//�{�[���֘A�̏���
	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);

	//�p�f�B���O�ŃT�C�Y���ς�����Ⴄ�ƃ{�[���f�[�^�ǂݍ��݂����܂������Ȃ��̂ŕK�{
#pragma pack(1)
	//�Ǎ��p�{�[���\����
	struct PMDBone
	{
		char boneName[20]; //�{�[����
		unsigned short parentNo;    //�e�̃{�[���ԍ�
		unsigned short nextNo; //��[�̃{�[���ԍ�
		unsigned char type; //�{�[����� ��]��
		unsigned short ikBoneNo;    //IK�{�[���ԍ�
		DirectX::XMFLOAT3 pos;   //�{�[���̊�_���W
	};
#pragma pack()
	std::vector<PMDBone> pmdBones(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);
	fclose(fp);

	//�C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z�̂��߂Ɍ�Ŏg��
	std::vector<std::string> boneNames(pmdBones.size());
	//�{�[���m�[�h�}�b�v�����
	for (int idx = 0; idx < pmdBones.size(); idx++)
	{
		PMDBone& pb = pmdBones[idx];
		boneNames[idx] = pb.boneName;
		BoneNode& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.pos;
	}
	//�e�q�֌W���\�z����
	for (auto& pb : pmdBones)
	{
		//�e�C���f�b�N�X���`�F�b�N(���蓾�Ȃ��ԍ��Ȃ��΂�)
		//�{�[���T�C�Y�ȏぁ���肦�Ȃ��ԍ����e�����Ȃ������[�g�m�[�h�H
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}
		auto parentName = boneNames[pb.parentNo];
		_boneNodeTable[parentName].children.emplace_back(&_boneNodeTable[pb.boneName]);
	}
	_boneMatrices.resize(pmdBones.size());

	//�{�[����S�ď���������
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

}

//���[�V�����f�[�^(.vmd)�ǂݍ���
void PMDActor::LoadMotionData(const char* path)
{
	FILE* fp = nullptr;
	fopen_s(&fp, path, "rb");

	//�t�@�C���ǂݍ��ݎ��s
	if (fp == nullptr)
	{
		assert(0);
		return;
	}
	//�w�b�_�[50�o�C�g���΂�
	fseek(fp, 50, SEEK_SET);

	unsigned int motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);

#pragma pack(1)
	struct VMDMotion
	{
		char boneName[15];	//�{�[����
		unsigned int frameNo;	//�t���[���ԍ�
		DirectX::XMFLOAT3 location;	//�ʒu
		DirectX::XMFLOAT4 quaternion;	//�N�H�[�^�j�I��
		unsigned char bezier[64];	//[4][4][4]�x�W�F��ԃp�����[�^
	};
#pragma pack()
	std::vector<VMDMotion> vmdMotionData(motionDataNum);

	//size_t withoutBoneNameSize = sizeof(vmdMotionData[0].frameNo)
	//	+ sizeof(vmdMotionData[0].location)
	//	+ sizeof(vmdMotionData[0].quaternion)
	//	+ sizeof(vmdMotionData[0].bazier);

	for (auto& motion : vmdMotionData)
	{
		//�{�[�����̃T�C�Y��16�o�C�g�Ȃ��߁A1�o�C�g�̃p�f�B���O�������Ă��܂�
		//�{�[��������x�W�F�܂ňꊇ�œǂݍ��ނƃp�f�B���O�̉e���ł���Ă��܂����߁A
		//�{�[�����Ƃ���ȊO�ɕ����ēǂݍ���
		//fread(motion.boneName, sizeof(motion.boneName), 1, fp);	//�{�[����
		////�{�[�����ȊO
		//fread(&motion.frameNo, withoutBoneNameSize, 1, fp);
		fread(&motion, sizeof(VMDMotion), 1, fp);

		//�ŏI�t���[�����L�^
		_duration = std::max<unsigned int>(_duration, motion.frameNo);
	}
	//VMD�̃��[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
	for (auto& motion : vmdMotionData)
	{
		_motionData[motion.boneName].emplace_back(
			KeyFrame(
				motion.frameNo, XMLoadFloat4(&motion.quaternion),
				XMFLOAT2((float)motion.bezier[3] / 127.f,(float)motion.bezier[7] / 127.f),
				XMFLOAT2((float)motion.bezier[11] / 127.f,(float)motion.bezier[15] / 127.f)
			));
	}

	//�L�[�t���[�����Ƀ\�[�g
	for (auto& bonemotion : _motionData)
	{
		std::sort(bonemotion.second.begin(), bonemotion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval)
			{return lval.frameNo <= rval.frameNo; }
		);
	}
	fclose(fp);
}

void PMDActor::MotionUpdate()
{
	if (!isStart) return;

	DWORD elapsedTime = timeGetTime() - _startTime;
	//30fps�Ȃ̂ŁA30 * �o�ߎ���(�b)�Ōo�߃t���[�������Z�o�ł���
	unsigned int frameNo = 30 * (static_cast<float>(elapsedTime) / 1000.f);

	if (frameNo > _duration)
	{
		_startTime = timeGetTime();
		frameNo = 0;
	}

	//�s����N���A
	//(�N���A���Ă��Ȃ��ƑO�t���[���̃|�[�Y���d�ˊ|������ă��f��������)
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//���[�V�����f�[�^�X�V
	for (auto& bonemotion : _motionData)
	{
		//���[�V�����f�[�^�Ɋ܂܂�Ă���{�[�������݂��邩
		auto itBoneNode = _boneNodeTable.find(bonemotion.first);
		if (itBoneNode == _boneNodeTable.end()) continue;

		BoneNode node = _boneNodeTable[bonemotion.first];

		//���v������̂�T��
		std::vector<KeyFrame> motions = bonemotion.second;
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const KeyFrame& motion)//�����_��
			{
				return motion.frameNo <= frameNo;
			}
		);
		//���v������̂��Ȃ���Ώ������΂�
		if (rit == motions.rend()) continue;

		XMMATRIX rotation;
		//rit�F�O�̃A�j���[�V����(���݂̃|�[�Y)
		//it�F���̃A�j���[�V����(���̃|�[�Y)
		auto it = rit.base();
		
		if (it != motions.end())
		{
			float t = static_cast<float>(frameNo - rit->frameNo)
				/ static_cast<float>(it->frameNo - rit->frameNo);

			t = GetYFromXOnBazier(t, it->p1, it->p2, 12);

			//���ʐ��`���
			rotation = XMMatrixRotationQuaternion(
				XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
			//���`���
			//rotation = XMMatrixRotationQuaternion(rit->quaternion)
			//	* (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion)
			//	* t;
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		XMFLOAT3 pos = node.startPos;
		XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		_boneMatrices[node.boneIdx] = mat;
	}

	//���̓��������Ȃ獶�r����ł��������A���[�g�m�[�h����̍ċA���ƑS�m�[�h�̃{�[���s��ϊ����ꊇ�œK�p�ł���
	RecursiveMatrixMultiply(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());

	//_mappedMatrices[1]����{�[���s����R�s�[����
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);
}

HRESULT
PMDActor::CreateTransformView() {
	//GPU�o�b�t�@�쐬
	//���[���h�s��{�{�[���s���1�̃o�b�t�@�\�ň�C�ɓn��
	auto buffSize = sizeof(XMMATRIX) * (1 + _boneMatrices.size());
	buffSize = (buffSize + 0xff) & ~0xff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	auto result = _dx12.Device()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_transformBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�ƃR�s�[
	result = _transformBuff->Map(0, nullptr, (void**)&_mappedMatrices);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	_mappedMatrices[0] = _transform.world;

	//auto armNode = _boneNodeTable["���r"];
	//auto& armPos = armNode.startPos;
	//auto armMat = XMMatrixTranslation(-armPos.x, -armPos.y, -armPos.z)
	//	* XMMatrixRotationZ(XM_PIDIV2)
	//	* XMMatrixTranslation(armPos.x, armPos.y, armPos.z);

	//auto elbowNode = _boneNodeTable["���Ђ�"];
	//auto& elbowPos = elbowNode.startPos;
	//auto elbowMat = XMMatrixTranslation(-elbowPos.x, -elbowPos.y, -elbowPos.z)
	//	* XMMatrixRotationZ(XM_PIDIV2)
	//	* XMMatrixTranslation(elbowPos.x, elbowPos.y, elbowPos.z);

	////�{�[�����Q���邽�߁A��ɑ�����Ă����Ĉꊇ�ōċA�������Ȃ��ƕϊ������������Ȃ�
	////���炭�A���r�E���Ђ��ŋ��ʂ̎q������A���x���Ă΂�邱�Ƃŉߏ�ɕϊ����Ă��܂��Ă���
	//_boneMatrices[armNode.boneIdx] = armMat;
	//_boneMatrices[elbowNode.boneIdx] = elbowMat;

	////���[�V�����f�[�^�̃N�H�[�^�j�I�������]�K�p
	//for (auto& bonemotion : _motionData)
	//{
	//	auto node = _boneNodeTable[bonemotion.first];
	//	auto& pos = node.startPos;
	//	auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
	//		* XMMatrixRotationQuaternion(bonemotion.second[0].quaternion)
	//		* XMMatrixTranslation(pos.x, pos.y, pos.z);
	//	_boneMatrices[node.boneIdx] = mat;
	//}

	//���̓��������Ȃ獶�r����ł��������A���[�g�m�[�h����̍ċA���ƑS�m�[�h�̃{�[���s��ϊ����ꊇ�œK�p�ł���
	RecursiveMatrixMultiply(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());

	//_mappedMatrices[1]����{�[���s����R�s�[����
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);

	//�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1;//�Ƃ肠�������[���h�ЂƂ�
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dx12.Device()->CreateDescriptorHeap(&transformDescHeapDesc, IID_PPV_ARGS(_transformHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _transformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	_dx12.Device()->CreateConstantBufferView(&cbvDesc, _transformHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

HRESULT
PMDActor::CreateMaterialData() {
	//�}�e���A���o�b�t�@���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * _materials.size());

	auto result = _dx12.Device()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//�ܑ̂Ȃ����ǎd���Ȃ��ł���
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = _materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	for (auto& m : _materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;//�f�[�^�R�s�[
		mapMaterial += materialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
	}
	_materialBuff->Unmap(0, nullptr);

	return S_OK;

}


HRESULT
PMDActor::CreateMaterialAndTextureView() {
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.NumDescriptors = _materials.size() * 5;//�}�e���A�����Ԃ�(�萔1�A�e�N�X�`��3��)
	materialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;

	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	auto result = _dx12.Device()->CreateDescriptorHeap(&materialDescHeapDesc, IID_PPV_ARGS(_materialHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = _materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//��q
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(_materialHeap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = _dx12.Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < _materials.size(); ++i) {
		//�}�e���A���Œ�o�b�t�@�r���[
		_dx12.Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += materialBuffSize;
		if (_textureResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _textureResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_textureResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.Offset(incSize);

		if (_sphResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _sphResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_sphResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (_spaResources[i] == nullptr) {
			srvDesc.Format = _renderer._blackTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _spaResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_spaResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;


		if (_toonResources[i] == nullptr) {
			srvDesc.Format = _renderer._gradTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_toonResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;
	}
}

//�{�[���s��̍ċA
//�e�m�[�h�̕ϊ����q�m�[�h�ɓ`���邽�߂̊֐�
void PMDActor::RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	_boneMatrices[node->boneIdx] *= mat;
	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, _boneMatrices[node->boneIdx]);
	}
}

void
PMDActor::Update() {
	//_angle += 0.03f;
	//_mappedTransform->world = XMMatrixRotationY(_angle);
	_mappedMatrices[0] = XMMatrixRotationY(_angle);
	MotionUpdate();
}
void
PMDActor::Draw(bool isShadow) {
	_dx12.CommandList()->IASetVertexBuffers(0, 1, &_vbView);
	_dx12.CommandList()->IASetIndexBuffer(&_ibView);

	ID3D12DescriptorHeap* transheaps[] = { _transformHeap.Get() };
	_dx12.CommandList()->SetDescriptorHeaps(1, transheaps);
	_dx12.CommandList()->SetGraphicsRootDescriptorTable(1, _transformHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* mdh[] = { _materialHeap.Get() };
	//�}�e���A��
	_dx12.CommandList()->SetDescriptorHeaps(1, mdh);

	auto materialH = _materialHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	if (isShadow) {
		_dx12.CommandList()->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);
	}
	else
	{
		auto cbvsrvIncSize = _dx12.Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		for (auto& m : _materials) {
			_dx12.CommandList()->SetGraphicsRootDescriptorTable(2, materialH);
			_dx12.CommandList()->DrawIndexedInstanced(m.indicesNum, 2, idxOffset, 0, 0);
			materialH.ptr += cbvsrvIncSize;
			idxOffset += m.indicesNum;
		}
	}
}

void PMDActor::PlayAnimation()
{
	_startTime = timeGetTime();
	isStart = true;
}
