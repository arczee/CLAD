#pragma once

#include <vector>
#include <iostream>
#include <memory>


/*
ÿ�������һ����СΪL1_BLOCK_SIZE��һ������, ���k��Ԫ�����Ϊ�㣬��ʾ���Ϊ[k*1000, (k+1)*1000)�����3D���ڸ������û��ͶӰ��
�����Ϊ�㣬����ֵl����ָ������������е�l���飬��ʾ�ÿ��Ӧ��[k*1000, (k+1)*1000)�������ڸ������ͶӰ��3D�㡣
*/

#define NON_EXIST_FLAG		-1

using namespace std;
class SfMdata		//����Ӧ��ֻ�������һ��ʵ����singleton
{
private:
	//============For cameras=========//
	double *Kdata;		//�����insintric parameters
	//double *initRot;	//��ʼ����ת����Ԫ��(4*1)����ת����(9x1)
	double *Poses;		//�����Pose, 3+3����ʼ��ת��+��ʼλ�ơ� 
	bool isQaut;		//ָʾ��ת���Ƿ�ΪQauternion������ΪRodriguez��ʽ

	//============For points=========//
	int L1_BLOCK_SIZE;
	int L2_BLOCK_SIZE;
	int L3_BLOCK_SIZE;
	int nCams_;			//���������
	int nPts2D_;			//2D�������
	int nPts3D_;			//3D�������
	int nPoC_;			//PoCidxs������������
	int nCoP_;			//CoPidxs������������

	double *pts2D;		//nPts2D * 2, 2D������
	double *pts3D;		//nPts3D * 3, 3D������
	int *ji_idx;			//nPts2D * 1, 2D���Ӧ��3D������, ע��2D���ǰ�3D���ڸ�����е�˳�����е�
	//int *jidx;			//nPts2D * 1, 2D���Ӧ���������

	int *PoCidxs;		//��¼ÿ�����������3D������
	int *PoCidxs2;		//��¼ÿ�����������3D���Ӧ��2D�������
	int *posPoC;		//nCams*1, ��¼ÿ�����������3D��������p3dIdxOfCam�е�λ��
	int *CoPidxs;		//��¼ÿ��3D��������������
	int *CoPidxs2;		//��¼ÿ��3D������������Ӧ��2D�������
	int *posCoP;		//nPts3D*1, ��¼ÿ��3D����������������camIdxOfP3d�е�λ��
	int *cntPoC;		//
	int *cntCoP;

	//����ȷ��Dij��λ��
	int *DijL1Blocks;				//nCams* L1_BLOCK_SIZE, Dij�ĵ�һ������, 
	vector<int *> DijL2BlockList;	//Dij�ĵڶ�������
	vector<int *> DijL3BlockList;	//Dij�ĵ���������

	bool initialized;
	bool readFinished;
public:
	~SfMdata();
	SfMdata() {
		initialized = false; readFinished = false;
		nCams_ = 0;
	}

	int initCams(int nCams, int nRotPara);

	int init(int nPts3D, int nPts2D, int nPOC,int nCOP, int*posPoC, int* posCoP);

	void finishRead();

	int registerDijPos(int i, int j, int DijPos);

	int registerPOCandCOP(int i, int j, int idx2D);

	int getDijPos(int i, int j);
	void display_idxs();
	void display_pts(int firstN2D, int firstN3D);
	void display_info();
	void display_cameras(int firstNcams);
	
	void set_iidx(int pos, int i, int j) {
		if (initialized) {
			ji_idx[2 * pos] = i;
			ji_idx[2 * pos + 1] = j;
		}
	}

	int get_iidx(int pos) {
		if (initialized) return ji_idx[2*pos];
		else return -1;
	}

	int get_jidx(int pos) {
		if (initialized) return ji_idx[2*pos+1];
		else return -1;
	}

	void add_p3d(int pos, float X, float Y, float Z) {
		pts3D[pos * 3] = X;
		pts3D[pos * 3 + 1] = Y;
		pts3D[pos * 3 + 2] = Z;
	}

	void add_p2d(int pos, float x, float y) {
		pts2D[pos * 2] = x;
		pts2D[pos * 2 + 1] = y;
	}
	
	double* get_pt(int idx) { return &pts2D[2 * idx]; }

	int nPts3D() { return nPts3D_; }
	int nPts2D() { return nPts2D_; }
	int nCams() { return nCams_; }
	double* pts3D_ptr() { return pts3D; }
	double* pts2D_ptr() { return pts2D; }
	double* poses_ptr() { return Poses; }
	double* K_ptr() { return Kdata; }
	//double* initRot_ptr() { return initRot; }

	int* ji_idx_ptr() { return ji_idx; }
	int* PoCidxs_ptr() { return PoCidxs; }
	int* PoCidxs2_ptr() { return PoCidxs2; }
	int* posPoC_ptr() { return posPoC; }
	int* CoPidxs_ptr() { return CoPidxs; }
	int* CoPidxs2_ptr() { return CoPidxs2; }
	int* posCoP_ptr() { return posCoP;  }
	int nPOC() { return nPoC_; }
	int nCOP() { return nCoP_; }

	int* DijL1Blocks_ptr() { return DijL1Blocks; }
	int DijL1_BLOCK_SIZE() { return L1_BLOCK_SIZE; }
	vector<int*>* DijL2Blocks(){ return &DijL2BlockList; }
	int DijL2_BLOCK_SIZE() { return L2_BLOCK_SIZE; }
	vector<int*>* DijL3Blocks() { return &DijL3BlockList; }
	int DijL3_BLOCK_SIZE() { return L3_BLOCK_SIZE; }

	void test_DijLx();

	//ͨ��BAL�ļ���ʼ������, BAL��ʽ����ΪRodriguez
	bool initWithBAL(string BAL_fileName, bool useQaut);

	//ͨ��SBA��ʽ�ļ���ʼ������, SBA��ʽ����ΪQauternion
	bool initWithSBA(string CamsFile, string PtsFile, bool useQaut);

}; //end of class;