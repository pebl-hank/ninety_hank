#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "stdlib.h"
#include "string.h"

#include "config.h"
#include "my_math.h"
#include "suncalc.h"

#define MY_UUID {0xE5, 0x2B, 0xC3, 0x7B, 0x66, 0x13, 0x49, 0x70, 0x80, 0x11, 0xD6, 0xBC, 0x51, 0x97, 0xC5, 0x11}
PBL_APP_INFO(MY_UUID,
	     "90 Hank", "Hank",
	     0, 4, /* App major/minor version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_WATCH_FACE);

Window window;

TextLayer moonLayer; // The moonphase

TextLayer cwLayer; // The calendar week

TextLayer text_sunrise_layer;
TextLayer text_sunset_layer;

TextLayer text_addTimeZone1_layer;

BmpContainer background_image;

BmpContainer time_format_image;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

BmpContainer day_name_image;

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};

#define TOTAL_MOON_DIGITS 1
BmpContainer moon_digits_images[TOTAL_MOON_DIGITS];

const int MOON_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_MOON_0,
  RESOURCE_ID_IMAGE_MOON_1,
  RESOURCE_ID_IMAGE_MOON_2,
  RESOURCE_ID_IMAGE_MOON_3,
  RESOURCE_ID_IMAGE_MOON_4,
  RESOURCE_ID_IMAGE_MOON_5,
  RESOURCE_ID_IMAGE_MOON_6,
  RESOURCE_ID_IMAGE_MOON_7
};

#define TOTAL_DATE_DIGITS 8
BmpContainer date_digits_images[TOTAL_DATE_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_TIME_DIGITS 4
BmpContainer time_digits_images[TOTAL_TIME_DIGITS];

void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin) {
  layer_remove_from_parent(&bmp_container->layer.layer);
  bmp_deinit_container(bmp_container);

  bmp_init_container(resource_id, bmp_container);

  GRect frame = layer_get_frame(&bmp_container->layer.layer);
  frame.origin.x = origin.x;
  frame.origin.y = origin.y;
  layer_set_frame(&bmp_container->layer.layer, frame);

  layer_add_child(&window.layer, &bmp_container->layer.layer);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}


int moon_phase(int y, int m, int d)
{
    /*
      calculates the moon phase (0-7), accurate to 1 segment.
      0 = > new moon.
      4 => full moon.
      */
    int c,e;
    double jd;
    int b;

    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25*y;
    e = 30.6*m;
    jd = c+e+d-694039.09;  	/* jd is total days elapsed */
    jd /= 29.53;        	/* divide by the moon cycle (29.53 days) */
    b = jd;		   			/* int(jd) -> b, take integer part of jd */
    jd -= b;		   		/* subtract integer part to leave fractional part of original jd */
    b = jd*8 + 0.5;	   		/* scale fraction from 0-8 and round by adding 0.5 */
    b = b & 7;		   		/* 0 and 8 are the same so turn 8 into 0 */
    return b;
}

void adjustTimezone(float* time) 
{
  *time += TIMEZONE;
  if (*time > 24) *time -= 24;
  if (*time < 0) *time += 24;
}

