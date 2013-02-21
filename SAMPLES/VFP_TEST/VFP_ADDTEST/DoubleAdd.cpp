//
// DoubleAdd.cpp
//

#include <windows.h>
#include <nkintr.h>

double VFPtest(double x, double y)
{    return x + y;
}
int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
    double val1 = 3.1234e-30; // denormalized number
    double val2 = 4.99e-32; // denormalized number
    double res1 = 0.0;

    while(1)
    {
        DWORD    RUNCOUNT = 1000000;
        while (RUNCOUNT--)
        {
            res1=VFPtest(val1,val2);
        }
        printf("Add result for val1 = 3.1234e-308 val2 = 4.99e-32 is %e\n", res1);
        Sleep(1);

    }

    return 0;
}
