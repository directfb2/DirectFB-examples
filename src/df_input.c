/*
   This file is part of DirectFB-examples.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <direct/util.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "joystick.h"
#include "keys.h"
#include "mouse.h"
#endif

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x)                                                   \
     do {                                                             \
          DFBResult ret = x;                                          \
          if (ret != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, ret );                         \
          }                                                           \
     } while (0)

/* main interface */
static IDirectFB *dfb = NULL;

/* event buffer interface */
static IDirectFBEventBuffer *event_buffer = NULL;

/* primary surface */
static IDirectFBSurface *primary = NULL;

/* fonts */
static IDirectFBFont *font_small  = NULL;
static IDirectFBFont *font_normal = NULL;
static IDirectFBFont *font_large  = NULL;

/* images */
typedef enum {
     JOYSTICK,
     KEYS,
     MOUSE,
     NUM_IMAGES
} Image;

#ifdef USE_IMAGE_HEADERS
static const void *image_data[] = {
     GET_IMAGEDATA( joystick ), GET_IMAGEDATA( keys ), GET_IMAGEDATA( mouse )
};
static unsigned int image_size[] = {
     GET_IMAGESIZE( joystick ), GET_IMAGESIZE( keys ), GET_IMAGESIZE( mouse )
};
#else
static const char *joystick() { return GET_IMAGEFILE( joystick ); }
static const char *keys()     { return GET_IMAGEFILE( keys );     }
static const char *mouse()    { return GET_IMAGEFILE( mouse );    }

static const char *(*image_file[])() = {
     joystick, keys, mouse
};
#endif

static IDirectFBSurface *images[NUM_IMAGES] = { NULL, NULL, NULL };

/* screen width and height */
static int screen_width, screen_height;

/* mouse information */
static int  mouse_x[10], mouse_y[10];
static bool mouse_pressure[10];

/* joystick information */
static int joy_axis[8];

/* command line options */
static int max_slots = 1;

#ifdef HAVE_MT
#define DFB_EVENT_SLOT_ID(e) e->slot_id
#else
#define DFB_EVENT_SLOT_ID(e) 0
#endif

/**************************************************************************************************/

static const DirectFBKeySymbolNames(symbolnames);
static const DirectFBKeyIdentifierNames(idnames);

static int compare_symbol( const void *a, const void *b )
{
     const DFBInputDeviceKeySymbol *symbol     = a;
     const struct DFBKeySymbolName *symbolname = b;

     return *symbol - symbolname->symbol;
}

static int compare_id( const void *a, const void *b )
{
     const DFBInputDeviceKeyIdentifier *id     = a;
     const struct DFBKeyIdentifierName *idname = b;

     return *id - idname->identifier;
}

static void show_key_modifier_state( DFBInputEvent *evt )
{
     static struct {
          DFBInputDeviceModifierMask  modifier;
          const char                 *name;
          int                         x;
     } modifiers[] = {
          { DIMM_SHIFT,   "Shift", 0 },
          { DIMM_CONTROL, "Ctrl",  0 },
          { DIMM_ALT,     "Alt",   0 },
          { DIMM_ALTGR,   "AltGr", 0 },
          { DIMM_META,    "Meta",  0 },
          { DIMM_SUPER,   "Super", 0 },
          { DIMM_HYPER,   "Hyper", 0 }
     };
     static int y = 0;
     int        i, w;

     if (!(evt->flags & DIEF_MODIFIERS))
          return;

     if (!y) {
          y = 2 * screen_height / 3 + 20;

          modifiers[0].x = 40;

          for (i = 0; i < D_ARRAY_SIZE(modifiers) - 1; i++) {
               DFBCHECK(font_normal->GetStringWidth( font_small, modifiers[i].name, -1, &w ));
               modifiers[i+1].x = modifiers[i].x + w + 20;
          }
     }

     primary->SetFont( primary, font_small );

     for (i = 0; i < D_ARRAY_SIZE(modifiers); i++) {
          if (evt->modifiers & modifiers[i].modifier)
               primary->SetColor( primary, 0x90, 0x30, 0x90, 0xFF );
          else
               primary->SetColor( primary, 0x20, 0x20, 0x20, 0xFF );

          primary->DrawString( primary, modifiers[i].name, -1, modifiers[i].x, y, DSTF_TOPLEFT );
     }
}

