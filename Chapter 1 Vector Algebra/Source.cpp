#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
	XMFLOAT3 dest;
	XMStoreFloat3(&dest, v);

	return os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ")";
}

int main()
{
	cout.setf(ios_base::boolalpha);

	// Check support for SSE2
	if (!XMVerifyCPUSupport())
	{
		cout << "DirectX math not supported :C" << endl;

		return 0;
	}

	XMVECTOR n = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	XMVECTOR u = XMVectorSet(1.f, 2.f, 3.f, 0.f);
	XMVECTOR v = XMVectorSet(-2.f, 1.f, -3.f, 0.f);
	XMVECTOR w = XMVectorSet(0.707f, 0.707f, 0.f, 0.f);

	XMVECTOR a = u + v;

	XMVECTOR b = u - v;

	XMVECTOR c = 10.f * u;

	XMVECTOR L = XMVector3Length(u);

	XMVECTOR d = XMVector3Normalize(u);

	XMVECTOR s = XMVector3Dot(u, v);

	XMVECTOR e = XMVector3Cross(u, v);

	XMVECTOR projW, perpW;
	XMVector3ComponentsFromNormal(&projW, &perpW, w, n);

	// Does projW + perpW == w?
	bool equal = XMVector3Equal(projW + perpW, w);

	// The angle between projW and perpW should be 90 degrees
	XMVECTOR angleVec = XMVector3AngleBetweenVectors(projW, perpW);

	float angleRadians = XMVectorGetX(angleVec);
	float angleDegrees = XMConvertToDegrees(angleRadians);

	cout << "u = " << u << endl;
	cout << "v = " << v << endl;
	cout << "w = " << w << endl;
	cout << "n = " << n << endl;

	cout << "a = u + v = " << a << endl;
	cout << "b = u - v = " << b << endl;
	cout << "c = 10 * u = " << c << endl;
	cout << "d = u / ||u|| = " << d << endl;
	cout << "e = u x v = " << e << endl;
	cout << "L = ||u|| = " << L << endl;
	cout << "s = u . v = " << s << endl;
	cout << "projW = " << projW << endl;
	cout << "perpW = " << perpW << endl;
	cout << "projW + perpW == w = " << equal << endl;
	cout << "angle = " << angleDegrees << endl;

	return 0;
}
