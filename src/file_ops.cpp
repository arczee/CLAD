
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "SfMdata.h"
#include "file_ops.h"

#define MAX_LINE_DATA_NUM	4096
#define MAX_LINE_CHARS		4096*10
#define MAX_NUM_OF_P3D		1000000
#define MAX_NUM_OF_CAMS		100000


char line_buff[MAX_LINE_CHARS];
double datas[MAX_LINE_DATA_NUM];


int readDataFromLine(FILE *fp, double *data, int *nDatas);


/*
��ȡ����ļ��е�����ڲ�
======����ļ���ʽ======
#NumOfInsintrics
ax(px)  ay(px)  ppx(px)  ppy(px)  s  quaternion  transl
...
======����=============
filename	==>��������ļ�
======���=============
nCams		==>���(view)������
Kdata		==>nCams*9,����ڲ�����
initRot		==>nCams*4,����ĳ�ʼR,��Ԫ����ʾ
initTrans	==>nCams*3,����ĳ�ʼt
======����ֵ===========
==0��ʾ��ȡ��ȷ��>0��ʾ����һ�г��ִ���
*/
int readCamsFile(const char *filename, SfMdata &sfmdata)
{
	FILE *fp;
	int nDatas,nRotPara, ret, lineno, camCnt;
	double out_para[12];

	//��ȡ����ļ��͵��ļ�
	if (fopen_s(&fp, filename, "r") != 0) {
		fprintf(stderr, "cannot open file %s, exiting\n", filename);
		system("pause");
		exit(1);
	}

	//���������ļ���ȷ���ж��ٸ����
	int nCams = 0;
	lineno = 0;
	while (1) {
		ret = readDataFromLine(fp, datas, &nDatas);
		if (ret != 1) lineno++;
		else break; //�ļ�β
		if (nDatas == 0) continue;
		if (nDatas != 12 && nDatas !=17) { return lineno - 1; }
		nRotPara = nDatas - 3-5;
		//�����ж���2D��
		(nCams)++;
	}

	sfmdata.initCams(nCams, nRotPara);

	//�ص��ļ���ʼ��
	fpos_t pos = 0;
	fsetpos(fp, &pos);
	//�����������
	camCnt=0;
	while(1) {
		ret = readDataFromLine(fp, datas, &nDatas);
		if(ret!=1) lineno ++;
		else break; //�ļ�β
		if (nDatas == 0) continue;
		if (nDatas != 12 && nDatas!=17) goto err;
		//��������
		if (nDatas == 12) {
			quat2vec(datas, nDatas, out_para); //out_para��Ϊ11��ֵ
			memcpy(datas, out_para, 5); memcpy(&datas[6], &out_para[5], 3), memcpy(&datas[5 + nRotPara], &out_para[8], 3);
			datas[5] = sqrt(1.0 - out_para[5] * out_para[5] - out_para[6] * out_para[6] - out_para[7] * out_para[7]);
		}
		sfmdata.add_camera(camCnt, &datas[0], &datas[5], &datas[5+ nRotPara]);
		camCnt++;
	}

	/*
	double* ptr;
	for (int i = 0; i < camCnt; i++)
	{
		printf("=====Cam %d=====\n", i);
		ptr = (Kdata + i * 5);
		printf("K:     %f,%f,%f,%f,%f\n", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4));
		ptr = (initRot + i *4);
		printf("Rot:   %f,%f,%f,%f\n", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
		ptr = (initRot + i * 3);
		printf("Trans: %f,%f,%f\n", *ptr, *(ptr + 1), *(ptr + 2));
	}
	*/
	return 0;
err:
	return lineno - 1;
}


