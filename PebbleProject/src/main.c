// #define LOG_MSG
// #define LOG_updateArray
// #define LOG_rect_layer_update_callback
// #define LOG_update_time
// #define LOG_COLOR
// #define LOG_POP

// #define DEBUG_NO_NETWORK
//#define DEBUG_FETCH_MORE

// #define DEBUG_POP
// #define DEBUG_POP_IN_ROW4

#include <pebble.h>

///local storage keys
# define USERNAME_KEY 0
# define COND_KEY 1
# define HOUR_KEY 2
# define TEMP_KEY 3
# define WIND_KEY 4
# define COLOR_KEY 5
# define DEGREE_KEY 6
# define CONDITION_KEY 7
# define WIND_COND_KEY 8
# define POP_KEY 9

///setting and buf
# define MOON_KEY 100
# define POP_SETTING_KEY 101

#define DEVICE_KEY 10

///msg keys
#define KEY_TEMPERATURE 0
#define KEY_DEGREE 1
#define KEY_CONDITION 2
#define KEY_WIND 3

#define KEY_ARRAY_TEMPERATURE 5
#define KEY_ARRAY_CONDITION 6
#define KEY_ARRAY_TIME 7
#define KEY_ARRAY_WINDSPD 8
#define KEY_ARRAY_POP 9 ///TODO: % of rain fall
#define KEY_CITY 10
#define KEY_COLOR 11
#define KEY_POP 12
#define KEY_MOON 13

#define NUM_HOURLY_FORECAST 6

typedef struct persist_days_lang { // 238 bytes
  char DaysOfWeek[7][34];         //  238: 16-33 UTF8 characters for each of 7 weekdays
  //                                    238 bytes
} __attribute__((__packed__)) persist_days_lang;


typedef struct persist_months_lang { // 252 bytes
  char monthsNames[12][21];       // 252: 10-20 UTF8 characters for each of 12 months
  //                                   252 bytes
} __attribute__((__packed__)) persist_months_lang;

persist_days_lang lang_days = {
  .DaysOfWeek = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
};

persist_months_lang lang_months = {
  .monthsNames = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" },
};

////////////////////////////////////////////////////////

static int device_key = 1;
static bool color_key = true; ///1 is black

static int pop_key = 2; ///1 is black

static bool isShowPop = false; ///flag for switch icon/pop in every minute

#define BUF_SIZE 180 ///TODO: try to solve memory leak? 180 or 90 cause dirty screen memory bug?
static char weather_ar_temp_layer_buffer[BUF_SIZE];///string from JS
static char weather_ar_hour_buffer[BUF_SIZE];///4*10 = 40?
static char weather_ar_wspd_buffer[BUF_SIZE];///4*10 = 40?
static char weather_ar_condition_buffer[BUF_SIZE];///4*10 = 40?
static char weather_ar_pop_buffer[BUF_SIZE];///4*10 = 40?


static char * ar_temp[NUM_HOURLY_FORECAST];
static char * ar_cond[NUM_HOURLY_FORECAST];
static char * ar_pop[NUM_HOURLY_FORECAST];
static char * ar_time[NUM_HOURLY_FORECAST];
static char * ar_wspd[NUM_HOURLY_FORECAST];

///buf for current condition
//   static char weather_layer_buffer[80];///3 lines of text, 32 is not enough
///current condition buffer
static char temperature_buffer[80];///for local storage caching
static char degree_buffer[20];
static char condition_buffer[10];
static char wind_buffer[20];

static char moon_buffer[3];

///buffer for text drawing
static char buf[5];


static int isHourly = 0;
static int isHourlyTime = 0;

// primary coordinates
#define DEVICE_WIDTH        144
#define DEVICE_HEIGHT       168

#define W_MOON 35

#define LAYOUT_SLOT_TOP      -1
#define LAYOUT_SLOT_HEIGHT   60

///remember to release!!
static Window *s_main_window;

///layer to show the clock
static TextLayer *s_time_layer;
static TextLayer *s_time_min_layer;

static TextLayer *s_moon_layer;
static TextLayer *s_moon_bg_layer;
static TextLayer *s_moon_mask_layer;

static TextLayer *s_date_layer;
///layer below time
static TextLayer *s_city_layer;

///text layer at bottom
// static TextLayer *s_weather_layer;

static TextLayer *s_tempDeg_layer;
static TextLayer *s_condition_layer;
static TextLayer *s_wind_layer;

///layer for top panel
static Layer *slot_top;

static GFont s_icon_font;
static GFont s_icon_font_main;
static GFont s_icon_font_windmoon;
static GFont s_icon_font_moon;

static GFont s_font_degree;

static void updateArray(char * ar[], char buf[]);
static bool removeBufferFirstItem(char buf[]);
static bool readRemoveWriteUpdate();

struct tm *currentTime;

static void updatePop()
{
#ifdef DEBUG_POP
  APP_LOG(APP_LOG_LEVEL_INFO, "updatePop() - pop_key %d", pop_key);
#endif
  
  if(pop_key == 1)
  {
    isShowPop = true;
  }
  else if(pop_key == 2)
  {
    ///do not change the value
  }
  else if(pop_key == 3)
  {
    isShowPop = false;
  }
}


static void setLayerColor(TextLayer *layer, int color_key)
{
  text_layer_set_text_color(layer, color_key ? GColorWhite : GColorBlack);
  // text_layer_set_background_color(layer, color_key ? GColorBlack : GColorWhite);
}


