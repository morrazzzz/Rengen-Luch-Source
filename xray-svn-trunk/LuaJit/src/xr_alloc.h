#ifndef _XR_POOL_H
#define _XR_POOL_H

// � �������� x64 ���� � luajit`�� - ��-�� ����, ��� ������� ���������� ��������� ������ �� ������� �������,
// �� ������� �������� ������� �� ����� �������� ������, ��� ��� ��� ��� ������ ��� ������ ������� ����.
// ��� ������� �������� �������� ������� ����� ������(128��) � ������ ���� � ���� ��� ���������� �������� ��������

void XR_INIT();
void* XR_MMAP(unsigned long long size);
void XR_DESTROY(void* p, unsigned long long size);

#endif