/*
��ȡ�����ݣ�����3D�㣬�����Ӧ��view�е�ͶӰ��
======����ļ���ʽ======
#NumOfPts3D
X Y Z  nframes  frame0 x0 y0  frame1 x1 y1 ...
...
======����=============
filename	==>�������ļ�
======���=============

======����ֵ===========
==0 ��ʾ��ȡ��ȷ��>0 ��ʾ����һ�г��ִ���
*/
#include <vector>   //���ҷ���
#include <iostream> 
using namespace std;
int readPtsFile(const char *filename, SfMdata &sfmdata)
{
	int nPts3D, nPts2D, nCams;
	int viewid, p2dCnt, p3dCnt;
	int nDatas, ret, lineno;
	FILE *fp;

	nCams=sfmdata.nCams();
	if (nCams <= 0) return -1;

	//��ȡ����ļ��͵��ļ�
	if (fopen_s(&fp, filename, "r") != 0) {
		fprintf(stderr, "cannot open file %s, exiting\n", filename);
		system("pause");
		exit(1);
	}

	//���������ļ���ȷ���ж��ٸ�3D���2D��, ȷ��ÿ�����������3D���������ȷ��ÿ��3D����������������
	nPts2D = 0;
	nPts3D = 0;
	lineno = 0;
	int nPOC = 0, nCOP = 0;
	int *posPoC = new int[nCams+1]; memset(posPoC, 0, nCams * sizeof(int));
	int *posCoP = new int[MAX_NUM_OF_P3D]; memset(posCoP, 0, MAX_NUM_OF_P3D * sizeof(int));
	while (1) {
		ret = readDataFromLine(fp, datas, &nDatas);
		if (ret != 1) lineno++;
		else break; //�ļ�β
		if (nDatas == 0) continue;
		if ((nDatas - 4) % 3 != 0) goto err;
		if (datas[3] != (nDatas - 4) / 3) goto err;
		//�ж��ٸ�2D��
		nPts2D = nPts2D + (nDatas - 4) / 3;
		for (int pos = 4; pos < nDatas; pos = pos + 3) {
			//camera id, x,y
			viewid = (int)datas[pos];
			if (viewid < 0 && viewid >= nCams) goto err;
			//����nP3dOfCams��nCamsOfP3d
			posPoC[viewid]++;
			posCoP[nPts3D]++;
		}
		nPts3D++;
	}
	fpos_t fpos = 0;
	fsetpos(fp, &fpos);

	/*
	cout << "The amount of 3D points for each view:" << endl;
	for (int i = 0; i < nCams; i++)
		cout << posPoC[i] << " ";
	cout << endl;

	cout << "The amount of views for each 3D point:" << endl;
	for (int i = 0; i < nPts3D; i++)
		cout << posCoP[i] << " ";
	cout << endl;
	*/

	//ȷ��ÿ�����������3D��������p3dIdxOfCam�е�λ��, �Լ�POC���ܴ�С
	int cnt=0;
	int tt, i;
	for (i = 0; i < nCams; i++)	{
		nPOC += posPoC[i];
		if (posPoC[i] == 0) posPoC[i] = -1;
		else {
			tt = posPoC[i];
			posPoC[i] = cnt;
			cnt = tt + cnt;
		}
	}
	posPoC[i] = cnt;
	//ȷ��ÿ��3D����������������camIdxOfP3d�е�λ�ã��Լ�COP���ܴ�С
	cnt = 0;
	for (i = 0; i < nPts3D; i++) {
		nCOP += posCoP[i];
		if (posCoP[i] == 0) posCoP[i] = -1;
		else {
			tt = posCoP[i];
			posCoP[i] = cnt;
			cnt = tt + cnt;
		}
	}
	posCoP[i] = cnt;
	/*
	cout << "The amount of 3D points for each view:" << endl;
	for (int i = 0; i < nCams; i++)
		cout << posPoC[i] << " ";
	cout << endl;

	cout << "The amount of views for each 3D point:" << endl;
	for (int i = 0; i < nPts3D; i++)
		cout << posCoP[i] << " ";
	cout << endl;
	*/

	//Ϊ������ݷ���ռ�
	sfmdata.init(nPts3D, nPts2D, nPOC,nCOP, posPoC,posCoP);
	//sfmdata.display_idxs();

	//�����������
	p3dCnt = 0;
	p2dCnt = 0; //��ǰ��ȡ��2D�����
	while (1) {
		ret = readDataFromLine(fp, datas, &nDatas);
		if (ret != 1) lineno++;
		else break; //�ļ�β
		if (nDatas == 0) continue;
		if ((nDatas-4)%3 != 0) goto err;
		if (datas[3] != (nDatas - 4) / 3) goto err;
		//=====��������
		//��ȡ3D�㣬�洢��pts3D��
		sfmdata.add_p3d(p3dCnt, (float)datas[0],(float)datas[1],(float)datas[2]);
		//��ȡ��Ӧ��2D��
		int p2dCntOfLine = 0;
		for (int pos = 4; pos < nDatas; pos = pos + 3) {
			//camera id, x,y
			viewid = (int)datas[pos];
			//x = datas[pos++]; y = datas[pos++];
			if (viewid < 0 && viewid >= nCams) goto err;
			//�洢��pts2D��
			sfmdata.add_p2d(p2dCnt, (float)datas[pos + 1], (float)datas[pos + 2]);
			//��������
			sfmdata.set_iidx(p2dCnt, p3dCnt);
			sfmdata.set_jidx(p2dCnt, viewid);
			sfmdata.registerDijPos(p3dCnt, viewid, p2dCnt);
			sfmdata.registerPOCandCOP(p3dCnt, viewid, p2dCnt);
			p2dCnt++;
			p2dCntOfLine++;
		}
		p3dCnt++;
	}

	sfmdata.finishRead();
	//sfmdata.display_idxs();
	//sfmdata.display_pts();
	delete[]posPoC; delete[]posCoP;
	return 0;
err:
	delete[]posPoC; delete[]posCoP;
	return lineno-1;
}

