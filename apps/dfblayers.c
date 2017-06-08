#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <direct/direct.h>
#include <direct/messages.h>

#include <directfb.h>
#include <directfb_util.h>

#define FONT  "/usr/share/directfb-1.7.7/decker.dgiff"

#include <direct/clock.h>
#include <direct/debug.h>

typedef struct {
     int       magic;

    unsigned int stat_total;
    unsigned int stat_idle;
     float     idle;
     long long idle_time;
     char      idle_string[20];
} IdleData;

typedef struct statData
{
    char cpu[3];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
    unsigned int un1;
    unsigned int un2;
} statData;

static void
idle_read( unsigned int* total, unsigned int* idle)
{

     const char* statFormat ="%s %d %d %d %d %d %d %d %d %d";
     FILE *procStat;
     statData data;

     procStat = fopen("/proc/stat","r");

     if (procStat)
     {
         if (fscanf(procStat,statFormat,&data.cpu,&data.user,&data.nice,&data.system,&data.idle,&data.iowait,&data.irq,&data.softirq,&data.un1,&data.un2))
         {
             fclose(procStat);
             *total = data.user + data.nice + data.system + data.idle + data.iowait + data.irq + data.softirq + data.un1 + data.un2;
             *idle = data.idle;
         }
     }
}

static inline void
idle_init( IdleData *data )
{

     D_ASSERT( data != NULL );

     memset( data, 0, sizeof(IdleData) );

     idle_read(&data->stat_total, &data->stat_idle);

     data->idle_time = direct_clock_get_millis();

     D_MAGIC_SET( data, IdleData );
}

static inline void
idle_count( IdleData *data,
           int      interval )
{
     long long diff;
     long long now = direct_clock_get_millis();

     D_MAGIC_ASSERT( data, IdleData );

     diff = now - data->idle_time;
     if (diff >= interval) {

         unsigned int total = 0, idle = 0;

         idle_read(&total, &idle);

         if (idle != data->stat_idle)
             data->idle = ((float)(idle - data->stat_idle) / (total - data->stat_total)) * 100;
         else
             data->idle = 0;

          snprintf( data->idle_string, sizeof(data->idle_string), "%.1f", data->idle );

          data->stat_idle = idle;
          data->stat_total = total;

          data->idle_time = now;
     }
}

static IDirectFB       *g_dfb;
static IDirectFBScreen *g_screen;
static IDirectFBFont   *g_font;

static DFBDimension     g_screen_size;

typedef struct {
     IDirectFBDisplayLayer *layer;
     IDirectFBSurface      *surface;

     int                    dx;
     int                    dy;

     int                    x;
     int                    y;
     int                    width;
     int                    height;
} Plane;


static Plane g_planes[64];
static int   g_plane_count;


static DFBResult
InitializeLayer( DFBDisplayLayerID  layer_id,
                 int                width,
                 int                height,
                 Plane             *plane )
{
     static DFBColor colors[8] = {
          { 0xff, 0x00, 0x00, 0xff },
          { 0xff, 0xff, 0x00, 0x00 },
          { 0xff, 0xff, 0xff, 0x00 },
          { 0xff, 0x00, 0xff, 0x00 },
          { 0xff, 0xff, 0x00, 0xff },
          { 0xff, 0x00, 0xff, 0xff },
          { 0xff, 0xff, 0xff, 0xff },
          { 0xff, 0x80, 0x80, 0x80 }
     };

     DFBResult              ret;
     DFBDisplayLayerConfig  config;
     IDirectFBDisplayLayer *layer   = NULL;
     IDirectFBSurface      *surface = NULL;
     char                   buf[256];
     DFBSurfacePixelFormat  format = 0;

     D_INFO( "dfbtest_layers: Initializing layer with ID %d...\n", layer_id );


     ret = g_dfb->GetDisplayLayer( g_dfb, layer_id, &layer );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFB::GetDisplayLayer() failed!\n" );
          goto error;
     }

     ret = layer->SetCooperativeLevel( layer, DLSCL_EXCLUSIVE );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFBDisplayLayer::SetCooperativeLevel() failed!\n" );
          goto error;
     }

     config.flags       = DLCONF_PIXELFORMAT;
