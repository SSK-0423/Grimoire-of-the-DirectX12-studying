#include"BasicType.hlsli"

BasicType BasicVS(float4 pos : POSITION , float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT, uint instNo : SV_InstanceID) {
    float w = float(weight) / 100.f; //�E�F�C�g��0�`100�����̂ŁA���`��Ԃň�����悤��100�Ŋ���
	BasicType output;//�s�N�Z���V�F�[�_�֓n���l
	//�n�����{�[���ԍ���2��
    matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.f - w);
    pos = mul(bm,pos);	
	pos = mul(world, pos);
    if (instNo == 1)
    {
		//�V���h�E�s�����Z
        pos = mul(shadow, pos);
    }
	
	output.svpos = mul(mul(proj,view),pos);//�V�F�[�_�ł͗�D��Ȃ̂Œ���
    //output.svpos = mul(lightCamera, pos);
	//�V���h�E�}�b�s���O���ł��Ȃ���������
	//���C�g���猩���[�x�l�ƃ��C�g���猩�����W���r����(�v�m�F)
	//������view�A�܂萳�ʃJ��������̎��_�ɐ؂�ւ��Ă��܂��ƂQ�̃J�����ϊ����������Ă��܂����ƂɂȂ�
	//���C�g���猩�����W�𐳊m�Ɍv�Z�ł��Ȃ��Ȃ�
	//output.pos = mul(view, pos);	
    output.pos = pos;
	output.tpos = mul(lightCamera, output.pos);
	normal.w = 0;//�����d�v(���s�ړ������𖳌��ɂ���)
	output.normal = mul(world,normal);//�@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - mul(view,eye));//�����x�N�g��
    output.instNo = instNo;
	
	return output;
}
