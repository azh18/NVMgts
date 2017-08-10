#include "MBB.h"
#include <cstdlib>
#include <random>
#include <vector>
#include <cmath>

using namespace std;


MBB::MBB()
{
	xmin = 0;
	xmax = 0;
	ymin = 0;
	ymax = 0;
}

MBB::MBB(float val_xmin,float val_ymin,float val_xmax,float val_ymax)
{
	xmin = val_xmin;
	xmax = val_xmax;
	ymin = val_ymin;
	ymax = val_ymax;
}

bool MBB::pInBox(float x, float y)
{
	if (x <= xmax &&x >= xmin&&y <= ymax&&y >= ymin)
		return true;
	else
		return false;
}

int BoxIntersect(MBB& a1, MBB& b1)
{
	MBB a, b;
	bool swaped = false;
	if (a1.xmin < b1.xmin)
	{
		a = a1;
		b = b1;
	}
	else if (a1.xmin == b1.xmin)
	{
		if (a1.xmax > b1.xmax)
		{
			a = a1;
			b = b1;
		}
		else
		{
			b = a1;
			a = b1;
			swaped = true;
		}
	}
	else
	{
		b = a1;
		a = b1;
		swaped = true;
	}
	if (b.xmin >= a.xmax)
		return 0;
	else
	{
		if (b.ymax <= a.ymin)
			return 0;
		else if (b.ymin >= a.ymax)
			return 0;
		else
		{
			if (a.pInBox(b.xmin, b.ymin) && a.pInBox(b.xmin, b.ymax) && a.pInBox(b.xmax, b.ymin) && a.pInBox(b.xmax, b.ymax))
			{
				if (!swaped)
					return 2;
				else
					return 3;
			}
			else
				return 1;
		}
	}
}
/* return 0:不相交
return 1:相交但不包含
return 2:a1包含b1
return 3:b1包含a1
*/

int MBB::intersect(MBB& b)
{
	return (BoxIntersect(*this, b));
}
/* return 0:不相交
   return 1:相交但不包含
   return 2:this包含b
   return 3:b包含this
*/

int MBB::randomGenerateInnerMBB(MBB *generated, int generateNum)
{
	float minx, maxx, miny, maxy;
	minx = this->xmin;
	miny = this->ymin;
	maxx = this->xmax;
	maxy = this->ymax;
	default_random_engine generator;
        // query center coordinate
        normal_distribution<double> x(121.46,0.03);
        normal_distribution<double> y(31.22,0.01);
        // query MBB length
        normal_distribution<double> a(0.01,0.005);
        normal_distribution<double> b(0.01,0.005);

	for(int i=0; i<=generateNum-1; i++)
	{
		float x1, x2;
		float y1, y2;
		x1 = x(generator);
		y1 = y(generator);

		x1 = x1 > maxx? maxx : x1;
                x1 = x1 < minx ? minx : x1;
                y1 = y1 > maxy? maxy : y1;
                y1 = y1 < miny? miny : y1;

		x2 = x1 + fabs(a(generator));
		y2 = y1 + fabs(b(generator));

                x2 = x2 > maxx? maxx : x2;
                x2 = x2 < minx ? minx : x2;
                y2 = y2 > maxy? maxy : y2;
                y2 = y2 < miny? miny : y2;

		generated[i].xmin = x1;
		generated[i].xmax = x2;
		generated[i].ymin = y1;
		generated[i].ymax = y2;
	}
	return 0;
}

MBB::~MBB()
{
}