void updateSunsetSunrise()
{
	// Calculating Sunrise/sunset with courtesy of Michael Ehrmann
	// https://github.com/mehrmann/pebble-sunclock
	static char sunrise_text[] = "00:00";
	static char sunset_text[]  = "00:00";
	
	PblTm pblTime;
	get_time(&pblTime);

	char *time_format;

	if (clock_is_24h_style()) 
	{
	  time_format = "%R";
	} 
	else 
	{
	  time_format = "%I:%M";
	}

	float sunriseTime = calcSunRise(pblTime.tm_year, pblTime.tm_mon+1, pblTime.tm_mday, LATITUDE, LONGITUDE, 91.0f);
	float sunsetTime = calcSunSet(pblTime.tm_year, pblTime.tm_mon+1, pblTime.tm_mday, LATITUDE, LONGITUDE, 91.0f);
	adjustTimezone(&sunriseTime);
	adjustTimezone(&sunsetTime);

	if (!pblTime.tm_isdst) 
	{
	  sunriseTime+=1;
	  sunsetTime+=1;
	} 

	pblTime.tm_min = (int)(60*(sunriseTime-((int)(sunriseTime))));
	pblTime.tm_hour = (int)sunriseTime;
	string_format_time(sunrise_text, sizeof(sunrise_text), time_format, &pblTime);
	text_layer_set_text(&text_sunrise_layer, sunrise_text);

	pblTime.tm_min = (int)(60*(sunsetTime-((int)(sunsetTime))));
	pblTime.tm_hour = (int)sunsetTime;
	string_format_time(sunset_text, sizeof(sunset_text), time_format, &pblTime);
	text_layer_set_text(&text_sunset_layer, sunset_text);
}

unsigned short the_last_hour = 0;