static void show_key_lock_state( DFBInputEvent *evt )
{
     static struct {
          DFBInputDeviceLockState  lock;
          const char              *name;
          int                      x;
     } locks[] = {
          { DILS_SCROLL, "ScrollLock", 0 },
          { DILS_NUM,    "NumLock",    0 },
          { DILS_CAPS,   "CapsLock",   0 },
     };
     static int y = 0;
     int        i, w;

     if (!(evt->flags & DIEF_LOCKS))
          return;

     if (!y) {
          y = screen_height - 40;

          DFBCHECK(font_normal->GetStringWidth( font_normal, locks[D_ARRAY_SIZE(locks)-1].name, -1, &w ));
          locks[D_ARRAY_SIZE(locks)-1].x = screen_width - 40 - w;

          for (i = D_ARRAY_SIZE(locks) - 1; i > 0; i--) {
               DFBCHECK(font_normal->GetStringWidth( font_normal, locks[i-1].name, -1, &w ));
               locks[i-1].x = locks[i].x - w - 20;
          }
     }

     primary->SetFont( primary, font_normal );

     for (i = 0; i < D_ARRAY_SIZE(locks); i++) {
          if (evt->locks & locks[i].lock)
               primary->SetColor( primary, 0x90, 0x30, 0x90, 0xFF );
          else
               primary->SetColor( primary, 0x20, 0x20, 0x20, 0xFF );

          primary->DrawString( primary, locks[i].name, -1, locks[i].x, y, DSTF_LEFT );
     }
}

static void show_key_event( DFBInputEvent *evt )
{
     static DFBInputDeviceKeyIdentifier  last_id = DIKI_UNKNOWN;
     static int                          count   = 0;
     char                                buf[16];
     struct DFBKeySymbolName            *symbol_name;
     struct DFBKeyIdentifierName        *id_name;

     if (DFB_KEY_TYPE( evt->key_symbol ) == DIKT_UNICODE) {
          primary->SetFont( primary, font_large );
          primary->SetColor( primary, 0x70, 0x80, 0xE0, 0xFF );
          primary->DrawGlyph( primary, evt->key_symbol, screen_width / 2, screen_height / 2, DSTF_LEFT );
     }

     symbol_name = bsearch( &evt->key_symbol, symbolnames, D_ARRAY_SIZE(symbolnames) - 1, sizeof(symbolnames[0]),
                            compare_symbol );

     primary->SetFont( primary, font_normal );

     if (symbol_name) {
          primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
          primary->DrawString( primary, symbol_name->name, -1, 40, screen_height / 3, DSTF_LEFT );
     }

     primary->SetColor( primary, 0x60, 0x60, 0x60, 0xFF );
     snprintf( buf, sizeof(buf), "0x%X", evt->key_symbol );
     primary->DrawString( primary, buf, -1, screen_width - 40, screen_height / 3, DSTF_RIGHT );

     primary->SetFont( primary, font_small );

     primary->SetColor( primary, 0x80, 0x80, 0x80, 0xFF );
     snprintf( buf, sizeof(buf), "%d", evt->key_code );
     primary->DrawString( primary, buf, -1, screen_width - 40, screen_height / 4, DSTF_RIGHT );

     primary->SetFont( primary, font_normal );

     id_name = bsearch( &evt->key_id, idnames, D_ARRAY_SIZE(idnames) - 1, sizeof(idnames[0]),
                        compare_id );

     if (id_name) {
          primary->SetColor( primary, 0x60, 0x60, 0x60, 0xFF );
          primary->DrawString( primary, id_name->name, -1, 40, 2 * screen_height / 3, DSTF_LEFT );
     }

     show_key_modifier_state( evt );
     show_key_lock_state( evt );

     primary->SetFont( primary, font_normal );

     if (evt->type == DIET_KEYPRESS) {
          if (evt->key_id != DIKI_UNKNOWN && evt->key_id == last_id)
               count++;
          else
               count = 0;
          last_id = evt->key_id;
     }
     else {
          count = 0;
          last_id = DIKI_UNKNOWN;
     }

     primary->SetColor( primary, 0x60, 0x60, 0x60, 0xFF );

     if (count > 0) {
          snprintf( buf, sizeof(buf), "%dx PRESS", count + 1 );
          primary->DrawString( primary, buf, -1,
                               screen_width - 40, 2 * screen_height / 3, DSTF_RIGHT );
     }
     else
          primary->DrawString( primary, (evt->type == DIET_KEYPRESS) ?  "PRESS" : "RELEASE", -1,
                               screen_width - 40, 2 * screen_height / 3, DSTF_RIGHT );


     if (evt->key_symbol == DIKS_ESCAPE || evt->key_symbol == DIKS_EXIT) {
          primary->SetFont( primary, font_small );
          primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
          primary->DrawString( primary, "Press ESC/EXIT again to quit.", -1,
                               screen_width / 2, screen_height / 6, DSTF_CENTER );
     }
}

