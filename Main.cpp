//include <stdio.h>
#include <math.h>
//#include <conio.h>//
//#include <locale.h>//
//#include <iostream>//output text
#include <Windows.h>//drawing
#include <stdlib.h>// min max
#include <cstdlib>//random
#include <ctime>//timer
#include <mmsystem.h>//playsound
#pragma comment( lib, "winmm" )//playsound

//#include <amp.h>
//#include <amp_math.h>

using namespace std;

double projectionPlaneDist = 1.0;

HDC hCmpDC, hdc;
HBITMAP hBmp;

struct coord {
	double x;
	double y;
	double z;
};
struct coord2d {
	double x;
	double y;
};

enum class objTypes {
	obj_point,
	obj_line,
	obj_triangle,
	obj_quad};

//coord2d center = { 480.0, 240.0 };
coord2d center = { 960.0, 540.0 };

coord vecMinus(coord v, coord v2)  {
	return { v.x - v2.x, v.y - v2.y, v.z - v2.z };
}

coord vecPlus(coord v, coord v2) {
	return { v.x + v2.x, v.y + v2.y, v.z + v2.z };
}

coord vecDivideDoub(coord v, double d) {
	return { v.x / d, v.y / d, v.z / d };
}

double vecLen(coord v) {
	return sqrt(pow(v.x, 2)+ pow(v.y, 2)+ pow(v.z, 2));
}

double vecScalarMul(coord v, coord v2) {
	return (v.x * v2.x + v.y * v2.y + v.z * v2.z);
}

coord vecProduct(coord A, coord B)
{
	coord VP = {};
	VP.x = A.y * B.z - B.y * A.z;
	VP.y = A.z * B.x - B.z * A.x;
	VP.z = A.x * B.y - B.x * A.y;
	return VP;
}

coord lineClip(coord X, coord Y)
{
	/*
	coord A = { 0.0, 0.0, 0.0 };
	coord B = { 1.0, 0.0, 0.0 };
	coord C = { 1.0, 1.0, 0.0 };
	coord rv, V, W;
	double e, d;

	coord N = { 0.0, 0.0, 1.0 };
	//N = vecProduct(CreateVector(A, B), CreateVector(A, C));
	//Normalize(N);

	V = vecMinus(A, X);//CreateVector(X, A);

	d = vecScalarMul(N, V);
	W = vecMinus(Y, X);//CreateVector(X, Y);

	e = vecScalarMul(N, W);

	//if (e != 0)
	//{
		rv.x = X.x + W.x * d / e;
		rv.y = X.y + W.y * d / e;
		rv.z = 0;//X.z + W.z * d / e;
	//}
	*/
	
	coord x1 = {}, x2 = {};
	if (X.z > Y.z) {
		x1.x = X.x;
		x1.y = X.y;
		x1.z = X.z;
		x2.x = Y.x;
		x2.y = Y.y;
		x2.z = Y.z;
	}
	else {
		x1.x = Y.x;
		x1.y = Y.y;
		x1.z = Y.z;
		x2.x = X.x;
		x2.y = X.y;
		x2.z = X.z;
	}
	coord rv = {};
	rv.z = 0.0;
	rv.x = x1.x + ((1.0 - x1.z) * (x2.x - x1.x) / (x2.z - x1.z));
	rv.y = x1.y + ((1.0 - x1.z) * (x2.y - x1.y) / (x2.z - x1.z));//*1.1;
	
	return rv;
}

double vecCos(coord v, coord v2) {
	return ( vecScalarMul(v, v2) / (vecLen(v) + vecLen(v2)) );
}

coord rotP(coord v, coord R, coord p) {
	coord vr = { v.x - p.x, v.y - p.y, v.z - p.z };
	coord vrf{};
	vrf.x = (vr.z * (cos(R.x) * cos(R.z) * sin(R.y) + sin(R.x) * sin(R.z)) + vr.y * (cos(R.z) * sin(R.x) * sin(R.y) - cos(R.x) * sin(R.z)) + vr.x * (cos(R.y) * cos(R.z)))+p.x;
	vrf.y = (vr.z * (cos(R.x) * sin(R.y) * sin(R.z) - cos(R.z) * sin(R.x)) + vr.y * (sin(R.x) * sin(R.y) * sin(R.z) + cos(R.x) * cos(R.z)) + vr.x * (cos(R.y) * sin(R.z)))+p.y;
	vrf.z = (vr.z * (cos(R.x) * cos(R.y)) + vr.y * (cos(R.y) * sin(R.x)) - vr.x * (sin(R.y)))+p.z;
	return vrf;
}

coord scaleP(coord v, coord s, coord p) {
	return { (((v.x - p.x) * s.x) + p.x), (((v.y - p.y) * s.y) + p.y), (((v.z - p.z) * s.z) + p.z) };
}

bool checkN(coord n, coord p1, coord camP) {
	return (vecCos(n, vecMinus(p1, camP)) < 0.0);
}

coord2d projectPoint(coord p) {
	if (p.z > 0.0) return { (p.x * projectionPlaneDist / p.z * max(center.x, center.y)) + center.x, ((p.y * projectionPlaneDist * -1.0) / p.z * max(center.x, center.y)) + center.y };
	else {
		return { (p.x * max(center.x, center.y)) + center.x, (-1.0 * p.y * max(center.x, center.y)) + center.y};
	}
}



class obj3d {
public:
	objTypes type;

	long color;
	long colorOutl;

	coord n;
	bool checkNor;

	coord p1;
	coord p2;
	coord p3;
	coord p4;

	int outlSize = 1;

	bool visibility = true;

	coord p1t, p2t, p3t, p4t, nt;

	void transform(coord offs, coord addR, coord scale, coord p) {
		p1t = vecPlus(rotP(scaleP(p1, scale, p), addR, p), offs);
		p2t = vecPlus(rotP(scaleP(p2, scale, p), addR, p), offs);
		p3t = vecPlus(rotP(scaleP(p3, scale, p), addR, p), offs);
		p4t = vecPlus(rotP(scaleP(p4, scale, p), addR, p), offs);
		nt = vecMinus(rotP(vecPlus(n,p1), addR, p), rotP(p1, addR, p));
		/*
		p1t = rotP(p1, addR, p);
		p2t = rotP(p2, addR, p);
		p3t = rotP(p3, addR, p);
		p4t = rotP(p4, addR, p);
		nt = vecMinus(rotP(vecPlus(n, p1), addR, p), rotP(p1, addR, p));
		*/
	}

	double DistCh(coord camL) {
		switch (type)
		{
		case objTypes::obj_point:
			return vecLen(vecMinus(p1t, camL));
			break;
		case objTypes::obj_line:
			return max(vecLen(vecMinus(p1t, camL)), vecLen(vecMinus(p2t, camL)));
			break;
		case objTypes::obj_triangle:
			return max(vecLen(vecMinus(p1t, camL)), max(vecLen(vecMinus(p2t, camL)), vecLen(vecMinus(p3t, camL))));
			break;
		case objTypes::obj_quad:
			return max(vecLen(vecMinus(p1t, camL)), max(vecLen(vecMinus(p2t, camL)), max(vecLen(vecMinus(p3t, camL)), vecLen(vecMinus(p4t, camL)))));
			break;
		default:
			return max(vecLen(vecMinus(p1t, camL)), max(vecLen(vecMinus(p2t, camL)), max(vecLen(vecMinus(p3t, camL)), vecLen(vecMinus(p4t, camL)))));
			break;
		}
	}

