#pragma once
#include<Windows.h>
#ifdef __cplusplus
extern "C"{
#endif
//������ѡ����ѡ����Ŀ����ѡ��
//hwnd: �����ھ��
//title: ���⣬����Ϊ��
//options: ѡ��
//cOptions: �ж��ٸ�ѡ��
//nDefault: Ĭ��ѡ�Ĭ��ѡ����֣�
//pcszCheck: ��ѡ������
//retChosen: ��ѡ���Ƿ�ѡ��
int ChooseList(HWND hwnd, LPCTSTR title, LPCTSTR *options, int cOptions, int nDefault,LPCTSTR pcszCheck,BOOL *retChosen);
#ifdef __cplusplus
}
#endif