void update_display(PblTm *current_time) {
  
  unsigned short display_hour = get_display_hour(current_time->tm_hour);
  
  //Hour
  set_container_image(&time_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(4, 94));
  set_container_image(&time_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(37, 94));
  //Minute
  set_container_image(&time_digits_images[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(80, 94));
  set_container_image(&time_digits_images[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(111, 94));
  
  // ======== Time Zone 1  
  text_layer_set_text(&text_addTimeZone1_layer, AdditionalTimezone_1_Description); 
  short  display_hour_tz1 = display_hour AdditionalTimezone_1;
  if (display_hour_tz1 > 24) display_hour_tz1 -= 24;
  if (display_hour_tz1 < 0) display_hour_tz1 += 24;
  set_container_image(&date_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[display_hour_tz1/10], GPoint(75, 5));
  set_container_image(&date_digits_images[5], DATENUM_IMAGE_RESOURCE_IDS[display_hour_tz1%10], GPoint(88, 5));  
  set_container_image(&date_digits_images[6], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(108, 5));
  set_container_image(&date_digits_images[7], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(121, 5));  
  // ======== Time Zone 1  
  
  
  if (the_last_hour != display_hour){
	  // Day of week
	  set_container_image(&day_name_image, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(30, 71));

	  // Day
	  set_container_image(&date_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday/10], GPoint(75, 71));
	  set_container_image(&date_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday%10], GPoint(88, 71));
	 
	  // Month
	  set_container_image(&date_digits_images[2], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon+1)/10], GPoint(108, 71));
	  set_container_image(&date_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon+1)%10], GPoint(121, 71));

	  if (!clock_is_24h_style()) {
		if (current_time->tm_hour >= 12) {
		  set_container_image(&time_format_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(10, 78));
		} else {
		  layer_remove_from_parent(&time_format_image.layer.layer);
		  bmp_deinit_container(&time_format_image);
		}

		if (display_hour/10 == 0) {
		  layer_remove_from_parent(&time_digits_images[0].layer.layer);
		  bmp_deinit_container(&time_digits_images[0]);
		}
	  }
		 
	// -------------------- Moon_phase
	  int moonphase_number;
	  moonphase_number = moon_phase(current_time->tm_year+1900,current_time->tm_mon,current_time->tm_mday);

	  set_container_image(&moon_digits_images[0], MOON_IMAGE_RESOURCE_IDS[moonphase_number], GPoint(1, 1));  // ---------- Moon phase Image

	  char * moonphase_text = "--";
	  if (moonphase_number == 0) moonphase_text = "NM"; 
	  if (moonphase_number == 1) moonphase_text = "NM+"; 
	  if (moonphase_number == 2) moonphase_text = "NM++"; 
	  if (moonphase_number == 3) moonphase_text = "VM-"; 
	  if (moonphase_number == 4) moonphase_text = "VM"; 
	  if (moonphase_number == 5) moonphase_text = "VM+"; 
	  if (moonphase_number == 6) moonphase_text = "VM++"; 
	  if (moonphase_number == 7) moonphase_text = "NM-"; 

	  text_layer_set_text(&moonLayer, moonphase_text);
	// -------------------- Moon_phase
	  
	// -------------------- Calendar week  
	  static char cw_text[] = "XX00";
	  string_format_time(cw_text, sizeof(cw_text), "KW%V", current_time);
	  text_layer_set_text(&cwLayer, cw_text); 
	// ------------------- Calendar week  
	the_last_hour = display_hour;
	updateSunsetSunrise();
  }
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  update_display(t->tick_time);
}


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "90 Hank");
  window_stack_push(&window, true /* Animated */);
  
  window_set_background_color(&window, GColorBlack);
  
  resource_init_current_app(&APP_RESOURCES);

  bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image);
  layer_add_child(&window.layer, &background_image.layer.layer);

 
  if (clock_is_24h_style()) {
    bmp_init_container(RESOURCE_ID_IMAGE_24_HOUR_MODE, &time_format_image);

    time_format_image.layer.layer.frame.origin.x = 2;
    time_format_image.layer.layer.frame.origin.y = 78;

    layer_add_child(&window.layer, &time_format_image.layer.layer);
  }

  // Moon Text
  text_layer_init(&moonLayer, GRect(2, 25, 50 /* width */, 30 /* height */));
  layer_add_child(&background_image.layer.layer, &moonLayer.layer);
  text_layer_set_text_color(&moonLayer, GColorWhite);
  text_layer_set_background_color(&moonLayer, GColorClear);
  text_layer_set_font(&moonLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  // Calendar Week Text
  text_layer_init(&cwLayer, GRect(2, 40, 80 /* width */, 30 /* height */));
  layer_add_child(&background_image.layer.layer, &cwLayer.layer);
  text_layer_set_text_color(&cwLayer, GColorWhite);
  text_layer_set_background_color(&cwLayer, GColorClear);
  text_layer_set_font(&cwLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  // Sunrise Text
  text_layer_init(&text_sunrise_layer, window.layer.frame);
  text_layer_set_text_color(&text_sunrise_layer, GColorWhite);
  text_layer_set_background_color(&text_sunrise_layer, GColorClear);
  layer_set_frame(&text_sunrise_layer.layer, GRect(7, 152, 100, 30));
  text_layer_set_font(&text_sunrise_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(&window.layer, &text_sunrise_layer.layer);

  // Sunset Text
  text_layer_init(&text_sunset_layer, window.layer.frame);
  text_layer_set_text_color(&text_sunset_layer, GColorWhite);
  text_layer_set_background_color(&text_sunset_layer, GColorClear);
  layer_set_frame(&text_sunset_layer.layer, GRect(110, 152, 100, 30));
  text_layer_set_font(&text_sunset_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(&window.layer, &text_sunset_layer.layer); 
  
  // Text for Additional Time Zone 1
  text_layer_init(&text_addTimeZone1_layer, window.layer.frame);
  text_layer_set_text_color(&text_addTimeZone1_layer, GColorWhite);
  text_layer_set_background_color(&text_addTimeZone1_layer, GColorClear);
  layer_set_frame(&text_addTimeZone1_layer.layer, GRect(55, 5, 100, 30));
  text_layer_set_font(&text_addTimeZone1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(&window.layer, &text_addTimeZone1_layer.layer);  
   
  // Avoids a blank screen on watch start.
  PblTm tick_time;

  get_time(&tick_time);
  update_display(&tick_time);

}


void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&background_image);
  bmp_deinit_container(&time_format_image);
  bmp_deinit_container(&day_name_image);

  for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    bmp_deinit_container(&date_digits_images[i]);
  }

  for (int i = 0; i < TOTAL_MOON_DIGITS; i++) {
    bmp_deinit_container(&moon_digits_images[i]);
  }  

  for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    bmp_deinit_container(&time_digits_images[i]);
  }

}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
