//
// DoubleMult.cpp
//

#include <windows.h>
#include <nkintr.h>

double VFPtest(double x, double y)
{    return x * y;
}
int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
    double val1 = 3.1234e-32; // denormalized number
    double val2 = 7.99e-32; // denormalized number
    double res1 = 0.0;
    while(1)
    {
        DWORD    RUNCOUNT = 1000000;
        while (RUNCOUNT--)
        {
            res1=VFPtest(val1,val2);
        }
        printf("VFP Multiplication (3.1234e-322 * 7.99e-32) = %e\n", res1);
        Sleep(1);
    }
    return 0;
}