/**************************************************************************************************/

static void show_mouse_buttons( DFBInputEvent *evt )
{
     static struct {
          DFBInputDeviceButtonMask  mask;
          const char               *name;
          int                       x;
     } buttons[] = {
          { DIBM_LEFT,   "Left",   0 },
          { DIBM_MIDDLE, "Middle", 0 },
          { DIBM_RIGHT,  "Right",  0 },
     };
     static int y = 0;
     char       str[64];
     int        i, w;

     if (!y) {
          y = screen_height - 40;

          DFBCHECK(font_normal->GetStringWidth( font_normal, buttons[D_ARRAY_SIZE(buttons)-1].name, -1, &w ));
          buttons[D_ARRAY_SIZE(buttons)-1].x = screen_width - 40 - w;

          for (i = D_ARRAY_SIZE(buttons) - 1; i > 0; i--) {
               DFBCHECK(font_normal->GetStringWidth( font_normal, buttons[i-1].name, -1, &w ));
               buttons[i-1].x = buttons[i].x - w - 20;
          }
     }

     primary->SetFont( primary, font_normal );

     for (i = 0; i < D_ARRAY_SIZE(buttons); i++) {
          if (evt->flags & DIEF_BUTTONS && evt->buttons & buttons[i].mask)
               primary->SetColor( primary, 0x90, 0x30, 0x90, 0xFF );
          else
               primary->SetColor( primary, 0x20, 0x20, 0x20, 0xFF );

          primary->DrawString( primary, buttons[i].name, -1, buttons[i].x, y, DSTF_LEFT );
     }

     if (max_slots > 1)
          snprintf( str, sizeof(str), "Slot %d (%d,%d)", DFB_EVENT_SLOT_ID(evt),
                    mouse_x[DFB_EVENT_SLOT_ID(evt)], mouse_y[DFB_EVENT_SLOT_ID(evt)] );
     else
          snprintf( str, sizeof(str), "(%d,%d)", mouse_x[0], mouse_y[0] );

     primary->SetFont( primary, font_small );

     DFBCHECK(font_small->GetStringWidth( font_small, str, -1, &w ));
     primary->SetColor( primary, 0xF0, 0xF0, 0xF0, 0xFF );
     primary->DrawString( primary, str, -1, screen_width - 40 - w, y + 32, DSTF_LEFT );
}

static void show_mouse_event( DFBInputEvent *evt )
{
     int  i;
     char buf[32];

     show_mouse_buttons( evt );

     primary->SetFont( primary, font_normal );

     *buf = 0;

     if (evt->type == DIET_AXISMOTION) {
          if (evt->flags & DIEF_AXISABS) {
               switch (evt->axis) {
                    case DIAI_X:
                         mouse_x[DFB_EVENT_SLOT_ID(evt)] = evt->axisabs;
                         break;
                    case DIAI_Y:
                         mouse_y[DFB_EVENT_SLOT_ID(evt)] = evt->axisabs;
                         break;
                    case DIAI_Z:
                         snprintf( buf, sizeof(buf), "Z axis (abs): %d", evt->axisabs );
                         break;
                    default:
                         snprintf( buf, sizeof(buf), "Axis %u (abs): %d", evt->axis, evt->axisabs );
                         break;
               }
          }
          else if (evt->flags & DIEF_AXISREL) {
               switch (evt->axis) {
                    case DIAI_X:
                         mouse_x[DFB_EVENT_SLOT_ID(evt)] += evt->axisrel;
                         break;
                    case DIAI_Y:
                         mouse_y[DFB_EVENT_SLOT_ID(evt)] += evt->axisrel;
                         break;
                    case DIAI_Z:
                         snprintf( buf, sizeof(buf), "Z axis (rel): %d", evt->axisrel );
                         break;
                    default:
                         snprintf( buf, sizeof(buf), "Axis %u (rel): %d", evt->axis, evt->axisrel );
                         break;
               }
          }
     }
     else { /* DIET_BUTTONPRESS or DIET_BUTTONRELEASE */
          snprintf( buf, sizeof(buf), "Button %u", evt->button );
          mouse_pressure[DFB_EVENT_SLOT_ID(evt)] = evt->type == DIET_BUTTONPRESS ? true : false;
     }

     if (*buf) {
          primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
          primary->DrawString( primary, buf, -1, 40, screen_height - 40, DSTF_LEFT );
     }

     for (i = 0; i < max_slots; i++) {
          if (!i || mouse_pressure[i]) {
               primary->SetColor( primary, 0x70, 0x80, 0xE0, 0xFF );
               primary->FillRectangle( primary, mouse_x[i], 0, 1, screen_height );
               primary->FillRectangle( primary, 0, mouse_y[i], screen_width, 1 );
          }
     }
}

