// #define LOG_MSG

#include <pebble.h>

//#define DEBUG_FETCH_MORE

///local storage keys
# define USERNAME_KEY 0
# define COND_KEY 1
# define HOUR_KEY 2
# define TEMP_KEY 3
# define WIND_KEY 4
# define COLOR_KEY 5

#define DEVICE_KEY 10
///msg keys
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_FEELS_LIKE 2
#define KEY_WIND_GUST 3
#define KEY_WIND_DEG 4

#define KEY_ARRAY_TEMPERATURE 5
#define KEY_ARRAY_CONDITION 6
#define KEY_ARRAY_TIME 7
#define KEY_ARRAY_WINDSPD 8
//#define KEY_ARRAY_POP 9 ///TODO: % of rain fall
#define KEY_CITY 10
#define KEY_COLOR 11
  
#define NUM_HOURLY_FORECAST 6

static int device_key = 1;
static bool color_key = true; ///1 is black

///set to 36*6-> crash?
#define BUF_SIZE 36*3 ///5 char + space, 36 hr buffer
static char weather_ar_temp_layer_buffer[BUF_SIZE];///string from JS
static char weather_ar_hour_buffer[BUF_SIZE];///4*10 = 40?
static char weather_ar_wspd_buffer[BUF_SIZE];///4*10 = 40?
static char weather_ar_condition_buffer[BUF_SIZE];///4*10 = 40?


static char * ar_temp[NUM_HOURLY_FORECAST];
static char * ar_cond[NUM_HOURLY_FORECAST];
static char * ar_time[NUM_HOURLY_FORECAST];
static char * ar_wspd[NUM_HOURLY_FORECAST];
// static char * ar_pop[NUM_HOURLY_FORECAST];

///buffer for text drawing
static char buf[5];

///current condition buffer
static char temperature_buffer[80];///for local storage caching

static int isHourly = 0;
static int isHourlyTime = 0;

// primary coordinates
#define DEVICE_WIDTH        144
#define DEVICE_HEIGHT       168

#define LAYOUT_SLOT_BOT      0 // 72 tall, 4px gap above
#define LAYOUT_SLOT_HEIGHT   64

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

static Layer *slot_bot;

static GFont s_time_font;
static GFont s_weather_font;
static GFont s_icon_font;

static void updateArray(char * ar[], char buf[]);
static void removeBufferFirstItem(char buf[]);
static void cropBuffer(int key, char buf[], char * ar[]);

