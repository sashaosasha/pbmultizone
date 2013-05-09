#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "resource_ids.auto.h"

#define MY_UUID { 0x20, 0xfc, 0x19, 0x2c, 0xb6, 0xc6, 0x11, 0xe2, 0xa5, 0x19, 0x00, 0x0c, 0x29, 0x7a, 0x46, 0xb4 }

PBL_APP_INFO(MY_UUID,
             "Many Time Zones", "Sasha Ognev",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

// App-specific data
Window window; // All apps must have at least one window

TextLayer timeLayer; // The main clock
TextLayer secondaryLayer[3]; // All other clocks
TextLayer text_date_layer;
Layer line_layer;


static const int x0 = 9;
static const int y0 = 18;
static const int width = 144-14;
static const int secondaryOffset = 36;
static const int dh = 20;;

void line_layer_update_callback(Layer *me, GContext* ctx) {
  (void)me;

  graphics_context_set_stroke_color(ctx, GColorWhite);

  int y = y0 + secondaryOffset + dh * 3 + 3;
  graphics_draw_line(ctx, GPoint(x0, y), GPoint(x0+width, y));
  graphics_draw_line(ctx, GPoint(x0, y+1), GPoint(x0+width, y+1));

  y = y0 + secondaryOffset - 3;
  graphics_draw_line(ctx, GPoint(x0, y), GPoint(x0+width, y));
  graphics_draw_line(ctx, GPoint(x0, y+1), GPoint(x0+width, y+1));
}

int day_of_week(int y, int m, int d)
{
   static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
   y -= m < 3;
   return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int nth_date(int y, int m, int dow, int nth_week)
{
    int target_date = 1;
    int first_dow = day_of_week(y, m, target_date);
    while (first_dow != dow)
    {
        first_dow = (first_dow + 1) % 7;
        ++target_date;
    }
    target_date += (nth_week - 1) * 7;
    return target_date;
}

bool is_usa_dst(PblTm* time)
{
    if (time->tm_mon > 3 && time->tm_mon < 11)
        return true;
    if (time->tm_mon == 3)
        return time->tm_mday >= nth_date(time->tm_year, 3, 0, 2);
    if (time->tm_mon == 11)
        return time->tm_mday < nth_date(time->tm_year, 11, 0, 2);
    return false;
}

// Called once per second
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {

  (void)t;
  (void)ctx;

  static char timeText[] = "00:00:00"; // Needs to be static because it's used by the system later.
  static char* secondaryText[] = {"00:00 UTC",
                                   "00:00 Singapore",
                                   "00:00 Moscow"};

  PblTm currentTime;
  get_time(&currentTime);

  int offset = is_usa_dst(&currentTime) ? 5 : 6;  // TODO: How do we not assume CST/CDT here?
  int offsets[] = {offset, offset + 8, offset + 4};

  const char* time_format = clock_is_24h_style() ? "%H:%M:%S" : "%I:%M:%S";
  const char* short_time_format = clock_is_24h_style() ? "%H:%M" : "%I:%M";
  string_format_time(timeText, sizeof(timeText), time_format, &currentTime);
  text_layer_set_text(&timeLayer, timeText);

  for(unsigned int i = 0; i < sizeof(offsets)/sizeof(offsets[0]); ++i)
  {
      int h = currentTime.tm_hour;
      currentTime.tm_hour = (currentTime.tm_hour + offsets[i]) % 24;
      string_format_time(secondaryText[i], 13, short_time_format, &currentTime);
      secondaryText[i][5] = ' ';
      currentTime.tm_hour = h;
      text_layer_set_text(&secondaryLayer[i], secondaryText[i]);
  }

  static char dateText[] = "Xxx, Xxxxxxxxx 00";

  string_format_time(dateText, sizeof(dateText), "%a, %B %e", t->tick_time);
  text_layer_set_text(&text_date_layer, dateText);
}


// Handle the start-up of the app
void handle_init_app(AppContextRef app_ctx) {

  // Create our app's base window
  window_init(&window, "Time");
  window_stack_push(&window, true);
  window_set_background_color(&window, GColorBlack);

  // Init the text layer used to show the time
  // TODO: Wrap this boilerplate in a function?
  text_layer_init(&timeLayer, GRect(x0 , y0, width, 168-54 /* height */));
  text_layer_set_text_color(&timeLayer, GColorWhite);
  text_layer_set_background_color(&timeLayer, GColorClear);
  text_layer_set_font(&timeLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(&timeLayer, GTextAlignmentCenter);
  layer_add_child(&window.layer, &timeLayer.layer);

  for (unsigned int i = 0; i < sizeof(secondaryLayer)/sizeof(secondaryLayer[0]); ++i)
  {
      text_layer_init(&secondaryLayer[i], GRect(x0 , y0 + secondaryOffset + i * dh, width, dh /* height */));
      text_layer_set_text_color(&secondaryLayer[i], GColorWhite);
      text_layer_set_background_color(&secondaryLayer[i], GColorClear);
      text_layer_set_font(&secondaryLayer[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      layer_add_child(&window.layer, &secondaryLayer[i].layer);
  }

  text_layer_init(&text_date_layer, window.layer.frame);
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  int y = y0 + secondaryOffset + dh * 3 + 6;
  layer_set_frame(&text_date_layer.layer, GRect(x0, y, x0 + width, 22));
  text_layer_set_font(&text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(&text_date_layer, GTextAlignmentCenter);

  layer_add_child(&window.layer, &text_date_layer.layer);

  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  handle_second_tick(app_ctx, NULL);
}


// The main event/run loop for our app
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {

    // Handle app start
    .init_handler = &handle_init_app,

    // Handle time updates
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }

  };
  app_event_loop(params, &handlers);
}
