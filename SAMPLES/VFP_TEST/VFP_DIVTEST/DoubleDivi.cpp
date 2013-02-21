//
// DoubleDivi.cpp
//

#include <windows.h>
#include <nkintr.h>

double VFPtest(double x, double y)
{    return x / y;
}
int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
    double val1 = 22.0; // denormalized number
    double val2 = 0.0; // denormalized number
    double res1 = 0.0;
    int x = 0;
    // one time Exception test is suffcient about Divive by zero
    printf(" TEST FPV Exception For Division By Zero (22.0/0.0)%e\n", VFPtest(val1,val2));
    printf(" TEST FPV Exception For Division By Zero (22.0/0)%e\n", val1/x);        
    
    while(1)
    {
        DWORD    RUNCOUNT = 1000000;
        while (RUNCOUNT--)
        {
            res1 = VFPtest(val1, 7.0);
        }
        printf("Test 22.0/7.0 %e\n", res1);
        Sleep(1);        
    }

    return 0;
}