static bool checkBufferWithLimit(char s[], int limit)
{
  char * sOri = s;
  if (!s)
  {
    return false;
  }
  
  int i = 0;
  
  for (i = 0; s[i]; s[i] == ' ' ? i++ : *s++);
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "checkBuffer %s#%d", sOri, i);
#endif
  
  if (i <= limit) ///end with ' ' is also not ok, i.e. at least 6 ' '
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "checkBuffer return false");
    return false;
  }
  
  return true;
}

static bool checkBuffer(char s[])
{
  return checkBufferWithLimit(s, NUM_HOURLY_FORECAST);
}

static void update_time() {
#ifdef LOG_update_time
  APP_LOG(APP_LOG_LEVEL_INFO, "update_time starts");
#endif
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char buffer[] = "00";
  static char bufferMin[] = "00";
  
  // Write the current hours and minutes into the buffer
  if (clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00"), "%H", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00"), "%I", tick_time);
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
  ////////////
  strftime(bufferMin, sizeof("00"), "%M", tick_time);///TODO: 00 bug?
  // Display this time on the TextLayer
  text_layer_set_text(s_time_min_layer, bufferMin);
  
  
#ifdef LOG_update_time
  APP_LOG(APP_LOG_LEVEL_INFO, "update_time ends");
#endif
  
  ///cover the time layer
  text_layer_set_text(s_moon_layer, moon_buffer);
  
}

void setColors(GContext* ctx) {
  window_set_background_color(s_main_window, color_key ? GColorBlack : GColorWhite);
  graphics_context_set_stroke_color(ctx, color_key ? GColorWhite : GColorBlack);
  graphics_context_set_fill_color(ctx, color_key ? GColorBlack : GColorWhite);
  graphics_context_set_text_color(ctx, color_key ? GColorWhite : GColorBlack);
}

void setInvColors(GContext* ctx) {
  window_set_background_color(s_main_window, color_key ? GColorWhite : GColorBlack);
  graphics_context_set_stroke_color(ctx, color_key ? GColorBlack : GColorWhite);
  graphics_context_set_fill_color(ctx, color_key ? GColorWhite : GColorBlack);
  graphics_context_set_text_color(ctx, color_key ? GColorBlack : GColorWhite);
}

//#define CAL_HOURS   4   // number of columns (4 hour forecast)
#define CAL_WIDTH  (DEVICE_WIDTH/NUM_HOURLY_FORECAST)  // width of columns
#define CAL_GAP    0   // gap around calendar
#define CAL_LEFT   0   // left side of calendar
#define CAL_HEIGHT 82  // How tall rows should be depends on how many weeks there are

