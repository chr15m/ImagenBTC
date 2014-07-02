#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "util.h"
#include "http.h"

#define MY_UUID { 0x4F, 0x4E, 0xEE, 0xFA, 0x16, 0xB0, 0x46, 0xFE, 0x82, 0x1E, 0xDC, 0xCF, 0xB5, 0xD2, 0x27, 0x23 }
PBL_APP_INFO(MY_UUID,
	"Torino", "Chris McCormick",
	1, 0, /* App version */
	RESOURCE_ID_IMAGE_MENU_ICON,
	APP_INFO_WATCH_FACE);

#include "build_config.h"
#define HOUR_VIBRATION_START 8
#define HOUR_VIBRATION_END 22

/***** weather stuff *****/

// POST variables
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2
#define WEATHER_KEY_UNIT_SYSTEM 3

// Received variables
#define WEATHER_KEY_ICON 1
#define WEATHER_KEY_TEMPERATURE 2
#define WEATHER_KEY_CITYNAME 3
#define INFO_KEY_MESSAGE 4
#define INFO_KEY_ERROR 5

#define WEATHER_HTTP_COOKIE 1949327673

static int our_latitude, our_longitude;
static bool located;

/*void request_weather();
void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);
void failed(int32_t cookie, int http_status, void* context);
void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context);
void reconnect(void* context);*/

/***** everything else *****/

Window window;
BmpContainer background_image_container;

Layer minute_display_layer;
Layer hour_display_layer;
Layer center_display_layer;
#if DISPLAY_SECONDS
Layer second_display_layer;
#endif

TextLayer info_layer;
GFont info_font;
static char info_text[] = "             ";

TextLayer location_layer;
GFont location_font;
static char location_text[] = "pebble                                  ";

TextLayer date_layer;
GFont date_font;
static char date_text[] = "          ";

TextLayer weather_layer;
GFont weather_font;
static char weather_text[] = "     ";

TextLayer temperature_layer;
GFont temperature_font;
static char temperature_text[] = "     ";

const GPathInfo MINUTE_HAND_PATH_POINTS = {
	4,
	(GPoint []) {
		{-3, 0},
		{3, 0},
		{3, -60},
		{-3,	-60},
	}
};

const GPathInfo HOUR_HAND_PATH_POINTS = {
	4,
	(GPoint []) {
		{-3, 0},
		{3, 0},
		{3, -40},
		{-3,	-40},
	}
};

GPath hour_hand_path;
GPath minute_hand_path;

#if DISPLAY_SECONDS && INVERTED
void second_display_layer_update_callback(Layer *me, GContext* ctx) {
	(void)me;

	PblTm t;
	get_time(&t);

	int32_t second_angle = t.tm_sec * (0xffff/60);
	int32_t counter_second_angle = t.tm_sec * (0xffff/60);
	if(t.tm_sec<30)
	{
		 counter_second_angle += 0xffff/2;
	}
	else
	{
		 counter_second_angle -= 0xffff/2;
	}
	int second_hand_length = 70;
	int counter_second_hand_length = 15;

	GPoint center = grect_center_point(&me->frame);
	GPoint counter_second = GPoint(center.x + counter_second_hand_length * sin_lookup(counter_second_angle)/0xffff,
				center.y + (-counter_second_hand_length) * cos_lookup(counter_second_angle)/0xffff);
	GPoint second = GPoint(center.x + second_hand_length * sin_lookup(second_angle)/0xffff,
				center.y + (-second_hand_length) * cos_lookup(second_angle)/0xffff);

	graphics_context_set_stroke_color(ctx, GColorBlack);

	graphics_draw_line(ctx, counter_second, second);
}
#elif DISPLAY_SECONDS
void second_display_layer_update_callback(Layer *me, GContext* ctx) {
	(void)me;

	PblTm t;
	get_time(&t);

	int32_t second_angle = t.tm_sec * (0xffff/60);
	int32_t counter_second_angle = t.tm_sec * (0xffff/60);
	if(t.tm_sec<30)
	{
		 counter_second_angle += 0xffff/2;
	}
	else
	{
		 counter_second_angle -= 0xffff/2;
	}
	int second_hand_length = 70;
	int counter_second_hand_length = 15;

	GPoint center = grect_center_point(&me->frame);
	GPoint counter_second = GPoint(center.x + counter_second_hand_length * sin_lookup(counter_second_angle)/0xffff,
				center.y + (-counter_second_hand_length) * cos_lookup(counter_second_angle)/0xffff);
	GPoint second = GPoint(center.x + second_hand_length * sin_lookup(second_angle)/0xffff,
				center.y + (-second_hand_length) * cos_lookup(second_angle)/0xffff);

	graphics_context_set_stroke_color(ctx, GColorWhite);

	graphics_draw_line(ctx, counter_second, second);
}
#endif

