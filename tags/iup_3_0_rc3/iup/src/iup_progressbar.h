/** \file
 * \brief ProgressBar Control
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_PROGRESSBAR_H 
#define __IUP_PROGRESSBAR_H

#ifdef __cplusplus
extern "C" {
#endif


struct _IcontrolData
{
  int dashed,
      marquee;

  double value,  /* value is min < value < max */
         vmin,
         vmax;
};

void  iProgressBarCropValue(Ihandle* ih);
char* iProgressBarGetValueAttrib(Ihandle* ih);
char* iProgressBarGetDashedAttrib(Ihandle* ih);

void iupdrvProgressBarInitClass(Iclass* ic);

#ifdef __cplusplus
}
#endif

#endif