static void rect_layer_update_callback(Layer *layer, GContext *ctx) {
  
#ifdef LOG_rect_layer_update_callback
  APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback start");
#endif
  int  specialHour = 0; ///highlight this hour
  
  for (int col = 0; col < NUM_HOURLY_FORECAST; col++) {
    setColors(ctx);
    
    // draw the cell background
    graphics_fill_rect(ctx, GRect (CAL_WIDTH * col + CAL_LEFT + CAL_GAP,
                                   0 , CAL_WIDTH - CAL_GAP, CAL_HEIGHT - CAL_GAP), 0, GCornerNone);
    
    graphics_context_set_text_color(ctx, color_key ? GColorWhite : GColorBlack);
    
    ///time cells
    if (isHourlyTime == 1)
    {
      if (ar_time[col])
      {
        graphics_draw_text(ctx, ar_time[col], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                           GRect(CAL_WIDTH * col + CAL_LEFT,
                                 -1,
                                 CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      }
      
    }
#ifdef LOG_rect_layer_update_callback
    APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback ar_temp");
#endif
    if (ar_temp[col])
    {
      snprintf(buf, sizeof(buf), "%s°", ar_temp[col]);
      
      graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                         GRect(CAL_WIDTH * col + CAL_LEFT - 1,
                               11,
                               CAL_WIDTH + 2,
                               CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      
    }
    
#ifdef LOG_rect_layer_update_callback
    APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback ar_cond");
#endif
    {
      
      int iPop = -1;
      if(ar_pop[col])
      {
        iPop = atoi(ar_pop[col]);
#ifdef LOG_rect_layer_update_callback
        APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback ar_pop %d %s", iPop, ar_pop[col]);
#endif
      }
      
#ifdef LOG_rect_layer_update_callback
      APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback ar_pop %d", iPop);
#endif
      
      ///debug digits
#ifdef DEBUG_POP
      iPop = 100;
#endif
      
      ///ignore < 10% pop
      if(iPop > 10 && ar_pop[col])
      {
        ///debug digits
#ifdef DEBUG_POP
        snprintf(buf, sizeof(buf), "%d%%", iPop);
#else
        snprintf(buf, sizeof(buf), "%s%%", ar_pop[col]);
#endif
        
#ifdef DEBUG_POP
        APP_LOG(APP_LOG_LEVEL_ERROR, "isShowPop %d", isShowPop);
#endif
        
        if(isShowPop)
        {
          if(iPop == 100)///show the umbrella
          {
            snprintf(buf, sizeof(buf), "R");
            
            graphics_draw_text(ctx, buf,
                               s_icon_font_windmoon,
                               GRect(CAL_WIDTH * col + CAL_LEFT - 2,
                                     31,
                                     30, 30),
                               GTextOverflowModeFill, GTextAlignmentCenter, NULL);
            
          }
          else if(iPop > 0)
          {
            graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                               GRect(CAL_WIDTH * col + CAL_LEFT - 2,
                                     31,
                                     30, 30),
                               GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
            
          }
        }
        else
        {
          ///weather icon
          if (ar_cond[col])
          {
            graphics_draw_text(ctx, ar_cond[col], s_icon_font,
                               GRect(CAL_WIDTH * col + CAL_LEFT - 2,
                                     31,
                                     30, 30),
                               GTextOverflowModeFill, GTextAlignmentCenter, NULL);
          }
        }
      }///end of pop
      else
      {
        ///weather icon
        if (ar_cond[col])
        {
          graphics_draw_text(ctx, ar_cond[col], s_icon_font,
                             GRect(CAL_WIDTH * col + CAL_LEFT - 2,
                                   31,
                                   30, 30),
                             GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        }
      }
      
#ifdef DEBUG_POP_IN_ROW4
      if (ar_pop[col])
      {
        snprintf(buf, sizeof(buf), "%s%%", ar_pop[col]);
        
        graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                           GRect(CAL_WIDTH * col + CAL_LEFT - 2,// -2 pixel, for left most 8NW case
                                 45,
                                 CAL_WIDTH + 6, ///more space for case like "99NW
                                 CAL_HEIGHT * 2), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      }
#else
      if (ar_wspd[col])
      {
        graphics_draw_text(ctx, ar_wspd[col], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                           GRect(CAL_WIDTH * col + CAL_LEFT - 2,// -2 pixel, for left most 8NW case
                                 45,
                                 CAL_WIDTH + 6, ///more space for case like "99NW
                                 CAL_HEIGHT * 2), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      }
#endif
      ///wind spd
      //snprintf(buf, sizeof(buf), "%s", ar_cond[col]);
      //APP_LOG(KEY_ARRAY_TEMPERATURE, "buf %s", buf);
    }
    
  }
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "rect_layer_update_callback end");
#endif
}

struct tm *get_time() {
  time_t tt = time(0);
  return localtime(&tt);
}

#define Y_TIME_LBL 56
#define HGH_TIME_LBL 50
#define HGH_DATE_LBL 20

static char date_string[48];
static char weekday_string[48];

static void updateDate()
{
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "updateDate start");
#endif
  currentTime = get_time();
  
  char date_text[3];
  char month_text[24];
  
  ///http://www.cplusplus.com/reference/ctime/strftime/
  strftime(date_text, sizeof(date_text), "%e", currentTime); // DD
  
  strftime(month_text, sizeof(month_text), "%m", currentTime);
  
  snprintf(weekday_string, sizeof(weekday_string), "%s %s %s",
           lang_days.DaysOfWeek[currentTime->tm_wday],
           date_text,
           lang_months.monthsNames[currentTime->tm_mon]
           ); // prefix Month
  
  text_layer_set_text(s_date_layer, weekday_string);
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "updateDate end");
#endif
}


static void adjustDegFont()
{
  if (strlen(degree_buffer) >= 7)
  {
    ///2 lines
    text_layer_set_font(s_tempDeg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  }
  else if (strlen(degree_buffer) >= 5)///100 deg F with eod
  {
    ///2 lines
    text_layer_set_font(s_tempDeg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  }
  else
  {
    text_layer_set_font(s_tempDeg_layer, s_font_degree);
  }
  
}

static void main_window_load(Window *window) {
  //Create GFont
  s_icon_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_15));
  s_icon_font_main = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_40));
  
  s_icon_font_windmoon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_WIND_MOON_15));
  s_icon_font_moon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_WIND_MOON_33));
  
  s_font_degree = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_BOOK_30));

  int w_time = (DEVICE_WIDTH - W_MOON) / 2;
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "w_time %d", w_time);
  // APP_LOG(APP_LOG_LEVEL_INFO, "W_MOON %d", W_MOON);
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(-1, Y_TIME_LBL+5,
                                         w_time + 7, HGH_TIME_LBL));///TO prevent "box", w_time + 5 is needed
  text_layer_set_background_color(s_time_layer, GColorClear);
  
  text_layer_set_text(s_time_layer, "00");
  //Apply to TextLayer
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  ////////
  // Create time-min TextLayer
  s_time_min_layer = text_layer_create(GRect(W_MOON + w_time-4, Y_TIME_LBL+5,
                                             w_time + 5, HGH_TIME_LBL));
  text_layer_set_background_color(s_time_min_layer, GColorClear);
  
  text_layer_set_text(s_time_min_layer, "00");
  //Apply to TextLayer
  text_layer_set_font(s_time_min_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_min_layer, GTextAlignmentCenter);
  
  ////////////////////
  int xMoon = w_time + 4;
  int yMoon = Y_TIME_LBL+15;
  int sz = W_MOON;///font size + 2 to prevent char not drawn issue
  ///moon layer
  s_moon_layer = text_layer_create(GRect(xMoon, yMoon, sz, sz));
  text_layer_set_background_color(s_moon_layer, GColorClear);
  // text_layer_set_background_color(s_moon_layer, GColorBlack);
  
  // text_layer_set_text_color(s_moon_layer, color_key ? GColorWhite : GColorBlack);
  text_layer_set_text_color(s_moon_layer, GColorWhite);
  //Apply to TextLayer
  text_layer_set_font(s_moon_layer, s_icon_font_moon);
  text_layer_set_text_alignment(s_moon_layer, GTextAlignmentCenter);
  
  text_layer_set_text(s_moon_layer, "a");
  
  
  s_moon_bg_layer = text_layer_create(GRect(xMoon, yMoon, sz, sz));
  text_layer_set_background_color(s_moon_bg_layer, GColorClear);
  // text_layer_set_background_color(s_moon_layer, GColorBlack);
  
  // text_layer_set_text_color(s_moon_layer, color_key ? GColorWhite : GColorBlack);
  text_layer_set_text_color(s_moon_bg_layer, GColorBlack);
  //Apply to TextLayer
  text_layer_set_font(s_moon_bg_layer, s_icon_font_moon);
  text_layer_set_text_alignment(s_moon_bg_layer, GTextAlignmentCenter);
  
  text_layer_set_text(s_moon_bg_layer, "o");
  
  // text_layer_set_text_color(s_time_layer, color_key ? GColorWhite : GColorBlack);
  // text_layer_set_text_color(s_time_min_layer, color_key ? GColorWhite : GColorBlack);
  
  setLayerColor(s_time_layer, color_key);
  setLayerColor(s_time_min_layer, color_key);
  
  //////////
  
  s_moon_mask_layer = text_layer_create(GRect(xMoon, yMoon, sz, sz));
  text_layer_set_background_color(s_moon_mask_layer, GColorClear);
  // text_layer_set_background_color(s_moon_layer, GColorBlack);
  
  // text_layer_set_text_color(s_moon_layer, color_key ? GColorWhite : GColorBlack);
  text_layer_set_text_color(s_moon_mask_layer, GColorBlack);
  //Apply to TextLayer
  text_layer_set_font(s_moon_mask_layer, s_icon_font_moon);
  text_layer_set_text_alignment(s_moon_mask_layer, GTextAlignmentCenter);
  
  text_layer_set_text(s_moon_mask_layer, "a");
  
  layer_set_hidden((Layer *)s_moon_mask_layer, color_key ? true : false);
  
  
  //////////
  
  ///city layer
  s_city_layer = text_layer_create(GRect(0, DEVICE_HEIGHT - HGH_DATE_LBL +4, DEVICE_WIDTH, HGH_DATE_LBL));
  
  setLayerColor(s_city_layer, color_key);
  text_layer_set_background_color(s_city_layer, GColorClear);
  
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_city_layer, GTextOverflowModeFill);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_city_layer));
  
  ///date  layer
  
  s_date_layer = text_layer_create(GRect(5, Y_TIME_LBL + 0, 139, HGH_DATE_LBL));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, color_key ? GColorWhite : GColorBlack);
  
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  ////////////////////////////////
  
  updateDate();
  //    APP_LOG(APP_LOG_LEVEL_INFO, "weekDay %d", weekDay);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_min_layer));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_moon_bg_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_moon_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_moon_mask_layer));
  
  // Create weather Layer
  int y_weather = 112;
  
  /////////////////////////////////////////////////
  
  int width = DEVICE_WIDTH/3;
  int hgh = DEVICE_HEIGHT - y_weather;
  int y = y_weather - HGH_DATE_LBL;
  
  s_tempDeg_layer = text_layer_create(GRect(-1  , y+16, 
    width+8, ///for bigger font, 5 is not enough, 10 is too much, touched the icon
   hgh));

  text_layer_set_background_color(s_tempDeg_layer, GColorClear);
  text_layer_set_text_color(s_tempDeg_layer, color_key ? GColorWhite : GColorBlack);
  text_layer_set_text_alignment(s_tempDeg_layer, GTextAlignmentCenter);
  // Create second custom font, apply it and add to Window
  //text_layer_set_font(s_tempDeg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));///FONT_KEY_BITHAM_30_BLACK is too big for 20deg
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_tempDeg_layer));
  text_layer_set_text(s_tempDeg_layer, "?");
  text_layer_set_overflow_mode(s_tempDeg_layer, GTextOverflowModeFill);
  
  /////////////////
  s_condition_layer = text_layer_create(GRect(width-4, y+12, width+8, hgh+10));///expend 4 px for wider icon like nt_fog
  
  if(1)
  {
    text_layer_set_background_color(s_condition_layer, GColorClear);
    text_layer_set_text_color(s_condition_layer, color_key ? GColorWhite : GColorBlack);
    text_layer_set_font(s_condition_layer, s_icon_font_main);
  }
  else
  {
    text_layer_set_background_color(s_condition_layer, GColorBlack);
    text_layer_set_text_color(s_condition_layer, GColorWhite);
    text_layer_set_font(s_condition_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  }
  
  text_layer_set_text_alignment(s_condition_layer, GTextAlignmentCenter);
  // Create second custom font, apply it and add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_condition_layer));
  text_layer_set_text(s_condition_layer, "U");
  //  text_layer_set_overflow_mode(s_condition_layer, GTextOverflowModeWordWrap);
  
  s_wind_layer = text_layer_create(GRect(width * 2, y+7, width, hgh));
  text_layer_set_background_color(s_wind_layer, GColorClear);
  text_layer_set_text_color(s_wind_layer, color_key ? GColorWhite : GColorBlack);
  text_layer_set_text_alignment(s_wind_layer, GTextAlignmentCenter);
  // Create second custom font, apply it and add to Window
  text_layer_set_font(s_wind_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_wind_layer));
  text_layer_set_text(s_wind_layer, "?");
  text_layer_set_overflow_mode(s_wind_layer, GTextOverflowModeWordWrap);
  
  //////////////////////////////////////
  slot_top = layer_create(GRect(0, LAYOUT_SLOT_TOP, DEVICE_WIDTH, LAYOUT_SLOT_HEIGHT));
  layer_add_child(window_get_root_layer(window), slot_top);
  
  layer_set_update_proc(slot_top, rect_layer_update_callback);
  
  update_time();
  
  ////read storage
  if (strlen(temperature_buffer) <= 0)
  {
    persist_read_string(USERNAME_KEY, temperature_buffer, sizeof(temperature_buffer));
  }
  
  if (strlen(degree_buffer) <= 0)
  {
    persist_read_string(DEGREE_KEY, degree_buffer, sizeof(degree_buffer));
    text_layer_set_text(s_tempDeg_layer, degree_buffer);
  }
  
  adjustDegFont();
  
  if (strlen(condition_buffer) <= 0)
  {
    persist_read_string(CONDITION_KEY, condition_buffer, sizeof(condition_buffer));
    text_layer_set_text(s_condition_layer, condition_buffer);
  }
  if (strlen(wind_buffer) <= 0)
  {
    persist_read_string(WIND_COND_KEY, wind_buffer, sizeof(wind_buffer));
    text_layer_set_text(s_wind_layer, wind_buffer);
  }
  if (strlen(moon_buffer) <= 0)
  {
    persist_read_string(MOON_KEY, moon_buffer, sizeof(moon_buffer));
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "moon_buffer %s loaded", moon_buffer);
#endif
    text_layer_set_text(s_moon_layer, moon_buffer);
  }
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load temperature_buffer %s", temperature_buffer);
#endif
  
  if (strlen(temperature_buffer) > 0)
  {
    text_layer_set_text(s_city_layer, temperature_buffer);
  }
  else
  {
    text_layer_set_text(s_city_layer, "Loading...");
  }
  ///TODO: buf for degree
  
  ///////////
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load weather_ar_condition_buffer before");
#endif
  persist_read_string(COND_KEY, weather_ar_condition_buffer, sizeof(weather_ar_condition_buffer));
  //if (checkBuffer(weather_ar_condition_buffer))
  {
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load weather_ar_condition_buffer %s", weather_ar_condition_buffer);
#endif
    updateArray(ar_cond, weather_ar_condition_buffer);
    
  }
  
  persist_read_string(TEMP_KEY, weather_ar_temp_layer_buffer, sizeof(weather_ar_temp_layer_buffer));
  if (strlen(weather_ar_temp_layer_buffer) > 10)
  {
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load weather_ar_temp_layer_buffer %s", weather_ar_temp_layer_buffer);
#endif
    updateArray(ar_temp, weather_ar_temp_layer_buffer);
    isHourly = 1;
  }
  persist_read_string(HOUR_KEY, weather_ar_hour_buffer, sizeof(weather_ar_hour_buffer));
  if (strlen(weather_ar_hour_buffer) > 10)
  {
    updateArray(ar_time, weather_ar_hour_buffer);
    isHourlyTime = 1;
  }
  
  persist_read_string(WIND_KEY, weather_ar_wspd_buffer, sizeof(weather_ar_wspd_buffer));
  if (strlen(weather_ar_wspd_buffer) > 3 * NUM_HOURLY_FORECAST)
  {
    updateArray(ar_wspd, weather_ar_wspd_buffer);
  }
  
  persist_read_string(POP_KEY, weather_ar_pop_buffer, sizeof(weather_ar_pop_buffer));
  if (strlen(weather_ar_pop_buffer) > 2 * NUM_HOURLY_FORECAST)
  {
    updateArray(ar_pop, weather_ar_pop_buffer);
  }
  
  
  pop_key = persist_read_int(POP_SETTING_KEY);
  
  isShowPop = false;
  
  updatePop();
  
  
  ///app launch, check the cache is valid? Kill the old cache
  if (ar_time[0])
  {
    int iItem = atoi(ar_time[0]);
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    char buf_time_tick[3];
    strftime(buf_time_tick, sizeof("00"), "%H", tick_time);
    int iBuf = atoi(buf_time_tick);
    
    if (iItem != iBuf + 1)///TODO: test with new install!
    {
      APP_LOG(APP_LOG_LEVEL_INFO, "need clean up! %d", iItem);
      
      ///search for the item I want...
      persist_read_string(HOUR_KEY, weather_ar_hour_buffer, sizeof(weather_ar_hour_buffer));
      
      APP_LOG(APP_LOG_LEVEL_INFO, "weather_ar_hour_buffer = %s", weather_ar_hour_buffer);
      
      
      char str[3];
      int hr = iBuf + 1;
      if (hr == 24)
      {
        hr = 0;
      }
      snprintf(str, sizeof(str), " %d", hr);
      
      char * strSearch = strstr(weather_ar_hour_buffer, str);
#ifdef LOG_MSG
      APP_LOG(APP_LOG_LEVEL_INFO, "str %s", str);
#endif
      if (strSearch != NULL) {
        // contains
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "found cache! %s", str);
#endif
        if (checkBufferWithLimit(strSearch, NUM_HOURLY_FORECAST - 1))
        {
#ifdef LOG_MSG
          APP_LOG(APP_LOG_LEVEL_INFO, "enough items %s", strSearch);
#endif
          ///keep remove first item til correct
          
          ///TODO: check it works?
          //while (true)
          {
            bool ret = readRemoveWriteUpdate();
          }
        }
        
      }
      
    }
  }
  
  layer_mark_dirty(slot_top);
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load done");
#endif
}