//   if ( layer_id == 1)
//        config.pixelformat = DSPF_NV12;
//   else
          config.pixelformat = DSPF_ARGB;

     if (layer->TestConfiguration( layer, &config, NULL ) == DFB_OK)
          format = config.pixelformat;
     else
          format = DSPF_RGB32;


//   if (layer->TestConfiguration( layer, &config, NULL ) == DFB_OK) {
//     format = DSPF_ARGB;
//
//         config.options = DLOP_ALPHACHANNEL;
//   }
//   else
           config.options = DLOP_NONE;

     /* Fill in configuration. */
	   config.flags        = DLCONF_BUFFERMODE | DLCONF_OPTIONS | DLCONF_SURFACE_CAPS;
     if (width && height) {
          config.flags |= DLCONF_WIDTH | DLCONF_HEIGHT;

          config.width   = width;
          config.height  = height;

          plane->x       = (layer_id * 200) % (g_screen_size.w - width);
          plane->y       = (layer_id * 200) % (g_screen_size.h - height);

     }
     else {
          plane->x       = 0;
          plane->y       = 0;
     }

     config.buffermode   = DLBM_BACKSYSTEM/*DLBM_FRONTONLY*/;
     config.surface_caps = DSCAPS_NONE;

     if (format) {
          config.flags       |= DLCONF_PIXELFORMAT;
          config.pixelformat  = format;
     }


     /* Set new configuration. */
     ret = layer->SetConfiguration( layer, &config );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFBDisplayLayer::SetConfiguration() failed!\n" );
          goto error;
     }

     layer->SetOpacity( layer, 0xbb );

     /* Get the layer surface. */
     ret = layer->GetSurface( layer, &surface );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFBDisplayLayer::GetSurface() failed!\n" );
          goto error;
     }

     surface->GetPixelFormat( surface, &format );

     surface->GetSize( surface, &width, &height );

     surface->SetDrawingFlags( surface, DSDRAW_SRC_PREMULTIPLY );

     surface->SetColor( surface, colors[layer_id].r, colors[layer_id].g, colors[layer_id].b, 0xbb );
     surface->FillRectangle( surface, 0, 0, width, height );

     surface->SetColor( surface, colors[layer_id].r, colors[layer_id].g, colors[layer_id].b, 0x99 );
     surface->FillRectangle( surface, 50, 50, width - 100, height - 100);

     surface->SetColor( surface, 0xff, 0xff, 0xff, 0xff );
     surface->SetFont( surface, g_font );

     snprintf( buf, sizeof(buf), "Plane %u %dx%d (%s)", layer_id, width, height, dfb_pixelformat_name(format) );
     surface->DrawString( surface, buf, -1, 20, 20, DSTF_TOPLEFT );

     surface->Flip( surface, NULL, DSFLIP_NONE );


     plane->layer   = layer;
     plane->surface = surface;

     plane->dx      = (layer_id & 1) ? 1 : -1;
     plane->dy      = (layer_id & 2) ? 1 : -1;

     plane->width   = width;
     plane->height  = height;

     return DFB_OK;


error:
     if (layer)
          layer->Release( layer );

     return ret;
}


static DFBEnumerationResult
DisplayLayerCallback( DFBDisplayLayerID           layer_id,
                      DFBDisplayLayerDescription  desc,
                      void                       *callbackdata )
{
	if (layer_id == 0) {
		if (InitializeLayer( layer_id, 800, 480, &g_planes[g_plane_count] ) == DFB_OK) {
			g_plane_count++;
		}
		else {
			D_INFO( "Layer id %d could not be set to 400x400, trying fullscreen\n", layer_id );
			if (InitializeLayer( layer_id, 0, 0, &g_planes[g_plane_count] ) == DFB_OK)
				g_plane_count++;
		}
	}
	else
	{
		if (InitializeLayer( layer_id, 250, 200, &g_planes[g_plane_count] ) == DFB_OK) {
			g_plane_count++;
		}
		else {
			D_INFO( "Layer id %d could not be set to 400x400, trying fullscreen\n", layer_id );
			if (InitializeLayer( layer_id, 0, 0, &g_planes[g_plane_count] ) == DFB_OK)
				g_plane_count++;
		}
	}

	return DFENUM_OK;
}