static void update_time() {
  APP_LOG(APP_LOG_LEVEL_INFO, "update_time starts");

    // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

    // Create a long-lived buffer
  static char buffer[] = "00:00";

    // Write the current hours and minutes into the buffer
  if (clock_is_24h_style() == true) {
        //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
        //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

    // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);

    //   last_counter = -1;
  APP_LOG(APP_LOG_LEVEL_INFO, "update_time ends");
}

void setColors(GContext* ctx) {
  window_set_background_color(s_main_window, color_key?GColorBlack:GColorWhite);
  graphics_context_set_stroke_color(ctx, color_key?GColorWhite:GColorBlack);
  graphics_context_set_fill_color(ctx, color_key?GColorBlack:GColorWhite);
  graphics_context_set_text_color(ctx, color_key?GColorWhite:GColorBlack);
}

void setInvColors(GContext* ctx) {
  window_set_background_color(s_main_window, color_key?GColorWhite:GColorBlack);
  graphics_context_set_stroke_color(ctx, color_key?GColorBlack:GColorWhite);
  graphics_context_set_fill_color(ctx, color_key?GColorWhite:GColorBlack);
  graphics_context_set_text_color(ctx, color_key?GColorBlack:GColorWhite);
}

//#define CAL_HOURS   4   // number of columns (4 hour forecast)
#define CAL_WIDTH  (DEVICE_WIDTH/NUM_HOURLY_FORECAST)  // width of columns
#define CAL_GAP    1   // gap around calendar
#define CAL_LEFT   0   // left side of calendar
#define CAL_HEIGHT 85  // How tall rows should be depends on how many weeks there are

static void rect_layer_update_callback(Layer *layer, GContext *ctx) {

    int  specialHour = 0; ///highlight this hour

    for (int col = 0; col < NUM_HOURLY_FORECAST; col++) {
        //current = s_normal_font;
      if (col == specialHour) {
        setInvColors(ctx);
      }

        // draw the cell background
      graphics_fill_rect(ctx, GRect (CAL_WIDTH * col + CAL_LEFT + CAL_GAP,
       0 , CAL_WIDTH - CAL_GAP, CAL_HEIGHT - CAL_GAP), 0, GCornerNone);

      if (col == specialHour) {
        graphics_context_set_text_color(ctx, color_key?GColorBlack:GColorWhite);
      }
      else
      {
        graphics_context_set_text_color(ctx, color_key?GColorWhite:GColorBlack);
      }

      if (isHourly == 1)
      {
        snprintf(buf, sizeof(buf), "%s°", ar_temp[col]);

        graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
         GRect(CAL_WIDTH * col + CAL_LEFT,
           17,
           CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

      }

      if (isHourlyTime == 1)
      {
        if(ar_time[col])
        {
          graphics_draw_text(ctx, ar_time[col], fonts_get_system_font(FONT_KEY_GOTHIC_18),
           GRect(CAL_WIDTH * col + CAL_LEFT,
             0,
             CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
        }

      }

      {
            ///wind spd
        graphics_draw_text(ctx, ar_wspd[col], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                               GRect(CAL_WIDTH * col + CAL_LEFT - 2,// -2 pixel, for left most 8NW case
                                 45,
                                     CAL_WIDTH + 6, ///more space for case like "99NW
                                     CAL_HEIGHT * 2), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

        graphics_draw_text(ctx, ar_cond[col], s_icon_font,
         GRect(CAL_WIDTH * col + CAL_LEFT,
           32,
           30, 30), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

            //snprintf(buf, sizeof(buf), "%s", ar_cond[col]);
            //APP_LOG(KEY_ARRAY_TEMPERATURE, "buf %s", buf);
      }

      if (col == specialHour) {
        setColors(ctx);
      }
    }
  }

struct tm *currentTime;

struct tm *get_time() {
    time_t tt = time(0);
    return localtime(&tt);
}

  static void main_window_load(Window *window) {
    // Create time TextLayer
    s_time_layer = text_layer_create(GRect(5, 52, 139, 50));
    text_layer_set_background_color(s_time_layer, GColorClear);
    
    text_layer_set_text_color(s_time_layer, color_key?GColorWhite:GColorBlack);
    text_layer_set_text(s_time_layer, "00:00");

    //Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
    s_icon_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_15));

    //Apply to TextLayer
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    ///date and week day layer
    currentTime = get_time();
    int weekDay = currentTime->tm_wday;
    APP_LOG(APP_LOG_LEVEL_INFO, "weekDay %d", weekDay);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

    // Create weather Layer
    int y_weather = 105;
    s_weather_layer = text_layer_create(GRect(0, y_weather, 144, 168 - y_weather));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, color_key?GColorWhite:GColorBlack);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);

    // Create second custom font, apply it and add to Window
    s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

    slot_bot = layer_create(GRect(0, LAYOUT_SLOT_BOT, DEVICE_WIDTH, LAYOUT_SLOT_HEIGHT));
    layer_add_child(window_get_root_layer(window), slot_bot);

    layer_set_update_proc(slot_bot, rect_layer_update_callback);

    update_time();

    ////read storage
    if (strlen(temperature_buffer) <= 0)
    {
      persist_read_string(USERNAME_KEY, temperature_buffer, sizeof(temperature_buffer));
    }

#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load temperature_buffer %s", temperature_buffer);
#endif
    
    if (strlen(temperature_buffer) > 0)
    {
      text_layer_set_text(s_weather_layer, temperature_buffer);
    }
    else
    {
      text_layer_set_text(s_weather_layer, "Loading...");
    }

    text_layer_set_overflow_mode(s_weather_layer, GTextOverflowModeWordWrap);
    ///////////

    persist_read_string(COND_KEY, weather_ar_condition_buffer, sizeof(weather_ar_condition_buffer));
    if (strlen(weather_ar_condition_buffer) > 10)
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
    if (strlen(weather_ar_wspd_buffer) > 10)
    {
      updateArray(ar_wspd, weather_ar_wspd_buffer);
    }

    layer_mark_dirty(slot_bot);

  }

  static void main_window_unload(Window *window) {
    //Unload GFont
    fonts_unload_custom_font(s_time_font);
    fonts_unload_custom_font(s_weather_font);
    fonts_unload_custom_font(s_icon_font);

    // Destroy TextLayer
    text_layer_destroy(s_time_layer);

    layer_destroy(slot_bot);
    //layer_destroy(s_weather_table_layer);

  }

  static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler starts");

    update_time();

    // Get weather update every 60 minutes
#ifdef DEBUG_FETCH_MORE
    if (tick_time->tm_min % 5 == 0)
#else
//    APP_LOG(APP_LOG_LEVEL_INFO, "before if check %d, device_key = %d", tick_time->tm_min, device_key);
    if (tick_time->tm_min == device_key)
#endif
      {
      //  APP_LOG(APP_LOG_LEVEL_INFO, "tick_time->tm_min == device_key");

  // Begin dictionary
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);

        // Add a key-value pair
        dict_write_uint8(iter, 0, 0);

        // Send the message!
        app_message_outbox_send();
      }
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
      
      bool isDebug = false;
      
      int iItem = atoi(ar_time[0]);
      int iBuf = atoi(buf_time_tick);
      int iMin = atoi(buf_time_min);

      if(isDebug || (ar_time[0] && iItem == iBuf && iMin == 0))
      {
       APP_LOG(KEY_ARRAY_TEMPERATURE, "buf_time: %s", buf_time_tick);
       APP_LOG(KEY_ARRAY_TEMPERATURE, "ar_time[0]: %s", ar_time[0]);
       APP_LOG(KEY_ARRAY_TEMPERATURE, "ar_temp[0]: %s", ar_temp[0]);
       
       persist_read_string(HOUR_KEY, weather_ar_hour_buffer, sizeof(weather_ar_hour_buffer));
       int i=0;
       char * s = weather_ar_hour_buffer;
       
       if(!s)
       {
        return;
      }
      for (i=0; s[i]; s[i]==' ' ? i++ : *s++);
       if(i > 5)/// slot buffer
       {
         //APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_hour_buffer:%s", weather_ar_hour_buffer);
         removeBufferFirstItem(weather_ar_hour_buffer);
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
         APP_LOG(KEY_ARRAY_TEMPERATURE, "weather_ar_condition_buffer cropped %s", weather_ar_condition_buffer);
         updateArray(ar_cond, weather_ar_condition_buffer);
         
         persist_read_string(WIND_KEY, weather_ar_wspd_buffer, sizeof(weather_ar_wspd_buffer));
         removeBufferFirstItem(weather_ar_wspd_buffer);
         persist_write_string(WIND_KEY, weather_ar_wspd_buffer);
         updateArray(ar_wspd, weather_ar_wspd_buffer);
         
//          cropBuffer(TEMP_KEY, weather_ar_temp_layer_buffer, ar_temp);

         //        cropBuffer(COND_KEY, weather_ar_condition_buffer, ar_cond);
//        cropBuffer(WIND_KEY, weather_ar_wspd_buffer, ar_wspd);
         
         
         layer_mark_dirty(slot_bot);
       }
     }
     else
     {
         //APP_LOG(KEY_ARRAY_TEMPERATURE, "else %d %d ", iItem, iBuf);
       //APP_LOG(KEY_ARRAY_TEMPERATURE, "else buf_time: %s", buf_time_tick);
     }
//     APP_LOG(APP_LOG_LEVEL_INFO, "tick_handler ends");

   }

   static void cropBuffer(int key, char buf[], char * ar[])
   {
     persist_read_string(key, buf, sizeof(buf));
     APP_LOG(KEY_ARRAY_TEMPERATURE, "buf: %s", buf);
     removeBufferFirstItem(buf);
     APP_LOG(KEY_ARRAY_TEMPERATURE, "buf cropped:%s", buf);
     persist_write_string(key, buf);
     updateArray(ar, buf);
   }

   static void removeBufferFirstItem(char buf[])
   {
    char * pch = 0;
    pch = strstr (buf," ");
    if(!pch || pch == buf)
    {
      return;
    }
    int offset = pch - buf;
    int size = BUF_SIZE-offset-1;

           //APP_LOG(KEY_ARRAY_TEMPERATURE, "#offset: %d, size = %d", offset, size);

    strncpy ( buf, buf+offset+1, size );
  }

  static void updateArray(char * ar[], char buf[]){
    int word_count = 0;

    char* word_start = NULL;

    int len = (int)(strlen(buf));
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

    }