static void main_window_unload(Window *window) {
  //Unload GFont, 88 bytes
  fonts_unload_custom_font(s_icon_font);
  fonts_unload_custom_font(s_icon_font_main);
  fonts_unload_custom_font(s_icon_font_windmoon);
  fonts_unload_custom_font(s_icon_font_moon);
  fonts_unload_custom_font(s_font_degree);
  

  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_time_min_layer);
  
  text_layer_destroy(s_moon_layer);
  text_layer_destroy(s_moon_bg_layer);
  text_layer_destroy(s_moon_mask_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_city_layer);
  
  text_layer_destroy(s_tempDeg_layer);
  text_layer_destroy(s_condition_layer);
  text_layer_destroy(s_wind_layer);
  
  layer_destroy(slot_top);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
  if(pop_key == 1)
  {
    isShowPop = true;
  }
  else if(pop_key == 2)
  {
    ///do not change the value
    isShowPop = !isShowPop;
  }
  else if(pop_key == 3)
  {
    isShowPop = false;
  }
  
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler starts");
#endif
  update_time();
  
  ///update date once a day.
  if (tick_time->tm_min  == 0 && tick_time->tm_hour  == 0 )
  {
    updateDate();
  }
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler-updateDate");
#endif
  // Get weather update every 60 minutes
