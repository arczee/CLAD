
enum OpType
{
	PLACEHOLDER,
	CONST_VAL,
	EQUAL,
	ADD,
	SUB,			//4
	MUL,
	DIV,
	SIN,			//7
	COS,
	EXP,
	LOGE,			//10
	LOG10,
	LOG2,
	SQRT,
	NEG
};
void compute_func(int op, global double *NodeVals, int idx, int idx1, int idx2);
void compute_diff(global int*NodeOps, global double *NodeVals, global double *NodeDiffs, int idx, int idx1, int idx2);

/*
ǰ�����
*/
__kernel void autodiff(
	const int CUsize,
	const int Multiple,
	const int Ns,					//����(section)����  checked
	const int Nfp,					//�̶�����������  checked
	const int Nn,					//����ڵ������		checked
	const int fwIdxSize,			//forwardIdx����ĳߴ�	checked
	const int Nf,					//��������ĸ���		checked
	const int Ncb,					//�ϳɲ���������		checked
	const int Nadp,					//AD������ǹ̶���������
	__global int *PI,				//����(section)��Ϣ��|AD����ĸ�section���׵�ַ����������params�е���ʼλ��, ���鵥Ԫ�����Ĳ�������|	checked

	__global int *NodeIds,		//�ڵ��ID
	__global int *NodeOps,		//�ڵ�Ĳ�����
	__global int *NodeVarId1s,		//
	__global int *NodeVarId2s,		//
	__global double *NodeVals,		//
	__global double *NodeDiffs,		//

	__global int* fwIdx,		//����������е�����	checked
	__global int* revIdx,		//
	__global int* inputNodeIds,		//����ڵ��IDs	��������������鵥Ԫ�Ĳ����ĺ�һ��, checked
	__global int* funcNodeIds,		//����ڵ��IDs, 			checked
	__global int* combIdxs,			//�ϳ�������ȡ�������ж�Ӧ�����ϳ�������Ϊ�Զ�΢�ֵ�����		checked
	__global double *fixedParams,	//�̶��������������΢�ֽ��
	__global double * params,		//�ṩ���������		checked
	__global double * outDiffs,		//΢�ֽ������� ������Ϊ(nOutputs*nInputs)*nCombIdxs
	__global double * outVals		//ǰ��������� ������ΪnOutputs*nCombIdxs

	//__local int *NodeIds_local,		//�����������
	//__local int *NodeOps_local,		//�����������
	//__local int *NodeVarId1s_local,		//�����������
	//__local int *NodeVarId2s_local,		//�����������
	//__local int *fwIdx_local,
	//__local int *revIdx_local
)
{
	int idx, id, part, Node_base, base, pos, Ntp, adi_pos;
	int n, j;
	int i = get_global_id(0);	//��ǰ������Զ�΢��������
	int Node_i =  (get_group_id(0) % (Multiple*CUsize))*get_local_size(0) + get_local_id(0);
	Node_base = Node_i*Nn;

	//��ȫ�ֵ�Node������Node_local
	/*
	event_t evt = async_work_group_copy((local int*)NodeIds_local, (global int*)NodeIds, Nn, 0);
	wait_group_events(1, &evt);
	evt = async_work_group_copy((local int*)NodeOps_local, (global int*)NodeOps, Nn, 0);
	wait_group_events(1, &evt);
	evt = async_work_group_copy((local int*)NodeVarId1s_local, (global int*)NodeVarId1s, Nn, 0);
	wait_group_events(1, &evt);
	evt = async_work_group_copy((local int*)NodeVarId2s_local, (global int*)NodeVarId2s, Nn, 0);
	wait_group_events(1, &evt);
	evt = async_work_group_copy((local int*)fwIdx_local, (global int*)fwIdx, fwIdxSize, 0);
	wait_group_events(1, &evt);
	evt = async_work_group_copy((local int*)revIdx_local, (global int*)revIdx, Nn, 0);
	wait_group_events(1, &evt);
	*/

	//==========����inputNodeIds�ĳ�ʼֵ
	//1,����̶�����
	for (n = 0; n < Nfp; n++)	{
		 id = inputNodeIds[n];		//�����id
		 idx = fwIdx[id];
		 NodeVals[Node_base+idx] = fixedParams[n];
	}
	//2,�Ӹ���������ȡ��Ӧ����Ԫ�Ĳ���ֵ
	pos = Nfp;
	for (n = 0; n < Ns; n++) {
		base = PI[5 * n + 2]; //����n������Ps�еĵĻ�ַ
		part = combIdxs[Ns*i + n];	//����n��ȡ��idx��Ĳ���
		Ntp = PI[5 * n + 3];		//�÷����鵥Ԫ�Ĳ�������
		for (j = 0; j <Ntp; j++) {
			id = inputNodeIds[pos++];
			idx= fwIdx[id];  //fwIdx_local
			NodeVals[Node_base+idx] = params[base + part*Ntp + j];
		}
	}
	//���㺯��ֵ
	for (id = 0; id < fwIdxSize; id++) { //fwIdxSize
		idx = fwIdx[id];
		if (idx < 0) continue;
		int idx1 = fwIdx[NodeVarId1s[idx]];
		int idx2 = fwIdx[NodeVarId2s[idx]];
		int op = NodeOps[idx];
		compute_func(op, &NodeVals[Node_base], idx,idx1,idx2);
	}
	//�������ֵ���
	for (n = 0; n < Nf;n++) {
		id = funcNodeIds[n];
		idx = fwIdx[id];
		outVals[i*Nf + n] = NodeVals[Node_base + idx];
	}
	//�ֱ����ÿ�����������������ƫ��
	base = i*Nadp*Nf;
	for (int f = 0; f < Nf; f++)
	{
		//������нڵ��ϵ�diffֵ
		for (n = 0; n < Nn; n++)
			NodeDiffs[Node_base+n] = 0;
		//�ֽ��ýڵ��diff��1
		idx = fwIdx[funcNodeIds[f]];
		NodeDiffs[Node_base+idx] = 1;	//Ҫ����ĺ����ڵ�Ϊ������1
		for (n = 0; n < Nn; n++) {
			idx = revIdx[n];
			int idx1 = fwIdx[NodeVarId1s[idx]];	//�ӽڵ���Node�е�����
			int idx2 = fwIdx[NodeVarId2s[idx]];
			compute_diff(NodeOps, &NodeVals[Node_base], &NodeDiffs[Node_base], idx, idx1, idx2);
		}
		//���������outDiffs
		for (int p = 0; p < Ns; p++) {
			int p5 = 5 * p;
			int Ndp = PI[p5 + 3] - PI[p5 + 4];			//NdpΪ�÷�����������ƫ���Ĳ����ĸ���
			pos = base + p*Nf*PI[p5 + 1] + f*Ndp;		//�������ʼλ��  p*Nf*PI[5*p+1]
			adi_pos = PI[5+p];
			for (n = 0; n < Ndp; n++)
			{
				id = inputNodeIds[adi_pos + n]; idx = fwIdx[id];
				outDiffs[pos + n] = NodeDiffs[Node_base+idx];
			}
		}
	}
	//barrier(CLK_LOCAL_MEM_FENCE);
	//barrier(CLK_GLOBAL_MEM_FENCE);
} //end of function