void center_display_layer_update_callback(Layer *me, GContext* ctx) {
	(void)me;

	GPoint center = grect_center_point(&me->frame);
#if INVERTED
	//graphics_context_set_fill_color(ctx, GColorWhite);
	//graphics_fill_circle(ctx, center, 5);
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_circle(ctx, center, 4);
#else
	//graphics_context_set_fill_color(ctx, GColorBlack);
	//graphics_fill_circle(ctx, center, 5);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_circle(ctx, center, 4);
#endif
}

void minute_display_layer_update_callback(Layer *me, GContext* ctx) {
	(void)me;

	PblTm t;

	get_time(&t);

	unsigned int angle = t.tm_min * 6 + t.tm_sec / 10;
	gpath_rotate_to(&minute_hand_path, (TRIG_MAX_ANGLE / 360) * angle);
	
#if INVERTED
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_context_set_stroke_color(ctx, GColorWhite);
#else
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorBlack);
#endif

	gpath_draw_filled(ctx, &minute_hand_path);
	gpath_draw_outline(ctx, &minute_hand_path);
}

void hour_display_layer_update_callback(Layer *me, GContext* ctx) {
	(void)me;

	PblTm t;

	get_time(&t);

	unsigned int angle = t.tm_hour * 30 + t.tm_min / 2;
	gpath_rotate_to(&hour_hand_path, (TRIG_MAX_ANGLE / 360) * angle);

#if INVERTED
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_context_set_stroke_color(ctx, GColorWhite);
#else
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorBlack);
#endif

	gpath_draw_filled(ctx, &hour_hand_path);
	gpath_draw_outline(ctx, &hour_hand_path);
}

void draw_date(){
	PblTm t;
	get_time(&t);
	
	string_format_time(date_text, sizeof(date_text), "%a %d %b", &t);
	text_layer_set_text(&date_layer, date_text);
}

void draw_weather(char *icon, int temperature) {
	// strcpy(weather_text, "");
	strcpy(weather_text, icon);
	text_layer_set_text(&weather_layer, weather_text);
	// strcpy(temperature_text, "31°");
	// strcpy(temperature_text, temperature);
	if (temperature != -127) {
		// sprintf(temperature_text, "%d°", temperature);
		strcpy(temperature_text, itoa(temperature));
		strcat(temperature_text, "°");
	} else {
		strcpy(temperature_text, "");
	}
	text_layer_set_text(&temperature_layer, temperature_text);
}

void draw_location(char *location) {
	strcpy(location_text, location);
	text_layer_set_text(&location_layer, location_text);
}

void draw_info(char *info) {
	strcpy(info_text, info);
	text_layer_set_text(&info_layer, info_text);
}

void clear() {
	// clear to base view
	strcpy(location_text, "pebble");
	text_layer_set_text(&location_layer, location_text);
	strcpy(weather_text, "");
	text_layer_set_text(&weather_layer, weather_text);
	strcpy(temperature_text, "");
	text_layer_set_text(&temperature_layer, temperature_text);
	strcpy(info_text, "");
	text_layer_set_text(&info_layer, info_text);
}

void set_timer(AppContextRef ctx) {
	app_timer_send_event(ctx, 420000, 1);
}

/***** HTTP stuff *****/