#ifdef DEBUG_FETCH_MORE
  if (tick_time->tm_min % 5 == 0)
#else
    
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "before if check %d, device_key = %d", tick_time->tm_min, device_key);
#endif
  if (tick_time->tm_min == device_key)
#endif
  {
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "tick_time->tm_min == device_key");
#endif
#ifndef DEBUG_NO_NETWORK
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
    
    // Send the message!
    app_message_outbox_send();
#endif
  }
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "not match == device_key");
#endif
  ///clean up the old cache array
  char buf_time_tick[3];
  char buf_time_min[3];
  
  //   if (tick_time->tm_min == 0)
  {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    strftime(buf_time_tick, sizeof("00"), "%H", tick_time);
    strftime(buf_time_min, sizeof("00"), "%M", tick_time);
    
    ///TODO: crop the array elements if contains same elements.
  }
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "time init done");
#endif
  ///if enable this, it will crop an item every tick
  bool isDebug = false;
  
  if (ar_time[0] == 0)
  {
    return;
  }
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler-ar_time[0] %s", ar_time[0]);
#endif
  int iItem = atoi(ar_time[0]);
  int iBuf = atoi(buf_time_tick);
  int iMin = atoi(buf_time_min);
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler-isDebug");
#endif
  if (isDebug || (ar_time[0] && iItem == iBuf && iMin == 0))
  {
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler-buf_time: %s", buf_time_tick);
    // APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler-ar_time[0]: %s", ar_time[0]);
#endif
    
    ///read the array strings
    persist_read_string(COND_KEY, weather_ar_temp_layer_buffer, sizeof(weather_ar_temp_layer_buffer));
    persist_read_string(HOUR_KEY, weather_ar_hour_buffer,       sizeof(weather_ar_hour_buffer));
    persist_read_string(TEMP_KEY, weather_ar_wspd_buffer,       sizeof(weather_ar_wspd_buffer));
    persist_read_string(WIND_KEY, weather_ar_condition_buffer,  sizeof(weather_ar_condition_buffer));
    persist_read_string(POP_KEY, weather_ar_pop_buffer,  sizeof(weather_ar_pop_buffer));
    {
      readRemoveWriteUpdate();
      layer_mark_dirty(slot_top);
    }
  }
  else
  {
  }
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler ends");
#endif
  
  layer_mark_dirty(slot_top);
}