static IdleData idle;
static int fontheight;

static void
TickPlane( Plane *plane )
{
     DFBDisplayLayerID id;
     plane->layer->GetID(plane->layer, &id);
     if (id == 0)
     {
          char buf[32];
	  int ret;
	  IDirectFBSurface      *surface = NULL;
	  DFBRegion region =
		  { plane->width-200, 20, plane->width-200 + 190, 20+fontheight+5 };

	  ret = plane->layer->GetSurface( plane->layer, &surface );
	  if (ret) {
	       D_DERROR( ret, "dfbtest_layers: IDirectFBDisplayLayer::GetSurface() failed!\n" );
	  }
	  surface->SetDrawingFlags( surface, DSDRAW_SRC_PREMULTIPLY );

	  surface->SetColor( surface, 0, 0, 0xff, 0xbb );
	  surface->FillRectangle( surface, plane->width-200, 20, 190, fontheight+5 );

	  snprintf( buf, sizeof(buf), "CPU Idle: %s%%", idle.idle_string );

	  surface->SetColor( surface, 255, 255, 255, 0xff );
	  surface->SetFont( surface, g_font );

	  surface->DrawString( surface, buf, -1, plane->width-195, 20, DSTF_LEFT | DSTF_TOP );

	  surface->Flip( surface, &region, DSFLIP_WAIT);
	  surface->Release(surface);
     }
#if 0
     else if (id == 2)
     {
	     printf("rotate\n");
	     static int rotate = 0;
	     rotate += 90;
	     if (rotate >= 360)
		     rotate = 0;
	     plane->layer->SetRotation(plane->layer, rotate);
     }
#endif

     if (plane->x == g_screen_size.w - plane->width)
          plane->dx = -1;
     else if (plane->x == 0)
          plane->dx = 1;

     if (plane->y == g_screen_size.h - plane->height)
          plane->dy = -1;
     else if (plane->y == 0)
          plane->dy = 1;

     plane->x += plane->dx;
     plane->y += plane->dy;

     plane->layer->SetScreenPosition( plane->layer, plane->x, plane->y );
     //plane->surface->Flip( plane->surface, 0, 0);
}

int
main( int argc, char *argv[] )
{
     DFBResult          ret;
     DFBFontDescription fdsc;
     int                i;

     direct_initialize();


     D_INFO( "dfbtest_layers: Starting test program...\n" );

     ret = DirectFBInit( &argc, &argv );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: DirectFBInit() failed!\n" );
          return ret;
     }

     ret = DirectFBCreate( &g_dfb );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: DirectFBCreate() failed!\n" );
          return ret;
     }

     ret = g_dfb->GetScreen( g_dfb, DSCID_PRIMARY, &g_screen );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFB::GetScreen( PRIMARY ) failed!\n" );
          return ret;
     }

     g_screen->GetSize( g_screen, &g_screen_size.w, &g_screen_size.h );

     D_INFO( "dfbtest_layers: Screen size is %ux%u\n", g_screen_size.w, g_screen_size.h );

     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = 18;

     ret = g_dfb->CreateFont( g_dfb, FONT, &fdsc, &g_font );
     if (ret) {
          D_DERROR( ret, "dfbtest_layers: IDirectFB::CreateFont( %s ) failed!\n", FONT );
          return ret;
     }

     g_font->GetHeight( g_font, &fontheight );

     idle_init( &idle );

     g_dfb->EnumDisplayLayers( g_dfb, DisplayLayerCallback, NULL );

     while (1) {
	     idle_count( &idle, 1000 );

	     for (i=0; i<g_plane_count; i++)
               TickPlane( &g_planes[i] );

	     usleep( 10000 );
     }


     g_font->Release( g_font );
     g_dfb->Release( g_dfb );

     direct_shutdown();

     return 0;
}