void request_weather() {
	if(!located) {
		// draw_info("request loc");
		http_location_request();
		return;
	}
	// Build the HTTP request
	DictionaryIterator *body;
	HTTPResult result = http_out_get(URL, WEATHER_HTTP_COOKIE, &body);
	if(result != HTTP_OK) {
		// draw_info("!HTTP_OK");
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		//clear();
		draw_info("");
		return;
	}
	dict_write_int32(body, WEATHER_KEY_LATITUDE, our_latitude);
	dict_write_int32(body, WEATHER_KEY_LONGITUDE, our_longitude);
	dict_write_cstring(body, WEATHER_KEY_UNIT_SYSTEM, TEMPERATURE_UNIT);
	// Send it.
	if(http_out_send() != HTTP_OK) {
		// draw_info("!HTTP_OK2");
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		//clear();
		draw_info("");
		return;
	}
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
	// draw_info("location");
	// draw_info(itoa(latitude * 10000));
	// Fix the floats
	our_latitude = latitude * 10000;
	our_longitude = longitude * 10000;
	located = true;
	request_weather();
}

void failed(int32_t cookie, int http_status, void* context) {
	if(cookie == 0 || cookie == WEATHER_HTTP_COOKIE) {
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		// clear();
		// draw_info("failed");
	}
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	if(cookie != WEATHER_HTTP_COOKIE) return;
	
	Tuple* message_tuple = dict_find(received, INFO_KEY_MESSAGE);
	if (message_tuple) {
		draw_info(message_tuple->value->cstring);
	}
	
	Tuple* cityname_tuple = dict_find(received, WEATHER_KEY_CITYNAME);
	if (cityname_tuple) {
		draw_location(cityname_tuple->value->cstring);
	}
	
	Tuple* icon_tuple = dict_find(received, WEATHER_KEY_ICON);
	Tuple* temperature_tuple = dict_find(received, WEATHER_KEY_TEMPERATURE);
	if (icon_tuple && temperature_tuple) {
		draw_weather(icon_tuple->value->cstring, temperature_tuple->value->int16);
	}
}

void reconnect(void* context) {
	// draw_info("reconnected");
	request_weather();
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
	request_weather();
	// Update again in seven minutes.
	if (cookie)
		set_timer(ctx);
}

void handle_init(AppContextRef ctx) {
	window_init(&window, "Milano Watch");
	window_stack_push(&window, true /* Animated */);
	window_set_fullscreen(&window, true);

	resource_init_current_app(&APP_RESOURCES);
#if INVERTED
	bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND_INVERTED, &background_image_container);
#else
	bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image_container);
#endif
	layer_add_child(&window.layer, &background_image_container.layer.layer);


	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SNAP_10));
	text_layer_init(&date_layer, GRect(32, 138, 80, 20));
	text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
#if INVERTED
	text_layer_set_text_color(&date_layer, GColorBlack);
#else
	text_layer_set_text_color(&date_layer, GColorWhite);
#endif
	text_layer_set_background_color(&date_layer, GColorClear);
	text_layer_set_font(&date_layer, date_font);
	layer_add_child(&window.layer, &date_layer.layer);
	draw_date();

	location_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SNAP_10));
	text_layer_init(&location_layer, GRect(32, 15, 80, 32));
	text_layer_set_text_alignment(&location_layer, GTextAlignmentCenter);
#if INVERTED
	text_layer_set_text_color(&location_layer, GColorBlack);
#else
	text_layer_set_text_color(&location_layer, GColorWhite);
#endif
	text_layer_set_background_color(&location_layer, GColorClear);
	text_layer_set_font(&location_layer, location_font);
	layer_add_child(&window.layer, &location_layer.layer);
	draw_location("pebble");

	weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_24));
	text_layer_init(&weather_layer, GRect(15, 110, 80, 32));
	text_layer_set_text_alignment(&weather_layer, GTextAlignmentCenter);
#if INVERTED
	text_layer_set_text_color(&weather_layer, GColorBlack);
#else
	text_layer_set_text_color(&weather_layer, GColorWhite);
#endif
	text_layer_set_background_color(&weather_layer, GColorClear);
	text_layer_set_font(&weather_layer, weather_font);
	layer_add_child(&window.layer, &weather_layer.layer);

	// temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STRIDER_16));
	//temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNSTEADY_18));
	// temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_17));
	//temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OPENSANS_17));
	//temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UMPUSH_18));
	//temperature_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NEVIS_18));
	temperature_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	text_layer_init(&temperature_layer, GRect(52, 113, 80, 32));
	text_layer_set_text_alignment(&temperature_layer, GTextAlignmentCenter);
