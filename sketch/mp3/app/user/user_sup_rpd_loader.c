/******************************************************************************
 * FileName: user_sup_rpd_loader.c
 * Description: Alternate SDK
 * Author: PV` (c) 2015
*******************************************************************************/

#include "user_config.h"

extern void call_user_start(void);
extern void Cache_Read_Disable(void);
extern void __real_Cache_Read_Enable(int a, int b, int c);

void user_start_trampoline(void)
{
	Cache_Read_Disable();
	call_user_start();
}

void __wrap_Cache_Read_Enable(int a, int b, int c)
{
	__real_Cache_Read_Enable(a, b, 0);
}