	void Draw(coord camL, coord camR) {
		coord p1c, p2c, p3c, p4c, nc;
		int pCount = 4;
		int pNow = 0;
			p1c = rotP(vecMinus(p1t, camL), camR, { 0.0, 0.0, 0.0 });
			p2c = rotP(vecMinus(p2t, camL), camR, { 0.0, 0.0, 0.0 });
			p3c = rotP(vecMinus(p3t, camL), camR, { 0.0, 0.0, 0.0 });
			p4c = rotP(vecMinus(p4t, camL), camR, { 0.0, 0.0, 0.0 });
			nc = vecMinus(rotP(vecPlus(nt, vecMinus(p1t, camL)), camR, { 0.0, 0.0, 0.0 }), p1c);

		if ((checkN(nt, p1t, camL)||!checkNor)&&(p1c.z > 0 || p2c.z > 0 || p3c.z > 0 || p4c.z > 0)&&visibility) {
			HBRUSH brush = CreateSolidBrush(color);
			HPEN pen = CreatePen(PS_SOLID, outlSize, colorOutl);
			SelectObject(hCmpDC, brush);
			SelectObject(hCmpDC, pen);
			//POINT poly3[3] = { {int(projectPoint(p1c).x), int(projectPoint(p1c).y)}, {int(projectPoint(p2c).x), int(projectPoint(p2c).y)}, {int(projectPoint(p3c).x), int(projectPoint(p3c).y)} };;
			//POINT poly4[4] = { {int(projectPoint(p1c).x), int(projectPoint(p1c).y)}, {int(projectPoint(p2c).x), int(projectPoint(p2c).y)}, {int(projectPoint(p3c).x), int(projectPoint(p3c).y)}, {int(projectPoint(p4c).x), int(projectPoint(p4c).y)} };
			POINT poly5[6] = {};
			switch (type)
			{
			case objTypes::obj_point:
				MoveToEx(hCmpDC, int(projectPoint(p1c).x), int(projectPoint(p1c).y), NULL);
				LineTo(hCmpDC, int(projectPoint(p1c).x), int(projectPoint(p1c).y));
				break;
			case objTypes::obj_line:
				if (p1c.z < 1.0) {
					pCount--;
				}
				else {
					poly5[pNow] = { int(projectPoint(p1c).x), int(projectPoint(p1c).y) };
					pNow++;
				}
				if (p2c.z < 1.0) {
					pCount--;
					if (p1c.z >= 1.0) {
						pCount++;
						//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
				}
				else {
					if (p1c.z < 1.0) {
						pCount++;
						//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
					poly5[pNow] = { int(projectPoint(p2c).x), int(projectPoint(p2c).y) };
					pNow++;
				}
				//Polygon(hCmpDC, poly5, pCount);
				MoveToEx(hCmpDC, poly5[0].x, poly5[0].y, NULL);
				LineTo(hCmpDC, poly5[1].x, poly5[1].y );
				//MoveToEx(hCmpDC, int(projectPoint(p1c).x), int(projectPoint(p1c).y), NULL);
				//LineTo(hCmpDC, int(projectPoint(p2c).x), int(projectPoint(p2c).y));
				break;
			case objTypes::obj_triangle:
				pCount = 3;
				if (p1c.z < 1.0) {
					pCount--;
					if (p3c.z >= 1.0) {
						pCount++;
						//printf_s("3 1|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p4c.x, p4c.y, lineClip(p1c, p4c).x, lineClip(p1c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p3c, p1c)).x), int(projectPoint(lineClip(p3c, p1c)).y) };
						pNow++;
						//отрезок 3 1
					}
				}
				else {
					if (p3c.z < 1.0) {
						pCount++;
						//printf_s("3 1|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p4c.x, p4c.y, lineClip(p1c, p4c).x, lineClip(p1c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p3c, p1c)).x), int(projectPoint(lineClip(p3c, p1c)).y) };
						pNow++;
						//отрезок 3 1
					}
					//printf_s("1|\n");
					poly5[pNow] = { int(projectPoint(p1c).x), int(projectPoint(p1c).y) };
					pNow++;
				}
				if (p2c.z < 1.0) {
					pCount--;
					if (p1c.z >= 1.0) {
						pCount++;
						//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
				}
				else {
					if (p1c.z < 1.0) {
						pCount++;
						//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
					poly5[pNow] = { int(projectPoint(p2c).x), int(projectPoint(p2c).y) };
					pNow++;
				}
				if (p3c.z < 1.0) {
					pCount--;
					if (p2c.z >= 1.0) {
						pCount++;
						//	printf_s("2 3|%f %f | %f %f | %f %f\n", p2c.x, p2c.y, p3c.x, p3c.y, lineClip(p2c, p3c).x, lineClip(p2c, p3c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p2c, p3c)).x), int(projectPoint(lineClip(p2c, p3c)).y) };
						pNow++;
						//отрезок 2 3
					}
				}
				else {
					if (p2c.z < 1.0) {
						pCount++;
						//	printf_s("2 3|%f %f | %f %f | %f %f\n", p2c.x, p2c.y, p3c.x, p3c.y, lineClip(p2c, p3c).x, lineClip(p2c, p3c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p2c, p3c)).x), int(projectPoint(lineClip(p2c, p3c)).y) };
						pNow++;
						//отрезок 2 3
					}
					poly5[pNow] = { int(projectPoint(p3c).x), int(projectPoint(p3c).y) };
					pNow++;
				}
				Polygon(hCmpDC, poly5, pCount);
				break;
			case objTypes::obj_quad:
				if (p1c.z < 1.0) {
					pCount--;
					if (p4c.z >= 1.0) {
						pCount++;
						//printf_s("4 1|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p4c.x, p4c.y, lineClip(p1c, p4c).x, lineClip(p1c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p4c, p1c)).x), int(projectPoint(lineClip(p4c, p1c)).y) };
						pNow++;
						//отрезок 4 1
					}
				}
				else {
					if (p4c.z < 1.0) {
						pCount++;
						//printf_s("4 1|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p4c.x, p4c.y, lineClip(p1c, p4c).x, lineClip(p1c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p4c, p1c)).x), int(projectPoint(lineClip(p4c, p1c)).y) };
						pNow++;
						//отрезок 4 1
					}
					//printf_s("1|\n");
					poly5[pNow] = { int(projectPoint(p1c).x), int(projectPoint(p1c).y) };
					pNow++;
				}
				if (p2c.z < 1.0) {
					pCount--;
					if (p1c.z >= 1.0) {
						pCount++;
					//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
				}
				else {
					if (p1c.z < 1.0) {
						pCount++;
					//	printf_s("1 2|%f %f | %f %f | %f %f\n", p1c.x, p1c.y, p2c.x, p2c.y, lineClip(p1c, p2c).x, lineClip(p1c, p2c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p1c, p2c)).x), int(projectPoint(lineClip(p1c, p2c)).y) };
						pNow++;
						//отрезок 1 2
					}
					poly5[pNow] = { int(projectPoint(p2c).x), int(projectPoint(p2c).y) };
					pNow++;
				}
				if (p3c.z < 1.0) {
					pCount--;
					if (p2c.z >= 1.0) {
						pCount++;
					//	printf_s("2 3|%f %f | %f %f | %f %f\n", p2c.x, p2c.y, p3c.x, p3c.y, lineClip(p2c, p3c).x, lineClip(p2c, p3c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p2c, p3c)).x), int(projectPoint(lineClip(p2c, p3c)).y) };
						pNow++;
						//отрезок 2 3
					}
				}
				else {
					if (p2c.z < 1.0) {
						pCount++;
					//	printf_s("2 3|%f %f | %f %f | %f %f\n", p2c.x, p2c.y, p3c.x, p3c.y, lineClip(p2c, p3c).x, lineClip(p2c, p3c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p2c, p3c)).x), int(projectPoint(lineClip(p2c, p3c)).y) };
						pNow++;
						//отрезок 2 3
					}
					poly5[pNow] = { int(projectPoint(p3c).x), int(projectPoint(p3c).y) };
					pNow++;
				}
				if (p4c.z < 1.0) {
					pCount--;
					if (p3c.z >= 1.0) {
						pCount++;
					//	printf_s("3 4|%f %f | %f %f | %f %f\n", p3c.x, p3c.y, p4c.x, p4c.y, lineClip(p3c, p4c).x, lineClip(p3c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p3c, p4c)).x), int(projectPoint(lineClip(p3c, p4c)).y) };
						pNow++;
						//отрезок 3 4
					}
				}
				else {
					if (p3c.z < 1.0) {
						pCount++;
					//	printf_s("3 4|%f %f | %f %f | %f %f\n", p3c.x, p3c.y, p4c.x, p4c.y, lineClip(p3c, p4c).x, lineClip(p3c, p4c).y);
						poly5[pNow] = { int(projectPoint(lineClip(p3c, p4c)).x), int(projectPoint(lineClip(p3c, p4c)).y) };
						pNow++;
						//отрезок 3 4
					}
					poly5[pNow] = POINT{ int(projectPoint(p4c).x), int(projectPoint(p4c).y) };
					pNow++;
				}
				Polygon(hCmpDC, poly5, pCount);
				break;
			default:
				//Polygon(hCmpDC, poly4, 4);
				break;
			}

			DeleteObject(brush);
			DeleteObject(pen);
		}

	}
};

class model3d {
public:
	int ObjAmount = 0;
	obj3d *objects3d[200] = {};

	void transform(coord offs, coord addR, coord scale, coord p) {
		for (int i = 0; i < ObjAmount; i++) {
			objects3d[i]->transform(offs, addR, scale, p);
		}
	}

	void addObj(objTypes type, long color, long colorOutl, coord N, bool UseNormal, coord p1, coord p2, coord p3, coord p4) {
		objects3d[ObjAmount] = new obj3d { type, color, colorOutl, N, UseNormal, p1, p2, p3, p4 };
		ObjAmount++;
	}

	void setColor(long color, long colorOutl) {
		for (int i = 0; i < ObjAmount; i++) {
			objects3d[i]->color = color;
			objects3d[i]->colorOutl = colorOutl;
		}
	}

	void setVisibility(bool vis) {
		for (int i = 0; i < ObjAmount; i++) {
			objects3d[i]->visibility = vis;
		}
	}
};

void addModelToSc(bool ResetCounter, int counterR, model3d* model, obj3d** scArr) {
	static int counter;
	if (ResetCounter) counter = counterR;
	int i = 0;
	for (i = 0; i < model->ObjAmount; i++) {
		scArr[counter] = model->objects3d[i];
		counter++;
	}
};

void copyModel(model3d* modelC, model3d* modelP) {
	//modelC->ObjAmount
	for (int i = 0; i < modelC->ObjAmount; i++) {
		modelP->addObj(modelC->objects3d[i]->type, modelC->objects3d[i]->color, modelC->objects3d[i]->colorOutl, modelC->objects3d[i]->n, modelC->objects3d[i]->checkNor, modelC->objects3d[i]->p1, modelC->objects3d[i]->p2, modelC->objects3d[i]->p3, modelC->objects3d[i]->p4);
	}
};

void drawScene(obj3d* sc[], int sSize, coord camL, coord camR) {
	obj3d* drawlist[400]{};
	double scDistanses[400]{};

	int i, j, k;
	
	for (i = 0; i <= sSize; i++) {
		double DistI = sc[i]->DistCh(camL);
		for (j = 0; j<i&&(scDistanses[j] < DistI); j++) {};
		for (k = i; k > j; k--) {
			drawlist[k] = drawlist[k - 1];
			scDistanses[k] = scDistanses[k - 1];
		}
		drawlist[j] = sc[i];
		scDistanses[j] = drawlist[j]->DistCh(camL);
	}
	for (i = sSize; i >= 0; i--) {
		drawlist[i]->Draw(camL, camR);
		
	}
	

	/*
	for (i = 0; i <= sSize; i++) {
		sc[i]->Draw(camL, camR);
	}
	*/
}

int getKey() 
{
	int i;
	for (i = 8; i <= 256; i++)
	{
		if (GetAsyncKeyState(i) & 0x7FFF)
		{
			return i;
		}
	}
	return -1;
}

int main() {
	HANDLE han = GetStdHandle(STD_OUTPUT_HANDLE);
	hdc = GetDC(GetConsoleWindow());
	hCmpDC = CreateCompatibleDC(hdc);
	hBmp = CreateCompatibleBitmap(hdc, int(center.x*2),int(center.y*2));
	SelectObject(hCmpDC, hBmp);

	HBRUSH bbrush = CreateSolidBrush(RGB(200, 200, 200));
	HPEN bpen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));

	/*
	obj3d *q1 = new obj3d{ objTypes::obj_quad, RGB(0, 0, 255), RGB(0, 0, 255), {0.0, 0.0, -1.0}, true, {-1.0, -1.0, 2.0},{-1.0, 1.0, 2.0},{1.0, 1.0, 2.0},{1.0, -1.0, 2.0} };
	obj3d *q2 = new obj3d{ objTypes::obj_quad, RGB(0, 255, 0), RGB(0, 255, 0), {0.0, 0.0, 1.0}, true, {-1.0, -1.0, 4.0},{-1.0, 1.0, 4.0},{1.0, 1.0, 4.0},{1.0, -1.0, 4.0} };

	obj3d *q3 = new obj3d{ objTypes::obj_quad, RGB(255, 0, 255), RGB(255, 0, 255), {-1.0, 0.0, 0.0}, true, {-1.0, -1.0, 2.0},{-1.0, 1.0, 2.0},{-1.0, 1.0, 4.0},{-1.0, -1.0, 4.0} };
	obj3d *q4 = new obj3d{ objTypes::obj_quad, RGB(255, 0, 0), RGB(255, 0, 0), {1.0, 0.0, 0.0}, true, {1.0, -1.0, 2.0},{1.0, 1.0, 2.0},{1.0, 1.0, 4.0},{1.0, -1.0, 4.0}};

	obj3d *q5 = new obj3d{ objTypes::obj_quad, RGB(255, 255, 255), RGB(255, 255, 255), {0.0, 1.0, 0.0}, true, {-1.0, 1.0, 2.0},{-1.0, 1.0, 4.0},{1.0, 1.0, 4.0},{1.0, 1.0, 2.0} };
	obj3d *q6 = new obj3d{ objTypes::obj_quad, RGB(52, 4, 52), RGB(52, 4, 52), {0.0, -1.0, 0.0}, true, {-1.0, -1.0, 2.0},{-1.0, -1.0, 4.0},{1.0, -1.0, 4.0},{1.0, -1.0, 2.0} };



	obj3d *q7 = new obj3d{ objTypes::obj_quad, RGB(0, 33, 52), RGB(0, 33, 52), {0.0, 0.0, -1.0}, true, {-5.0, -30.0, 6.0},{-5.0, 1.0, 6.0},{3.0, 1.0, 6.0},{3.0, -30.0, 6.0} };
	q7->transform({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
	*/

	//VVad sc
	obj3d* qVv1 = new obj3d{ objTypes::obj_quad, RGB(0, 0, 0), RGB(255, 255, 255), {0.0, 0.0, -1.0}, false, {-2.0, 5.0, 2.0},{-2.0, 5.0, 4.0},{0.0, 0.0, 4.0},{0.0, 0.0, 2.0} };
	obj3d* qVv2 = new obj3d{ objTypes::obj_quad, RGB(0, 0, 0), RGB(255, 255, 255), {0.0, 0.0, -1.0}, false, { 2.0, 5.0, 2.0},{ 2.0, 5.0, 4.0},{0.0, 0.0, 4.0},{0.0, 0.0, 2.0} };
	qVv1->transform({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
	qVv2->transform({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
	obj3d *scVv[70] = { qVv1, qVv2 };
	model3d *mVvTxt = new model3d;
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -2.0,  0.0,  3.0 }, { -3.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -3.0,  0.0,  3.0 }, { -3.0,  3.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -3.5,  2.0,  3.0 }, { -2.5,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//t

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -4.0,  0.0,  3.0 }, { -5.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -5.0,  0.0,  3.0 }, { -5.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -5.0,  2.0,  3.0 }, { -4.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//c

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -6.0,  0.0,  3.0 }, { -7.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -6.0,  2.0,  3.0 }, { -7.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -6.5,  0.0,  3.0 }, { -6.5,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//i

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -8.0,  0.0,  3.0 }, { -8.0,  3.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -8.0,  0.0,  3.0 }, { -9.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -8.0,  2.0,  3.0 }, { -9.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -9.0,  0.0,  3.0 }, { -9.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//d

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -10.0,  0.0,  3.0 }, { -11.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -10.0,  1.0,  3.0 }, { -11.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -10.0,  2.0,  3.0 }, { -11.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -11.0,  0.0,  3.0 }, { -11.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//e

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -12.0,  0.0,  3.0 }, { -12.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -13.0,  0.0,  3.0 }, { -13.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -13.0,  2.0,  3.0 }, { -12.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//n

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -14.0,  0.0,  3.0 }, { -15.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -14.0,  1.0,  3.0 }, { -15.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -14.0,  2.0,  3.0 }, { -15.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -15.0,  0.0,  3.0 }, { -15.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//e

	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -16.0,  3.0,  3.0 }, { -17.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { -18.0,  3.0,  3.0 }, { -17.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//v

	mVvTxt->transform({ -5.0,0.0,20.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	addModelToSc(true, 2, mVvTxt, scVv);

	model3d* mVvTxt2 = new model3d;
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 2.0,  3.0,  3.0 }, { 3.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 4.0,  3.0,  3.0 }, { 3.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//v

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 5.0,  0.0,  3.0 }, { 5.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 6.0,  0.0,  3.0 }, { 6.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 5.0,  1.0,  3.0 }, { 6.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 5.0,  2.0,  3.0 }, { 6.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//a

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 7.0,  0.0,  3.0 }, { 7.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 7.0,  0.0,  3.0 }, { 8.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 7.0,  2.0,  3.0 }, { 8.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 8.0,  0.0,  3.0 }, { 8.0,  3.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//d

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 9.5,  0.0,  3.0 }, { 9.5,  2.0,  3.0  }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 9.0,  0.0,  3.0 }, { 10.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 9.0,  2.0,  3.0 }, { 10.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//i

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 11.0,  0.0,  3.0 }, { 11.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 12.0,  0.0,  3.0 }, { 12.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 13.0,  0.0,  3.0 }, { 13.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 11.0,  2.0,  3.0 }, { 12.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 12.0,  2.0,  3.0 }, { 13.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//m

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 14.0,  0.0,  3.0 }, { 14.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 15.0,  0.0,  3.0 }, { 15.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 14.0,  0.0,  3.0 }, { 15.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 14.0,  2.0,  3.0 }, { 15.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//o

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 16.5,  0.0,  3.0 }, { 16.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 16.5,  0.0,  3.0 }, { 17.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//v

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 18.5,  0.0,  3.0 }, { 18.5,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 18.0,  0.0,  3.0 }, { 19.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 18.0,  2.0,  3.0 }, { 19.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//i

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 20.0,  0.0,  3.0 }, { 20.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 20.0,  0.0,  3.0 }, { 21.0,  0.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 20.0,  2.0,  3.0 }, { 21.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//c

	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 22.0,  0.0,  3.0 }, { 22.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 23.0,  0.0,  3.0 }, { 23.0,  2.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
	mVvTxt2->addObj(objTypes::obj_line, RGB(0, 0, 0), RGB(255, 255, 255), { 0.0,  0.0, -1.0 }, false, { 22.0,  1.0,  3.0 }, { 23.0,  1.0,  3.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });//h

	mVvTxt2->transform({ 4.0,0.0,20.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	mVvTxt2->setColor(RGB(0, 0, 0), RGB(0, 0, 0));
	mVvTxt->setColor(RGB(0, 0, 0), RGB(0, 0, 0));
	addModelToSc(false, 0, mVvTxt2, scVv);

	//---------
	obj3d* qRoad = new obj3d{ objTypes::obj_quad, RGB(15, 15, 15), RGB(0, 0, 0), {0.0, 1.0, 0.0}, true, {-1000.0, -1.0, -8.0}, {-1000.0, -1.0, 8.0}, {1000.0, -1.0, 8.0}, {1000.0, -1.0, -8.0} };
	qRoad->transform({ 0.0,0.0,0.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	obj3d* qEnd = new obj3d{ objTypes::obj_quad, RGB(255, 255, 255), RGB(255, 255, 255), {-1.0, 0.0, 0.0}, true, {1000.0, -1.0, -8.0}, {1000.0, -1.0, 8.0}, {1000.0, 100.0, 8.0}, {1000.0, 100.0, -8.0} };
	qEnd->transform({ 0.0,0.0,0.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	qEnd->visibility = false;
	obj3d* scBcgr[30] = { qRoad, qEnd };
	obj3d* sc1[400] = {};

	//addModelToSc(true, 7, mCube, sc1);
	//model car
	model3d* mCar = new model3d;
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), {  0, 0.196116, -0.980581 }, true, { -8.1, 1.0, -3.2 }, { -7.8, 2.0, -3.0 }, { -2.0,  2.0, -3.0 }, { -2.0, 1.0, -3.2 });//1
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), {  0, 0.196116, -0.980581 }, true, { -2.0, 1.0, -3.2 }, { -2.0, 2.0, -3.0 }, {  5.0,  2.0, -3.0 }, {  5.0, 1.0, -3.2 });//1-1
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { -3.0, 0.0, -3.0 }, { -3.0, 1.0, -3.2 }, {  5.0,  1.0, -3.2 }, {  5.0, 0.0, -3.0 });//2
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), { 0,  0.196116, -0.980581 }, true, {  5.0, 1.0, -3.2 }, {  5.0, 2.0, -3.0 }, { 11.0, 1.62, -3.0 }, { 11.0, 1.0, -3.2 });//3
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, {  8.0, 0.0, -3.0 }, {  8.0, 1.0, -3.2 }, { 10.2,  1.0, -3.2 }, { 10.2, 0.0, -3.0 });//4
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { -7.8, 0.0, -3.0 }, { -7.8, 1.0, -3.2 }, { -6.0,  1.0, -3.2 }, { -6.0, 0.0, -3.0 });//5
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { -7.8, 0.0, -3.0 }, { -8.1, 1.0, -3.2 }, { -7.8,  1.0, -3.2 }, {0.0, 0.0, 0.0});//6
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { -6.0, 0.0, -3.0 }, { -6.0, 1.0, -3.2 }, { -5.0,  1.0, -3.2 }, {0.0, 0.0, 0.0});//7
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { -4.0, 1.0, -3.2 }, { -3.0, 1.0, -3.2 }, { -3.0,  0.0, -3.0 }, {0.0, 0.0, 0.0});//8
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, {  5.0, 0.0, -3.0 }, {  5.0, 1.0, -3.2 }, {  6.0,  1.0, -3.2 }, {0.0, 0.0, 0.0});//9
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, {  7.0, 1.0, -3.2 }, {  8.0, 1.0, -3.2 }, {  8.0,  0.0, -3.0 }, {0.0, 0.0, 0.0});//10
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, -0.980581 }, true, { 10.2, 0.0, -3.0 }, { 10.2, 1.0, -3.2 }, { 11.0,  1.0, -3.2 }, {0.0, 0.0, 0.0});//11
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0,  0.242536, -0.970143 }, true, { -7.8, 2.0, -3.0 }, { -3.8, 2.0, -3.0 }, { -3.8,  3.6, -2.6 }, {0.0, 0.0, 0.0});//12
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0,  0.242536, -0.970143 }, true, { -3.8, 2.0, -3.0 }, { -3.8, 2.9, -2.775 }, { -1.8,  2.0, -3.0 }, { 0.0, 0.0, 0.0 });//13
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0,  0.242536, -0.970143 }, true, { -3.8, 2.9, -2.775 }, { -3.8,  3.6, -2.6 }, { -3.0,  3.8, -2.5 }, { -2.5, 3.6, -2.6 });//14
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0,  0.447214, -0.894427 }, true, { -3.0,  3.8, -2.5 }, { -2.5, 3.6, -2.6 }, { 1.5,  3.6, -2.6 }, { 2.0, 3.8, -2.5 });//15
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0,  0.447214, -0.894427 }, true, { 1.5,  3.6, -2.6 }, { 2.0, 3.8, -2.5 }, { 5.0,  2.0, -3.0 }, { 4.0, 2.0, -3.0 });//16
	mCar->addObj(objTypes::obj_quad,     RGB(20, 20, 70), RGB(0, 0, 70), { 0,  0.242536, -0.970143 }, true, { 1.5,  3.6, -2.6 }, { 4.0, 2.0, -3.0 }, { -7.8, 2.0, -2.99 }, { -2.5, 3.6, -2.6} );//windowR

	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), { 0, 0.196116, 0.980581  }, true, { -8.1, 1.0, 3.2 }, { -7.8, 2.0, 3.0 }, { -2.0,  2.0, 3.0 }, { -2.0, 1.0, 3.2 });//1
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), { 0, 0.196116, 0.980581  }, true, { -2.0, 1.0, 3.2 }, { -2.0, 2.0, 3.0 }, { 5.0,  2.0, 3.0 }, { 5.0, 1.0, 3.2 });//1-1
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { -3.0, 0.0, 3.0 }, { -3.0, 1.0, 3.2 }, { 5.0,  1.0, 3.2 }, { 5.0, 0.0, 3.0 });//2
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), { 0, 0.196116, 0.980581  }, true, { 5.0, 1.0, 3.2 }, { 5.0, 2.0, 3.0 }, { 11.0, 1.62, 3.0 }, { 11.0, 1.0, 3.2 });//3
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { 8.0, 0.0, 3.0 }, { 8.0, 1.0, 3.2 }, { 10.2,  1.0, 3.2 }, { 10.2, 0.0, 3.0 });//4
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { -7.8, 0.0, 3.0 }, { -7.8, 1.0, 3.2 }, { -6.0,  1.0, 3.2 }, { -6.0, 0.0, 3.0 });//5
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { -7.8, 0.0, 3.0 }, { -8.1, 1.0, 3.2 }, { -7.8,  1.0, 3.2 }, { 0.0, 0.0, 0.0 });//6
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { -6.0, 0.0, 3.0 }, { -6.0, 1.0, 3.2 }, { -5.0,  1.0, 3.2 }, { 0.0, 0.0, 0.0 });//7
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { -4.0, 1.0, 3.2 }, { -3.0, 1.0, 3.2 }, { -3.0,  0.0, 3.0 }, { 0.0, 0.0, 0.0 });//8
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { 5.0, 0.0, 3.0 }, { 5.0, 1.0, 3.2 }, { 6.0,  1.0, 3.2 }, { 0.0, 0.0, 0.0 });//9
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { 7.0, 1.0, 3.2 }, { 8.0, 1.0, 3.2 }, { 8.0,  0.0, 3.0 }, { 0.0, 0.0, 0.0 });//10
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, -0.196116, 0.980581 }, true, { 10.2, 0.0, 3.0 }, { 10.2, 1.0, 3.2 }, { 11.0,  1.0, 3.2 }, { 0.0, 0.0, 0.0 });//11
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, 0.242536, 0.970143  }, true, { -7.8, 2.0, 3.0 }, { -3.8, 2.0, 3.0 }, { -3.8,  3.6, 2.6 }, { 0.0, 0.0, 0.0 });//12
	mCar->addObj(objTypes::obj_triangle, RGB(5, 5, 5),     RGB(5, 5, 5), { 0, 0.242536, 0.970143  }, true, { -3.8, 2.0, 3.0 }, { -3.8, 2.9, 2.775 }, { -1.8,  2.0, 3.0 }, { 0.0, 0.0, 0.0 });//13
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, 0.242536, 0.970143  }, true, { -3.8, 2.9, 2.775 }, { -3.8,  3.6, 2.6 }, { -3.0,  3.8, 2.5 }, { -2.5, 3.6, 2.6 });//14
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, 0.447214, 0.894427  }, true, { -3.0,  3.8, 2.5 }, { -2.5, 3.6, 2.6 }, { 1.5,  3.6, 2.6 }, { 2.0, 3.8, 2.5 });//15
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { 0, 0.447214, 0.894427  }, true, { 1.5,  3.6, 2.6 }, { 2.0, 3.8, 2.5 }, { 5.0,  2.0, 3.0 }, { 4.0, 2.0, 3.0 });//16
	mCar->addObj(objTypes::obj_quad,     RGB(20, 20, 70), RGB(0, 0, 70), { 0, 0.242536, 0.970143  }, true, { 1.5,  3.6, 2.6 }, { 4.0, 2.0, 3.0 }, { -7.8, 2.0, 2.99 }, { -2.5, 3.6, 2.6 });//windowR

	mCar->addObj(objTypes::obj_quad,     RGB(4, 4, 4),     RGB(4, 4, 4), { -0.957826, -0.287348, 0 }, true, { -7.95, 0.5,  0  }, {  -8.1, 1.0,   0 }, {  -8.1, 1.0, -3.2 }, { -7.95, 0.5, -3.1  }); //b1
	mCar->addObj(objTypes::obj_quad,     RGB(4, 4, 4),     RGB(4, 4, 4), { -0.957826, -0.287348, 0 }, true, { -7.95, 0.5,  3.1  }, {  -8.1, 1.0,   3.2 }, {  -8.1, 1.0, 0 }, { -7.95, 0.5, 0  }); //b1-1
	mCar->addObj(objTypes::obj_quad,     RGB(5, 5, 5),     RGB(5, 5, 5), { -0.957826,  0.287348, 0 }, true, { -8.1,  1.0, -3.2  }, {  -8.1, 1.0,   3.2 }, {  -7.8, 2.0,  3.0 }, {  -7.8, 2.0, -3.0  }); //b2
	mCar->addObj(objTypes::obj_quad,     RGB(4, 4, 4),     RGB(4, 4, 4), { -0.957826, -0.287348, 0 }, true, { -7.95, 0.5,  3.1  }, {  -7.8, 0.0,   3.0 }, {  -7.8, 0.0, -1.25}, { -7.95, 0.5, -1.5  }); //b3
	mCar->addObj(objTypes::obj_quad,     RGB(4, 4, 4),     RGB(4, 4, 4), { -0.957826, -0.287348, 0 }, true, { -7.95, 0.5, -2.0  }, {  -7.8, 0.0, -2.25 }, {  -7.8, 0.0, -3.0 }, { -7.95, 0.5, -3.1  }); //b4
	mCar->addObj(objTypes::obj_quad,     RGB(0, 0, 0),     RGB(0, 0, 0), { -0.957826, -0.287348, 0 }, true, { -7.8, 0.0, -2.25  }, { -7.95, 0.5,  -2.0 }, { -7.95, 0.5, -1.5 }, { -7.8,  0.0, -1.25 }); //b_zagl

	mCar->addObj(objTypes::obj_quad,  RGB(10, 10, 10),  RGB(10, 10, 10), { 0, 0, 0 },                false, { -7.8, 0.16, -1.5  }, {  -7.8, 0.16, -2.0 }, { -8.2, 0.16, -2.0 }, { -8.2, 0.16, -1.5 }); //b_tube
	mCar->addObj(objTypes::obj_quad,  RGB(10, 10, 10),  RGB(10, 10, 10), { 0, 0, 0 },                false, { -7.8, 0.33, -1.5  }, {  -7.8, 0.33, -2.0 }, { -8.2, 0.33, -2.0 }, { -8.2, 0.33, -1.5 }); //b_tube2
	mCar->addObj(objTypes::obj_quad,  RGB(10, 10, 10),  RGB(10, 10, 10), { 0, 0, 0 },                false, { -7.8, 0.16, -1.5  }, {  -7.8, 0.33, -1.5 }, { -8.2, 0.33, -1.5 }, { -8.2, 0.16, -1.5 }); //b_tube3
	mCar->addObj(objTypes::obj_quad,  RGB(10, 10, 10),  RGB(10, 10, 10), { 0, 0, 0 },                false, { -7.8, 0.16, -2.0  }, {  -7.8, 0.33, -2.0 }, { -8.2, 0.33, -2.0 }, { -8.2, 0.16, -2.0 }); //b_tube4

	mCar->addObj(objTypes::obj_quad,    RGB(50, 0, 0),     RGB(5, 5, 5), { -0.957826,  0.287348, 0 }, true, { -7.98, 1.4,  3.12 }, { -7.98, 1.4,   2.2 }, { -7.92, 1.6,  2.2 }, { -7.92, 1.6,  3.08 }); //bLightL
	mCar->addObj(objTypes::obj_quad,    RGB(50, 0, 0),     RGB(5, 5, 5), { -0.957826,  0.287348, 0 }, true, { -7.98, 1.4, -3.12 }, { -7.98, 1.4,  -2.2 }, { -7.92, 1.6, -2.2 }, { -7.92, 1.6, -3.08 }); //bLightR
	mCar->addObj(objTypes::obj_quad,    RGB(50, 50, 50),   RGB(5, 5, 5), { -0.957826,  0.287348, 0 }, true, { -7.86, 1.8, -1.0 },  { -8.04, 1.2,  -1.0 }, { -8.04, 1.2,  1.0 }, { -7.86, 1.8,  1.0  }); //bNumber

	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), { -0.371391,  0.928477, 0 }, true, { -7.8, 2.0,   3.0 },  {  -7.8, 2.0,  -3.0 }, {  -6.3, 2.6, -2.85 }, { -6.3, 2.6,  2.85 }); //bU1
	mCar->addObj(objTypes::obj_quad,  RGB(20, 20, 70),    RGB(0, 0, 70), { -0.371391,  0.928477, 0 }, true, { -3.8, 3.6,   2.6 },  {  -3.8, 3.6,  -2.6 }, {  -6.3, 2.6, -2.85 }, { -6.3, 2.6,  2.85 }); //bUwindow
	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), { -0.371391,  0.928477, 0 }, true, { -3.8, 3.6,   2.6 },  {  -3.8, 3.6,   2.2 }, {  -6.3, 2.6,  2.25 }, { -6.3, 2.6,  2.85 }); //bU2
	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), { -0.371391,  0.928477, 0 }, true, { -3.8, 3.6,  -2.2 },  {  -3.8, 3.6,  -2.6 }, {  -6.3, 2.6, -2.85 }, { -6.3, 2.6, -2.25 }); //bU3
	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), { -0.242536,  0.970143, 0 }, true, { -3.8, 3.6,  -2.6 },  {  -3.8, 3.6,   2.6 }, {  -3.0, 3.8,  2.5  }, { -3.0, 3.8,  -2.5 }); //bU4

	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), { 0, 1, 0 },                 true, {  2.0, 3.8,  -2.5 },  {   2.0, 3.8,   2.5 }, {  -3.0, 3.8,  2.5  }, { -3.0, 3.8,  -2.5 }); //u1

	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), {  0.514496, 0.857493, 0  }, true, {  2.0, 3.8,  -2.5 },  {   2.0, 3.8,   2.5 }, {  2.25, 3.65, 2.5416  }, { 2.25, 3.65,  -2.5416}); //fU1
	mCar->addObj(objTypes::obj_quad,  RGB(20, 20, 70),    RGB(0, 0, 70), {  0.514496, 0.857493, 0  }, true, {  5.0, 2.0,  -2.9 },  {   5.0, 2.0,   2.9 }, {  2.25, 3.65, 2.4416  }, { 2.25, 3.65,  -2.4416}); //fUwindow
	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), {  0.514496, 0.857493, 0  }, true, {  5.0, 2.0,  -3.0 },  {   5.0, 2.0,  -2.9 }, {  2.25, 3.65, -2.4416 }, { 2.25, 3.65,  -2.5416}); //fU2
	mCar->addObj(objTypes::obj_quad,     RGB(3, 3, 3),     RGB(3, 3, 3), {  0.514496, 0.857493, 0  }, true, {  5.0, 2.0,   2.9 },  {   5.0, 2.0,   3.0 }, {  2.25, 3.65, 2.5416  }, { 2.25, 3.65,   2.4416}); //fU2

	mCar->addObj(objTypes::obj_quad,     RGB(2, 2, 2),     RGB(2, 2, 2), { 0.0632067,    0.998, 0  }, true, {  5.0, 2.0,  -3.0 },  {   5.0, 2.0,   3.0 }, {  11.0, 1.62, 3.0     }, { 11.0, 1.62,  -3.0   }); //f1

	mCar->addObj(objTypes::obj_quad,     RGB(1, 1, 1),     RGB(2, 2, 2), { 1, 0, 0 },                 true, {  11.0, 1.0,  -3.2 }, {   11.0, 1.0,   3.2 }, {  11.0, 1.62, 3.0     }, { 11.0, 1.62,  -3.0   }); //f2
	mCar->addObj(objTypes::obj_quad,     RGB(255, 255, 255),     RGB(2, 2, 2), { 1, 0, 0 },           true, {  11.0, 1.2,   2.0 }, {   11.0, 1.2,   3.0 }, {  11.0, 1.42, 3.0     }, { 11.0, 1.42,   2.0   }); //fLightL
	mCar->addObj(objTypes::obj_quad,     RGB(255, 255, 255),     RGB(2, 2, 2), { 1, 0, 0 },           true, {  11.0, 1.2,  -3.0 }, {   11.0, 1.2,  -2.0 }, {  11.0, 1.42,-2.0     }, { 11.0, 1.42,  -3.0   }); //fLightR
	mCar->addObj(objTypes::obj_quad,     RGB(80, 80, 80),  RGB(80, 80, 80), { 0.780869, -0.624695, 0 }, true, {  11.0, 1.0,  0 }, {   11.0, 1.0,   3.2 }, { 10.84,  0.8, 3.16    }, {10.84,  0.8,  0  }); //f3
	mCar->addObj(objTypes::obj_quad,     RGB(80, 80, 80),  RGB(80, 80, 80), { 0.780869, -0.624695, 0 }, true, {  11.0, 1.0,  -3.2 }, {   11.0, 1.0,   0 }, { 10.84,  0.8, 0    }, {10.84,  0.8,  -3.16  }); //f3-1
	mCar->addObj(objTypes::obj_quad,     RGB(1, 1, 1),     RGB(2, 2, 2), { 0.780869, -0.624695, 0 },    true, { 10.84, 0.8, -3.16 }, {  10.84, 0.8,  3.16 }, {  10.2,  0.0, 3.0     }, { 10.2,  0.0,  -3.0   }); //f4

	addModelToSc(true, 0, mCar, sc1); //62
	mCar->transform({ 0.0,0.0,0.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	//-----------

	//model Wheel 31
	model3d* mWheel = new model3d;
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});

	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});
	mWheel->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});

	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});

	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5,  0.5, 0.5}, { 0.0,  0.7, 0.5}, { 0.5,  0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5,  0.5, 0.5});
	mWheel->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, {  0.0, -0.7, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5, -0.5, 0.5});

	mWheel->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0, -0.624695, -0.780869}, true, { -0.2,  0.7, 0.0}, { 0.2,  0.7, 0.0}, { 0.1,  0.2, 0.4}, { -0.1,  0.2, 0.4});
	mWheel->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0,  0.624695, -0.780869}, true, { -0.2, -0.7, 0.0}, { 0.2, -0.7, 0.0}, { 0.1, -0.2, 0.4}, { -0.1, -0.2, 0.4});
	mWheel->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {-0.624695, 0, -0.780869}, true, {  0.7, -0.2, 0.0}, { 0.7,  0.2, 0.0}, { 0.2,  0.1, 0.4}, {  0.2, -0.1, 0.4});
	mWheel->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), { 0.624695, 0, -0.780869}, true, { -0.7, -0.2, 0.0}, { -0.7, 0.2, 0.0}, {-0.2,  0.1, 0.4}, { -0.2, -0.1, 0.4});

	addModelToSc(false, 0, mWheel, sc1);
	mWheel->transform({ 6.5,0.0,-3.2 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	//-------------

	//model Wheel 31
	model3d* mWheel2 = new model3d;
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});

	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});
	mWheel2->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});

	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});

	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5,  0.5, 0.5}, { 0.0,  0.7, 0.5}, { 0.5,  0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5,  0.5, 0.5});
	mWheel2->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, {  0.0, -0.7, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5, -0.5, 0.5});

	mWheel2->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0, -0.624695, -0.780869}, true, { -0.2,  0.7, 0.0}, { 0.2,  0.7, 0.0}, { 0.1,  0.2, 0.4}, { -0.1,  0.2, 0.4});
	mWheel2->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0,  0.624695, -0.780869}, true, { -0.2, -0.7, 0.0}, { 0.2, -0.7, 0.0}, { 0.1, -0.2, 0.4}, { -0.1, -0.2, 0.4});
	mWheel2->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {-0.624695, 0, -0.780869}, true, {  0.7, -0.2, 0.0}, { 0.7,  0.2, 0.0}, { 0.2,  0.1, 0.4}, {  0.2, -0.1, 0.4});
	mWheel2->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), { 0.624695, 0, -0.780869}, true, { -0.7, -0.2, 0.0}, { -0.7, 0.2, 0.0}, {-0.2,  0.1, 0.4}, { -0.2, -0.1, 0.4});

	addModelToSc(false, 0, mWheel2, sc1);
	mWheel2->transform({ -4.5,0.0,-3.2 }, { 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	//-------------

	//model Wheel 31
	model3d* mWheel3 = new model3d;
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});

	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});
	mWheel3->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});

	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});

	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5,  0.5, 0.5}, { 0.0,  0.7, 0.5}, { 0.5,  0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5,  0.5, 0.5});
	mWheel3->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, {  0.0, -0.7, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5, -0.5, 0.5});

	mWheel3->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0, -0.624695, -0.780869}, true, { -0.2,  0.7, 0.0}, { 0.2,  0.7, 0.0}, { 0.1,  0.2, 0.4}, { -0.1,  0.2, 0.4});
	mWheel3->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0,  0.624695, -0.780869}, true, { -0.2, -0.7, 0.0}, { 0.2, -0.7, 0.0}, { 0.1, -0.2, 0.4}, { -0.1, -0.2, 0.4});
	mWheel3->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {-0.624695, 0, -0.780869}, true, {  0.7, -0.2, 0.0}, { 0.7,  0.2, 0.0}, { 0.2,  0.1, 0.4}, {  0.2, -0.1, 0.4});
	mWheel3->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), { 0.624695, 0, -0.780869}, true, { -0.7, -0.2, 0.0}, { -0.7, 0.2, 0.0}, {-0.2,  0.1, 0.4}, { -0.2, -0.1, 0.4});

	addModelToSc(false, 0, mWheel3, sc1);
	mWheel3->transform({ 6.5,0.0,3.2 }, { 0.0, 3.14, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	mWheel3->setVisibility(false);
	//-------------

	//model Wheel 31
	model3d* mWheel4 = new model3d;
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 1.0,  0.0, 0.0}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, {-1.0,  0.0, 0.0}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.0, 0.0, -1.0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.7,  0.7, 0.0});

	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7,  0.7, 0.5}, { 0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, { 1.0,  0.0, 0.0}, { 1.0,  0.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, { 0.7, -0.7, 0.5}, { 0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0, -1.0, 0.0}, { 0.0, -1.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7, -0.7, 0.5}, {-0.7, -0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, {-1.0,  0.0, 0.0}, {-1.0,  0.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});
	mWheel4->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0,  1.0, 0.0}, { 0.0,  1.0, 0.5}, {-0.7,  0.7, 0.5}, {-0.7,  0.7, 0.0});

	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145, -0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5,  0.5, 0.0}, { 0.5,  0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.919145,  0.393919, 0 }, true, { 0.7,  0.0, 0.5}, { 0.7,  0.0, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), {-0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, { 0.5, -0.5, 0.0}, { 0.5, -0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919,  0.919145, 0 }, true, { 0.0, -0.7, 0.5}, { 0.0, -0.7, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145,  0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5, -0.5, 0.0}, {-0.5, -0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.919145, -0.393919, 0 }, true, {-0.7,  0.0, 0.5}, {-0.7,  0.0, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(0, 0, 0), { 0.393919, -0.919145, 0 }, true, { 0.0,  0.7, 0.5}, { 0.0,  0.7, 0.0}, {-0.5,  0.5, 0.0}, {-0.5,  0.5, 0.5});

	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5,  0.5, 0.5}, { 0.0,  0.7, 0.5}, { 0.5,  0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, { -0.7,  0.0, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5,  0.5, 0.5});
	mWheel4->addObj(objTypes::obj_quad, RGB(80, 80, 80), RGB(80, 80, 80), { 0.0, 0.0, -1.0 }, true, {  0.0, -0.7, 0.5}, { -0.5, -0.5, 0.5}, { 0.7,  0.0, 0.5}, { 0.5, -0.5, 0.5});

	mWheel4->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0, -0.624695, -0.780869}, true, { -0.2,  0.7, 0.0}, { 0.2,  0.7, 0.0}, { 0.1,  0.2, 0.4}, { -0.1,  0.2, 0.4});
	mWheel4->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {0,  0.624695, -0.780869}, true, { -0.2, -0.7, 0.0}, { 0.2, -0.7, 0.0}, { 0.1, -0.2, 0.4}, { -0.1, -0.2, 0.4});
	mWheel4->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), {-0.624695, 0, -0.780869}, true, {  0.7, -0.2, 0.0}, { 0.7,  0.2, 0.0}, { 0.2,  0.1, 0.4}, {  0.2, -0.1, 0.4});
	mWheel4->addObj(objTypes::obj_quad, RGB(90, 90, 90), RGB(0, 0, 0), { 0.624695, 0, -0.780869}, true, { -0.7, -0.2, 0.0}, { -0.7, 0.2, 0.0}, {-0.2,  0.1, 0.4}, { -0.2, -0.1, 0.4});

	addModelToSc(false, 0, mWheel4, sc1);
	mWheel4->transform({ -4.5,0.0,3.2 }, { 0.0,3.14,0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	mWheel4->setVisibility(false);
	//-------------
	
	//model PointsPt
	srand(38);
	model3d* mPointsPt = new model3d;
	for (int a = 0; a < 200; a++) {
		mPointsPt->addObj(objTypes::obj_point, RGB(150, 150, 255), RGB(150, 150, 255), { 0, 1, 0 }, false, { double(rand() % 200), (rand() % 100) - 50.0, double(rand() % 100) - 50.0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 });
		mPointsPt->objects3d[a]->outlSize = 3;
	}
	mPointsPt->setVisibility(false);
	mPointsPt->transform({ 50.0,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
	addModelToSc(false, 0, mPointsPt, sc1);
	//-------------

	model3d* mCube = new model3d;
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { 0.0,  0.0, -1.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0,  1.0, -1.0 }, { 1.0,  1.0, -1.0 }, { 1.0, -1.0, -1.0 });
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { 0.0,  0.0,  1.0 }, true, { -1.0, -1.0,  1.0 }, { -1.0,  1.0,  1.0 }, { 1.0,  1.0,  1.0 }, { 1.0, -1.0,  1.0 });
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { -1.0,  0.0,  0.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0,  1.0, -1.0 }, { -1.0,  1.0,  1.0 }, { -1.0, -1.0,  1.0 });
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { 1.0,  0.0,  0.0 }, true, { 1.0, -1.0, -1.0 }, { 1.0,  1.0, -1.0 }, { 1.0,  1.0,  1.0 }, { 1.0, -1.0,  1.0 });
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { 0.0,  1.0,  0.0 }, true, { -1.0,  1.0, -1.0 }, { -1.0,  1.0,  1.0 }, { 1.0,  1.0,  1.0 }, { 1.0,  1.0, -1.0 });
	mCube->addObj(objTypes::obj_quad, RGB(150, 150, 255), RGB(0, 0, 0), { 0.0, -1.0,  0.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0, -1.0,  1.0 }, { 1.0, -1.0,  1.0 }, { 1.0, -1.0, -1.0 });
	mCube->transform({ 0.0,  0.0,  5.0 }, { 0.0,  0.0,  0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 });
	mCube->setVisibility(false);
	addModelToSc(false, 0, mCube, sc1);

	/*
	model3d* mCubeB = new model3d;
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { 0.0,  0.0, -1.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0,  1.0, -1.0 }, { 1.0,  1.0, -1.0 }, { 1.0, -1.0, -1.0 });
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { 0.0,  0.0,  1.0 }, true, { -1.0, -1.0,  1.0 }, { -1.0,  1.0,  1.0 }, { 1.0,  1.0,  1.0 }, { 1.0, -1.0,  1.0 });
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { -1.0,  0.0,  0.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0,  1.0, -1.0 }, { -1.0,  1.0,  1.0 }, { -1.0, -1.0,  1.0 });
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { 1.0,  0.0,  0.0 }, true, { 1.0, -1.0, -1.0 }, { 1.0,  1.0, -1.0 }, { 1.0,  1.0,  1.0 }, { 1.0, -1.0,  1.0 });
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { 0.0,  1.0,  0.0 }, true, { -1.0,  1.0, -1.0 }, { -1.0,  1.0,  1.0 }, { 1.0,  1.0,  1.0 }, { 1.0,  1.0, -1.0 });
	mCubeB->addObj(objTypes::obj_quad, RGB(0, 0, 0), RGB(150, 150, 255), { 0.0, -1.0,  0.0 }, true, { -1.0, -1.0, -1.0 }, { -1.0, -1.0,  1.0 }, { 1.0, -1.0,  1.0 }, { 1.0, -1.0, -1.0 });
	mCubeB->transform({ 0.0,  0.0,  5.0 }, { 0.0,  0.0,  0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 });
	mCubeB->setVisibility(false);
	//addModelToSc(false, 0, mCubeB, sc1);
	model3d* mCubesL[41] = {};
	for (int a = 0; a <= 40; a++) {
		mCubesL[a] = new model3d;
		copyModel(mCubeB, mCubesL[a]);
		mCubesL[a]->setVisibility(false);
		addModelToSc(false, 0, mCubesL[a], sc1);
	}
	model3d* mCubesR[41] = {};
	for (int a = 0; a <= 40; a++) {
		mCubesR[a] = new model3d;
		copyModel(mCubeB, mCubesR[a]);
		mCubesR[a]->setVisibility(false);
		addModelToSc(false, 0, mCubes[a], sc1);
	}*/

	//model Roadline
	model3d* mRoadline = new model3d;
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, {  3, -1, -0.5 }, {  3, -1, 0.5 }, {  9, -1, 0.5 }, {  9, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { 15, -1, -0.5 }, { 15, -1, 0.5 }, { 21, -1, 0.5 }, { 21, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { 27, -1, -0.5 }, { 27, -1, 0.5 }, { 33, -1, 0.5 }, { 33, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { 39, -1, -0.5 }, { 39, -1, 0.5 }, { 45, -1, 0.5 }, { 45, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { 51, -1, -0.5 }, { 51, -1, 0.5 }, { 57, -1, 0.5 }, { 57, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { 63, -1, -0.5 }, { 63, -1, 0.5 }, { 69, -1, 0.5 }, { 69, -1, -0.5 });

	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, {  -3, -1, -0.5 }, {  -3, -1, 0.5 }, {  -9, -1, 0.5 }, {  -9, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { -15, -1, -0.5 }, { -15, -1, 0.5 }, { -21, -1, 0.5 }, { -21, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { -27, -1, -0.5 }, { -27, -1, 0.5 }, { -33, -1, 0.5 }, { -33, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { -39, -1, -0.5 }, { -39, -1, 0.5 }, { -45, -1, 0.5 }, { -45, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { -51, -1, -0.5 }, { -51, -1, 0.5 }, { -57, -1, 0.5 }, { -57, -1, -0.5 });
	mRoadline->addObj(objTypes::obj_quad, RGB(30, 30, 30), RGB(0, 0, 0), { 0, 1, 0 }, true, { -63, -1, -0.5 }, { -63, -1, 0.5 }, { -69, -1, 0.5 }, { -69, -1, -0.5 });
	addModelToSc(true, 2, mRoadline, scBcgr);
	//-------------


	coord camL = { 0.0, 0.0, 0.0 };
	coord camR = { 0.0, 0.0, 0.0 };
	int key = -1;

	double rotator1 = 0.0;
	double rotator2 = 0.0;
	
	double delta_t = 0.0;

	/*
	while (true) {
		int start_t = clock();
		key = getKey();
		switch (key)
		{
		case 65: //left
			camL.x -= 0.1;
			break;
		case 68: //right
			camL.x += 0.1;
			break;
		case 32: //up
			camL.y += 0.1;
			break;
		case 16: //down
			camL.y -= 0.1;
			break;
		case 87: //forward
			camL.z += 0.1;
			break;
		case 83: //back
			camL.z -= 0.1;
			break;
		case 38: //Rf
			camR.x -= 0.1;
			break;
		case 40: //Rb
			camR.x += 0.1;
			break;
		case 37:
			camR.y += 0.1;
			break;
		case 39:
			camR.y -= 0.1;
			break;
		default:
			if (key!=-1) //printf_s("%d\n", key);
			break;
		}

		rotator1 += 1 * delta_t;
		if (rotator1 > 3.14 * 2) rotator1 = 0.0;
		rotator2 += 3 * delta_t;;
		if (rotator2 > 3.14 * 2) rotator2 = 0.0;

		SelectObject(hCmpDC, bbrush);
		SelectObject(hCmpDC, bpen);
		Rectangle(hCmpDC, 0, 0, int(center.x * 2), int(center.y * 2));

		
		q1->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		q2->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		q3->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		q4->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		q5->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		q6->transform({ 0.0, 0.0, 0.0 }, { rotator2, rotator1, rotator1 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		

		drawScene(sc1, 185, camL, camR);

		SetStretchBltMode(hdc, COLORONCOLOR);
		BitBlt(hdc, 0, 0, int(center.x * 2), int(center.y * 2), hCmpDC, 0, 0, SRCCOPY);

		int end_t = clock();
		delta_t = (double(end_t) - double(start_t)) / CLOCKS_PER_SEC;
	}
	*/


	rotator1 = 1.57;
	camL = { 0.0, 3.0, -5.0 };
	camR = { 0.0, 0.0, 0.0 };
	bbrush = CreateSolidBrush(RGB(0, 0, 0));

	while(key==-1){ 
		key = getKey(); 
	}

	PlaySound(TEXT("vvad_sfx.wav"), NULL, SND_ASYNC);
	int g_start_t = clock();
	int g_end_t = g_start_t;
	double g_time = (double(g_end_t) - double(g_start_t)) / CLOCKS_PER_SEC;

	float colCh = 0;

	while (g_time<15.0)//15.0)
	{
		int start_t = clock();

		rotator1 += 0.23 * delta_t;
		if (rotator1 > 3.14) rotator1 = 3.14;
		if (g_time > 5 && g_time < 9)
		{
			colCh += float(0.25 * delta_t);
			mVvTxt->setColor(RGB(0, 0, 0), RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh)));
			mVvTxt2->setColor(RGB(0, 0, 0), RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh)));
		}
		if (g_time > 12) {
			colCh -= float(0.4 * delta_t);
			if (colCh < 0) colCh = 0;
			mVvTxt->setColor(RGB(0, 0, 0), RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh)));
			mVvTxt2->setColor(RGB(0, 0, 0), RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh)));
			qVv1->colorOutl = RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh));
			qVv2->colorOutl = RGB(int(255 * colCh), int(255 * colCh), int(255 * colCh));
		}

		qVv1->transform({ 0.0, 0.0, 0.0 }, { 0.0, rotator1, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });
		qVv2->transform({ 0.0, 0.0, 0.0 }, { 0.0, rotator1, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 3.0 });

		SelectObject(hCmpDC, bbrush);
		SelectObject(hCmpDC, bpen);
		Rectangle(hCmpDC, 0, 0, int(center.x * 2), int(center.y * 2));
		drawScene(scVv, 60, camL, camR);
		SetStretchBltMode(hdc, COLORONCOLOR);
		BitBlt(hdc, 0, 0, int(center.x * 2), int(center.y * 2), hCmpDC, 0, 0, SRCCOPY);

		int end_t = clock();
		delta_t = (double(end_t) - double(start_t)) / CLOCKS_PER_SEC;
		g_end_t = clock();
		g_time = (double(g_end_t) - double(g_start_t)) / CLOCKS_PER_SEC;
	}

	delete mVvTxt;
	delete mVvTxt2;
	delete qVv1;
	delete qVv2;

	int camRacurs = 0, cubesAmount = 0;
	double colCH = 0, scaleCH = 1, colCH2 = 1;
	double offsRoad = 0.0, offsPt = 0.0, offsEnd = 0.0;

	camL = { -1.0, 1.448, -4.2 };

	//bbrush = CreateSolidBrush(RGB(200, 200, 200));
	PlaySound(TEXT("untitled33.wav"), NULL, SND_ASYNC);
	g_start_t = clock();
	g_end_t = g_start_t;
	g_time = (double(g_end_t) - double(g_start_t)) / CLOCKS_PER_SEC;
	
	while (g_time < 228) {
		int start_t = clock();

		rotator1 -= 10 * delta_t;
		offsRoad -= 40 * delta_t;
		if (offsRoad <= -12.0) offsRoad = 0.0;
		if (rotator1 < -3.14*2) rotator1 = 0.0;

		 mWheel->transform({ 6.5,0.0,-3.2 }, { 0.0, 0.0,  rotator1}, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		mWheel2->transform({-4.5,0.0,-3.2 }, { 0.0, 0.0,  rotator1}, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		mWheel3->transform({ 6.5,0.0,3.2  }, { 0.0, 3.14, rotator1}, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		mWheel4->transform({ -4.5,0.0,3.2 }, { 0.0, 3.14, rotator1}, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		mRoadline->transform({ offsRoad, 0.0, 0.0 }, { 0.0,0.0,0.0 }, { 1.0, 1.0, scaleCH }, { 0.0,0.0,0.0 });

		if ((g_time > 3) && (g_time < 15)) {
			camL.z -= 0.2 * delta_t;
			camL.x -= 0.17 * delta_t;
		}else
		if ((g_time > 15) && (camRacurs == 0)) {
			camRacurs = 1;
			camL = { -10.0, 1.0, -5.0 };
			camR = { 0.0, -1.1, 0.2 };
		}else
		if ((g_time > 15) && (g_time < 30)) {
			camL.z += 0.4 * delta_t;
		}else
		if ((g_time > 30) && (camRacurs == 1)) {
			camRacurs = 2;
			//mWheel3->setVisibility(true);
			//mWheel4->setVisibility(true);
			camL = { -9.0, 17.0, -5.0 };
			camR = { -1.3, 0.0, 0.0 };
		}else
		if ((g_time > 30) && (g_time < 60)) {
			camL.x += 0.5 * delta_t;
		}else
		if ((g_time >= 60) && (g_time < 61)) {
			camL.x -= 40 * delta_t;
		}
		if ((g_time > 61) && (camRacurs == 2)) {
			camRacurs = 3;
			//mWheel3->setVisibility(true);
			//mWheel4->setVisibility(true);
			//camL = {  13,  1.3, -5.0 };
			//camR = {  0.0, 1.1, 0.0 };
			camL = {  13,  1.0, 0.0 };
			camR = {  0.0, -3.14/2, 0.0 };
		}else
		if ((g_time > 61) && (g_time < 90)) {
			camL.x -= 1.3 * delta_t;
			if (camL.y<5)camL.y += 0.5 * delta_t;
		}
		if ((g_time > 75) && (g_time < 95)) {
			colCH = (sin(g_time) + 1) / 2.0;
			DeleteObject(bbrush);
			bbrush = CreateSolidBrush(RGB(int(255*(colCH)), int(255 * (colCH)), int(255 * (colCH))));
		}else
		if ((g_time > 100) && (g_time < 110)) {
			colCH -= 0.1 * delta_t;
			scaleCH -= 0.1 * delta_t;
			if (colCH < 0) colCH = 0;
			if (scaleCH < 0) scaleCH = 0;
			DeleteObject(bbrush);
			bbrush = CreateSolidBrush(RGB(int(255 * (colCH)), int(255 * (colCH)), int(255 * (colCH))));
			qRoad->transform({ 0.0,0.0,0.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0, scaleCH }, { 0.0,0.0,0.0 });
		}else
		if ((g_time > 110) && (camRacurs == 3)) {
			camRacurs = 4;
			camL = { 20.0, 1.0, -80.0 };
			camR = { 0.0, 0.0, 0.0 };
			mPointsPt->setVisibility(true);
		}else
		if ((g_time > 110) && (g_time < 125)) {
			camL.x -= 1 * delta_t;
			offsPt -= 3 * delta_t;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		}else
		if ((g_time >= 125) && (camRacurs == 4)) {
			camRacurs = 5;
			camL = { 5.0, 1.0, -8.0 };
			camR = { -0.2, 0.0, 0.0 };
			mRoadline->setColor(RGB(0, 0, 0), RGB(0, 0, 0));
			qRoad->color = RGB(0, 0, 0);
			qRoad->colorOutl = RGB(0, 0, 0);
			scaleCH = 1.0;
			qRoad->transform({ 0.0,0.0,0.0 }, { 0.0,0.0,0.0 }, { 1.0,1.0, scaleCH }, { 0.0,0.0,0.0 });
			colCH = 0;
		}else
		if ((g_time > 125) && (g_time < 140)) {
			colCH += 0.05 * delta_t;
			if (colCH > 1.0) colCH = 1;
			qRoad->colorOutl = RGB(int(150 * (colCH)), int(150 * (colCH)), int(255 * (colCH)));
			mRoadline->setColor(RGB(0, 0, 0), RGB(int(150 * (colCH)), int(150 * (colCH)), int(255 * (colCH))));

			offsPt -= 3 * delta_t;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			camL.z -= 2 * delta_t;
			camL.y += 2 * delta_t;
			camR.x -= 0.1 * delta_t;
			if (camR.x < -0.67) camR.x = -0.67;
			if (camL.y > 20.0) camL.y = 20.0;
			if (camL.z < -20.0) camL.z = -20.0;
			if (colCH2 > 0) colCH2 -= 0.5 * delta_t;
			mCube->setColor(RGB(int(150 * (colCH2)), int(150 * (colCH2)), int(255 * (colCH2))), RGB(0, 0, 0));
		}else
		if ((g_time >= 140) && (camRacurs == 5)) {
			camRacurs = 6;
			camL = { -5.0, 30.0, -1.0 };
			camR = { -1.2, 0.0, 0.0 };
			offsPt = -60;
		}else
		if ((g_time >= 140) && (g_time < 162.24)) {
			if (colCH2 > 0) colCH2 -= 0.5 * delta_t;
			if (colCH2 < 0) colCH2 = 0;
			mCube->setColor(RGB(int(150 * (colCH2)), int(150 * (colCH2)), int(255 * (colCH2))), RGB(0, 0, 0));
			offsPt -= 3 * delta_t;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			camR.z -= 0.2 * delta_t;
			camL.y += 0.4 * delta_t;
		}else
		if ((g_time > 162) && (camRacurs == 6)) {
			camRacurs = 7;
			camL = { 13,  1.3, -5.0 };
			camR = {  0.0, 1.1, 0.0 };
		}else
		if ((g_time > 162) && (g_time < 180)) {
			camL.z += 0.2 * delta_t;
			offsPt -= 3 * delta_t;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		}else
		if ((g_time > 180) && (camRacurs == 7)) {
			camRacurs = 8;
			camL = { 12,  1.3, -5.0 };
			camR = { 0.0, 1.3, 0.0 };
			offsPt = -80;
		}
		else
		if ((g_time > 180) && (g_time < 200)) {
			camL.x -= 0.3 * delta_t;
			offsPt -= 3 * delta_t;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
		}else
		if ((g_time > 200) && (camRacurs == 8)) {
				camRacurs = 9;
				camL = { 13,  1.0, 0.0 };
				camR = { 0.0, -3.14 / 2, 0.0 };
				offsPt = -80;
				qEnd->visibility = true;
				colCH = 0;
		}else
		if ((g_time > 200) && (g_time < 230)) {
			camL.x -= 1.3 * delta_t;
			if (camL.y < 5)camL.y += 0.5 * delta_t;
			offsPt -= 3 * delta_t;
			offsEnd -= 40 * delta_t;
			if (offsEnd < -1000) offsEnd = -1000;
			mPointsPt->transform({ 50.0 + offsPt,0.0,0.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			qEnd->transform({ offsEnd, 0.0, 0.0 }, { 0.0,0.0,0.0 }, { 1.0, 1.0, 1.0 }, { 0.0,0.0,0.0 });
			camR.x += 0.1 * delta_t;
		}

		if ((g_time > 220)) {
			colCH += 0.3 * delta_t;
			if (colCH > 1)  colCH = 1;
			DeleteObject(bbrush);
			//camR.z += 1 * delta_t;
			bbrush = CreateSolidBrush(RGB(int(255 * (colCH)), int(255 * (colCH)), int(255 * (colCH))));
		}
		if (g_time > 230) {
			
		}


		if ((g_time >= 137.928) && (cubesAmount <= 0)) {
			cubesAmount++;
			mCube->setVisibility(true);
			mCube->transform({ 10.0, 3.0, 10.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			colCH2 = 1;
		}
		if ((g_time >= 139.448) && (cubesAmount <= 1)) {
			cubesAmount++;
			//mCube->setVisibility(true);
			mCube->transform({ 14.0, 3.0, 15.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			colCH2 = 1;
		}
		if ((g_time >= 140.969) && (cubesAmount <= 2)) {
			cubesAmount++;
			//mCube->setVisibility(true);
			mCube->transform({ -2.0, 8.0, 5.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			colCH2 = 1;
		}
		if ((g_time >= 142.448) && (cubesAmount <= 3)) {
			cubesAmount++;
			//mCube->setVisibility(true);
			mCube->transform({ -4.0, 16.0, 6.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			colCH2 = 1;
		}
		if ((g_time >= 144.006) && (cubesAmount <= 4)) {
			cubesAmount++;
			//mCube->setVisibility(true);
			mCube->transform({ -6.0, 20.0, -6.0 }, { 0.0, 0.0, 0.0 }, { 1.0,1.0,1.0 }, { 0.0,0.0,0.0 });
			colCH2 = 1;
		}


		SelectObject(hCmpDC, bbrush);
		SelectObject(hCmpDC, bpen);
		Rectangle(hCmpDC, 0, 0, int(center.x * 2), int(center.y * 2));
		drawScene(scBcgr, 13, camL, camR);
		drawScene(sc1, 395, camL, camR);
		SetStretchBltMode(hdc, COLORONCOLOR);
		BitBlt(hdc, 0, 0, int(center.x * 2), int(center.y * 2), hCmpDC, 0, 0, SRCCOPY);

		int end_t = clock();
		delta_t = (double(end_t) - double(start_t)) / CLOCKS_PER_SEC;
		g_end_t = clock();
		g_time = (double(g_end_t) - double(g_start_t)) / CLOCKS_PER_SEC;// +115;
	}

	key = -1;

	Rectangle(hdc, 0, 0, int(center.x * 2), int(center.y * 2));

	while (key == -1) {
		key = getKey();
	}
	}