/**************************************************************************************************/

static void show_any_button_event( DFBInputEvent *evt )
{
     char buf[40];

     primary->SetFont( primary, font_normal );

     snprintf( buf, sizeof(buf), "Button %u %s", evt->button, evt->type == DIET_BUTTONPRESS ? "pressed" : "released" );

     primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
     primary->DrawString( primary, buf, -1, 40, screen_height - 40, DSTF_LEFT );
}

static void show_any_axis_event( DFBInputEvent *evt )
{
     char buf[32];

     primary->SetFont( primary, font_normal );

     if (evt->flags & DIEF_AXISABS)
          snprintf( buf, sizeof(buf), "Axis %u (abs): %d", evt->axis, evt->axisabs );
     else
          snprintf( buf, sizeof(buf), "Axis %u (rel): %d", evt->axis, evt->axisrel );

     primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
     primary->DrawString( primary, buf, -1, 40, screen_height - 40, DSTF_LEFT );
}

/**************************************************************************************************/

static inline int joystick_calc_screenlocation( int screenres, int axisvalue )
{
     return ((axisvalue + 32768) / 65535.0f) * (screenres - 1);
}

static void joystick_show_axisgroup( DFBRectangle *rect, int axis_x, int axis_y )
{
     int screen_x, screen_y;

     screen_x = joystick_calc_screenlocation( rect->w, axis_x );
     screen_y = joystick_calc_screenlocation( rect->h, axis_y );

     primary->SetColor( primary, 0x80, 0x80, 0x80, 0xFF );
     primary->DrawRectangle( primary, rect->x, rect->y, rect->w, rect->h );

     primary->SetColor( primary, 0x00, 0x00, 0xFF, 0xFF );
     primary->DrawLine( primary, screen_x + rect->x, rect->y, screen_x + rect->x, rect->y + rect->h - 1 );
     primary->DrawLine( primary, rect->x, screen_y + rect->y, rect->x + rect->w - 1, screen_y + rect->y );
}

static void show_joystick_event( DFBInputEvent *evt )
{
     char         buf[32];
     DFBRectangle rect;

     primary->SetFont( primary, font_normal );

     *buf = 0;

     if ((evt->type == DIET_AXISMOTION) && (evt->axis < 8)) {
          if (evt->flags & DIEF_AXISABS)
               joy_axis[evt->axis] = evt->axisabs;
          else if (evt->flags & DIEF_AXISREL)
               joy_axis[evt->axis] += evt->axisrel;
     }
     else { /* DIET_BUTTONPRESS or DIET_BUTTONRELEASE */
          snprintf( buf, sizeof(buf), "Button %u", evt->button );
     }

     if (*buf) {
          primary->SetColor( primary, 0xF0, 0xC0, 0x30, 0xFF );
          primary->DrawString( primary, buf, -1, 40, screen_height - 40, DSTF_LEFT );
     }

     rect.x = 0;
     rect.y = 0;
     rect.w = screen_width / 2 - 10;
     rect.h = screen_height / 2 - 10;
     joystick_show_axisgroup( &rect, joy_axis[0], joy_axis[1] );

     rect.x += screen_width / 2;
     joystick_show_axisgroup( &rect, joy_axis[2], joy_axis[3] );

     rect.y += screen_height / 2;
     joystick_show_axisgroup( &rect, joy_axis[4], joy_axis[5] );
}

/**************************************************************************************************/

