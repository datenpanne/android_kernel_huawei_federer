/*< DTS2015012606635 tangying/205982 20150126 begin*/
/*add camera ois driver*/
/*< DTS2015012906386 tangying/205982 20150130 begin*/
/*adjust back camera resolution and optimize ois driver*/
#ifndef _IMX278_SUNNY_OIS_HEAD
#define _IMX278_SUNNY_OIS_HEAD

#include "OisDef.h"

void imx278_sunny_InitOISData(void *oisdata, uint32_t oislenth);
int imx278_sunny_ois_init(void);
int imx278_sunny_ois_initSpecial(void);
int imx278_sunny_ois_turn_onoff(uint32_t on);
/*< DTS2015042909943 ganfan/206212 20150505 begin*/
int imx278_sunny_ois_turn_onoff_lin(uint32_t on);
/*DTS2015042909943 ganfan/206212 20150505 end >*/
//for test interface
int imx278_sunny_ois_AF_SetPos(short);
int imx278_sunny_ois_RamAccFixMod(uint8_t);
int imx278_sunny_ois_RtnCen(uint8_t);
int imx278_sunny_ois_SetPanTiltMode(uint8_t);
int imx278_sunny_ois_RunGea(void);
int imx278_sunny_ois_RunHea(void);
int imx278_sunny_ois_OisEna(void);
/*< DTS2015042007464 ganfan/206212 20150420 begin*/
int imx278_sunny_ois_MagnetismRead(int32_t *otpcenX, int32_t *otpcenY, int32_t *srvoffX, int32_t *srvoffY);
int imx278_sunny_ois_StbOnnN( uint8_t UcStbY , uint8_t UcStbX );
/*DTS2015042007464 ganfan/206212 20150420 end >*/
/* <DTS2015073106553 xurenjie xwx208548 20150731 begin */
int imx278_sunny_ois_setGyroGain(int32_t xgain, int32_t ygain);
int imx278_sunny_ois_getGyroGain(int32_t* xgain, int32_t* ygain);
/* DTS2015073106553 xurenjie xwx208548 20150731 end > */
#endif
/*DTS2015012906386 tangying/205982 20150130 end >*/
/*DTS2015012606635 tangying/205982 20150126 end >*/
