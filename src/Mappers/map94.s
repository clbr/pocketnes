#include "../equates.h"
MAPPER_OVERLAY_TEXT(6)

	global_func mapper94init

@----------------------------------------------------------------------------
mapper94init:	@Capcom - Senjou no Ookami
@----------------------------------------------------------------------------
	.word void,void,void,write94
	mov pc,lr
@-------------------------------------------------------
write94:
@-------------------------------------------------------
	mov r0,r0,lsr#2
	b_long map89AB_
@-------------------------------------------------------
	@.end