static void show_event( const char *device_name, DFBInputDeviceTypeFlags device_type, DFBInputEvent *evt )
{
     char buf[128];

     primary->SetFont( primary, font_small );

     snprintf( buf, sizeof(buf), "%s (Device ID %u)", device_name, evt->device_id );
     primary->SetColor( primary, 0x60, 0x60, 0x60, 0xFF );
     primary->DrawString( primary, buf, -1, 100, 40, DSTF_TOP );

     switch (evt->type) {
          case DIET_KEYPRESS:
          case DIET_KEYRELEASE:
               primary->Blit( primary, images[KEYS], NULL, 40, 40 );
               show_key_event( evt );
               break;

          case DIET_BUTTONPRESS:
          case DIET_BUTTONRELEASE:
          case DIET_AXISMOTION:
               if (device_type & DIDTF_MOUSE) {
                    primary->Blit( primary, images[MOUSE], NULL, 40, 40 );
                    show_mouse_event( evt );
               }
               else if (device_type & DIDTF_JOYSTICK) {
                    primary->Blit( primary, images[JOYSTICK], NULL, 40, 40 );
                    show_joystick_event( evt );
               }
               else {
                    if (evt->type == DIET_BUTTONPRESS || evt->type == DIET_BUTTONRELEASE)
                         show_any_button_event( evt );
                    else
                         show_any_axis_event( evt );
               }
               break;

          default:
               break;
     }
}

typedef struct _DeviceInfo {
     DFBInputDeviceID           device_id;
     DFBInputDeviceDescription  desc;
     struct _DeviceInfo        *next;
} DeviceInfo;

static DFBEnumerationResult
input_device_callback( DFBInputDeviceID device_id, DFBInputDeviceDescription desc, void *data )
{
     DeviceInfo **devices = data;
     DeviceInfo  *device;

#ifdef HAVE_MT
     /* set input device configuration */
     if ((desc.type & DIDTF_MOUSE) && (max_slots > 1)) {
          DFBInputDeviceConfig  config;
          IDirectFBInputDevice *mouse;

          DFBCHECK(dfb->GetInputDevice( dfb, device_id, &mouse ));

          config.flags     = DIDCONF_MAX_SLOTS;
          config.max_slots = max_slots;

          mouse->SetConfiguration( mouse, &config );

          mouse->Release( mouse );
     }
#endif

     device = malloc( sizeof(DeviceInfo) );

     device->device_id = device_id;
     device->desc      = desc;
     device->next      = *devices;

     *devices = device;

     return DFENUM_OK;
}

static const char *get_device_name( DeviceInfo *devices, DFBInputDeviceID device_id )
{
     while (devices) {
          if (devices->device_id == device_id)
               return devices->desc.name;

          devices = devices->next;
     }

     return "<unknown>";
}

static DFBInputDeviceTypeFlags get_device_type( DeviceInfo *devices, DFBInputDeviceID device_id )
{
     while (devices) {
          if (devices->device_id == device_id)
               return devices->desc.type;

          devices = devices->next;
     }

     return DIDTF_NONE;
}

static void dfb_shutdown()
{
     int n;

     for (n = 0; n < NUM_IMAGES; n++)
          if (images[n]) images[n]->Release( images[n] );

     if (font_large)     font_large->Release( font_large );
     if (font_normal)    font_normal->Release( font_normal );
     if (font_small)     font_small->Release( font_small );
     if (primary)        primary->Release( primary );
     if (event_buffer)   event_buffer->Release( event_buffer );
     if (dfb)            dfb->Release( dfb );
}

static void print_usage()
{
     printf( "DirectFB Input Demo\n\n" );
     printf( "Usage: df_input [options]\n\n" );
     printf( "Options:\n\n" );
     printf( "  --slots <num>  Number of possible touch contacts (default = 1, max = 10).\n" );
     printf( "  --help         Print usage information.\n" );
     printf( "  --dfb-help     Output DirectFB usage information.\n\n" );
}