#ifdef LOG_MSG
    APP_LOG(KEY_ARRAY_TEMPERATURE, "updateArray %d: %s", NUM_HOURLY_FORECAST - 1, ar[NUM_HOURLY_FORECAST - 1]);
#endif 
    
    if (word_start != NULL) {

      ar[word_count] = word_start;

      ++word_count;

    }

  }

  static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Store incoming information
    static char conditions_buffer[32];
    static char weather_layer_buffer[80];///3 lines of text, 32 is not enough

    //     static char weather_wind_speed_layer_buffer[32];
    static char weather_wind_gust_layer_buffer[32];
    static char weather_wind_deg_layer_buffer[32];

    static char weather_city_layer_buffer[32];
    static char weather_feels_like_layer_buffer[32];


    // Read first item
    Tuple *t = dict_read_first(iterator);

    // For all items
    while (t != NULL) {
        // Which key was received?
      switch (t->key) {
        case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%s", t->value->cstring);
        break;
        case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
        break;
        case KEY_WIND_GUST:
        snprintf(weather_wind_gust_layer_buffer, sizeof(weather_wind_gust_layer_buffer), "%s", t->value->cstring);
        break;
        case KEY_WIND_DEG:
        snprintf(weather_wind_deg_layer_buffer, sizeof(weather_wind_deg_layer_buffer), "%d°", (int)t->value->int32);
        break;
        case KEY_CITY:
        snprintf(weather_city_layer_buffer, sizeof(weather_city_layer_buffer), "%s", t->value->cstring);
        break;
        case KEY_FEELS_LIKE:
        snprintf(weather_feels_like_layer_buffer, sizeof(weather_feels_like_layer_buffer), "%d°", (int)t->value->int32);
        break;

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
        APP_LOG(KEY_ARRAY_TEMPERATURE, "%s received", weather_ar_temp_layer_buffer);
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
        case KEY_ARRAY_CONDITION:
        snprintf(weather_ar_condition_buffer, sizeof(weather_ar_condition_buffer), "%s", t->value->cstring);
        persist_write_string(COND_KEY, weather_ar_condition_buffer);

#ifdef LOG_MSG
        APP_LOG(KEY_ARRAY_TEMPERATURE, "%s received", weather_ar_condition_buffer);
#endif
            ///this line would destroy the array
        updateArray(ar_cond, weather_ar_condition_buffer);
        isHourly = 1;

        break;
        case KEY_COLOR:
        color_key = ((int)t->value->int32 != 0);
                persist_write_bool(COLOR_KEY, color_key);
        APP_LOG(APP_LOG_LEVEL_ERROR, "color_key %d", color_key);
        
    layer_mark_dirty(s_time_layer);
    layer_mark_dirty(s_weather_layer);

        break;

        default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
      }

        // Look for next item
      t = dict_read_next(iterator);
    }

    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);

#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_ERROR, "weather_layer_buffer: %s", weather_layer_buffer);
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

  if(device_key == 0)
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
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    if (strlen(temperature_buffer) <= 0)
    {
      persist_read_string(USERNAME_KEY, temperature_buffer, sizeof(temperature_buffer));
    }
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "init temperature_buffer %s", temperature_buffer);
#endif
  }

  static void deinit() {
    
    // Destroy Window
    window_destroy(s_main_window);

    persist_write_string(USERNAME_KEY, temperature_buffer);
#ifdef LOG_MSG
    APP_LOG(APP_LOG_LEVEL_INFO, "deinit temperature_buffer %s", temperature_buffer);
#endif
  }

  int main(void) {
    init();
    app_event_loop();
    deinit();
  }
