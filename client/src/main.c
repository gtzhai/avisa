#include <visa.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv)
{
    ViSession rm = 0;
    ViStatus s = viOpenDefaultRM (&rm);
    if(s != VI_SUCCESS){
        printf("rm open failed\n");
        return -1;
    }

    ViFindList fl = 0;
    uint32_t cnt = 0;
    char buf[256];
    //s = viFindRsrc(rm, "USB0::.*::INSTR", &fl, &cnt, buf);
    s = viFindRsrc(rm, "TCPIP0::.*::SOCKET", &fl, &cnt, buf);
    if(s != VI_SUCCESS){
        printf("find rsrc failed\n");
        return -1;
    }

    printf("find %d visa device\n", cnt);

    if(cnt > 0)
    {
        ViSession se = 0;
        s = viOpen(rm, buf, 0, 0, &se);
        if(s == VI_SUCCESS){
            unsigned char buf[256];
            s = viWrite(se, "*IDN?\n", 6, &cnt);
            if(s != VI_SUCCESS){
                printf("write failed\n");
                return -1;
            } else {
                printf("write %d bytes\n", cnt);
            }
            memset(buf, 0, sizeof(buf));
            s = viRead(se, buf, sizeof(buf), &cnt);
            if(s != VI_SUCCESS){
                printf("read failed\n");
                return -1;
            } else {
                printf("got %d bytes\n", cnt);
                printf("%s\n", buf);
            }

            viClose(se);
        }
        else
        {
            printf("open rsrc failed\n");
        }
    }
    viClose(fl);
    viClose(rm);
    return 0;
}

