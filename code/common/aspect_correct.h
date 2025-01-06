typedef enum {
	HSCALE_CENTER,
	HSCALE_STRETCH,
	HSCALE_LEFT,
	HSCALE_RIGHT,
} uiHorizontalMode_t;

typedef enum {
	VSCALE_CENTER,
	VSCALE_STRETCH,
	VSCALE_TOP,
	VSCALE_BOTTOM,
} uiVerticalMode_t;

void AspectCorrect_SetMode( uiHorizontalMode_t hMode, uiVerticalMode_t vMode );
void AspectCorrect_SetLoadingMode( uiHorizontalMode_t hMode, uiVerticalMode_t vMode, qboolean overlay );
void AspectCorrect_ResetMode( void );
void AspectCorrect_AdjustFrom640( float *x, float *y, float *w, float *h );
void AspectCorrect_DrawAdjustedStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
qboolean AspectCorrect_UseCorrectedGunPosition( void );
qboolean AspectCorrect_UseAdjustedIntermissionFov( void );
float AspectCorrect_WidthScale( void );
void AspectCorrect_RunFrame( void );
void AspectCorrect_Shutdown( void );
void AspectCorrect_Init( int width, int height );