static bool readRemoveWriteUpdate()
{
  bool ret = true;
  //APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_hour_buffer:%s", weather_ar_hour_buffer);
  ret = removeBufferFirstItem(weather_ar_hour_buffer);
  // APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_hour_buffer cropped:%s", weather_ar_hour_buffer);
  persist_write_string(HOUR_KEY, weather_ar_hour_buffer);
  updateArray(ar_time, weather_ar_hour_buffer);
  
  persist_read_string(TEMP_KEY, weather_ar_temp_layer_buffer, sizeof(weather_ar_temp_layer_buffer));
  // APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_temp_layer_buffer:%s", weather_ar_temp_layer_buffer);
  removeBufferFirstItem(weather_ar_temp_layer_buffer);
  // APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_temp_layer_buffer cropped:%s", weather_ar_temp_layer_buffer);
  persist_write_string(TEMP_KEY, weather_ar_temp_layer_buffer);
  updateArray(ar_temp, weather_ar_temp_layer_buffer);
  
  persist_read_string(COND_KEY, weather_ar_condition_buffer, sizeof(weather_ar_condition_buffer));
  removeBufferFirstItem(weather_ar_condition_buffer);
  persist_write_string(COND_KEY, weather_ar_condition_buffer);
  updateArray(ar_cond, weather_ar_condition_buffer);
  
  persist_read_string(WIND_KEY, weather_ar_wspd_buffer, sizeof(weather_ar_wspd_buffer));
  removeBufferFirstItem(weather_ar_wspd_buffer);
  persist_write_string(WIND_KEY, weather_ar_wspd_buffer);
  updateArray(ar_wspd, weather_ar_wspd_buffer);
  
  persist_read_string(POP_KEY, weather_ar_pop_buffer, sizeof(weather_ar_pop_buffer));
  removeBufferFirstItem(weather_ar_pop_buffer);
  persist_write_string(POP_KEY, weather_ar_pop_buffer);
  updateArray(ar_pop, weather_ar_pop_buffer);
  
  return ret;
}

static bool removeBufferFirstItem(char buf[])
{
  if (!checkBuffer(buf))
  {
    return false;
  }
  
  char * pch = 0;
  pch = strstr (buf, " ");
  if (!pch || pch == buf)
  {
    return false;
  }
  int offset = pch - buf;
  int size = BUF_SIZE - offset - 1;
  
  //APP_LOG(KEY_ARRAY_TEMPERATURE, "#offset: %d, size = %d", offset, size);
  ///TODO: free memory before cpy to itself?
  //   strncpy ( buf, buf + offset + 1, size );
  
  ///try to solve memory leak...
  ///http://stackoverflow.com/questions/12275381/strncpy-vs-sprintf
  snprintf(buf, size, "%s", buf + offset + 1);
  
  return true;
}