/*
���ļ��ĵ�ǰ�ж�ȡnDatas������, ������'\t' ','��ո�ָ�
======����==========
fp		==>�ļ�������
data	==>�ṩ��MAX_LINE_DATA_NUM�����ڴ�����ݵĿռ�
======���==========

======����ֵ========
==1 ����Ϊ�ļ�β�� ==0 ��ʾ������ ==2
*/
int readDataFromLine(FILE *fp, double *data, int *nDatas)
{
	char *ret;
	int nChars, pos, start;	 
	double val;

	*nDatas = 0;
	start = 0; pos = 0;
	//�����ж���line_buff
	ret=fgets(line_buff, MAX_LINE_CHARS - 1, fp);
	if (ret == NULL) return 1;
	nChars=(int)strlen(line_buff); //����'\n'
	while (pos < nChars)
	{
		//��������ķָ��
		while ( (line_buff[pos] == '\t' || line_buff[pos] == ',' || line_buff[pos] == ' ') && line_buff[pos] != '\n')
			pos++;
		if (line_buff[pos] == '\n' || line_buff[pos] == '#') break;

		//pos��ʼ���ַ�������һ������, ���н���
		sscanf_s(&line_buff[pos], "%lf", &val);
		data[(*nDatas)++] = val;

		//�������ݣ�ֱ�������ָ���
		while ( line_buff[pos] != '\t' && line_buff[pos] != ',' && line_buff[pos] != ' ' && line_buff[pos] != '\n')
			pos++;
	}

	return 0;
}


/*
���룺
inp		-->�����ڲ�K(5,��ѡ)������(5,��ѡ),��ת��4����Ԫ������λ�ƣ�3��
nin		-->���������ά��
�����
outp	-->�����ڲ�K(5,��ѡ),����(5,��ѡ),��ת��3����Ԫ�����������֣�,λ�ƣ�3��
nout	-->���������ά��
*/
void quat2vec(double *inp, int nin, double *outp)
{
	double mag, sg;
	register int i;

	/* intrinsics & distortion */
	if (nin>7) // are they present?
		for (i = 0; i<nin - 7; ++i)
			outp[i] = inp[i];
	else
		i = 0;

	/* rotation */
	/* normalize and ensure that the quaternion's scalar component is non-negative;
	* if not, negate the quaternion since two quaternions q and -q represent the
	* same rotation
	*/
	mag = sqrt(inp[i] * inp[i] + inp[i + 1] * inp[i + 1] + inp[i + 2] * inp[i + 2] + inp[i + 3] * inp[i + 3]);
	sg = (inp[i] >= 0.0) ? 1.0 : -1.0;
	mag = sg / mag;
	outp[i] = inp[i + 1] * mag;
	outp[i + 1] = inp[i + 2] * mag;
	outp[i + 2] = inp[i + 3] * mag;
	i += 3;

	/* translation*/
	for (; i<11; ++i)
		outp[i] = inp[i + 1];
}