int main( int argc, char *argv[] )
{
     int                       n;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     DeviceInfo               *devices    = NULL;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     for (n = 1; n < argc; n++) {
          if (strncmp( argv[n], "--", 2 ) == 0) {
               if (strcmp( argv[n] + 2, "help" ) == 0) {
                    print_usage();
                    return 0;
               }
               else if (strcmp( argv[n] + 2, "slots" ) == 0 && ++n < argc) {
                    max_slots = atoi( argv[n] );
                    if (max_slots > 10) {
                         print_usage();
                         return 1;
                    }
                    continue;
               }
          }

          print_usage();
          return 1;
     }

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create a list of input devices */
     dfb->EnumInputDevices( dfb, input_device_callback, &devices );

     /* create an event buffer for all devices */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_ALL, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* initialize the coordinates of the mouse to the center */
     mouse_x[0]        = screen_width  / 2;
     mouse_y[0]        = screen_height / 2;
     mouse_pressure[0] = false;

     /* load fonts */
     fdsc.flags = DFDESC_HEIGHT;

#ifdef USE_FONT_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_FONTDATA( decker );
     ddsc.memory.length = GET_FONTSIZE( decker );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_FONTFILE( decker );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));

     fdsc.height = CLAMP( (int) (screen_width / 30.0 / 8) * 8, 8, 96 );
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &font_small ));

     fdsc.height = CLAMP( (int) (screen_width / 20.0 / 8) * 8, 8, 96 );
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &font_normal ));

     fdsc.height = CLAMP( (int) (screen_width / 10.0 / 8) * 8, 8, 96 );
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &font_large ));

     /* load images */
     for (n = 0; n < NUM_IMAGES; n++) {
#ifdef USE_IMAGE_HEADERS
          ddsc.flags         = DBDESC_MEMORY;
          ddsc.memory.data   = image_data[n];
          ddsc.memory.length = image_size[n];
#else
          ddsc.flags         = DBDESC_FILE;
          ddsc.file          = image_file[n]();
#endif
          DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
          DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
          provider->GetSurfaceDescription( provider, &sdsc );
          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &images[n] ));
          provider->RenderTo( provider, images[n], NULL );
          provider->Release( provider );
     }

     /* clear with black */
     primary->Clear( primary, 0, 0, 0, 0 );

     /* start screen */
     primary->SetFont( primary, font_normal );
     primary->SetColor( primary, 0x60, 0x60, 0x60, 0xFF );
     primary->DrawString( primary, "Press any key to continue.", -1, screen_width / 2, screen_height / 2, DSTF_CENTER );
     primary->Flip( primary, NULL, DSFLIP_NONE );

     sleep( 1 );

     event_buffer->Reset( event_buffer );

     if (event_buffer->WaitForEventWithTimeout( event_buffer, 10, 0 ) == DFB_TIMEOUT) {
          primary->Clear( primary, 0, 0, 0, 0 );
          primary->DrawString( primary, "Timed out.", -1, screen_width / 2, screen_height / 2, DSTF_CENTER );
          primary->Flip( primary, NULL, DSFLIP_NONE );
          sleep( 1 );
     }
     else {
          DFBInputDeviceKeySymbol last_symbol = DIKS_NULL;

          /* main loop */
          while (1) {
               DFBInputEvent evt;

               /* process event buffer */
               while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
                    const char              *device_name;
                    DFBInputDeviceTypeFlags  device_type;
                    DeviceInfo              *devs = devices;

                    /* check if a new device is connected */
                    while (devs) {
                         if (devs->device_id == evt.device_id)
                              break;

                         devs = devs->next;
                    }

                    if (!devs) {
                         /* clear the old list */
                         while (devices) {
                              DeviceInfo *next = devices->next;
                              free( devices );
                              devices = next;
                         }

                         /* recreate a list of input devices */
                         dfb->EnumInputDevices( dfb, input_device_callback, &devices );
                    }

                    primary->Clear( primary, 0, 0, 0, 0 );

                    device_name = get_device_name( devices, evt.device_id );
                    device_type = get_device_type( devices, evt.device_id );

                    show_event( device_name, device_type, &evt );

                    primary->Flip( primary, NULL, DSFLIP_NONE );
               }

               if (evt.type == DIET_KEYRELEASE) {
                    if ((last_symbol    == DIKS_ESCAPE || last_symbol    == DIKS_EXIT) &&
                        (evt.key_symbol == DIKS_ESCAPE || evt.key_symbol == DIKS_EXIT))
                         break;
                    last_symbol = evt.key_symbol;
               }
               else if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT)
                         break;
                     else
                         continue;
               }

               event_buffer->WaitForEvent( event_buffer );
          }
     }

     while (devices) {
          DeviceInfo *next = devices->next;
          free( devices );
          devices = next;
     }

     return 42;
}