static void updateArray(char * ar[], char buf[]) {
  
  ///has NUM_HOURLY_FORECAST items, go on
  if (!checkBufferWithLimit(buf, NUM_HOURLY_FORECAST - 1))
  {
    return;
  }
  
#ifdef LOG_updateArray
  APP_LOG(APP_LOG_LEVEL_INFO, "updateArray-start = %s", buf);
#endif
  int len = (int)(strlen(buf));
#ifdef LOG_updateArray
  APP_LOG(APP_LOG_LEVEL_INFO, "updateArray-buf = %s %d", buf, len);
#endif
  int word_count = 0;
  
  char* word_start = NULL;
  
  //APP_LOG(KEY_ARRAY_TEMPERATURE, "len: %d", len);
  
  for (int i = 0; i < len; ++i)
  {
    
    if (word_start == NULL)
    {
      word_start = &buf[i];
    }
    
    if (buf[i] == ' ') {
      
      buf[i] = '\0';
      
      ar[word_count] = word_start;
      
      //APP_LOG(KEY_ARRAY_TEMPERATURE, "temp %d: %s", word_count, ar_temp[word_count]);
      
      ++word_count;
      if (word_count >= NUM_HOURLY_FORECAST)
      {
        word_start = NULL;
        break;
      }
      
      word_start = NULL;
      
    }
    
#ifdef LOG_updateArray
    APP_LOG(APP_LOG_LEVEL_INFO, "updateArray-word_count = %d", word_count);
#endif
  }
  
#ifdef LOG_updateArray
  APP_LOG(APP_LOG_LEVEL_INFO, "updateArray %d: %s", NUM_HOURLY_FORECAST - 1, ar[NUM_HOURLY_FORECAST - 1]);
#endif
  
  if (word_start != NULL) {
    
    ar[word_count] = word_start;
    
    ++word_count;
    
  }
  
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  // For all items
  while (t != NULL) {
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "key %u received", (unsigned)(t->key));
#endif
    // Which key was received?
    switch (t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%s", t->value->cstring);
        persist_write_string(USERNAME_KEY, temperature_buffer);
        break;
        
      case KEY_DEGREE:
        snprintf(degree_buffer, sizeof(degree_buffer), "%s", t->value->cstring);
        persist_write_string(DEGREE_KEY, degree_buffer);
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "degree_buffer %s received", degree_buffer);
#endif
        adjustDegFont();
        break;
        
      case KEY_MOON:
        snprintf(moon_buffer, sizeof(moon_buffer), "%s", t->value->cstring);
        persist_write_string(MOON_KEY, moon_buffer);
        text_layer_set_text(s_moon_layer, moon_buffer);
        
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "moon_buffer %s received", moon_buffer);
#endif
        break;
        
      case KEY_CONDITION:
        snprintf(condition_buffer, sizeof(condition_buffer), "%s", t->value->cstring);
        persist_write_string(CONDITION_KEY, condition_buffer);
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "condition_buffer %s received", condition_buffer);
#endif
        break;
        
      case KEY_WIND:
        snprintf(wind_buffer, sizeof(wind_buffer), "%s", t->value->cstring);
        persist_write_string(WIND_COND_KEY, wind_buffer);
        break;
        
        ///array of forecast
      case KEY_ARRAY_TIME:
        snprintf(weather_ar_hour_buffer, sizeof(weather_ar_hour_buffer), "%s", t->value->cstring);
        //      APP_LOG(KEY_ARRAY_TEMPERATURE, "KEY_ARRAY_TIME %s received", weather_ar_hour_buffer);
        persist_write_string(HOUR_KEY, weather_ar_hour_buffer);
        
        updateArray(ar_time, weather_ar_hour_buffer);
        isHourlyTime = 1;
        break;
      case KEY_ARRAY_TEMPERATURE:
        snprintf(weather_ar_temp_layer_buffer, sizeof(weather_ar_temp_layer_buffer), "%s", t->value->cstring);
        persist_write_string(TEMP_KEY, weather_ar_temp_layer_buffer);
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "%s received", weather_ar_temp_layer_buffer);
#endif
        
        updateArray(ar_temp, weather_ar_temp_layer_buffer);
        isHourly = 1;
        break;
      case KEY_ARRAY_WINDSPD:
        snprintf(weather_ar_wspd_buffer, sizeof(weather_ar_wspd_buffer), "%s", t->value->cstring);
        persist_write_string(WIND_KEY, weather_ar_wspd_buffer);
        
        updateArray(ar_wspd, weather_ar_wspd_buffer);
        isHourly = 1;
        
        break;
        
      case KEY_ARRAY_POP:
        snprintf(weather_ar_pop_buffer, sizeof(weather_ar_pop_buffer), "%s", t->value->cstring);
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "weather_ar_pop_buffer %s received", weather_ar_pop_buffer);
#endif
        persist_write_string(POP_KEY, weather_ar_pop_buffer);
        
        ///this line would destroy the array
        updateArray(ar_pop, weather_ar_pop_buffer);
        break;
        
      case KEY_ARRAY_CONDITION:
        snprintf(weather_ar_condition_buffer, sizeof(weather_ar_condition_buffer), "%s", t->value->cstring);
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "weather_ar_condition_buffer %s received", weather_ar_condition_buffer);
#endif
        persist_write_string(COND_KEY, weather_ar_condition_buffer);
        
#ifdef LOG_MSG
        APP_LOG(APP_LOG_LEVEL_INFO, "%s received", weather_ar_condition_buffer);
#endif
        ///this line would destroy the array
        updateArray(ar_cond, weather_ar_condition_buffer);
        isHourly = 1;
        
        break;
        
        ///setting
      case KEY_COLOR:
        color_key = ((int)t->value->int32 != 0);
        persist_write_bool(COLOR_KEY, color_key);
        
