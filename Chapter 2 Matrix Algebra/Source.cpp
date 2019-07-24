#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	return os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";
}

ostream& XM_CALLCONV operator << (ostream& os, FXMMATRIX m)
{
	for (int i = 0; i < 4; ++i)
	{
		os << XMVectorGetX(m.r[i]) << "\t";
		os << XMVectorGetY(m.r[i]) << "\t";
		os << XMVectorGetZ(m.r[i]) << "\t";
		os << XMVectorGetW(m.r[i]) << endl;
	}

	return os;
}

int main()
{
	// Check support for SSE2
	if (!XMVerifyCPUSupport())
	{
		cout << "DirectX math not supported :C" << endl;

		return 0;
	}

	XMMATRIX A(
		1.f, 0.f, 0.f, 0.f,
		0.f, 2.f, 0.f, 0.f,
		0.f, 0.f, 4.f, 0.f,
		1.f, 2.f, 3.f, 1.f
	);

	XMMATRIX B = XMMatrixIdentity();

	XMMATRIX C = A * B;

	XMMATRIX D = XMMatrixTranspose(A);

	XMVECTOR det = XMMatrixDeterminant(A);

	XMMATRIX E = XMMatrixInverse(&det, A);

	XMMATRIX F = A * E;

	cout << "A = " << endl << A << endl;
	cout << "B = " << endl << B << endl;
	cout << "C = A * B = " << endl << C << endl;
	cout << "D = transpose(A) = " << endl << D << endl;
	cout << "det = det(A) = " << det << endl << endl;
	cout << "E = inverse(A) = " << endl << E << endl;
	cout << "F = A * E = " << endl << F << endl;

	return 0;
}