#if INVERTED
	text_layer_set_text_color(&temperature_layer, GColorBlack);
#else
	text_layer_set_text_color(&temperature_layer, GColorWhite);
#endif
	text_layer_set_background_color(&temperature_layer, GColorClear);
	text_layer_set_font(&temperature_layer, temperature_font);
	layer_add_child(&window.layer, &temperature_layer.layer);
	
	// draw both of the above for the first time
	draw_weather("", -127);

	info_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	text_layer_init(&info_layer, GRect(32, 40, 80, 32));
	text_layer_set_text_alignment(&info_layer, GTextAlignmentCenter);
#if INVERTED
	text_layer_set_text_color(&info_layer, GColorBlack);
#else
	text_layer_set_text_color(&info_layer, GColorWhite);
#endif
	text_layer_set_background_color(&info_layer, GColorClear);
	text_layer_set_font(&info_layer, info_font);
	layer_add_child(&window.layer, &info_layer.layer);
	draw_info("");

	layer_init(&hour_display_layer, window.layer.frame);
	hour_display_layer.update_proc = &hour_display_layer_update_callback;
	layer_add_child(&window.layer, &hour_display_layer);
	gpath_init(&hour_hand_path, &HOUR_HAND_PATH_POINTS);
	gpath_move_to(&hour_hand_path, grect_center_point(&hour_display_layer.frame));

	layer_init(&minute_display_layer, window.layer.frame);
	minute_display_layer.update_proc = &minute_display_layer_update_callback;
	layer_add_child(&window.layer, &minute_display_layer);
	gpath_init(&minute_hand_path, &MINUTE_HAND_PATH_POINTS);
	gpath_move_to(&minute_hand_path, grect_center_point(&minute_display_layer.frame));

	layer_init(&center_display_layer, window.layer.frame);
	center_display_layer.update_proc = &center_display_layer_update_callback;
	layer_add_child(&window.layer, &center_display_layer);

#if DISPLAY_SECONDS
	layer_init(&second_display_layer, window.layer.frame);
	second_display_layer.update_proc = &second_display_layer_update_callback;
	layer_add_child(&window.layer, &second_display_layer);
#endif

	http_register_callbacks((HTTPCallbacks){
		.failure=failed,
		.success=success,
		.reconnect=reconnect,
		.location=location
	}, (void*)ctx);
	
	// start us off with a location poll
	located = false;
	// set up our timer to check the weather every fifteen minutes
	set_timer((AppContextRef)ctx);
	// initiate the first weather request
	request_weather();
}

void handle_deinit(AppContextRef ctx) {
	(void)ctx;

	bmp_deinit_container(&background_image_container);
	fonts_unload_custom_font(date_font);
	fonts_unload_custom_font(location_font);
	fonts_unload_custom_font(weather_font);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t){
	(void)t;
	(void)ctx;
	if(t->tick_time->tm_sec%10==0)
	{
		 layer_mark_dirty(&minute_display_layer);
		 
		 if(t->tick_time->tm_sec==0)
		 {
				if(t->tick_time->tm_min%2==0)
				{
					layer_mark_dirty(&hour_display_layer);
					if(t->tick_time->tm_min==0&&t->tick_time->tm_hour==0)
					{
							draw_date();
					}
					
					if(t->tick_time->tm_min==0) {
#if HOUR_VIBRATION
						if (t->tick_time->tm_hour>=HOUR_VIBRATION_START && t->tick_time->tm_hour<=HOUR_VIBRATION_END) {
							vibes_double_pulse();
						}
#endif
					}
					
					if (t->tick_time->tm_min%20==0) {
						// poll our current location every 20 minutes
						located = false;
					}
				}
		 }
	}

#if DISPLAY_SECONDS
	layer_mark_dirty(&second_display_layer);
#endif
}

void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info = {
			.tick_handler = &handle_tick,
#if DISPLAY_SECONDS
			.tick_units = SECOND_UNIT
#else
			.tick_units = MINUTE_UNIT
#endif
		},
		.timer_handler = handle_timer,
		.messaging_info = {
			.buffer_sizes = {
				.inbound = 124,
				.outbound = 256,
			}
		}
	};
	app_event_loop(params, &handlers);
}