#ifdef LOG_COLOR
        APP_LOG(APP_LOG_LEVEL_ERROR, "color_key %d", color_key);
#endif
        window_set_background_color(s_main_window, color_key ? GColorBlack : GColorWhite);
        
        setLayerColor(s_date_layer, color_key);
        setLayerColor(s_time_layer, color_key);
        setLayerColor(s_time_min_layer, color_key);
        
        setLayerColor(s_moon_layer, color_key);
        setLayerColor(s_city_layer, color_key);
        
        setLayerColor(s_tempDeg_layer, color_key);
        setLayerColor(s_condition_layer, color_key);
        setLayerColor(s_wind_layer, color_key);
        
        layer_set_hidden((Layer *)s_moon_mask_layer, color_key ? true : false);
        
        // text_layer_set_text(s_moon_bg_layer, "o");
        //     text_layer_set_text(s_moon_layer, moon_buffer);
        //       text_layer_set_text(s_moon_mask_layer, "a");
        
        //     layer_set_hidden((Layer *)s_moon_layer, false);
        
        break;
        
      case KEY_POP:
        pop_key = (int)t->value->int32;
        persist_write_int(POP_SETTING_KEY, pop_key);
        
#ifdef LOG_POP
        APP_LOG(APP_LOG_LEVEL_ERROR, "pop_key recived: %d", pop_key);
#endif
        updatePop();
        layer_mark_dirty(slot_top);
        
        break;
        
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  //   snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
  text_layer_set_text(s_date_layer, weekday_string);
  text_layer_set_text(s_city_layer, temperature_buffer);
  
  text_layer_set_text(s_tempDeg_layer, degree_buffer);
  text_layer_set_text(s_condition_layer, condition_buffer);
  text_layer_set_text(s_wind_layer, wind_buffer);
  
  // text_layer_set_text(s_moon_layer, moon_buffer);
  
  text_layer_set_text_color(s_moon_layer, GColorWhite);
  
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_ERROR, "moon_buffer after setting: %s", moon_buffer);
  APP_LOG(APP_LOG_LEVEL_ERROR, "temperature_buffer: %s", temperature_buffer);
#endif
}

static char reasonStr[20];

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  
  switch (reason) {
    case APP_MSG_OK:
      snprintf(reasonStr, 20, "%s", "APP_MSG_OK");
      break;
    case APP_MSG_SEND_TIMEOUT:
      snprintf(reasonStr, 20, "%s", "SEND TIMEOUT");
      break;
    case APP_MSG_SEND_REJECTED:
      snprintf(reasonStr, 20, "%s", "SEND REJECTED");
      break;
    case APP_MSG_NOT_CONNECTED:
      snprintf(reasonStr, 20, "%s", "NOT CONNECTED");
      break;
    case APP_MSG_APP_NOT_RUNNING:
      snprintf(reasonStr, 20, "%s", "NOT RUNNING");
      break;
    case APP_MSG_INVALID_ARGS:
      snprintf(reasonStr, 20, "%s", "INVALID ARGS");
      break;
    case APP_MSG_BUSY:
      snprintf(reasonStr, 20, "%s", "BUSY"); ///TODO: happens when debug
      break;
    case APP_MSG_BUFFER_OVERFLOW:
      snprintf(reasonStr, 20, "%s", "BUFFER OVERFLOW");
      break;
    case APP_MSG_ALREADY_RELEASED:
      snprintf(reasonStr, 20, "%s", "ALRDY RELEASED");
      break;
    case APP_MSG_CALLBACK_ALREADY_REGISTERED:
      snprintf(reasonStr, 20, "%s", "CLB ALR REG");
      break;
    case APP_MSG_CALLBACK_NOT_REGISTERED:
      snprintf(reasonStr, 20, "%s", "CLB NOT REG");
      break;
    case APP_MSG_OUT_OF_MEMORY:
      snprintf(reasonStr, 20, "%s", "OUT OF MEM");
      break;
    default:
      snprintf(reasonStr, 20, "%s", "default");
      break;
      
  }
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "reasonStr %s", reasonStr);
  
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  
  ///read/generate device key, base on first run second, 0 is invalid value.
  device_key = persist_read_int(DEVICE_KEY);///init value is 0
  
  color_key = persist_read_bool(COLOR_KEY);
  pop_key = persist_read_int(POP_KEY);
  
  if (device_key == 0)
  {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    // Create a long-lived buffer
    static char buffer[] = "00";
    
    
    //Use 12 hour format
    strftime(buffer, sizeof("00"), "%S", tick_time);
    
    int iSecond = atoi(buffer);
    
    //         APP_LOG(APP_LOG_LEVEL_INFO, "second %s", buffer);
    //         APP_LOG(APP_LOG_LEVEL_INFO, "second int %d", iSecond);
    
    
    persist_write_int(DEVICE_KEY, iSecond);
  }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "DEVICE_KEY %d", device_key);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  
  ///for debugging caching consumsion
  //   tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
#ifndef DEBUG_NO_NETWORK
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif
  if (strlen(temperature_buffer) <= 0)
  {
    persist_read_string(USERNAME_KEY, temperature_buffer, sizeof(temperature_buffer));
  }
  
  ///TODO degBug
#ifdef LOG_MSG
  APP_LOG(APP_LOG_LEVEL_INFO, "init end - temperature_buffer %s", temperature_buffer);
#endif
}

static void deinit() {
  
  // Destroy Window
  window_destroy(s_main_window);
  
  tick_timer_service_unsubscribe();
  currentTime = 0;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