/*
����ָ���ڵ��ֵ
*/
inline void compute_func(int op, global double *NodeVals, int idx,int idx1,int idx2)
{
	switch(op)
	{ 
	case ADD:
		NodeVals[idx] = NodeVals[idx1] + NodeVals[idx2];
		break;
	case SUB:
		NodeVals[idx] = NodeVals[idx1] - NodeVals[idx2];
		break;
	case MUL:
		NodeVals[idx] = NodeVals[idx1] * NodeVals[idx2];
		break;
	case DIV:
		NodeVals[idx] = NodeVals[idx1] / NodeVals[idx2];
		break;
	case SIN:			//
		NodeVals[idx] = sin(NodeVals[idx1]);
		break;
	case COS:
		NodeVals[idx] = cos(NodeVals[idx1]);
		break;
	case EXP:
		NodeVals[idx] = exp(NodeVals[idx1]);
		break;
	case LOGE:	//10
		NodeVals[idx] = log(NodeVals[idx1]);
		break;
	case LOG10:
		NodeVals[idx] = log10(NodeVals[idx1]);
		break;
	case LOG2:
		NodeVals[idx] = log2(NodeVals[idx1]);
		break;
	case SQRT:
		NodeVals[idx] = sqrt(NodeVals[idx1]);
		break;
	case NEG:
		NodeVals[idx] = -NodeVals[idx1];
		break;
	default:
		break;
	}
}

/*
����ָ���ڵ�����ӽڵ��ƫ��
*/
inline void compute_diff(global int*NodeOps, global double *NodeVals, global double *NodeDiffs, int idx, int idx1,int idx2)
{
	switch (NodeOps[idx])
	{
	case ADD:
		NodeDiffs[idx1] += NodeDiffs[idx];
		NodeDiffs[idx2] += NodeDiffs[idx];
		break;
	case SUB:
		NodeDiffs[idx1] += NodeDiffs[idx];
		NodeDiffs[idx2] += -NodeDiffs[idx];
		break;
	case MUL:
		NodeDiffs[idx1] += NodeDiffs[idx] * NodeVals[idx2];
		NodeDiffs[idx2] += NodeDiffs[idx] * NodeVals[idx1];
		break;
	case DIV:
		NodeDiffs[idx1] += NodeDiffs[idx] / NodeVals[idx2];
		NodeDiffs[idx2] += -NodeDiffs[idx]* NodeVals[idx1] /(NodeVals[idx2]* NodeVals[idx2]);
		break;
	case SIN:			//
		NodeDiffs[idx1] += NodeDiffs[idx]*cos(NodeVals[idx1]);
		break;
	case COS:
		NodeDiffs[idx1] += -NodeDiffs[idx]*sin(NodeVals[idx1]);
		break;
	case EXP:
		NodeDiffs[idx1] += NodeDiffs[idx]*exp(NodeVals[idx1]);
		break;
	case LOGE:	//10
		NodeDiffs[idx1] += NodeDiffs[idx]/NodeVals[idx1];
		break;
	case SQRT:
		NodeDiffs[idx1] += NodeDiffs[idx] /(2*sqrt(NodeVals[idx1]));
		break;
	case NEG:
		NodeDiffs[idx1] += -NodeDiffs[idx];
		break;
	}
}
